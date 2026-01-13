/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef VMW_CLEANUP_H
#define VWM_CLEANUP_H

#include <File.h>
#include <Directory.h>
#include <SupportDefs.h>


class VMWAddOnsStatusWindow;


class VMWAddOnsCleanup {
public:
	static int32	Start(void* /*data*/);

public:
	uint			fDevicesCount;
	dev_t*			fToCleanup;

private:
	status_t		ThreadLoop();
	status_t		WriteToFile(BFile* file, char* buffer);
	status_t		FillDirectory(BDirectory* rootDirectory, char* buffer);
	VMWAddOnsStatusWindow* fStatusWindow;
};


#endif
