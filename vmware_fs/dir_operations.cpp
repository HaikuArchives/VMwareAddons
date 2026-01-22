#include <dirent.h>

#include <KernelExport.h>

#include "vmwfs.h"


#define VMWFS_PERMS_MODE_SHIFT 6


typedef struct {
	file_handle handle;
	uint32 index;
} dir_cookie;


status_t
vmwfs_create_dir(fs_volume* volume, fs_vnode* parent, const char* name, int permissions)
{
	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	VMWNode* node = (VMWNode*)parent->private_node;
	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH, name);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	// Haiku passes permissions as a 32-bit value, but the VMWare RPC v1 calls we use accepts
	// permissions mode as `uint8` (expecting only the user bits of the original permissions
	// value)
	uint8 mode = (permissions & S_IRWXU) >> VMWFS_PERMS_MODE_SHIFT;

	status_t status = gSharedFolders->CreateDir(pathBuffer, mode);
	free(pathBuffer);

	ino_t inode = node->GetChild(name)->GetInode();
	notify_entry_created(volume->id, node->GetInode(), name, inode);
	return status;
}


status_t
vmwfs_remove_dir(fs_volume* volume, fs_vnode* parent, const char* name)
{
	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	VMWNode* node = (VMWNode*)parent->private_node;
	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH, name);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	status_t status = gSharedFolders->DeleteDir(pathBuffer);
	free(pathBuffer);

	if (status != B_OK)
		return status;

	ino_t inode = node->GetChild(name)->GetInode();

	node->DeleteChildIfExists(name);

	notify_entry_removed(volume->id, node->GetInode(), name, inode);
	return B_OK;
}


status_t
vmwfs_open_dir(fs_volume* volume, fs_vnode* vnode, void** _cookie)
{
	dir_cookie* cookie = (dir_cookie*)malloc(sizeof(dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->index = 0;

	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL) {
		free(cookie);
		return B_NO_MEMORY;
	}

	VMWNode* node = (VMWNode*)vnode->private_node;
	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH);
	if (length < 0) {
		free(cookie);
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	status_t status = gSharedFolders->OpenDir(pathBuffer, &cookie->handle);
	free(pathBuffer);

	if (status != B_OK) {
		free(cookie);
		return status;
	}

	*_cookie = cookie;

	return B_OK;
}


status_t
vmwfs_close_dir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	return gSharedFolders->CloseDir(((dir_cookie*)cookie)->handle);
}


status_t
vmwfs_free_dir_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	free(cookie);
	return B_OK;
}


status_t
vmwfs_read_dir(fs_volume* volume, fs_vnode* vnode, void* _cookie, struct dirent* buffer,
	size_t bufferSize, uint32* _num)
{
	struct dirent* entry = (struct dirent*)malloc(bufferSize);
	if (entry == NULL)
		return B_NO_MEMORY;

	dir_cookie* cookie = (dir_cookie*)_cookie;
	status_t status = gSharedFolders->ReadDir(cookie->handle, cookie->index, entry->d_name,
		bufferSize - offsetof(struct dirent, d_name));

	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		free(entry);
		return B_OK;
	}

	if (status != B_OK) {
		free(entry);
		return status;
	}

	VMWNode* node = (VMWNode*)vnode->private_node;
	VMWNode* childNode = node->GetChild(entry->d_name);
	if (childNode == NULL) {
		free(entry);
		return B_NO_MEMORY;
	}

	entry->d_dev = gDeviceId;
	entry->d_ino = childNode->GetInode();
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
