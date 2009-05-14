#include <stdlib.h>

#include "vmwfs.h"

status_t
vmwfs_create(fs_volume* volume, fs_vnode* dir, const char* name, int openMode, int perms, void** _cookie, ino_t* _newVnodeID)
{
	VMWNode* dir_node = (VMWNode*)dir->private_node;
	
	file_handle* cookie = (file_handle*)malloc(sizeof(file_handle));
	if (cookie == NULL)
		return B_NO_MEMORY;
	
	char* path = dir_node->GetChildPath(name);
	if (path == NULL) {
		free(cookie);
		return B_NO_MEMORY;
	}
	
	status_t ret = shared_folders->OpenFile(path, openMode | O_CREAT, cookie);
	free(path);
	
	if (ret != B_OK) {
		free(cookie);
		return ret;
	}
	
	*_cookie = cookie;
	
	VMWNode* new_node = dir_node->GetChild(name);
	if (new_node == NULL)
		return B_NO_MEMORY;
	
	*_newVnodeID = new_node->GetInode();
		
	return B_OK;

}

status_t
vmwfs_open(fs_volume* volume, fs_vnode* vnode, int openMode, void** _cookie)
{
	VMWNode* node = (VMWNode*)vnode->private_node;
	
	file_handle* cookie = (file_handle*)malloc(sizeof(file_handle));
	if (cookie == NULL)
		return B_NO_MEMORY;
	
	char* path = node->GetPath();
	if (path == NULL) {
		free(cookie);
		return B_NO_MEMORY;
	}
	
	status_t ret = shared_folders->OpenFile(path, openMode, cookie);
	free(path);
	
	if (ret != B_OK) {
		free(cookie);
		return ret;
	}
	
	*_cookie = cookie;
	
	return B_OK;
}

status_t
vmwfs_close(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
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
	
	status_t ret = shared_folders->ReadFile(*(file_handle*)cookie, pos, buffer, length);
	
	return ret;
}

status_t
vmwfs_write(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, const void* buffer, size_t* length)
{
	if (pos < 0)
		return B_BAD_VALUE;
	
	status_t ret = shared_folders->WriteFile(*(file_handle*)cookie, pos, buffer, length);
	
	return ret;
}
