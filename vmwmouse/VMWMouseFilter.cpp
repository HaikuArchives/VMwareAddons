/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWMouseFilter.h"

#include <Roster.h>
#include <NodeMonitor.h>

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
	if (backdoor.InVMware())
		return B_NO_ERROR;
	
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
	if (!activated)
		return B_DISPATCH_MESSAGE;
	
	if (message->what != B_MOUSE_MOVED && message->what != B_MOUSE_DOWN 
		&& message->what != B_MOUSE_UP && !message->HasFloat("be:delta_x"))
		return B_DISPATCH_MESSAGE;
	
	message->RemoveName("be:delta_x");
	message->RemoveName("be:delta_y");
	backdoor.SyncCursor(message);

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
