#ifndef VMW_FS_PARTITIONS_DETECT_H
#define VMW_FS_PARTITIONS_DETECT_H

float vmwfs_identify_partition(int fd, partition_data* partition, void** _cookie);
status_t vmwfs_scan_partition(int fd, partition_data* partition, void* _cookie);
void vmwfs_free_identify_partition_cookie(partition_data* partition, void* _cookie);

#endif
