#ifndef VMW_FS_VNODE_OPERATIONS_H
#define VMW_FS_VNODE_OPERATIONS_H

status_t	vmwfs_lookup(fs_volume* volume, fs_vnode* dir, const char* name, ino_t* _id);
status_t	vmwfs_get_vnode_name(fs_volume* volume, fs_vnode* vnode, char* buffer, size_t bufferSize);

status_t	vmwfs_put_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter);
status_t	vmwfs_remove_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter);

status_t	vmwfs_unlink(fs_volume* volume, fs_vnode* dir, const char* name);
status_t	vmwfs_rename(fs_volume* volume, fs_vnode* fromDir, const char* fromName, fs_vnode* toDir, const char* toName);

status_t	vmwfs_access(fs_volume* volume, fs_vnode* vnode, int mode);
status_t	vmwfs_read_stat(fs_volume* volume, fs_vnode* vnode, struct stat* stat);
status_t	vmwfs_write_stat(fs_volume* volume, fs_vnode* vnode, const struct stat* stat, uint32 statMask);

#endif
