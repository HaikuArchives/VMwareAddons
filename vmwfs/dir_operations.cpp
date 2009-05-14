#include <dirent.h>
#include <stdlib.h>

#include "vmwfs.h"

typedef struct {
	file_handle handle;
	uint32 index;
} dir_cookie;

status_t
vmwfs_create_dir(fs_volume* volume, fs_vnode* parent, const char* name, int perms)
{
	VMWNode* node = (VMWNode*)parent->private_node;
	
	char* path = node->GetChildPath(name);
	if (path == NULL)
		return B_NO_MEMORY;

	status_t ret = shared_folders->CreateDir(path, perms);
	free(path);
	
	return ret;
}

status_t
vmwfs_remove_dir(fs_volume* volume, fs_vnode* parent, const char* name)
{
	VMWNode* node = (VMWNode*)parent->private_node;
	
	char* path = node->GetChildPath(name);
	if (path == NULL)
		return B_NO_MEMORY;

	status_t ret = shared_folders->DeleteDir(path);
	free(path);
	
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
	
	char* path = node->GetPath();
	if (path == NULL) {
		free(cookie);
		return B_NO_MEMORY;
	}
	
	status_t ret = shared_folders->OpenDir(path, &cookie->handle);
	free(path);
	
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
	VMWNode* root = (VMWNode*)volume->private_volume;
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
	
	buffer->d_dev = root->GetInode();
	buffer->d_ino = child_node->GetInode();
	buffer->d_reclen = sizeof(struct dirent) + strlen(buffer->d_name);
	
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
