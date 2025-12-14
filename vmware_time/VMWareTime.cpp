/*
 * Copyright 2009-2025, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vincent Duvert, vincent.duvert@free.fr
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include "VMWBackdoor.h"
#include "VMWAddOnsSettings.h"

#include <OS.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	VMWBackdoor backdoor;
	VMWAddOnsSettings settings;

	if (!backdoor.InVMware()) {
		fprintf(stderr, "This add-on is for VMware only!\n");
		return 1;
	}

	while (true) {
		settings.Reload();
		if (settings.GetBool("time_sync_enabled", true)) {
			time_t hostTime;
			if (backdoor.GetHostTime(&hostTime) == B_OK) {
				set_real_time_clock(hostTime);
			}
		}
		snooze(10000000); // 10 seconds
	}

	return 0;
}
