/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "VMWAddOnsSelectWindow.h"

#include <stdlib.h>

#include <NodeMonitor.h>
#include <Screen.h>
#include <ScrollView.h>

#include "VMWAddOnsCleanup.h"

#define _H(x) static_cast<int>((x)->Frame().Height())
#define _W(x) static_cast<int>((x)->Frame().Width())

#define SPACING 8

#define CLEANUP_SELECTION 'clSe'
#define CANCEL_OPERATION 'ccOp'
#define SELECT_CHANGED 'seCh'


VMWAddOnsSelectWindow::VMWAddOnsSelectWindow()
	:
	BWindow(BRect(0, 0, 300, 1), "Clean up free space", B_TITLED_WINDOW,
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	fVolumeRoster = new BVolumeRoster();

	int y = SPACING, w = static_cast<int>(Frame().Width()) - 2 * SPACING;

	fDisksView = new BView(Bounds(), "disks view", B_FOLLOW_ALL, B_WILL_DRAW);
	fDisksView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInfoText = new BStringView(BRect(SPACING, y, 1, 1), NULL, "Volumes to cleanup :");
	fInfoText->ResizeToPreferred();
	y += _H(fInfoText) + SPACING;
	if (w < _W(fInfoText))
		w = _W(fInfoText);
	fDisksView->AddChild(fInfoText);

	fVolumesList = new VolumesList(BRect(SPACING, y, w - SPACING, 150), NULL,
		B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	y += _H(fVolumesList) + SPACING;

	BVolume currentVolume;
	fVolumeRoster->Rewind();

	while (fVolumeRoster->GetNextVolume(&currentVolume) == B_NO_ERROR) {
		if (!currentVolume.IsPersistent() || currentVolume.IsRemovable()
			|| currentVolume.IsReadOnly()) {
			continue;
		}

		char name[B_FILE_NAME_LENGTH];
		currentVolume.GetName(name);

		fVolumesList->AddItem(new VolumeItem(name, currentVolume.Device()));
	}

	fVolumeRoster->StartWatching(this);
	fVolumesList->Select(0, fVolumesList->CountItems() - 1);

	fDisksView->AddChild(new BScrollView(NULL, fVolumesList, B_FOLLOW_ALL_SIDES, 0, false, true));

	fCleanupButton = new BButton(BRect(0, 0, 0, 0), NULL, "Cleanup selection",
		new BMessage(CLEANUP_SELECTION), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fCleanupButton->MakeDefault(true);
	fCleanupButton->SetEnabled(fVolumesList->CurrentSelection() >= 0);
	fCleanupButton->ResizeToPreferred();
	fCleanupButton->MoveTo(w + SPACING - _W(fCleanupButton), y);

	fDisksView->AddChild(fCleanupButton);

	fCleanupButton->SetEnabled(fVolumesList->CurrentSelection() >= 0);

	fCancelButton = new BButton(BRect(0, 0, 0, 0), NULL, "Cancel", new BMessage(CANCEL_OPERATION),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fCancelButton->ResizeToPreferred();
	fCancelButton->MoveTo(w - _W(fCleanupButton) - _W(fCancelButton), y + 3);

	fDisksView->AddChild(fCancelButton);

	y += _H(fCleanupButton) + SPACING;

	fDisksView->ResizeTo(w + 2 * SPACING, y);

	ResizeTo(w + 2 * SPACING, y);

	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = 3 * SPACING + _W(fCleanupButton) + _W(fCancelButton);
	minHeight = 3 * SPACING + _H(fInfoText) + 2 * _H(fCleanupButton);
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	BScreen screen(this);
	if (screen.IsValid()) {
		MoveTo((screen.Frame().Width() - Frame().Width()) / 2,
			(screen.Frame().Height() - Frame().Height()) / 2);
	}

	AddChild(fDisksView);
	fCallerSemId = create_sem(0, "vmware caller semaphore");
}


VMWAddOnsSelectWindow::~VMWAddOnsSelectWindow()
{
	fVolumeRoster->StopWatching();
	delete fVolumeRoster;
	release_sem(fCallerSemId);
	delete_sem(fCallerSemId);
}


void
VMWAddOnsSelectWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SELECT_CHANGED:
			fCleanupButton->SetEnabled(fVolumesList->CurrentSelection() >= 0);
			break;

		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				return;

			switch (opcode) {
				case B_DEVICE_MOUNTED:
				{
					dev_t device;

					if (message->FindInt32("new device", &device) != B_OK)
						return;

					BVolume volume(device);

					if (volume.InitCheck() != B_OK || !volume.IsPersistent() || volume.IsRemovable()
						|| volume.IsReadOnly()) {
						return;
					}

					char name[B_FILE_NAME_LENGTH];
					volume.GetName(name);
					fVolumesList->AddItem(new VolumeItem(name, volume.Device()));
				} break;

				case B_DEVICE_UNMOUNTED:
					dev_t device;

					if (message->FindInt32("device", &device) != B_OK)
						return;

					for (int32 i = 0; i < fVolumesList->CountItems(); i++) {
						VolumeItem* item = (VolumeItem*)fVolumesList->ItemAt(i);
						if (item->fDevice == device)
							fVolumesList->RemoveItem(item);
					}
					break;

				default:
					break;
			}
		} break;

		case CLEANUP_SELECTION:
			fParent->fDevicesCount = 0;
			while (fVolumesList->CurrentSelection(fParent->fDevicesCount) >= 0)
				fParent->fDevicesCount++;

			if (fParent->fDevicesCount <= 0)
				return;

			fParent->fToCleanup = (dev_t*)malloc(fParent->fDevicesCount * sizeof(dev_t));

			for (uint i = 0; i < fParent->fDevicesCount; i++) {
				int32 selected = fVolumesList->CurrentSelection(i);
				if (selected < 0) {
					fParent->fDevicesCount = i;
					break;
				}

				VolumeItem* item = (VolumeItem*)fVolumesList->ItemAt(selected);
				fParent->fToCleanup[i] = item->fDevice;
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
VMWAddOnsSelectWindow::Go(VMWAddOnsCleanup* parent)
{
	fParent = parent;
	Show();
	acquire_sem(fCallerSemId);
}


// #pragma mark -


VolumeItem::VolumeItem(char* name, dev_t device)
	:
	BStringItem(name)
{
	fDevice = device;
}


// #pragma mark -


VolumesList::VolumesList(BRect frame, const char* name, list_view_type type, uint32 resizingMode,
	uint32 flags)
	:
	BListView(frame, name, type, resizingMode, flags)
{
}


void
VolumesList::SelectionChanged()
{
	if (this->Window() != NULL)
		this->Window()->PostMessage(SELECT_CHANGED);
}
