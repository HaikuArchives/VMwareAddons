/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "VMWareMouse.h"

#include <Message.h>
#include <NodeMonitor.h>
#include <Screen.h>

#include <new>
#include <syslog.h>


#define DEBUG_PREFIX		"VMWareMouseFilter: "
#define TRACE(x...)			// syslog(DEBUG_PREFIX x)
#define TRACE_ALWAYS(x...)	syslog(LOG_INFO, DEBUG_PREFIX x)
#define TRACE_ERROR(x...)	syslog(LOG_ERR, DEBUG_PREFIX x)


bool gActivated;
VMWBackdoor gBackdoor;
VMWAddOnsSettings* gSettings;


// #pragma mark - Public BInputServerFilter API


VMWareMouseFilter::VMWareMouseFilter()
	:
	BInputServerFilter()
{
	node_ref nref;
	gSettings = new VMWAddOnsSettings(&nref);
	fSettingsWatcher = new VMWareSettingsWatcher(&nref);
	gActivated = gSettings->GetBool("mouse_enabled", true);

	// At this point, the mouse may have not moved since the system was booted.
	// VMware will erase the mouse settings when the user will start using
	// the mouse.
	// So we'll wait for the user to start moving the mouse before enabling
	// the mouse sharing.
}


VMWareMouseFilter::~VMWareMouseFilter()
{
	gBackdoor.DisableMouseSharing(); // Harmless if it was not enabled

	delete gSettings;
	fSettingsWatcher->Lock();
	fSettingsWatcher->Quit();
}


status_t
VMWareMouseFilter::InitCheck()
{
	if (gBackdoor.InVMware())
		return B_OK;

	return B_ERROR;
}


filter_result
VMWareMouseFilter::Filter(BMessage* message, BList* outList)
{
	if (!gActivated)
		return B_DISPATCH_MESSAGE;

	switch (message->what) {
		case B_MOUSE_UP:
		case B_MOUSE_DOWN:
		case B_MOUSE_MOVED:
		{
			int32 x, y;
			status_t ret = gBackdoor.GetCursorPosition(x, y);

			if (ret == B_ERROR) {
				TRACE("Filter(): Re-enabling pointer sharing.\n");
				gBackdoor.DisableMouseSharing();
				gBackdoor.EnableMouseSharing();
				break;
			}

			if (ret == B_INTERRUPTED) {
				// Button clicks can generate B_MOUSE_MOVED without data. Skip those.
				if (message->what == B_MOUSE_MOVED)
					return B_SKIP_MESSAGE;
				// But do not skip actual button down/up events.
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
VMWareMouseFilter::_ScalePosition(int32& x, int32& y)
{
	static BScreen screen;
	BRect frame = screen.Frame();
	x = (int32)(x * (frame.Width() / 65535) + 0.5);
	y = (int32)(y * (frame.Height() / 65535) + 0.5);
}


VMWareSettingsWatcher::VMWareSettingsWatcher(node_ref* ref)
	:
	BLooper("vmwware_mouse settings watcher")
{
	Run();
	watch_node(ref, B_WATCH_STAT, this);
}


void
VMWareSettingsWatcher::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			gSettings->Reload();
			gActivated = gSettings->GetBool("mouse_enabled", true);

			if (gActivated)
				gBackdoor.EnableMouseSharing();
			else
				gBackdoor.DisableMouseSharing();
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


// #pragma mark - Instatiation Entry Point

extern "C" BInputServerFilter*
instantiate_input_filter()
{
	return new(std::nothrow) VMWareMouseFilter();
}
