#include "vmwfs.h"


#define TO_UNIX_TIME(x) ((x) / 10000000LL - 11644473600LL)
#define TO_VMW_TIME(x) ((x) + 11644473600LL) * 10000000LL


status_t
vmwfs_lookup(fs_volume* volume, fs_vnode* dir, const char* name, ino_t* _id)
{
	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	VMWNode* dirNode = (VMWNode*)dir->private_node;
	ssize_t length = dirNode->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH, name);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	status_t status = gSharedFolders->GetAttributes(pathBuffer);
	free(pathBuffer);
	if (status != B_OK)
		return status;

	VMWNode* node = dirNode->GetChild(name);
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
	char* name = strdup(node->GetName());

	if (name == NULL)
		return B_NO_MEMORY;

	VMWNode* parent = node->GetChild("..");
	parent->DeleteChildIfExists(name);

	free(name);

	return B_OK;
}


status_t
vmwfs_unlink(fs_volume* volume, fs_vnode* dir, const char* name)
{
	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	VMWNode* node = (VMWNode*)dir->private_node;
	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH, name);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	ino_t inode = node->GetChild(name)->GetInode();

	status_t status = gSharedFolders->DeleteFile(pathBuffer);
	free(pathBuffer);

	notify_entry_removed(volume->id, node->GetInode(), name, inode);
	return status;
}


status_t
vmwfs_rename(fs_volume* volume, fs_vnode* fromDir, const char* fromName, fs_vnode* toDir,
	const char* toName)
{
	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	char* pathBufferDest = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL || pathBufferDest == NULL) {
		free(pathBuffer);
		free(pathBufferDest);
		return B_NO_MEMORY;
	}

	VMWNode* srcDir = (VMWNode*)fromDir->private_node;
	ssize_t length = srcDir->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH, fromName);
	if (length < 0) {
		free(pathBuffer);
		free(pathBufferDest);
		return B_BUFFER_OVERFLOW;
	}

	VMWNode* dstDir = (VMWNode*)toDir->private_node;
	length = dstDir->CopyPathTo(pathBufferDest, B_PATH_NAME_LENGTH, toName);
	if (length < 0) {
		free(pathBuffer);
		free(pathBufferDest);
		return B_BUFFER_OVERFLOW;
	}

	ino_t inode = srcDir->GetChild(fromName)->GetInode();

	status_t status = gSharedFolders->Move(pathBuffer, pathBufferDest);
	free(pathBuffer);
	free(pathBufferDest);

	notify_entry_moved(volume->id, srcDir->GetInode(), fromName, dstDir->GetInode(), toName, inode);
	return status;
}


status_t
vmwfs_access(fs_volume* volume, fs_vnode* vnode, int mode)
{
	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	VMWNode* node = (VMWNode*)vnode->private_node;
	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	vmw_attributes attributes;
	status_t status = gSharedFolders->GetAttributes(pathBuffer, &attributes);
	free(pathBuffer);

	if (status != B_OK)
		return status;

	if (((mode & R_OK) == R_OK && !CAN_READ(attributes))
		|| ((mode & W_OK) == W_OK && !CAN_WRITE(attributes))
		|| ((mode & X_OK) == X_OK && !CAN_EXEC(attributes))) {
		return B_PERMISSION_DENIED;
	}

	return B_OK;
}


status_t
vmwfs_read_stat(fs_volume* volume, fs_vnode* vnode, struct stat* stat)
{
	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	VMWNode* node = (VMWNode*)vnode->private_node;
	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	vmw_attributes attributes;
	bool isDir;
	status_t status = gSharedFolders->GetAttributes(pathBuffer, &attributes, &isDir);
	free(pathBuffer);

	if (status != B_OK)
		return status;

	stat->st_dev = gDeviceId;
	stat->st_ino = node->GetInode();

	stat->st_mode = 0;
	stat->st_mode |= (CAN_READ(attributes) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
	stat->st_mode |= (CAN_WRITE(attributes) ? S_IWUSR : 0);

	// Ignore the exec bit for files. VMware sets it for *all* files for shared folders
	// on a Windows host, which makes double clicking files really annoying in Tracker.
	// ToDo: turn this into a mount-time option.
	if (isDir)
		stat->st_mode |= (CAN_EXEC(attributes) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);

	stat->st_mode |= (isDir ? S_IFDIR : S_IFREG);

	stat->st_nlink = 1;
	stat->st_uid = 0;
	stat->st_gid = 0;

	// VMware seems to calculate (in a strange way) directory sizes, but this is not needed
	stat->st_size = (isDir ? 0 : attributes.size);
	stat->st_blocks = stat->st_size / FAKE_BLOCK_SIZE;
	if (stat->st_size % FAKE_BLOCK_SIZE != 0)
		stat->st_blocks++;
	stat->st_blksize = FAKE_BLOCK_SIZE;

	// VMware dates are in Windows NT time format, which is
	// the number of 100 nanosecond intervals that has passed since January 1, 1601, UTC.
	// We need to convert them in number of seconds since 1/1/1970.

	stat->st_crtime = TO_UNIX_TIME(attributes.cr_time);
	stat->st_atime = TO_UNIX_TIME(attributes.a_time);
	stat->st_mtime = TO_UNIX_TIME(attributes.m_time);
	stat->st_ctime = TO_UNIX_TIME(attributes.c_time);

	return B_NO_ERROR;
}


// TODO : This enum was taken from haiku/headers/build/os/drivers/fs_interface.h, find where it is
// defined in the bundled headers.
enum write_stat_mask {
	FS_WRITE_STAT_MODE		= 0x0001,
	FS_WRITE_STAT_UID		= 0x0002,
	FS_WRITE_STAT_GID		= 0x0004,
	FS_WRITE_STAT_SIZE		= 0x0008,
	FS_WRITE_STAT_ATIME		= 0x0010,
	FS_WRITE_STAT_MTIME		= 0x0020,
	FS_WRITE_STAT_CRTIME	= 0x0040,
	FS_WRITE_STAT_CTIME		= 0x0080
};


status_t
vmwfs_write_stat(fs_volume* volume, fs_vnode* vnode, const struct stat* stat, uint32 statMask)
{
	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	VMWNode* node = (VMWNode*)vnode->private_node;
	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	vmw_attributes attributes;

	attributes.perms = 0;
	attributes.perms |= static_cast<uint8>((stat->st_mode & S_IRUSR) == S_IRUSR ? MSK_READ : 0);
	attributes.perms |= static_cast<uint8>((stat->st_mode & S_IWUSR) == S_IWUSR ? MSK_WRITE : 0);
	attributes.perms |= static_cast<uint8>((stat->st_mode & S_IXUSR) == S_IXUSR ? MSK_EXEC : 0);

	attributes.size = stat->st_size;

	attributes.cr_time = TO_VMW_TIME(stat->st_crtime);
	attributes.a_time = TO_VMW_TIME(stat->st_atime);
	attributes.m_time = TO_VMW_TIME(stat->st_mtime);
	attributes.c_time = TO_VMW_TIME(stat->st_ctime);

	uint32 mask = 0;
	mask |= ((statMask & FS_WRITE_STAT_MODE) == FS_WRITE_STAT_MODE ? VMW_SET_PERMS : 0);
	mask |= ((statMask & FS_WRITE_STAT_SIZE) == FS_WRITE_STAT_SIZE ? VMW_SET_SIZE : 0);
	mask |= ((statMask & FS_WRITE_STAT_ATIME) == FS_WRITE_STAT_ATIME ? VMW_SET_ATIME : 0);
	mask |= ((statMask & FS_WRITE_STAT_MTIME) == FS_WRITE_STAT_MTIME ? VMW_SET_MTIME : 0);
	mask |= ((statMask & FS_WRITE_STAT_CRTIME) == FS_WRITE_STAT_CRTIME ? VMW_SET_CRTIME : 0);
	mask |= ((statMask & FS_WRITE_STAT_CTIME) == FS_WRITE_STAT_CTIME ? VMW_SET_CTIME : 0);

	status_t status = gSharedFolders->SetAttributes(pathBuffer, &attributes, mask);
	free(pathBuffer);

	notify_stat_changed(volume->id, node->GetParent()->GetInode(), node->GetInode(), mask);
	return status;
}


status_t
vmwfs_fsync(fs_volume* _volume, fs_vnode* _node, bool dataOnly)
{
	return B_OK;
}
