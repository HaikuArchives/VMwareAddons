/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWAddOnsCleanupWindow.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Directory.h>
#include <NodeMonitor.h>
#include <Screen.h>
#include <ScrollView.h>

#include "VMWAddOns.h"

#define _H(x) static_cast<int>((x)->Frame().Height())
#define _W(x) static_cast<int>((x)->Frame().Width())
#define SPACING 8

#define CLEANUP_SELECTION 'clSe'
#define CANCEL_OPERATION 'bof!'
#define STOP_OPERATION 'non!'
#define CONT_SHRINK 'coSh'
#define UPDATE_PROGRESS 'plop'

#define MB 1024 * 1024
#define BUF_SIZE 8192

VMWAddOnsCleanupWindow::VMWAddOnsCleanupWindow(BView* _parent_view)
	: BWindow(BRect(0, 0, 300, 400), "Shrink virtual disks", B_MODAL_WINDOW, B_NOT_RESIZABLE 
		| B_NOT_CLOSABLE | B_NOT_MOVABLE | B_ASYNCHRONOUS_CONTROLS)
{
	parent_view = _parent_view;
	volume_roster = new BVolumeRoster();
	
	to_cleanup = NULL;
	space_sucking_file = NULL;
	
	int y = SPACING, w = static_cast<int>(Frame().Width());
	
	// Main view, allow the user to choose the disks s/he wants to cleanup
	disks_view = new BView(Bounds(), "disks view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	disks_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	info_text1 = new BStringView(BRect(SPACING, y, 1, 1), NULL,
		"Volumes to cleanup :");
	info_text1->ResizeToPreferred();
	y += _H(info_text1) + SPACING;
	if (w < _W(info_text1)) w = _W(info_text1);
	disks_view->AddChild(info_text1);
	
	volumes_list = new BListView(BRect(SPACING, y, w - SPACING, 200), NULL,
		B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	y += _H(volumes_list) + SPACING;
	if (w < _W(volumes_list)) w = _W(volumes_list);
	
	BVolume current_volume;
	volume_roster->Rewind();
	
	while (volume_roster->GetNextVolume(&current_volume) == B_NO_ERROR) {
		if (!current_volume.IsPersistent() || current_volume.IsRemovable()
			|| current_volume.IsReadOnly())
			continue;

		char name[B_FILE_NAME_LENGTH];
		current_volume.GetName(name);
		
		volumes_list->AddItem(new VolumeItem(name, current_volume.Device()));
	}
	
	volume_roster->StartWatching(this);
	volumes_list->Select(0, volumes_list->CountItems() - 1);
	
	disks_view->AddChild(new BScrollView(NULL, volumes_list,
         B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));
	
	cleanup_button = new BButton(BRect(0, 0, 0, 0), NULL, "Cleanup selection",
		new BMessage(CLEANUP_SELECTION));
	cleanup_button->MakeDefault(true);
	cleanup_button->ResizeToPreferred();
	cleanup_button->MoveTo(w - _W(cleanup_button), y);
	
	disks_view->AddChild(cleanup_button);
	
	cancel_button = new BButton(BRect(0, 0, 0, 0), NULL, "Cancel", new BMessage(CANCEL_OPERATION));
	cancel_button->ResizeToPreferred();
	cancel_button->MoveTo(w - _W(cleanup_button) - SPACING - _W(cancel_button), y + 3);
	
	disks_view->AddChild(cancel_button);
	
	y += _H(cleanup_button) + SPACING;
	
	disks_view->ResizeTo(w + 2 * SPACING, y);
	
	ResizeTo(w + 2 * SPACING, y);
	
	BScreen screen(this);
	if (screen.IsValid()) {
		MoveTo((screen.Frame().Width() - Frame().Width()) / 2,
			(screen.Frame().Height() - Frame().Height()) / 2);
	} 
	
	AddChild(disks_view);
	
	// Cleanup view
	cleanup_view = new BView(Bounds(), "cleanup view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	cleanup_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	progress_bar = new BStatusBar(BRect(SPACING, SPACING, 200, 45), NULL, "Starting...");
	
	cleanup_view->AddChild(progress_bar);
	
	stop_button = new BButton(BRect(0, 0, 0, 0), NULL, "Stop", new BMessage(STOP_OPERATION));
	stop_button->ResizeToPreferred();
	stop_button->SetEnabled(false);
	
	stop_button->MoveTo(2 * SPACING + _W(progress_bar), 
		progress_bar->Frame().bottom - _H(stop_button));
	
	cleanup_view->AddChild(stop_button);
	
	cleanup_view->ResizeTo(3 * SPACING + _W(progress_bar) + _W(stop_button), 
		2 * SPACING + _H(progress_bar));
	
}

VMWAddOnsCleanupWindow::~VMWAddOnsCleanupWindow()
{
	parent_view->MessageReceived(new BMessage(SHRINK_WINDOW_CLOSED));
	volume_roster->StopWatching();
	delete volume_roster;

	if (to_cleanup != NULL)
		free(to_cleanup);

	if (space_sucking_file != NULL)
		delete space_sucking_file;
}

void
VMWAddOnsCleanupWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK) 
				return;

			switch(opcode) {
				case B_DEVICE_MOUNTED:
				{
					dev_t device;
				
					if (message->FindInt32("new device", &device) != B_OK)
						return;
	
					BVolume volume(device);

					if (volume.InitCheck() != B_OK || !volume.IsPersistent() || volume.IsRemovable()
						|| volume.IsReadOnly())
						return;
			
					char name[B_FILE_NAME_LENGTH];
					volume.GetName(name);
					volumes_list->AddItem(new VolumeItem(name, volume.Device()));
				}
				break;

				case B_DEVICE_UNMOUNTED:
					dev_t device;
			
					if (message->FindInt32("device", &device) != B_OK)
						return;
				
					for (int32 i = 0 ; i < volumes_list->CountItems() ; i++) {
						VolumeItem* item = (VolumeItem*)volumes_list->ItemAt(i);
						if (item->device == device) {
							volumes_list->RemoveItem(item);
						}
					}
				break;
		
				default:
				break;
			}
		}
		break;
		
		case CLEANUP_SELECTION:
			count = 0;
			while (volumes_list->CurrentSelection(count) >= 0)
				count++;
			
			if (count <= 0)
				return;
			
			to_cleanup = (dev_t*)malloc(count * sizeof(dev_t));
			
			for (int i = 0 ; i < count ; i++) {
				int32 selected = volumes_list->CurrentSelection(i);
				if (selected < 0) {
					count = i;
					break;
				}
				
				VolumeItem* item = (VolumeItem*)volumes_list->ItemAt(selected);
				to_cleanup[i] = item->device;
			}
		
			Hide();
			
			RemoveChild(disks_view);
			delete disks_view;
			
			ResizeTo(_W(cleanup_view), _H(cleanup_view));
			AddChild(cleanup_view);
			
			SetType(B_TITLED_WINDOW);
			SetFlags(B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE | B_ASYNCHRONOUS_CONTROLS);
			Show();
			
			in_progress = -1;
		
		case CONT_SHRINK:
		{
			BVolume volume;
			in_progress++;
			
			if (space_sucking_file != NULL) {
				delete space_sucking_file;
				space_sucking_file = NULL;
			}
			
			while (in_progress < count) {
				if (volume.SetTo(to_cleanup[in_progress]) == B_OK && volume.IsPersistent() 
					&& !volume.IsRemovable() && !volume.IsReadOnly())
					break;
				// maybe the volume was unmounted/changed in the meantime ?
				in_progress++;
			}
			
			if (in_progress >= count) { // We're done
				parent_view->MessageReceived(new BMessage(START_SHRINK));
				Lock();
				Quit();
				return;
			}
			
			char name[B_FILE_NAME_LENGTH];
			volume.GetName(name);

			progress_bar->Reset((BString("Cleaning ”") << name << "”" B_UTF8_ELLIPSIS).String());
			progress_bar->SetMaxValue(volume.FreeBytes() - 2 * BUF_SIZE);
			
			BDirectory root_directory;
			volume.GetRootDirectory(&root_directory);
			space_sucking_file = new BFile();
			status_t ret = root_directory.CreateFile("space_sucking_file", space_sucking_file);
			if (ret != B_OK) {
				char name[B_FILE_NAME_LENGTH];
				volume.GetName(name);
				(new BAlert("Error", (BString("An error occurred while cleaning ”") << name
					<< "” (" << strerror(ret) << "). This volume may be damaged.").String(),
					"Cancel", NULL, NULL, B_WIDTH_AS_USUAL,	B_STOP_ALERT))->Go();
				Lock();
				Quit();
			}
			
			BEntry file_entry;
			root_directory.FindEntry("space_sucking_file", &file_entry);
			file_entry.Remove(); // The file will be deleted when the BFile is closed
			
			stop_button->SetEnabled(true);
			th = spawn_thread(filling_thread, "disk eater", B_LOW_PRIORITY, (void*)this);
			if (th > 0) {
				resume_thread(th);
			} else {
				(new BAlert("Error", (BString("An error occurred while cleaning ”") << name
					<< "” (" << strerror(th) << ").").String(), "Cancel", NULL, NULL,
						B_WIDTH_AS_USUAL,	B_STOP_ALERT))->Go();
				Lock();
				Quit();
			}
		}
		break;
		
		case UPDATE_PROGRESS:
			progress_bar->Update(MB);
		break;
		
		case STOP_OPERATION:
			kill_thread(th);
			Lock();
			Quit();
		break;
		
		case CANCEL_OPERATION:
			Lock();
			Quit();
		break;
		
		default:
			message->PrintToStream();
			BWindow::MessageReceived(message);
		break;
	}
}

extern "C" status_t filling_thread(void* data) {
	VMWAddOnsCleanupWindow* main_window = (VMWAddOnsCleanupWindow*)data;
		
	dev_t device = main_window->to_cleanup[main_window->in_progress];
	BVolume volume(device);
	BFile* space_sucking_file = main_window->space_sucking_file;
	uint32 i = 0;
	
	char* buffer = (char*)malloc(BUF_SIZE);
	memset(buffer, 0, BUF_SIZE);
	
	while (volume.FreeBytes() > 2 * BUF_SIZE) {
		space_sucking_file->Write(buffer, BUF_SIZE);
		i++;
		
		if (i % (MB / BUF_SIZE) == 0) { // We wrote a MB
			//space_sucking_file->Sync();
			main_window->PostMessage(UPDATE_PROGRESS);
		}
	}
	
	free(buffer);
	
	main_window->PostMessage(CONT_SHRINK);
	
	return B_OK;
}

VolumeItem::VolumeItem(char* name, dev_t _device)
	: BStringItem(name)
{
	device = _device;
}

VolumeItem::~VolumeItem()
{
}	

