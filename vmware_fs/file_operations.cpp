#include "vmwfs.h"


status_t
vmwfs_create(fs_volume* volume, fs_vnode* dir, const char* name, int openMode, int /*permissions*/,
	void** _cookie, ino_t* _newVnodeID)
{
	VMWNode* node = (VMWNode*)dir->private_node;

	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH, name);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	file_handle* cookie = (file_handle*)malloc(sizeof(file_handle));
	if (cookie == NULL) {
		free(pathBuffer);
		return B_NO_MEMORY;
	}

	status_t status = gSharedFolders->OpenFile(pathBuffer, openMode | O_CREAT, cookie);
	free(pathBuffer);

	if (status != B_OK) {
		free(cookie);
		return status;
	}

	*_cookie = cookie;

	VMWNode* newNode = node->GetChild(name);
	if (newNode == NULL)
		return B_NO_MEMORY;

	*_newVnodeID = newNode->GetInode();

	return get_vnode(volume, newNode->GetInode(), NULL);
}


status_t
vmwfs_open(fs_volume* volume, fs_vnode* vnode, int openMode, void** _cookie)
{
	VMWNode* node = (VMWNode*)vnode->private_node;

	char* pathBuffer = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathBuffer == NULL)
		return B_NO_MEMORY;

	ssize_t length = node->CopyPathTo(pathBuffer, B_PATH_NAME_LENGTH);
	if (length < 0) {
		free(pathBuffer);
		return B_BUFFER_OVERFLOW;
	}

	file_handle* cookie = (file_handle*)malloc(sizeof(file_handle));
	if (cookie == NULL) {
		free(pathBuffer);
		return B_NO_MEMORY;
	}

	*_cookie = cookie;

	if (length == 0) {
		// We allow opening the root node as a file because it allows the volume
		// to be shown in Tracker.
		*cookie = (file_handle)(-1);
		free(pathBuffer);
		return B_OK;
	}

	status_t status = gSharedFolders->OpenFile(pathBuffer, openMode, cookie);
	free(pathBuffer);

	if (status != B_OK) {
		free(cookie);
		return status;
	}

	return B_OK;
}


status_t
vmwfs_close(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	if (*(file_handle*)cookie == (file_handle)(-1))
		return B_OK;
	return gSharedFolders->CloseFile(*(file_handle*)cookie);
}


status_t
vmwfs_free_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	free(cookie);
	return B_OK;
}


status_t
vmwfs_read(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, void* buffer,
	size_t* length)
{
	if (pos < 0)
		return B_BAD_VALUE;

	status_t status;
	uint32 toRead;
	size_t read = 0;

	do {
		toRead = static_cast<uint32>((*length - read) < IO_SIZE ? *length - read : IO_SIZE);
		status = gSharedFolders->ReadFile(*(file_handle*)cookie, pos + read, (uint8*)buffer + read,
			&toRead);

		read += toRead;
	} while (toRead != 0 && read < *length && status == B_OK); // toRead == 0 means EOF

	*length = read;

	return status;
}


status_t
vmwfs_write(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, const void* buffer,
	size_t* length)
{
	if (pos < 0)
		return B_BAD_VALUE;

	size_t written = 0;
	status_t status = B_OK;

	while (written < *length && status == B_OK) {
		uint32 toWrite
			= static_cast<uint32>((*length - written) < IO_SIZE ? *length - written : IO_SIZE);

		status = gSharedFolders->WriteFile(*(file_handle*)cookie, pos + written,
			(const uint8*)buffer + written, &toWrite);
		written += toWrite;
	}

	*length = written;

	return status;
}
