#include <stdlib.h>

#include <fs_info.h>

#include "vmwfs.h"

VMWNode* root_node;

vint32 mount_count = 0;

status_t
vmwfs_mount(fs_volume *_vol, const char *device, uint32 flags, const char *args, ino_t *_rootID)
{
	CALLED();
	if (device != NULL)
		return B_BAD_VALUE;

	if (atomic_add(&mount_count, 1))
		return B_UNSUPPORTED;

	shared_folders = new VMWSharedFolders(IO_SIZE);
	status_t ret = shared_folders->InitCheck();
	if (ret != B_OK) {
		atomic_add(&mount_count, -1);
		delete shared_folders;
		return ret;
	}

	root_node = new VMWNode("", NULL);

	if (root_node == NULL) {
		atomic_add(&mount_count, -1);
		return B_NO_MEMORY;
	}

	*_rootID = root_node->GetInode();

	_vol->private_volume = root_node;
	_vol->ops = &volume_ops;

	ret = publish_vnode(_vol, *_rootID, (void*)_vol->private_volume,
			&vnode_ops, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH | S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH, 0);
	if (ret != B_OK)
		atomic_add(&mount_count, -1);

	return ret;
}

status_t
vmwfs_unmount(fs_volume* volume)
{
	CALLED();
	delete root_node;
	delete shared_folders;

	atomic_add(&mount_count, -1);

	return B_OK;
}

status_t
vmwfs_read_fs_info(fs_volume* volume, struct fs_info* info)
{
	CALLED();
	info->flags = B_FS_IS_PERSISTENT;
	info->block_size = FAKE_BLOCK_SIZE;
	info->io_size = IO_SIZE;
	info->total_blocks = 4 * 1024 * 1024; // 2GB
	info->free_blocks = info->total_blocks;
	info->total_nodes = info->block_size;
	info->free_nodes = info->block_size;

	strcpy(info->device_name, "");
	strcpy(info->volume_name, "VMW Shared Folders");

	return B_OK;
}

status_t
vmwfs_write_fs_info(fs_volume* volume, const struct fs_info* info, uint32 mask)
{
	CALLED();
	// TODO : Store volume name ?
	return B_OK;
}

status_t
vmwfs_get_vnode(fs_volume* volume, ino_t id, fs_vnode* vnode, int* _type, uint32* _flags, bool reenter)
{
	CALLED();
	vnode->private_node = NULL;
	vnode->ops = &vnode_ops;
	_flags = 0;

	VMWNode* node = root_node->GetChild(id);

	if (node == NULL)
		return B_ENTRY_NOT_FOUND;

	vnode->private_node = node;

	const char* path = node->GetPath();
	if (path == NULL)
		return B_NO_MEMORY;

	vmw_attributes attributes;
	bool is_dir;
	status_t ret = shared_folders->GetAttributes(path, &attributes, &is_dir);
	if (ret != B_OK)
		return ret;

	*_type = 0;
	*_type |= (CAN_READ(attributes) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
	*_type |= (CAN_WRITE(attributes) ? S_IWUSR : 0);
	*_type |= (CAN_EXEC(attributes) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);
	*_type |= (is_dir ? S_IFDIR : S_IFREG);

	return B_OK;
}
