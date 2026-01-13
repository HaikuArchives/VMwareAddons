/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "VMWAddOnsCleanup.h"

#include <Alert.h>

#include "VMWAddOnsSelectWindow.h"
#include "VMWAddOnsStatusWindow.h"
#include "VMWAddOnsTray.h"


#define MB (1024 * 1024)
#define BUF_SIZE (MB / 2)
#define MAX_FILES 200
#define MAX_FILE_SIZE 4096 // In MB


status_t
VMWAddOnsCleanup::ThreadLoop()
{
	fDevicesCount = 0;
	VMWAddOnsSelectWindow* selectWindow = new VMWAddOnsSelectWindow();
	selectWindow->Go(this);

	if (fDevicesCount == 0)
		return B_INTERRUPTED;

	fStatusWindow = new VMWAddOnsStatusWindow();
	fStatusWindow->Show();

	char* buffer = (char*)malloc(BUF_SIZE);
	memset(buffer, 0, BUF_SIZE);

	for (uint i = 0; i < fDevicesCount; i++) {
		BVolume volume(fToCleanup[i]);
		if (volume.InitCheck() != B_OK || !volume.IsPersistent() || volume.IsRemovable()
			|| volume.IsReadOnly()) {
			continue;
		}

		char name[B_FILE_NAME_LENGTH];
		volume.GetName(name);
		BMessage* message = new BMessage(RESET_PROGRESS);
		message->AddString("volume_name", name);
		message->AddInt64("max_size", (int64)(volume.FreeBytes() - 2 * BUF_SIZE));
		be_app_messenger.SendMessage(message, fStatusWindow);
		fStatusWindow->PostMessage(message);

		BDirectory rootDirectory;
		volume.GetRootDirectory(&rootDirectory);

		status_t ret = FillDirectory(&rootDirectory, buffer);

		if (ret == B_INTERRUPTED)
			break;

		if (ret != B_DEVICE_FULL && ret != B_OK) {
			(new BAlert("Error",
				 (BString("An error occurred while cleaning ”")
					 << name << "” (" << strerror(ret) << "). This volume may be damaged.")
					 .String(),
				 "Cancel", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))
				->Go();
		}
	}

	free(buffer);
	fStatusWindow->Lock();
	fStatusWindow->Quit();

	return B_OK;
}


int32
VMWAddOnsCleanup::Start(void* data)
{
	VMWAddOnsTray* parentTray = (VMWAddOnsTray*)data;

	int32 result = (new BAlert("Shrink disks",
						"Disk shrinking will operate on all auto-expanding disks "
						"attached to this virtual machine.\nFor best results it is "
						"recommanded to clean up free space on these disks before starting "
						"the process.\n",
						"Cancel", "Shrink now" B_UTF8_ELLIPSIS, "Clean up disks" B_UTF8_ELLIPSIS,
						B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT))
					   ->Go();

	// Cancel or quit
	if (result <= 0) {
		parentTray->fCleanupInProgress = false;
		return B_OK;
	}

	// Clean up disks
	if (result == 2) {
		VMWAddOnsCleanup* cleanup_th = new VMWAddOnsCleanup;
		if (cleanup_th->ThreadLoop() == B_INTERRUPTED) {
			parentTray->fCleanupInProgress = false;
			return B_INTERRUPTED;
		}
	}

	result = (new BAlert("Shrink disks",
				  "The shrink operation will now be launched in VMWare."
				  "This may take a long time ; the virtual machine will be "
				  "suspended during the process.",
				  "Cancel", "OK"))
				 ->Go();

	// OK...
	if (result == 1) {
		// Wait for things to calm down a bit before freezing the VM
		snooze(500000);
		parentTray->StartShrink();
	}

	parentTray->fCleanupInProgress = false;
	return B_OK;
}


status_t
VMWAddOnsCleanup::WriteToFile(BFile* file, char* buffer)
{
	ulong currentSize = 0; // In MB
	ulong writeProgress = 0; // In bytes

	do {
		ssize_t written = file->Write(buffer, BUF_SIZE);

		if (written < B_OK)
			return written;

		if (written < BUF_SIZE)
			return B_DEVICE_FULL;

		writeProgress += written;

		while (writeProgress >= MB) { // We wrote at least an MB
			writeProgress -= MB;
			currentSize++;
			file->Sync();
			fStatusWindow->PostMessage(new BMessage(UPDATE_PROGRESS));
		}

		if (fStatusWindow->fCancelled)
			return B_INTERRUPTED;

	} while (currentSize < MAX_FILE_SIZE);

	return B_OK;
}


status_t
VMWAddOnsCleanup::FillDirectory(BDirectory* rootDirectory, char* buffer)
{
	BFile* spaceSuckingFiles[MAX_FILES];
	uint filesCount = 0;
	status_t status;

	for (uint i = 0; i < MAX_FILES; i++) {
		// Create a new file
		spaceSuckingFiles[i] = new BFile();

		status = rootDirectory->CreateFile("space_sucking_file", spaceSuckingFiles[i]);
		if (status != B_OK)
			break;

		filesCount++;

		BEntry fileEntry;
		rootDirectory->FindEntry("space_sucking_file", &fileEntry);
		fileEntry.Remove(); // The file will be deleted when the BFile is closed

		status = WriteToFile(spaceSuckingFiles[i], buffer);

		if (status != B_OK)
			break;
	}

	// We can now delete the files
	for (uint i = 0; i < filesCount; i++)
		delete spaceSuckingFiles[i];

	return status;
}
