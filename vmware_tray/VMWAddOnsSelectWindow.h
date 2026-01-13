/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef VMW_SELECT_WINDOW_H
#define VMW_SELECT_WINDOW_H

#include <Button.h>
#include <ListView.h>
#include <StringItem.h>
#include <StringView.h>
#include <VolumeRoster.h>
#include <Window.h>


class VMWAddOnsCleanup;


class VolumeItem : public BStringItem {
public:
			VolumeItem(char* name, dev_t device);

	dev_t	fDevice;
};


class VolumesList : public BListView {
public:
					VolumesList(BRect frame, const char* name,
						list_view_type type = B_SINGLE_SELECTION_LIST,
						uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
						uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);

	virtual void	SelectionChanged();
};


class VMWAddOnsSelectWindow : public BWindow {
public:
						VMWAddOnsSelectWindow();
	virtual				~VMWAddOnsSelectWindow();
	virtual void		MessageReceived(BMessage* message);
	void				Go(VMWAddOnsCleanup* parent);

private:
	VMWAddOnsCleanup*	fParent;

	sem_id				fCallerSemId;
	BButton*			fCancelButton;
	BButton*			fCleanupButton;
	BView*				fDisksView;
	BStringView*		fInfoText;
	VolumesList*		fVolumesList;
	BVolumeRoster*		fVolumeRoster;
};


#endif
