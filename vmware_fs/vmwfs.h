#ifndef VMW_FS_KERNEL_INTERFACE_H
#define VMW_FS_KERNEL_INTERFACE_H

#include <SupportDefs.h>
#include <KernelExport.h>

#include <fs_interface.h>

#include "VMWNode.h"
#include "VMWSharedFolders.h"

#define FAKE_BLOCK_SIZE 512
#define IO_SIZE 4096

extern VMWSharedFolders* shared_folders;

extern fs_volume_ops volume_ops;
extern fs_vnode_ops vnode_ops;

extern dev_t device_id;

#endif
