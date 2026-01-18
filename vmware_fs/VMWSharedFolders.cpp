/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWSharedFolders.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define ASSERT(x) if (!(x)) panic("ASSERT FAILED : " #x);

#include <KernelExport.h>

enum {
	VMW_CMD_OPEN_FILE = 0,
	VMW_CMD_READ_FILE,
	VMW_CMD_WRITE_FILE,
	VMW_CMD_CLOSE_FILE,
	VMW_CMD_OPEN_DIR,
	VMW_CMD_READ_DIR,
	VMW_CMD_CLOSE_DIR,
	VMW_CMD_GET_ATTR,
	VMW_CMD_SET_ATTR,
	VMW_CMD_NEW_DIR,
	VMW_CMD_DEL_FILE,
	VMW_CMD_DEL_DIR,
	VMW_CMD_MOVE_FILE
};

#define SET_8(offset, val) *(uint8*)(rpc_buffer + offset) = (uint8)val; offset += 1
#define SET_32(offset, val) *(uint32*)(rpc_buffer + offset) = (uint32)val; offset += 4
#define SET_64(offset, val) *(uint64*)(rpc_buffer + offset) = (uint64)val; offset += 8

#define SIZE_8 1
#define SIZE_32 4
#define SIZE_64 8
#define SIZE_START 6

VMWSharedFolders::VMWSharedFolders(size_t max_io_size)
{
	if (!backdoor.InVMware()) {
		init_check = B_UNSUPPORTED;
		return;
	}

	init_check = backdoor.OpenRPCChannel();

	if (init_check != B_OK)
		return;

	init_check = backdoor.SendMessage("f ", true);

	if (init_check != B_OK)
		return;

	// max_io_size indicates the max buffer size used with ReadFile and WriteFile.
	// We need additional room on the send/receive buffer for the command header.
	rpc_buffer_size = max_io_size + SIZE_START + SIZE_32 + SIZE_32 + SIZE_8 + SIZE_64 + SIZE_32;
	rpc_buffer = (char*)malloc(rpc_buffer_size);
	if (rpc_buffer == NULL) {
		init_check = B_NO_MEMORY;
		return;
	}
	
	fLock = create_sem(1, "vmwfs_lock");
	if (fLock < B_OK) {
		free(rpc_buffer);
		init_check = fLock;
	}
}

VMWSharedFolders::~VMWSharedFolders()
{
	delete_sem(fLock);
	free(rpc_buffer);
	backdoor.CloseRPCChannel();
}

status_t
VMWSharedFolders::OpenFile(const char* path, int open_mode, file_handle* handle)
{
	// Command string :
	// 0) Magic value (6 bytes)
	// 1) Command number (32-bits)
	// 2) Access mode : 0 => RO, 1 => WO, 2 => RW (32-bits)
	// 3) Open mode (32-bits)
	// 4) Permissions : 1 => exec, 2 => write, 4 => read (8-bits)
	// 5) The path length, with ending null (32-bits)
	// 6) The path itself (with / path delimiters replaced by null characters)

	const size_t path_length = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_32 + SIZE_8 + SIZE_32 + path_length + 1;

	if (length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_OPEN_FILE);
	SET_32(pos, open_mode & 3);

	uint32 vmw_openmode;
	if ((open_mode & O_TRUNC) == O_TRUNC) {
		vmw_openmode = 0x04;
	} else if ((open_mode & O_CREAT) == O_CREAT) {
		if ((open_mode & O_EXCL) == O_EXCL)
			vmw_openmode = 0x03;
		else
			vmw_openmode = 0x02;
	} else {
		vmw_openmode = 0;
	}

	SET_32(pos, vmw_openmode);
	SET_8(pos, 0x06);
	SET_32(pos, path_length);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length != 14) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));
	*handle = *(uint32*)(rpc_buffer + 10);

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::ReadFile(file_handle handle, uint64 offset, void* read_buffer, uint32* read_length)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Handle (32-bits)
	// 3) Offset (64-bits)
	// 4) Length (32-bits)

	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_64 + SIZE_32;

	ASSERT(*read_length + length <= rpc_buffer_size);
	if (*read_length + length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_READ_FILE);
	SET_32(pos, handle);
	SET_64(pos, offset);
	uint32 max_length = *read_length;
	SET_32(pos, *read_length);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length < 14) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	uint32 returned_length = *(uint32*)(rpc_buffer + 10);
	if (returned_length > max_length) {
		release_sem(fLock);
		return B_BUFFER_OVERFLOW;
	}

	*read_length = returned_length;

	if (user_memcpy(read_buffer, rpc_buffer + 14, *read_length) != B_OK) {
		release_sem(fLock);
		return B_BAD_ADDRESS;
	}

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::WriteFile(file_handle handle, uint64 offset, const void* write_buffer, uint32* write_length)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Handle (32-bits)
	// 3) ???? (8-bits)
	// 4) Offset (64-bits)
	// 5) Length (32-bits)

	// /!\ should be changed in the constructor, too
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_8 + SIZE_64 + SIZE_32 + *write_length;

	ASSERT(length <= rpc_buffer_size);
	if (length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_WRITE_FILE);
	SET_32(pos, handle);
	SET_8(pos, 0);
	SET_64(pos, offset);
	SET_32(pos, *write_length);

	if (user_memcpy(rpc_buffer + pos, write_buffer, *write_length) != B_OK) {
		release_sem(fLock);
		return B_BAD_ADDRESS;
	}

	pos += *write_length;

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length != 14) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));
	*write_length = *(uint32*)(rpc_buffer + 10);

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::CloseFile(file_handle handle)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Handle (32-bits)

	size_t length = SIZE_START + SIZE_32 + SIZE_32;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_CLOSE_FILE);
	SET_32(pos, handle);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::OpenDir(const char* path, folder_handle* handle)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Path length (32-bits)
	// 3) The path itself (with / path delimiters replaced by null characters)

	const size_t path_length = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + path_length + 1;

	if (length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_OPEN_DIR);
	SET_32(pos, path_length);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length != 14) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));
	*handle = *(uint32*)(rpc_buffer + 10);

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::ReadDir(folder_handle handle, uint32 index, char* name, size_t max_length)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Handle (32-bits)
	// 3) Max name length (32-bits)

	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_32;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_READ_DIR);
	SET_32(pos, handle);
	SET_32(pos, index);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length < 59) {
		release_sem(fLock);
		return B_ERROR;
	}

	size_t name_length = *(uint32*)(rpc_buffer + 55);
	if (name_length == 0) {
		release_sem(fLock);
		return B_ENTRY_NOT_FOUND;
	}

	if (name_length > max_length - 1) {
		release_sem(fLock);
		return B_BUFFER_OVERFLOW;
	}

	strncpy(name, rpc_buffer + 59, max_length - 1);
	name[name_length] = '\0';

	release_sem(fLock);
	return B_OK;
}

status_t
VMWSharedFolders::CloseDir(folder_handle handle)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Handle (32-bits)

	size_t length = SIZE_START + SIZE_32 + SIZE_32;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_CLOSE_DIR);
	SET_32(pos, handle);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);
	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::GetAttributes(const char* path, vmw_attributes* attributes, bool* is_dir)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Path length (32-bits)
	// 3) The path itself (with / path delimiters replaced by null characters)

	const size_t path_length = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + path_length + 1;

	if (length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_GET_ATTR);
	SET_32(pos, path_length);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length == 10) {
		release_sem(fLock);
		return B_ENTRY_NOT_FOUND;
	}

	if (length != 55) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (is_dir != NULL)
		*is_dir = (*(uint32*)(rpc_buffer + 10) == 0 ? false : true);

	if (attributes != NULL)
		memcpy(attributes, rpc_buffer + 14, sizeof(vmw_attributes));

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::SetAttributes(const char* path, const vmw_attributes* attributes, uint32 mask)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Attribute mask (32-bits)
	// 3) ???? (8-bits)
	// 4) attributes
	// 5) Path length (32-bits)
	// 6) The path itself (with / path delimiters replaced by null characters)

	const size_t path_length = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_8 + sizeof(vmw_attributes) + SIZE_32 + path_length + 1;

	if (length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_SET_ATTR);
	SET_32(pos, mask);
	SET_8(pos, 0);
	memcpy(rpc_buffer + pos, attributes, sizeof(vmw_attributes));
	pos += sizeof(vmw_attributes);
	SET_32(pos, path_length);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length != 10) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::CreateDir(const char* path, uint8 mode)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) mode (8-bits)
	// 3) Path length (32-bits)
	// 4) The path itself (with / path delimiters replaced by null characters)

	const size_t path_length = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_8 + SIZE_32 + path_length + 1;

	if (length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_NEW_DIR);
	SET_32(pos, 0);
	pos -= SIZE_32;
	SET_8(pos, mode);
	SET_32(pos, path_length);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length != 10) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::Delete(const char* path, bool is_dir)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Path length (32-bits)
	// 3) The path itself (with / path delimiters replaced by null characters)

	const size_t path_length = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + path_length + 1;

	if (length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, (is_dir ? VMW_CMD_DEL_DIR : VMW_CMD_DEL_FILE));
	SET_32(pos, path_length);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length != 10) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));

	release_sem(fLock);
	return ret;
}

status_t
VMWSharedFolders::DeleteFile(const char* path)
{
	return Delete(path, false);
}

status_t
VMWSharedFolders::DeleteDir(const char* path)
{
	return Delete(path, true);
}

status_t
VMWSharedFolders::Move(const char* path_orig, const char* path_dest)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Original path length (32-bits)
	// 3) The path itself (with / path delimiters replaced by null characters)
	// 4) Destination path length (32-bits)
	// 5) The path itself (with / path delimiters replaced by null characters)


	const size_t path_orig_length = strlen(path_orig);
	const size_t path_dest_length = strlen(path_dest);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + path_orig_length + 1 \
		+ SIZE_32 + path_dest_length + 1;

	if (length > rpc_buffer_size)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_MOVE_FILE);
	SET_32(pos, path_orig_length);
	CopyPath(path_orig, &pos);
	SET_32(pos, path_dest_length);
	CopyPath(path_dest, &pos);

	ASSERT(pos == length);

	status_t ret = backdoor.SendAndGet(rpc_buffer, &length, rpc_buffer_size);

	if (ret != B_OK) {
		release_sem(fLock);
		return ret;
	}

	if (length != 10) {
		release_sem(fLock);
		return B_ERROR;
	}

	ret = ConvertStatus(*(uint32*)(rpc_buffer + 6));
	release_sem(fLock);
	return ret;
}

size_t
VMWSharedFolders::StartCommand()
{
	const char start_bytes[] = { 'f', ' ', '\0', '\0', '\0', '\0' };

	memcpy(rpc_buffer, start_bytes, sizeof(start_bytes));

	return sizeof(start_bytes);
}

status_t
VMWSharedFolders::ConvertStatus(int vmw_status)
{
	switch (vmw_status) {
		case 0:		return B_OK;
		case 1:		return B_ENTRY_NOT_FOUND;
		case 3:		return B_PERMISSION_DENIED;
		case 4:		return B_FILE_EXISTS;
		case 6:		return B_DIRECTORY_NOT_EMPTY;
		case 8:		return B_NAME_IN_USE;
		// 10 is returned when trying to change permissions of a folder.
		// Since the permissions are changed, I guess this is a success status...
		case 10:	return B_OK;
		default:
			dprintf("ConvertStatus: unknown status %d.\n", vmw_status);
			return B_ERROR;

	}
}

void
VMWSharedFolders::CopyPath(const char* path, size_t* pos)
{
	char* dest = rpc_buffer + *pos;
	while (*path != '\0') {
		*dest = (*path == '/' ? '\0' : *path);
		dest++;
		path++;
		(*pos)++;
	}
	*dest = '\0';
	(*pos)++;
}
