/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMWCLEANUPWINDOW_H
#define VMWCLEANUPWINDOW_H

#include <File.h>
#include <ListView.h>
#include <StatusBar.h>
#include <StringItem.h>
#include <StringView.h>
#include <View.h>
#include <VolumeRoster.h>
#include <Window.h>

class VolumeItem : public BStringItem {
public:
						VolumeItem(char* name, dev_t _device);
	virtual				~VolumeItem();
	dev_t device;
};

class VMWAddOnsCleanupWindow : public BWindow {
public:
					VMWAddOnsCleanupWindow(BView* _parent_view);
	virtual			~VMWAddOnsCleanupWindow();
	virtual void	MessageReceived(BMessage* message);

	dev_t*			to_cleanup;
	int				count; // of items of the above array
	int				in_progress;
	BFile*			space_sucking_file;

private:
	BView*			parent_view;
	
	BView*			disks_view;
	BStringView*	info_text1;
	BListView*		volumes_list;
	BVolumeRoster*	volume_roster;
	BButton*		cleanup_button;
	BButton*		cancel_button;
	
	BView*			cleanup_view;
	BStatusBar*		progress_bar;
	BButton*		stop_button;
	
	thread_id th;
};

extern "C" status_t filling_thread(void* data);

#endif
