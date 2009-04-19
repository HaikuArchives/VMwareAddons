/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_SELECT_WINDOW_H
#define VMW_SELECT_WINDOW_H

#include <VolumeRoster.h>
#include <ListView.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#include "VMWAddOnsCleanup.h"

class VolumeItem: public BStringItem {
public:
						VolumeItem(char* name, dev_t _device);
	virtual				~VolumeItem();
	dev_t device;
};

class VolumesList: public BListView {
public:
					VolumesList(BRect frame,
						const char* name,
						list_view_type type = B_SINGLE_SELECTION_LIST,
						uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
						uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);
	
	virtual			~VolumesList();
	virtual void	SelectionChanged();
};

class VMWAddOnsSelectWindow: public BWindow {
public:
						VMWAddOnsSelectWindow();
	virtual				~VMWAddOnsSelectWindow();
	virtual void		MessageReceived(BMessage* message);
	void				Go(VMWAddOnsCleanup* _parent);

private:
	VMWAddOnsCleanup*	parent;

	BView*				disks_view;
	BStringView*		info_text1;
	VolumesList*		volumes_list;
	BVolumeRoster*		volume_roster;
	BButton*			cleanup_button;
	BButton*			cancel_button;

	sem_id				caller_lock;
};

#endif
