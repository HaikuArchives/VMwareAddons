/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWAddOnsSelectWindow.h"

#include <stdlib.h>

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
#define CANCEL_OPERATION 'ccOp'
#define SELECT_CHANGED 'seCh'

VMWAddOnsSelectWindow::VMWAddOnsSelectWindow()
	: BWindow(BRect(0, 0, 300, 1), "Clean up free space", B_TITLED_WINDOW, 
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	volume_roster = new BVolumeRoster();
	
	int y = SPACING, w = static_cast<int>(Frame().Width()) - 2 * SPACING;

	disks_view = new BView(Bounds(), "disks view", B_FOLLOW_ALL, B_WILL_DRAW);
	disks_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	info_text1 = new BStringView(BRect(SPACING, y, 1, 1), NULL,
		"Volumes to cleanup :");
	info_text1->ResizeToPreferred();
	y += _H(info_text1) + SPACING;
	if (w < _W(info_text1)) w = _W(info_text1);
	disks_view->AddChild(info_text1);
	
	volumes_list = new VolumesList(BRect(SPACING, y, w - SPACING, 150), NULL,
		B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	y += _H(volumes_list) + SPACING;
	
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
         B_FOLLOW_ALL_SIDES, 0, false, true));
	
	cleanup_button = new BButton(BRect(0, 0, 0, 0), NULL, "Cleanup selection",
		new BMessage(CLEANUP_SELECTION), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	cleanup_button->MakeDefault(true);
	cleanup_button->SetEnabled(volumes_list->CurrentSelection() >= 0);
	cleanup_button->ResizeToPreferred();
	cleanup_button->MoveTo(w + SPACING - _W(cleanup_button), y);
	
	disks_view->AddChild(cleanup_button);
	
	cleanup_button->SetEnabled(volumes_list->CurrentSelection() >= 0);
	
	cancel_button = new BButton(BRect(0, 0, 0, 0), NULL, "Cancel", new BMessage(CANCEL_OPERATION),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	cancel_button->ResizeToPreferred();
	cancel_button->MoveTo(w - _W(cleanup_button) - _W(cancel_button), y + 3);
	
	disks_view->AddChild(cancel_button);
	
	y += _H(cleanup_button) + SPACING;
	
	disks_view->ResizeTo(w + 2 * SPACING, y);
	
	ResizeTo(w + 2 * SPACING, y);
	
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = 3 * SPACING + _W(cleanup_button) + _W(cancel_button);
	minHeight = 3 * SPACING + _H(info_text1) + 2 * _H(cleanup_button);
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
	
	BScreen screen(this);
	if (screen.IsValid()) {
		MoveTo((screen.Frame().Width() - Frame().Width()) / 2,
			(screen.Frame().Height() - Frame().Height()) / 2);
	}
	
	AddChild(disks_view);
	caller_lock = create_sem(0, "caller lock");
}

VMWAddOnsSelectWindow::~VMWAddOnsSelectWindow()
{
	volume_roster->StopWatching();
	delete volume_roster;
	release_sem(caller_lock);
	delete_sem(caller_lock);
}

void
VMWAddOnsSelectWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SELECT_CHANGED:
			cleanup_button->SetEnabled(volumes_list->CurrentSelection() >= 0);
		break;
		
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
			parent->devices_count = 0;
			while (volumes_list->CurrentSelection(parent->devices_count) >= 0)
				parent->devices_count++;
			
			if (parent->devices_count <= 0)
				return;
			
			parent->to_cleanup = (dev_t*)malloc(parent->devices_count * sizeof(dev_t));
			
			for (uint i = 0 ; i < parent->devices_count ; i++) {
				int32 selected = volumes_list->CurrentSelection(i);
				if (selected < 0) {
					parent->devices_count = i;
					break;
				}
				
				VolumeItem* item = (VolumeItem*)volumes_list->ItemAt(selected);
				parent->to_cleanup[i] = item->device;
			}
		// no break here - we are done
		
		case CANCEL_OPERATION:
			Lock();
			Quit();
		break;
		
		default:
			BWindow::MessageReceived(message);
		break;
	}
}

void
VMWAddOnsSelectWindow::Go(VMWAddOnsCleanup* _parent)
{
	parent = _parent;
	Show();
	acquire_sem(caller_lock);
}

VolumeItem::VolumeItem(char* name, dev_t _device)
	: BStringItem(name)
{
	device = _device;
}

VolumeItem::~VolumeItem()
{
}

VolumesList::VolumesList(BRect frame, const char* name, list_view_type type, uint32 resizingMode, uint32 flags)
	: BListView(frame, name, type, resizingMode, flags)
{
}

VolumesList::~VolumesList()
{
}

void
VolumesList::SelectionChanged()
{
	if (this->Window() != NULL)
		this->Window()->PostMessage(SELECT_CHANGED);
}
