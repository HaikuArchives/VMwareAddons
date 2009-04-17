/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Directory.h>
#include <SupportDefs.h>

#include "VMWAddOnsSelectWindow.h"
#include "VMWAddOnsTray.h"

#define MB (1024 * 1024)
#define BUF_SIZE 8192
#define MAX_FILES 200
#define MAX_FILE_SIZE 4096 // In MB

VMWAddOnsCleanup::VMWAddOnsCleanup()
{
}

VMWAddOnsCleanup::~VMWAddOnsCleanup()
{
}

void
VMWAddOnsCleanup::ThreadLoop()
{
	devices_count = 0;
	VMWAddOnsSelectWindow* select_window = new VMWAddOnsSelectWindow();
	select_window->Go(this);
	
	if (devices_count == 0)
		return;
	
	status_window = new VMWAddOnsStatusWindow();
	status_window->Show();
	
	char* buffer = (char*)malloc(BUF_SIZE);
	memset(buffer, 0, BUF_SIZE);
	
	for (uint i = 0 ; i < devices_count ; i++) {
		BVolume volume(to_cleanup[i]);
		if (volume.InitCheck() != B_OK || !volume.IsPersistent() 
			|| volume.IsRemovable() || volume.IsReadOnly())
			continue;
		
		char name[B_FILE_NAME_LENGTH];
		volume.GetName(name);
		BMessage* message = new BMessage(RESET_PROGRESS);
		message->AddString("volume_name", name);
		message->AddInt64("max_size", (int64)(volume.FreeBytes() - 2 * BUF_SIZE));
		be_app_messenger.SendMessage(message, status_window);
		status_window->PostMessage(message);

		BDirectory root_directory;
		volume.GetRootDirectory(&root_directory);
	
		status_t ret = FillDirectory(&root_directory, buffer);
	
		if (ret == B_INTERRUPTED)
			break;
		
		if (ret != B_DEVICE_FULL && ret != B_OK) {
			(new BAlert("Error", (BString("An error occurred while cleaning ”") << name
				<< "” (" << strerror(ret) << "). This volume may be damaged.").String(),
					"Cancel", NULL, NULL, B_WIDTH_AS_USUAL,	B_STOP_ALERT))->Go();
		}
	}
	
	free(buffer);
	status_window->Lock();
	status_window->Quit();
}

int32
VMWAddOnsCleanup::Start(void* data)
{
	VMWAddOnsTray* parent_tray = (VMWAddOnsTray*)data;
	
	int32 result = (new BAlert("Shrink disks",
		"Disk shrinking will operate on all auto-expanding disks "
		"attached to this virtual machine.\nFor best results it is "
		"recommanded to clean up free space on these disks before starting "
		"the process.\n", "Cancel", "Shrink now" B_UTF8_ELLIPSIS, 
		"Clean up disks" B_UTF8_ELLIPSIS, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, 
		B_INFO_ALERT))->Go();
			
	if (result <= 0) { // Cancel or quit
		parent_tray->cleanup_in_process = false;
		return B_OK;
	}
			
	if (result == 2) { // Clean up disks
		VMWAddOnsCleanup* cleanup_th = new VMWAddOnsCleanup;
		cleanup_th->ThreadLoop();
	}
	
	result = (new BAlert("Shrink disks",
		"The shrink operation will now be launched in VMWare."
		"This may take a long time ; the virtual machine will be "
		"suspended during the process.", "Cancel", "OK"))->Go();

	if (result == 1) {
		// Wait for things to calm down a bit before freezing the VM 
		snooze(500000);
		parent_tray->StartShrink();
	}
	
	parent_tray->cleanup_in_process = false;
	return B_OK;
}

status_t
VMWAddOnsCleanup::WriteToFile(BFile* file, char* buffer)
{
	ulong current_size = 0; // In MB
	ulong write_progress = 0; // In bytes
	
	do {
		ssize_t written = file->Write(buffer, BUF_SIZE);
		
		if (written < B_OK)
			return written;
		
		if (written < BUF_SIZE)
			return B_DEVICE_FULL;
		
		write_progress += written;
		
		if (write_progress >= MB) { // We wrote a MB
			write_progress %= MB;
			current_size++;
			file->Sync();
			status_window->PostMessage(new BMessage(UPDATE_PROGRESS));
		}
		
		if (status_window->cancelled)
			return B_INTERRUPTED;
		
	} while(current_size < MAX_FILE_SIZE);
	
	return B_OK;
}

status_t
VMWAddOnsCleanup::FillDirectory(BDirectory* root_directory, char* buffer)
{
	BFile* space_sucking_files[MAX_FILES];
	uint files_count = 0;
	status_t ret;
	
	for (uint i = 0 ; i < MAX_FILES ; i++) {
		// Create a new file
		space_sucking_files[i] = new BFile();
		
		ret = root_directory->CreateFile("space_sucking_file", space_sucking_files[i]);
		if (ret != B_OK)
			break;
		
		files_count++;
		
		BEntry file_entry;
		root_directory->FindEntry("space_sucking_file", &file_entry);
		file_entry.Remove(); // The file will be deleted when the BFile is closed
		
		ret = WriteToFile(space_sucking_files[i], buffer);
		
		if (ret != B_OK)
			break;	
	}
	
	// We can now delete the files
	for (uint i = 0 ; i < files_count ; i++)
		delete space_sucking_files[i];
	
	return ret;
}
