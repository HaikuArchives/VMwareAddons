/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "VMWSharedFolders.h"

#include <stdlib.h>

#include <KernelExport.h>


#define ASSERT(x) if (!(x)) panic("ASSERT FAILED : " #x);

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

#define SET_8(offset, val) *(uint8*)(fRPCBuffer + offset) = (uint8)val; offset += 1
#define SET_32(offset, val) *(uint32*)(fRPCBuffer + offset) = (uint32)val; offset += 4
#define SET_64(offset, val) *(uint64*)(fRPCBuffer + offset) = (uint64)val; offset += 8

#define SIZE_8 1
#define SIZE_32 4
#define SIZE_64 8
#define SIZE_START 6


VMWSharedFolders::VMWSharedFolders(size_t maxIOSize)
{
	if (!fBackdoor.InVMware()) {
		fInitCheck = B_UNSUPPORTED;
		return;
	}

	fInitCheck = fBackdoor.OpenRPCChannel();
	if (fInitCheck != B_OK)
		return;

	fInitCheck = fBackdoor.SendMessage("f ", true);
	if (fInitCheck != B_OK)
		return;

	// maxIOSize indicates the max buffer size used with ReadFile and WriteFile.
	// We need additional room on the send/receive buffer for the command header.
	fRPCBufferSize = maxIOSize + SIZE_START + SIZE_32 + SIZE_32 + SIZE_8 + SIZE_64 + SIZE_32;
	fRPCBuffer = (char*)malloc(fRPCBufferSize);
	if (fRPCBuffer == NULL) {
		fInitCheck = B_NO_MEMORY;
		return;
	}

	fLock = create_sem(1, "vmwfs_lock");
	if (fLock < B_OK) {
		free(fRPCBuffer);
		fInitCheck = fLock;
	}
}


VMWSharedFolders::~VMWSharedFolders()
{
	delete_sem(fLock);
	free(fRPCBuffer);
	fBackdoor.CloseRPCChannel();
}


status_t
VMWSharedFolders::OpenFile(const char* path, int openMode, file_handle* handle)
{
	// Command string :
	// 0) Magic value (6 bytes)
	// 1) Command number (32-bits)
	// 2) Access mode : 0 => RO, 1 => WO, 2 => RW (32-bits)
	// 3) Open mode (32-bits)
	// 4) Permissions : 1 => exec, 2 => write, 4 => read (8-bits)
	// 5) The path length, with ending null (32-bits)
	// 6) The path itself (with / path delimiters replaced by null characters)

	const size_t pathLength = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_32 + SIZE_8 + SIZE_32 + pathLength + 1;

	if (length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_OPEN_FILE);
	SET_32(pos, openMode & 3);

	uint32 vmwOpenMode;
	if ((openMode & O_TRUNC) == O_TRUNC)
		vmwOpenMode = 0x04;
	else if ((openMode & O_CREAT) == O_CREAT)
		if ((openMode & O_EXCL) == O_EXCL)
			vmwOpenMode = 0x03;
		else
			vmwOpenMode = 0x02;
	else
		vmwOpenMode = 0;

	SET_32(pos, vmwOpenMode);
	SET_8(pos, 0x06);
	SET_32(pos, pathLength);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length != 14) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));
	*handle = *(uint32*)(fRPCBuffer + 10);

	release_sem(fLock);
	return status;
}


status_t
VMWSharedFolders::ReadFile(file_handle handle, uint64 offset, void* readBuffer, uint32* readLength)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Handle (32-bits)
	// 3) Offset (64-bits)
	// 4) Length (32-bits)

	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_64 + SIZE_32;

	ASSERT(*readLength + length <= fRPCBufferSize);
	if (*readLength + length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_READ_FILE);
	SET_32(pos, handle);
	SET_64(pos, offset);
	uint32 maxLength = *readLength;
	SET_32(pos, *readLength);

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length < 14) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	uint32 returnedLength = *(uint32*)(fRPCBuffer + 10);
	if (returnedLength > maxLength) {
		release_sem(fLock);
		return B_BUFFER_OVERFLOW;
	}

	*readLength = returnedLength;

	if (user_memcpy(readBuffer, fRPCBuffer + 14, *readLength) != B_OK) {
		release_sem(fLock);
		return B_BAD_ADDRESS;
	}

	release_sem(fLock);
	return status;
}


status_t
VMWSharedFolders::WriteFile(file_handle handle, uint64 offset, const void* writeBuffer,
	uint32* writeLength)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Handle (32-bits)
	// 3) ???? (8-bits)
	// 4) Offset (64-bits)
	// 5) Length (32-bits)

	// /!\ should be changed in the constructor, too
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_8 + SIZE_64 + SIZE_32 + *writeLength;

	ASSERT(length <= fRPCBufferSize);
	if (length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_WRITE_FILE);
	SET_32(pos, handle);
	SET_8(pos, 0);
	SET_64(pos, offset);
	SET_32(pos, *writeLength);

	if (user_memcpy(fRPCBuffer + pos, writeBuffer, *writeLength) != B_OK) {
		release_sem(fLock);
		return B_BAD_ADDRESS;
	}

	pos += *writeLength;

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length != 14) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));
	*writeLength = *(uint32*)(fRPCBuffer + 10);

	release_sem(fLock);
	return status;
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

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	release_sem(fLock);
	return status;
}


status_t
VMWSharedFolders::OpenDir(const char* path, folder_handle* handle)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Path length (32-bits)
	// 3) The path itself (with / path delimiters replaced by null characters)

	const size_t pathLength = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + pathLength + 1;

	if (length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_OPEN_DIR);
	SET_32(pos, pathLength);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length != 14) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));
	*handle = *(uint32*)(fRPCBuffer + 10);

	release_sem(fLock);
	return status;
}


status_t
VMWSharedFolders::ReadDir(folder_handle handle, uint32 index, char* name, size_t maxLength)
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

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length < 59) {
		release_sem(fLock);
		return B_ERROR;
	}

	size_t nameLength = *(uint32*)(fRPCBuffer + 55);
	if (nameLength == 0) {
		release_sem(fLock);
		return B_ENTRY_NOT_FOUND;
	}

	if (nameLength > maxLength - 1) {
		release_sem(fLock);
		return B_BUFFER_OVERFLOW;
	}

	strncpy(name, fRPCBuffer + 59, maxLength - 1);
	name[nameLength] = '\0';

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

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);
	release_sem(fLock);
	return status;
}


status_t
VMWSharedFolders::GetAttributes(const char* path, vmw_attributes* attributes, bool* isDir)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Path length (32-bits)
	// 3) The path itself (with / path delimiters replaced by null characters)

	const size_t pathLength = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + pathLength + 1;

	if (length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_GET_ATTR);
	SET_32(pos, pathLength);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length == 10) {
		release_sem(fLock);
		return B_ENTRY_NOT_FOUND;
	}

	if (length != 55) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (isDir != NULL)
		*isDir = (*(uint32*)(fRPCBuffer + 10) == 0 ? false : true);

	if (attributes != NULL)
		memcpy(attributes, fRPCBuffer + 14, sizeof(vmw_attributes));

	release_sem(fLock);
	return status;
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

	const size_t pathLength = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + SIZE_8 + sizeof(vmw_attributes) + SIZE_32
		+ pathLength + 1;

	if (length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_SET_ATTR);
	SET_32(pos, mask);
	SET_8(pos, 0);
	memcpy(fRPCBuffer + pos, attributes, sizeof(vmw_attributes));
	pos += sizeof(vmw_attributes);
	SET_32(pos, pathLength);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length != 10) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));

	release_sem(fLock);
	return status;
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

	const size_t pathLength = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_8 + SIZE_32 + pathLength + 1;

	if (length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_NEW_DIR);
	SET_32(pos, 0);
	pos -= SIZE_32;
	SET_8(pos, mode);
	SET_32(pos, pathLength);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length != 10) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));

	release_sem(fLock);
	return status;
}


status_t
VMWSharedFolders::Delete(const char* path, bool isDir)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Path length (32-bits)
	// 3) The path itself (with / path delimiters replaced by null characters)

	const size_t pathLength = strlen(path);
	size_t length = SIZE_START + SIZE_32 + SIZE_32 + pathLength + 1;

	if (length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, isDir ? VMW_CMD_DEL_DIR : VMW_CMD_DEL_FILE);
	SET_32(pos, pathLength);
	CopyPath(path, &pos);

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length != 10) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));

	release_sem(fLock);
	return status;
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
VMWSharedFolders::Move(const char* pathOrig, const char* pathDest)
{
	// Command string :
	// 0) Magic value (6 bytes, in BuildCommand)
	// 1) Command number (32-bits)
	// 2) Original path length (32-bits)
	// 3) The path itself (with / path delimiters replaced by null characters)
	// 4) Destination path length (32-bits)
	// 5) The path itself (with / path delimiters replaced by null characters)


	const size_t pathOrigLength = strlen(pathOrig);
	const size_t pathDestLength = strlen(pathDest);
	size_t length
		= SIZE_START + SIZE_32 + SIZE_32 + pathOrigLength + 1 + SIZE_32 + pathDestLength + 1;

	if (length > fRPCBufferSize)
		return B_BUFFER_OVERFLOW;

	if (acquire_sem(fLock) != B_OK)
		return B_ERROR;

	size_t pos = StartCommand();
	SET_32(pos, VMW_CMD_MOVE_FILE);
	SET_32(pos, pathOrigLength);
	CopyPath(pathOrig, &pos);
	SET_32(pos, pathDestLength);
	CopyPath(pathDest, &pos);

	ASSERT(pos == length);

	status_t status = fBackdoor.SendAndGet(fRPCBuffer, &length, fRPCBufferSize);

	if (status != B_OK) {
		release_sem(fLock);
		return status;
	}

	if (length != 10) {
		release_sem(fLock);
		return B_ERROR;
	}

	status = ConvertStatus(*(uint32*)(fRPCBuffer + 6));
	release_sem(fLock);
	return status;
}


size_t
VMWSharedFolders::StartCommand()
{
	const char startBytes[] = { 'f', ' ', '\0', '\0', '\0', '\0' };

	memcpy(fRPCBuffer, startBytes, sizeof(startBytes));

	return sizeof(startBytes);
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
	char* dest = fRPCBuffer + *pos;
	while (*path != '\0') {
		*dest = (*path == '/' ? '\0' : *path);
		dest++;
		path++;
		(*pos)++;
	}
	*dest = '\0';
	(*pos)++;
}
