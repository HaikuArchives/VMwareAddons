#ifndef VMW_FS_FILE_OPERATIONS_H
#define VMW_FS_FILE_OPERATIONS_H

status_t	vmwfs_create(fs_volume* volume, fs_vnode* dir, const char* name, int openMode, int perms, void** _cookie, ino_t* _newVnodeID);
status_t	vmwfs_open(fs_volume* volume, fs_vnode* vnode, int openMode, void** _cookie);
status_t	vmwfs_close(fs_volume* volume, fs_vnode* vnode, void* cookie);
status_t	vmwfs_free_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie);
status_t	vmwfs_read(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, void* buffer, size_t* length);
status_t	vmwfs_write(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, const void* buffer, size_t* length);

#endif
