#include <fs_info.h>

#include "vmwfs.h"

extern fs_volume_ops gVolumeOps;
extern fs_vnode_ops gVnodeOps;

VMWNode* gRootNode;
int32 gMountCount = 0;
dev_t gDeviceId;


status_t
vmwfs_mount(fs_volume* _vol, const char* device, uint32 flags, const char* args, ino_t* _rootID)
{
	if (device != NULL)
		return B_BAD_VALUE;

	if (atomic_add(&gMountCount, 1))
		return B_UNSUPPORTED;

	gSharedFolders = new VMWSharedFolders(IO_SIZE);
	if (gSharedFolders == NULL || gSharedFolders->InitCheck() != B_OK) {
		atomic_add(&gMountCount, -1);
		delete gSharedFolders;
		return gSharedFolders == NULL ? B_NO_MEMORY : gSharedFolders->InitCheck();
	}

	gRootNode = new VMWNode("", NULL);

	if (gRootNode == NULL) {
		delete gRootNode;
		atomic_add(&gMountCount, -1);
		return B_NO_MEMORY;
	}

	*_rootID = gRootNode->GetInode();

	_vol->private_volume = gRootNode;
	_vol->ops = &gVolumeOps;
	gDeviceId = _vol->id;

	status_t status = publish_vnode(_vol, *_rootID, (void*)_vol->private_volume, &gVnodeOps,
		S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH | S_IFDIR | S_IXUSR | S_IXGRP
			| S_IXOTH,
		0);
	if (status != B_OK)
		atomic_add(&gMountCount, -1);

	return status;
}


status_t
vmwfs_unmount(fs_volume* volume)
{
	put_vnode(volume, gRootNode->GetInode());

	delete gRootNode;
	delete gSharedFolders;

	atomic_add(&gMountCount, -1);

	return B_OK;
}


status_t
vmwfs_read_fs_info(fs_volume* volume, struct fs_info* info)
{
	info->flags = B_FS_IS_PERSISTENT;
	info->block_size = FAKE_BLOCK_SIZE;
	info->io_size = IO_SIZE;
	info->total_blocks = 4 * 1024 * 1024; // 2GB
	info->free_blocks = info->total_blocks;
	info->total_nodes = 0;
	info->free_nodes = 0;

	strcpy(info->device_name, "");
	strcpy(info->volume_name, "VMware Shared Folders");

	return B_OK;
}


status_t
vmwfs_write_fs_info(fs_volume* volume, const struct fs_info* info, uint32 mask)
{
	// TODO : Store volume name?
	return B_OK;
}


status_t
vmwfs_get_vnode(fs_volume* volume, ino_t id, fs_vnode* vnode, int* _type, uint32* _flags,
	bool reenter)
{
	vnode->private_node = NULL;
	vnode->ops = &gVnodeOps;
	_flags = 0;

	VMWNode* node = gRootNode->GetChild(id);

	if (node == NULL)
		return B_ENTRY_NOT_FOUND;

	vnode->private_node = node;

	char* path_buffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (path_buffer == NULL)
		return B_NO_MEMORY;

	ssize_t length = node->CopyPathTo(path_buffer, B_PATH_NAME_LENGTH);
	if (length < 0) {
		free(path_buffer);
		return B_BUFFER_OVERFLOW;
	}

	vmw_attributes attributes;
	bool is_dir;
	status_t status = gSharedFolders->GetAttributes(path_buffer, &attributes, &is_dir);

	free(path_buffer);

	if (status != B_OK)
		return status;

	*_type = 0;
	*_type |= (CAN_READ(attributes) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
	*_type |= (CAN_WRITE(attributes) ? S_IWUSR : 0);
	*_type |= (CAN_EXEC(attributes) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);
	*_type |= (is_dir ? S_IFDIR : S_IFREG);

	return B_OK;
}
