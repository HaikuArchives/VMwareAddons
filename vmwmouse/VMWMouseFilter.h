/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef VMW_MOUSE_FILTER_H
#define VMW_MOUSE_FILTER_H

#include <Looper.h>
#include <InputServerFilter.h>

#include "VMWAddOnsSettings.h"
#include "VMWBackdoor.h"

class VMWSettingsWatcher: public BLooper {
public:
								VMWSettingsWatcher(node_ref* ref);
	virtual						~VMWSettingsWatcher();
	virtual void				MessageReceived(BMessage* message);
};

class VMWMouseFilter: public BInputServerFilter {
public:
								VMWMouseFilter();
	virtual						~VMWMouseFilter();

	virtual	status_t			InitCheck();

	virtual	filter_result		Filter(BMessage* message, BList* outList);

private:
	VMWSettingsWatcher*			settings_watcher;
};

#endif	/* VMW_MOUSE_FILTER_H */
