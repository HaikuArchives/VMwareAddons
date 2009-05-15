/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_SHARED_H
#define VMW_SHARED_H

#include <SupportDefs.h>

#include "VMWBackdoor.h"

typedef uint32 file_handle;
typedef uint32 folder_handle;

typedef struct vmw_attributes {
	uint64	size;
	
	uint64	c_time;
	uint64	a_time;
	uint64	m_time;
	uint64	s_time; // ?
	
	uint8	perms;
} __attribute__((packed)) vmw_attributes;

#define MSK_READ 0x04
#define MSK_WRITE 0x02
#define MSK_EXEC 0x01

#define CAN_READ(x) (((x).perms & MSK_READ) == MSK_READ)
#define CAN_WRITE(x) (((x).perms & MSK_WRITE) == MSK_WRITE)
#define CAN_EXEC(x) (((x).perms & MSK_EXEC) == MSK_EXEC)

#define VMW_SET_SIZE	0x01
#define VMW_SET_CTIME	0x02
#define VMW_SET_ATIME	0x04
#define VMW_SET_UTIME	0x08
#define VMW_SET_STIME	0x10
#define VMW_SET_PERMS	0x20

class VMWSharedFolders {
public:
					VMWSharedFolders();
	virtual			~VMWSharedFolders();
	
	status_t		InitCheck() const { return init_check; }
	status_t		OpenFile(const char* path, int open_mode, file_handle* handle);
	status_t		ReadFile(file_handle handle, uint64 offset, void* buffer, uint32* length);
	status_t		WriteFile(file_handle handle, uint64 offset, const void* buffer, uint32* length);
	status_t		CloseFile(file_handle handle);
	status_t		OpenDir(const char* path, folder_handle* handle);
	status_t		ReadDir(folder_handle handle, uint32 index, char* name, size_t max_length);
	status_t		CloseDir(folder_handle handle);
	status_t		GetAttributes(const char* path, vmw_attributes* attributes = NULL, bool* is_dir  = NULL);
	status_t		SetAttributes(const char* path, const vmw_attributes* attributes, uint32 mask);
	status_t		CreateDir(const char* path, uint8 mode);
	status_t		DeleteFile(const char* path);
	status_t		DeleteDir(const char* path);
	status_t		Move(const char* path_orig, const char* path_dest);

private:
	status_t		Delete(const char* path, bool is_dir);
	static off_t	BuildCommand(char* cmd_buffer, uint32 command, uint32 param);
	status_t		ConvertStatus(int vmw_status);
	void			CopyPath(const char* path, char* dest, off_t* pos);

	VMWCoreBackdoor	backdoor;
	status_t		init_check;
};

#endif
