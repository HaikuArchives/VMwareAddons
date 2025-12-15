#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "vmwfs.h"

#define VMWFS_PERMS_MODE_SHIFT 6

typedef struct {
	file_handle handle;
	uint32 index;
} dir_cookie;

status_t
vmwfs_create_dir(fs_volume* volume, fs_vnode* parent, const char* name, int perms)
{
	VMWNode* node = (VMWNode*)parent->private_node;

	char* path_buffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (path_buffer == NULL)
		return B_NO_MEMORY;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH, name);
	if (length < 0) {
		free(path_buffer);
		return B_BUFFER_OVERFLOW;
	}

	// Haiku passes permissions as a 32-bit value, but the VMWare RPC v1 calls we use
	// accept permissions mode as `uint8` (expecting only the user bits of the original permissions value)
	uint8 vmwfs_perms_mode = (perms & S_IRWXU) >> VMWFS_PERMS_MODE_SHIFT;

	status_t ret = shared_folders->CreateDir(path_buffer, vmwfs_perms_mode);
	free(path_buffer);

	return ret;
}

status_t
vmwfs_remove_dir(fs_volume* volume, fs_vnode* parent, const char* name)
{
	VMWNode* node = (VMWNode*)parent->private_node;

	char* path_buffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (path_buffer == NULL)
		return B_NO_MEMORY;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH, name);
	if (length < 0) {
		free(path_buffer);
		return B_BUFFER_OVERFLOW;
	}

	status_t ret = shared_folders->DeleteDir(path_buffer);
	free(path_buffer);

	if (ret != B_OK)
		return ret;

	node->DeleteChildIfExists(name);

	return B_OK;
}

status_t
vmwfs_open_dir(fs_volume* volume, fs_vnode* vnode, void** _cookie)
{
	VMWNode* node = (VMWNode*)vnode->private_node;

	dir_cookie* cookie = (dir_cookie*)malloc(sizeof(dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->index = 0;

	char* path_buffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (path_buffer == NULL) {
		free(cookie);
		return B_NO_MEMORY;
	}

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH);
	if (length < 0) {
		free(cookie);
		free(path_buffer);
		return B_BUFFER_OVERFLOW;
	}

	status_t ret = shared_folders->OpenDir(path_buffer, &cookie->handle);
	free(path_buffer);

	if (ret != B_OK) {
		free(cookie);
		return ret;
	}

	*_cookie = cookie;

	return B_OK;
}

status_t
vmwfs_close_dir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	return shared_folders->CloseDir(((dir_cookie*)cookie)->handle);
}

status_t
vmwfs_free_dir_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	free(cookie);
	return B_OK;
}

status_t
vmwfs_read_dir(fs_volume* volume, fs_vnode* vnode, void* _cookie, struct dirent* buffer, size_t bufferSize, uint32* _num)
{
	VMWNode* node = (VMWNode*)vnode->private_node;
	dir_cookie* cookie = (dir_cookie*)_cookie;

	struct dirent* entry = (struct dirent*)malloc(bufferSize);
	if (entry == NULL)
		return B_NO_MEMORY;

	status_t ret = shared_folders->ReadDir(cookie->handle, cookie->index, entry->d_name, bufferSize - offsetof(struct dirent, d_name));

	if (ret == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		free(entry);
		return B_OK;
	}

	if (ret != B_OK) {
		free(entry);
		return ret;
	}

	VMWNode* child_node = node->GetChild(entry->d_name);
	if (child_node == NULL) {
		free(entry);
		return B_NO_MEMORY;
	}

	entry->d_dev = device_id;
	entry->d_ino = child_node->GetInode();
	entry->d_reclen = static_cast<ushort>(sizeof(struct dirent) + strlen(entry->d_name));

	if (user_memcpy(buffer, entry, entry->d_reclen) != B_OK) {
		free(entry);
		return B_BAD_ADDRESS;
	}

	free(entry);

	*_num = 1;

	cookie->index++;

	return B_OK;
}

status_t
vmwfs_rewind_dir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	((dir_cookie*)cookie)->index = 0;
	return B_OK;
}
