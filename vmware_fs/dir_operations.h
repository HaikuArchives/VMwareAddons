#ifndef VMW_FS_DIR_OPERATIONS_H
#define VMW_FS_DIR_OPERATIONS_H

status_t vmwfs_create_dir(fs_volume* volume, fs_vnode* parent, const char* name, int permissions);
status_t vmwfs_remove_dir(fs_volume* volume, fs_vnode* parent, const char* name);
status_t vmwfs_open_dir(fs_volume* volume, fs_vnode* vnode, void** _cookie);
status_t vmwfs_close_dir(fs_volume* volume, fs_vnode* vnode, void* cookie);
status_t vmwfs_free_dir_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie);
status_t vmwfs_read_dir(fs_volume* volume, fs_vnode* vnode, void* cookie, struct dirent* buffer,
	size_t bufferSize, uint32* _num);
status_t vmwfs_rewind_dir(fs_volume* volume, fs_vnode* vnode, void* cookie);

#endif
