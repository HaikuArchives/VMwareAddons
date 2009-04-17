/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_CLEANUP_H
#define VWM_CLEANUP_H

#include <SupportDefs.h>

#include "VMWAddOnsStatusWindow.h"

class VMWAddOnsCleanup {
public:
					VMWAddOnsCleanup();
	virtual			~VMWAddOnsCleanup();
	static int32	Start(void* /*data*/);

	dev_t*			to_cleanup;
	uint			devices_count;

private:
	void			ThreadLoop();
	status_t		WriteToFile(BFile* file, char* buffer);
	status_t		FillDirectory(BDirectory* root_directory, char* buffer);
	VMWAddOnsStatusWindow* status_window;
};

#endif
