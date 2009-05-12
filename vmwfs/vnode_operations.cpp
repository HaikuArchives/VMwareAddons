#include <sys/stat.h>
#include <stdlib.h>

#include "vmwfs.h"

status_t
vmwfs_lookup(fs_volume* volume, fs_vnode* dir, const char* name, ino_t* _id)
{
	VMWNode* dir_node = (VMWNode*)dir->private_node;
	
	char* path = dir_node->GetChildPath(name);
	
	dprintf("vmwfs_lookup: looking up path “%s”...\n", path);
	
	status_t ret = shared_folders->GetAttributes(path);	
	free(path);
	if (ret != B_OK)
		return ret;
	
	VMWNode* node = dir_node->GetChild(name);
	if (node == NULL)
		return B_NO_MEMORY;
	
	*_id = node->GetInode();
	
	return get_vnode(volume, node->GetInode(), NULL);
}

status_t
vmwfs_get_vnode_name(fs_volume* volume, fs_vnode* vnode, char* buffer, size_t bufferSize)
{
	VMWNode* node = (VMWNode*)vnode->private_node;
	
	strncpy(buffer, node->GetName(), bufferSize);
	buffer[bufferSize] = '\0';
	return B_OK;
}

status_t
vmwfs_put_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter)
{
	return B_OK;
}

status_t
vmwfs_remove_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter)
{
	return B_UNSUPPORTED;
}

status_t
vmwfs_unlink(fs_volume* volume, fs_vnode* dir, const char* name)
{
	return B_UNSUPPORTED;
}

status_t
vmwfs_rename(fs_volume* volume, fs_vnode* fromDir, const char* fromName, fs_vnode* toDir, const char* toName)
{
	return B_UNSUPPORTED;
}

status_t
vmwfs_access(fs_volume* volume, fs_vnode* vnode, int mode)
{
	VMWNode* node = (VMWNode*)vnode->private_node;
	
	char* path = node->GetPath();
	if (path == NULL)
		return B_NO_MEMORY;
	
	vmw_attributes attributes;
	status_t ret = shared_folders->GetAttributes(path, &attributes);
	free(path);
	
	if (ret != B_OK)
		return ret;
	
	if (geteuid() == 0 && (mode & X_OK != X_OK || CAN_EXEC(attributes)))
		return B_OK;
	
	if ((mode & R_OK == R_OK && !CAN_READ(attributes))
		|| (mode & W_OK == W_OK && !CAN_WRITE(attributes))
			|| (mode & X_OK == X_OK && !CAN_EXEC(attributes)))
		return B_PERMISSION_DENIED;
	
	return B_OK;
}

status_t
vmwfs_read_stat(fs_volume* volume, fs_vnode* vnode, struct stat* stat)
{
	VMWNode* root = (VMWNode*)volume->private_volume;
	VMWNode* node = (VMWNode*)vnode->private_node;
	
	char* path = node->GetPath();
	if (path == NULL)
		return B_NO_MEMORY;
	
	vmw_attributes attributes;
	bool is_dir;
	status_t ret = shared_folders->GetAttributes(path, &attributes, &is_dir);
	free(path);
	
	if (ret != B_OK)
		return ret;
	
	dprintf("vmwfs_read_stat : %s has permissions %d\n", node->GetName(), attributes.perms);
	
	stat->st_dev = root->GetInode();
	stat->st_ino = node->GetInode();
	
	stat->st_mode = 0;
	stat->st_mode |= (CAN_READ(attributes) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
	stat->st_mode |= (CAN_WRITE(attributes) ? S_IWUSR : 0);
	stat->st_mode |= (CAN_EXEC(attributes) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);
	stat->st_mode |= (is_dir ? S_IFDIR : S_IFREG);	
	
	stat->st_nlink = 1;
	stat->st_uid = 0;
	stat->st_gid = 0;
	
	// VMware seems to give random sizes to directories
	stat->st_size = attributes.size; //(is_dir ? 0 : attributes.size);
	stat->st_blocks = stat->st_size / FAKE_BLOCK_SIZE; // TODO : Round this correctly
	stat->st_blksize = FAKE_BLOCK_SIZE;
	
	// VMware dates are in thenth of microseconds since the 1/1/1901 (on 64 bits).
	// We need to convert them in number of seconds since the 1/1/1970.
	
	stat->st_atime = (attributes.a_time / 10000000LL - 11644466400LL);
	stat->st_mtime = (attributes.m_time / 10000000LL - 11644466400LL);
	stat->st_ctime = stat->st_crtime = (attributes.c_time / 10000000LL - 11644466400LL);
	
	return B_NO_ERROR;
}

status_t
vmwfs_write_stat(fs_volume* volume, fs_vnode* vnode, const struct stat* stat, uint32 statMask)
{
	return B_UNSUPPORTED;
}
