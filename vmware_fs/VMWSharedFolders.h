/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_SHARED_H
#define VMW_SHARED_H

#include <SupportDefs.h>

#include "VMWCoreBackdoor.h"

typedef uint32 file_handle;
typedef uint32 folder_handle;

typedef struct vmw_attributes {
	uint64	size;

	uint64	c_time;
	uint64	a_time;
	uint64	m_time;
	uint64	s_time; // inode change time

	uint8	perms;
} __attribute__((packed)) vmw_attributes;

#define MSK_READ 0x04
#define MSK_WRITE 0x02
#define MSK_EXEC 0x01

#define CAN_READ(x) (((x).perms & MSK_READ) == MSK_READ)
#define CAN_WRITE(x) (((x).perms & MSK_WRITE) == MSK_WRITE)
#define CAN_EXEC(x) (((x).perms & MSK_EXEC) == MSK_EXEC)

#define VMW_SET_SIZE	0x01
#define VMW_SET_CRTIME	0x02
#define VMW_SET_ATIME	0x04
#define VMW_SET_MTIME	0x08
#define VMW_SET_CTIME	0x10
#define VMW_SET_PERMS	0x20

class VMWSharedFolders {
public:
					VMWSharedFolders(size_t max_io_size);
	virtual			~VMWSharedFolders();

	status_t		InitCheck() const { return init_check; }
	status_t		OpenFile(const char* path, int open_mode, file_handle* handle);
	status_t		ReadFile(file_handle handle, uint64 offset, void* read_buffer, uint32* read_length);
	status_t		WriteFile(file_handle handle, uint64 offset, const void* write_buffer, uint32* write_length);
	status_t		CloseFile(file_handle handle);
	status_t		OpenDir(const char* path, folder_handle* handle);
	status_t		ReadDir(folder_handle handle, uint32 index, char* name, size_t max_length);
	status_t		CloseDir(folder_handle handle);
	status_t		GetAttributes(const char* path, vmw_attributes* attributes = NULL, bool* is_dir = NULL);
	status_t		SetAttributes(const char* path, const vmw_attributes* attributes, uint32 mask);
	status_t		CreateDir(const char* path, uint8 mode);
	status_t		DeleteFile(const char* path);
	status_t		DeleteDir(const char* path);
	status_t		Move(const char* path_orig, const char* path_dest);

private:
	status_t		Delete(const char* path, bool is_dir);
	size_t			StartCommand();
	status_t		ConvertStatus(int vmw_status);
	void			CopyPath(const char* path, size_t* pos);

	VMWCoreBackdoor	backdoor;
	status_t		init_check;
	char*			rpc_buffer;
	size_t			rpc_buffer_size;
};

#endif
