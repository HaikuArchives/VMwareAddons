/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "VMWareMouse.h"

#include <Message.h>
#include <Screen.h>
#include <NodeMonitor.h>

#include <new>
#include <syslog.h>


#define DEBUG_PREFIX		"VMWareMouseFilter: "
#define TRACE(x...)			/*syslog(DEBUG_PREFIX x)*/
#define TRACE_ALWAYS(x...)	syslog(LOG_INFO, DEBUG_PREFIX x)
#define TRACE_ERROR(x...)	syslog(LOG_ERR, DEBUG_PREFIX x)


VMWAddOnsSettings* settings;
bool activated;
VMWBackdoor backdoor;

// #pragma mark - Public BInputServerFilter API


VMWareMouseFilter::VMWareMouseFilter()
	:
	BInputServerFilter()
{
	node_ref nref;
	settings = new VMWAddOnsSettings(&nref);
	settings_watcher = new VMWareSettingsWatcher(&nref);
	activated = settings->GetBool("mouse_enabled", true);

	// At this point, the mouse may have not moved since the system was booted.
	// VMware will erase the mouse settings when the user will start using
	// the mouse.
	// So we'll wait for the user to start moving the mouse before enabling
	// the mouse sharing.
}


VMWareMouseFilter::~VMWareMouseFilter()
{
	backdoor.DisableMouseSharing(); // Harmless if it was not enabled
	
	delete settings;
	settings_watcher->Lock();
	settings_watcher->Quit();
}


status_t
VMWareMouseFilter::InitCheck()
{
	if (backdoor.InVMware())
		return B_OK;

	return B_ERROR;	
}


filter_result
VMWareMouseFilter::Filter(BMessage *message, BList *outList)
{
	if (!activated)
		return B_DISPATCH_MESSAGE;

	switch(message->what) {
		case B_MOUSE_UP:
		case B_MOUSE_DOWN:
		case B_MOUSE_MOVED:
		{
			uint16 status, numWords;
			backdoor.GetCursorStatus(status, numWords);
			if (status == VMWARE_ERROR) {
				TRACE_ERROR("error indicated when reading status, resetting\n");
				backdoor.DisableMouseSharing();
				backdoor.EnableMouseSharing();
				break;
			}

			if (numWords == 0) {
				// no actual data availabe, spurious event happens on fast move
				return B_SKIP_MESSAGE;
			}

			int32 x, y;
			status_t ret = backdoor.GetCursorPosition(x, y);

			if (ret == B_INTERRUPTED) { // Spurious event
				TRACE_ERROR("B_INTERRUPTED\n");
				return B_SKIP_MESSAGE;
			}

			if (ret != B_OK) {
				TRACE_ERROR("ret != B_OK (%d)\n", ret);
				break;
			}

			_ScalePosition(x, y);

			if (x < 0 || y < 0) {
				TRACE_ERROR("got invalid coordinates %ld, %ld\n", x, y);
				break;
			}

			TRACE("setting position to %ld, %ld\n", x, y);
			message->ReplacePoint("where", BPoint(x, y));
			break;
		}
	}


	return B_DISPATCH_MESSAGE;
}


void
VMWareMouseFilter::_ScalePosition(int32 &x, int32 &y)
{
	static BScreen screen;
	BRect frame = screen.Frame();
	x = (int32)(x * (frame.Width() / 65535) + 0.5);
	y = (int32)(y * (frame.Height() / 65535) + 0.5);
}

VMWareSettingsWatcher::VMWareSettingsWatcher(node_ref* ref)
	: BLooper("vmwmouse settings watcher")
{
	Run();
	watch_node(ref, B_WATCH_STAT, this);
}

VMWareSettingsWatcher::~VMWareSettingsWatcher()
{
}

void
VMWareSettingsWatcher::MessageReceived(BMessage* message)
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


// #pragma mark - Instatiation Entry Point


extern "C"
BInputServerFilter *instantiate_input_filter()
{
	return new (std::nothrow) VMWareMouseFilter();
}
