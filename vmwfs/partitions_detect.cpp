#include "vmwfs.h"

float
vmwfs_identify_partition(int fd, partition_data* partition, void** _cookie)
{
	CALLED();
	return -1;
}

status_t
vmwfs_scan_partition(int fd, partition_data* partition, void* _cookie)
{
	CALLED();
	return B_ERROR;
}

void
vmwfs_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	CALLED();
}
