#ifndef VMW_FS_VOL_OPERATIONS_H
#define VMW_FS_VOL_OPERATIONS_H

status_t	vmwfs_mount(fs_volume *_vol, const char *device, uint32 flags, const char *args, ino_t *_rootID);
status_t	vmwfs_unmount(fs_volume* volume);

status_t	vmwfs_read_fs_info(fs_volume* volume, struct fs_info* info);
status_t	vmwfs_write_fs_info(fs_volume* volume, const struct fs_info* info, uint32 mask);

status_t	vmwfs_get_vnode(fs_volume* volume, ino_t id, fs_vnode* vnode, int* _type, uint32* _flags, bool reenter);

#endif
