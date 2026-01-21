#ifndef VMW_FS_KERNEL_INTERFACE_H
#define VMW_FS_KERNEL_INTERFACE_H

#include <stdlib.h>

#include <SupportDefs.h>
#include <fs_interface.h>

#include "VMWNode.h"
#include "VMWSharedFolders.h"

#define FAKE_BLOCK_SIZE 512
#define IO_SIZE 4096

extern VMWSharedFolders* gSharedFolders;

extern dev_t gDeviceId;

#endif
