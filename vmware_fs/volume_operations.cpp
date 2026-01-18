#include <stdlib.h>

#include <fs_info.h>

#include "vmwfs.h"

VMWNode* root_node;

int32 mount_count = 0;

dev_t device_id;

status_t
vmwfs_mount(fs_volume *_vol, const char *device, uint32 flags, const char *args, ino_t *_rootID)
{
	if (device != NULL)
		return B_BAD_VALUE;

	if (atomic_add(&mount_count, 1))
		return B_UNSUPPORTED;

	shared_folders = new VMWSharedFolders(IO_SIZE);
	if (shared_folders == NULL || shared_folders->InitCheck() != B_OK) {
		atomic_add(&mount_count, -1);
		delete shared_folders;
		return (shared_folders == NULL ? B_NO_MEMORY : shared_folders->InitCheck());
	}

	root_node = new VMWNode("", NULL);

	if (root_node == NULL) {
		delete root_node;
		atomic_add(&mount_count, -1);
		return B_NO_MEMORY;
	}

	*_rootID = root_node->GetInode();

	_vol->private_volume = root_node;
	_vol->ops = &volume_ops;
	device_id = _vol->id;

	status_t ret = publish_vnode(_vol, *_rootID, (void*)_vol->private_volume,
			&vnode_ops, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH | S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH, 0);
	if (ret != B_OK)
		atomic_add(&mount_count, -1);

	return ret;
}

status_t
vmwfs_unmount(fs_volume* volume)
{
	put_vnode(volume, root_node->GetInode());
	delete root_node;
	delete shared_folders;

	atomic_add(&mount_count, -1);

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
	strcpy(info->volume_name, "VMW Shared Folders");

	return B_OK;
}

status_t
vmwfs_write_fs_info(fs_volume* volume, const struct fs_info* info, uint32 mask)
{
	// TODO : Store volume name ?
	return B_OK;
}

status_t
vmwfs_get_vnode(fs_volume* volume, ino_t id, fs_vnode* vnode, int* _type, uint32* _flags, bool reenter)
{
	vnode->private_node = NULL;
	vnode->ops = &vnode_ops;
	_flags = 0;

	VMWNode* node = root_node->GetChild(id);

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
	status_t ret = shared_folders->GetAttributes(path_buffer, &attributes, &is_dir);
	
	free(path_buffer);
	
	if (ret != B_OK)
		return ret;

	*_type = 0;
	*_type |= (CAN_READ(attributes) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
	*_type |= (CAN_WRITE(attributes) ? S_IWUSR : 0);
	*_type |= (CAN_EXEC(attributes) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);
	*_type |= (is_dir ? S_IFDIR : S_IFREG);

	return B_OK;
}
