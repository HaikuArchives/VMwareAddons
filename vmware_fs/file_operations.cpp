#include <stdlib.h>

#include "vmwfs.h"

status_t
vmwfs_create(fs_volume* volume, fs_vnode* dir, const char* name, int openMode, int perms, void** _cookie, ino_t* _newVnodeID)
{
	VMWNode* dir_node = (VMWNode*)dir->private_node;

	ssize_t length = dir_node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH, name);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	file_handle* cookie = (file_handle*)malloc(sizeof(file_handle));
	if (cookie == NULL)
		return B_NO_MEMORY;

	status_t ret = shared_folders->OpenFile(path_buffer, openMode | O_CREAT, cookie);

	if (ret != B_OK) {
		free(cookie);
		return ret;
	}

	*_cookie = cookie;

	VMWNode* new_node = dir_node->GetChild(name);
	if (new_node == NULL)
		return B_NO_MEMORY;

	*_newVnodeID = new_node->GetInode();

	return get_vnode(volume, new_node->GetInode(), NULL);

}

status_t
vmwfs_open(fs_volume* volume, fs_vnode* vnode, int openMode, void** _cookie)
{
	VMWNode* node = (VMWNode*)vnode->private_node;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	file_handle* cookie = (file_handle*)malloc(sizeof(file_handle));
	if (cookie == NULL)
		return B_NO_MEMORY;

	*_cookie = cookie;

	if (length == 0) {
		// We allow opening the root node as a file because it allows the volume
		// to be shown in Tracker.
		*cookie = (file_handle)(-1);
		return B_OK;
	}

	status_t ret = shared_folders->OpenFile(path_buffer, openMode, cookie);

	if (ret != B_OK) {
		free(cookie);
		return ret;
	}

	return B_OK;
}

status_t
vmwfs_close(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	if (*(file_handle*)cookie == (file_handle)(-1))
		return B_OK;
	return shared_folders->CloseFile(*(file_handle*)cookie);
}

status_t
vmwfs_free_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	free(cookie);
	return B_OK;
}

status_t
vmwfs_read(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, void* buffer, size_t* length)
{
	if (pos < 0)
		return B_BAD_VALUE;

	status_t ret;
	uint32 to_read, read = 0;

	do {
		to_read = static_cast<uint32>((*length - read) < IO_SIZE ? *length - read : IO_SIZE);
		ret = shared_folders->ReadFile(*(file_handle*)cookie, pos + read, (uint8*)buffer + read, &to_read);
		read += to_read;
	} while(to_read != 0 && read < *length && ret == B_OK); // to_read == 0 means EOF

	*length = static_cast<size_t>(read);

	return ret;
}

status_t
vmwfs_write(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, const void* buffer, size_t* length)
{
	if (pos < 0)
		return B_BAD_VALUE;

	uint32 written = 0;
	status_t ret = B_OK;

	while (written < *length && ret == B_OK) {
		uint32 to_write = static_cast<uint32>((*length - written) < IO_SIZE ? *length - written : IO_SIZE);
		ret = shared_folders->WriteFile(*(file_handle*)cookie, pos + written, (const uint8*)buffer + written, &to_write);
		written += to_write;
	}

	*length = static_cast<size_t>(written);

	return ret;
}
