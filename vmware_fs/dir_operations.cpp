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

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH, name);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	// Haiku passes permissions as a 32-bit value, but the VMWare RPC v1 calls we use
	// accept permissions mode as `uint8` (expecting only the user bits of the original permissions value)
	uint8 vmwfs_perms_mode = (perms & S_IRWXU) >> VMWFS_PERMS_MODE_SHIFT;

	status_t ret = shared_folders->CreateDir(path_buffer, vmwfs_perms_mode);

	return ret;
}

status_t
vmwfs_remove_dir(fs_volume* volume, fs_vnode* parent, const char* name)
{
	VMWNode* node = (VMWNode*)parent->private_node;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH, name);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	status_t ret = shared_folders->DeleteDir(path_buffer);

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

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH);
	if (length < 0) {
		free(cookie);
		return B_BUFFER_OVERFLOW;
	}

	status_t ret = shared_folders->OpenDir(path_buffer, &cookie->handle);

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

	status_t ret = shared_folders->ReadDir(cookie->handle, cookie->index, buffer->d_name, bufferSize - offsetof(struct dirent, d_name));

	if (ret == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	}

	if (ret != B_OK)
		return ret;

	VMWNode* child_node = node->GetChild(buffer->d_name);
	if (child_node == NULL)
		return B_NO_MEMORY;

	buffer->d_dev = device_id;
	buffer->d_ino = child_node->GetInode();
	buffer->d_reclen = static_cast<ushort>(sizeof(struct dirent) + strlen(buffer->d_name));

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
