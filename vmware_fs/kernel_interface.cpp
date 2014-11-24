#include "vmwfs.h"

#include "dir_operations.h"
#include "file_operations.h"
#include "vnode_operations.h"
#include "volume_operations.h"

VMWSharedFolders* shared_folders;

status_t
vmw_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}

fs_volume_ops volume_ops = {
	&vmwfs_unmount,

	&vmwfs_read_fs_info,
	&vmwfs_write_fs_info,
	NULL,						//vmwfs_sync

	&vmwfs_get_vnode,

	/* index directory & index operations */
	NULL,						//vmwfs_open_index_dir
	NULL,						//vmwfs_close_index_dir
	NULL,						//vmwfs_free_index_dir_cookie
	NULL,						//vmwfs_read_index_dir
	NULL,						//vmwfs_rewind_index_dir

	NULL,						//vmwfs_create_index
	NULL,						//vmwfs_remove_index
	NULL,						//vmwfs_read_index_stat

	/* query operations */
	NULL,						//vmwfs_open_query
	NULL,						//vmwfs_close_query
	NULL,						//vmwfs_free_query_cookie
	NULL,						//vmwfs_read_query
	NULL,						//vmwfs_rewind_query

	/* support for FS layers */
	NULL,						//vmwfs_all_layers_mounted
	NULL,						//vmwfs_create_sub_vnode
	NULL,						//vmwfs_delete_sub_vnode
};

fs_vnode_ops vnode_ops = {
	/* vnode operations */
	&vmwfs_lookup,
	NULL,						//vmwfs_get_vnode_name

	&vmwfs_put_vnode,
	&vmwfs_remove_vnode,

	/* VM file access */
	NULL,						//vmwfs_can_page
	NULL,						//vmwfs_read_pages
	NULL,						//vmwfs_write_pages

	/* asynchronous I/O */
	NULL,						//vmwfs_io
	NULL,						//vmwfs_cancel_io

	/* cache file access */
	NULL,						//vmwfs_get_file_map

	/* common operations */
	NULL,						//vmwfs_ioctl
	NULL,						//vmwfs_set_flags
	NULL,						//vmwfs_select
	NULL,						//vmwfs_deselect
	NULL,						//vmwfs_fsync

	NULL,						//vmwfs_read_symlink
	NULL,						//vmwfs_create_symlink

	NULL,						//vmwfs_link
	&vmwfs_unlink,
	&vmwfs_rename,

	&vmwfs_access,
	&vmwfs_read_stat,
	&vmwfs_write_stat,
	NULL,

	/* file operations */
	&vmwfs_create,
	&vmwfs_open,
	&vmwfs_close,
	&vmwfs_free_cookie,
	&vmwfs_read,
	&vmwfs_write,

	/* directory operations */
	&vmwfs_create_dir,
	&vmwfs_remove_dir,
	&vmwfs_open_dir,
	&vmwfs_close_dir,
	&vmwfs_free_dir_cookie,
	&vmwfs_read_dir,
	&vmwfs_rewind_dir,

	/* attribute directory operations */
	NULL,						//vmwfs_open_attr_dir
	NULL,						//vmwfs_close_attr_dir
	NULL,						//vmwfs_free_attr_dir_cookie
	NULL,						//vmwfs_read_attr_dir
	NULL,						//vmwfs_rewind_attr_dir

	/* attribute operations */
	NULL,						//vmwfs_create_attr
	NULL,						//vmwfs_open_attr
	NULL,						//vmwfs_close_attr
	NULL,						//vmwfs_free_attr_cookie
	NULL,						//vmwfs_read_attr
	NULL,						//vmwfs_write_attr

	NULL,						//vmwfs_read_attr_stat
	NULL,						//vmwfs_write_attr_stat
	NULL,						//vmwfs_rename_attr
	NULL,						//vmwfs_remove_attr

	/* support for node and FS layers */
	NULL,						//vmwfs_create_special_node
	NULL,						//vmwfs_get_super_vnode
};

static file_system_module_info vmw_file_system = {
	{
		"file_systems/vmwfs" B_CURRENT_FS_API_VERSION,
		0,
		vmw_std_ops,
	},

	"vmwfs",						// short_name
	"VMware ShF",					// pretty_name
	B_DISK_SYSTEM_SUPPORTS_WRITING,	// DDM flags

	// scanning
	NULL,						//vmwfs_identify_partition
	NULL,						//vmwfs_scan_partition
	NULL,						//vmwfs_free_identify_partition_cookie
	NULL,						//free_partition_content_cookie

	&vmwfs_mount,
};

module_info *modules[] = {
	(module_info *)&vmw_file_system,
	NULL,
};
