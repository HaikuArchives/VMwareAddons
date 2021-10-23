#include <sys/stat.h>
#include <stdlib.h>

#include <fs_interface.h>

#include "vmwfs.h"

status_t
vmwfs_lookup(fs_volume* volume, fs_vnode* dir, const char* name, ino_t* _id)
{
	VMWNode* dir_node = (VMWNode*)dir->private_node;

	ssize_t length = dir_node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH, name);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	status_t ret = shared_folders->GetAttributes(path_buffer);
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
	VMWNode* node = (VMWNode*)vnode->private_node;
	VMWNode* parent = node->GetChild("..");

	char* name = strdup(node->GetName());

	if (name == NULL)
		return B_NO_MEMORY;

	parent->DeleteChildIfExists(name);

	free(name);

	return B_OK;
}

status_t
vmwfs_unlink(fs_volume* volume, fs_vnode* dir, const char* name)
{
	VMWNode* node = (VMWNode*)dir->private_node;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH, name);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	status_t ret = shared_folders->DeleteFile(path_buffer);

	return ret;
}

status_t
vmwfs_rename(fs_volume* volume, fs_vnode* fromDir, const char* fromName, fs_vnode* toDir, const char* toName)
{
	VMWNode* src_dir = (VMWNode*)fromDir->private_node;
	VMWNode* dst_dir = (VMWNode*)toDir->private_node;

	ssize_t length = src_dir->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH, fromName);
	if (length < 0)
		return B_BUFFER_OVERFLOW;


	length = dst_dir->CopyPathTo(path_buffer_dest, B_PATH_NAME_LENGTH, toName);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	status_t ret = shared_folders->Move(path_buffer, path_buffer_dest);

	return ret;
}

status_t
vmwfs_access(fs_volume* volume, fs_vnode* vnode, int mode)
{
	VMWNode* node = (VMWNode*)vnode->private_node;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	vmw_attributes attributes;
	status_t ret = shared_folders->GetAttributes(path_buffer, &attributes);

	if (ret != B_OK)
		return ret;

	if (((mode & R_OK) == R_OK && !CAN_READ(attributes))
		|| ((mode & W_OK) == W_OK && !CAN_WRITE(attributes))
			|| ((mode & X_OK) == X_OK && !CAN_EXEC(attributes)))
		return B_PERMISSION_DENIED;

	return B_OK;
}

status_t
vmwfs_read_stat(fs_volume* volume, fs_vnode* vnode, struct stat* stat)
{
	VMWNode* node = (VMWNode*)vnode->private_node;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	vmw_attributes attributes;
	bool is_dir;
	status_t ret = shared_folders->GetAttributes(path_buffer, &attributes, &is_dir);

	if (ret != B_OK)
		return ret;

	stat->st_dev = device_id;
	stat->st_ino = node->GetInode();

	stat->st_mode = 0;
	stat->st_mode |= (CAN_READ(attributes) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
	stat->st_mode |= (CAN_WRITE(attributes) ? S_IWUSR : 0);
	stat->st_mode |= (CAN_EXEC(attributes) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);
	stat->st_mode |= (is_dir ? S_IFDIR : S_IFREG);

	stat->st_nlink = 1;
	stat->st_uid = 0;
	stat->st_gid = 0;

	// VMware seems to calculate (in a strange way) directory sizes, but this is not needed
	stat->st_size = (is_dir ? 0 : attributes.size);
	stat->st_blocks = stat->st_size / FAKE_BLOCK_SIZE;
	if (stat->st_size % FAKE_BLOCK_SIZE != 0)
		stat->st_blocks++;
	stat->st_blksize = FAKE_BLOCK_SIZE;

	// VMware dates are in thenth of microseconds since the 1/1/1901 (on 64 bits).
	// We need to convert them in number of seconds since the 1/1/1970.

	stat->st_atime = (attributes.a_time / 10000000LL - 11644466400LL);
	stat->st_mtime = (attributes.m_time / 10000000LL - 11644466400LL);
	stat->st_ctime = stat->st_crtime = (attributes.c_time / 10000000LL - 11644466400LL);

	return B_NO_ERROR;
}

// TODO : This enum was taken from haiku/headers/build/os/drivers/fs_interface.h, find where it is defined
// in the bundled headers.
enum write_stat_mask {
	FS_WRITE_STAT_MODE		= 0x0001,
	FS_WRITE_STAT_UID		= 0x0002,
	FS_WRITE_STAT_GID		= 0x0004,
	FS_WRITE_STAT_SIZE		= 0x0008,
	FS_WRITE_STAT_ATIME		= 0x0010,
	FS_WRITE_STAT_MTIME		= 0x0020,
	FS_WRITE_STAT_CRTIME	= 0x0040
};

status_t
vmwfs_write_stat(fs_volume* volume, fs_vnode* vnode, const struct stat* stat, uint32 statMask)
{
	VMWNode* node = (VMWNode*)vnode->private_node;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH);
	if (length < 0)
		return B_BUFFER_OVERFLOW;

	vmw_attributes attributes;

	attributes.perms = 0;
	attributes.perms |= static_cast<uint8>((stat->st_mode & S_IRUSR) == S_IRUSR ? MSK_READ : 0);
	attributes.perms |= static_cast<uint8>((stat->st_mode & S_IWUSR) == S_IWUSR ? MSK_WRITE : 0);
	attributes.perms |= static_cast<uint8>((stat->st_mode & S_IXUSR) == S_IXUSR ? MSK_EXEC : 0);

	attributes.size = stat->st_size;

	attributes.a_time = (stat->st_atime + 11644466400LL) * 10000000LL;
	attributes.m_time = (stat->st_mtime + 11644466400LL) * 10000000LL;
	attributes.c_time = (stat->st_ctime + 11644466400LL) * 10000000LL;

	uint32 mask = 0;
	mask |= ((statMask & FS_WRITE_STAT_MODE) == FS_WRITE_STAT_MODE ? VMW_SET_PERMS : 0);
	mask |= ((statMask & FS_WRITE_STAT_SIZE) == FS_WRITE_STAT_SIZE ? VMW_SET_SIZE : 0);
	mask |= ((statMask & FS_WRITE_STAT_ATIME) == FS_WRITE_STAT_ATIME ? VMW_SET_ATIME : 0);
	mask |= ((statMask & FS_WRITE_STAT_MTIME) == FS_WRITE_STAT_MTIME ? VMW_SET_UTIME : 0);
	mask |= ((statMask & FS_WRITE_STAT_CRTIME) == FS_WRITE_STAT_CRTIME ? VMW_SET_CTIME : 0);

	status_t ret = shared_folders->SetAttributes(path_buffer, &attributes, mask);

	return ret;
}
