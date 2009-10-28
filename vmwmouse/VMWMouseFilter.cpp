/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWMouseFilter.h"

#include <stdio.h>
#include <string.h>

#include <Roster.h>
#include <Screen.h>
#include <NodeMonitor.h>

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

VMWAddOnsSettings* settings;
bool activated;
VMWBackdoor backdoor;

BInputServerFilter*
instantiate_input_filter()
{
	return new VMWMouseFilter;
}

status_t VMWMouseFilter::InitCheck()
{
	if (backdoor.InVMware())
		return B_OK;

	return B_ERROR;
}

VMWMouseFilter::VMWMouseFilter()
	: BInputServerFilter()
{
	node_ref nref;
	settings = new VMWAddOnsSettings(&nref);
	settings_watcher = new VMWSettingsWatcher(&nref);
	activated = settings->GetBool("mouse_enabled", true);
	
	// At this point, the mouse may have not moved since the system was booted.
	// VMware will erase the mouse settings when the user will start using
	// the mouse.
	// So we'll wait for the user to start moving the mouse before enabling
	// the mouse sharing.
}


VMWMouseFilter::~VMWMouseFilter()
{
	backdoor.DisableMouseSharing(); // Harmless if it was not enabled
	
	delete settings;
	settings_watcher->Lock();
	settings_watcher->Quit();
}

filter_result
VMWMouseFilter::Filter(BMessage* message, BList* /*outlist*/)
{
	static BScreen screen;
	
	if (!activated)
		return B_DISPATCH_MESSAGE;
	
	if (message->what != B_MOUSE_MOVED && message->what != B_MOUSE_DOWN
		&& message->what != B_MOUSE_UP && !message->HasFloat("be:delta_x"))
		return B_DISPATCH_MESSAGE;

	int32 x, y;
	status_t ret = backdoor.GetCursorPosition(x, y);
	if (ret == B_OK) {
		BRect frame = screen.Frame();
		message->ReplacePoint("where", BPoint(x * frame.Width() / 65535, y * frame.Height() / 65535));
	} else if (ret == B_ERROR) {
		// Reset the mouse settings
		backdoor.DisableMouseSharing();
		backdoor.EnableMouseSharing();
	}

	return B_DISPATCH_MESSAGE;
}

VMWSettingsWatcher::VMWSettingsWatcher(node_ref* ref)
	: BLooper("vmwmouse settings watcher")
{
	Run();
	watch_node(ref, B_WATCH_STAT, this);
}

VMWSettingsWatcher::~VMWSettingsWatcher()
{
}

void
VMWSettingsWatcher::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_NODE_MONITOR:
			settings->Reload();
			activated = settings->GetBool("mouse_enabled", true);
			
				if (activated)
					backdoor.EnableMouseSharing();
				else
					backdoor.DisableMouseSharing();
		break;

		default:
			BLooper::MessageReceived(message);
		break;
	}
}
