/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWAddOns.h"
#include "VMWMouseFilter.h"

#include <Debug.h>
#include <Roster.h>
#include <NodeMonitor.h>

#include <errno.h>

#include "vmwbackdoor.h"

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

VMWAddOnsSettings* settings;
bool activated;

BInputServerFilter*
instantiate_input_filter()
{
	return new VMWMouseFilter;
}

status_t VMWMouseFilter::InitCheck()
{
	if (VMCheckVirtual() == B_OK) {
		PRINT(("VMWMouseFilter: VMWare detected, filter enabled.\n"));
		return B_NO_ERROR;
	}
	
	PRINT(("VMWMouseFilter: filter disabled (not in VMWare, or unsupported version ?).\n"));
	return B_ERROR;
}

VMWMouseFilter::VMWMouseFilter()
{
	node_ref nref;
	settings = new VMWAddOnsSettings(&nref);
	settings_watcher = new VMWSettingsWatcher(&nref);
	activated = settings->GetBool("mouse_enabled", true);
}


VMWMouseFilter::~VMWMouseFilter()
{
	delete settings;
	settings_watcher->Lock();
	settings_watcher->Quit();
}

filter_result
VMWMouseFilter::Filter(BMessage* message, BList* /*outlist*/)
{
	int16_t cursor_x, cursor_y;
	
	if (!activated)
		return B_DISPATCH_MESSAGE;
	
	if (message->what != B_MOUSE_MOVED && message->what != B_MOUSE_DOWN
		&& message->what != B_MOUSE_UP && !message->HasFloat("be:delta_x"))
		return B_DISPATCH_MESSAGE;
	
	if(VMGetCursorPos(&cursor_x, &cursor_y) != B_OK // Error in VMW backdoor
		|| cursor_x < 0) // Cursor is offscreen 
		return B_DISPATCH_MESSAGE;
	
	message->RemoveName("be:delta_x");
	message->RemoveName("be:delta_y");
	message->ReplacePoint("where", BPoint(cursor_x, cursor_y));

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
		break;
		
		default:
			BLooper::MessageReceived(message);
		break;
	}
}
