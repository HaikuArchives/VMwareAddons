/*
	Author:
	s40in, 2021
*/

#include <stdio.h>
#include <sys/ioctl.h>
#include <Accelerant.h>

/* Get some info about the device */
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info * adi) 
{

	/* no info on version is provided, so presumably this is for my info */
	adi->version = 1;

	sprintf(adi->name, "VMware SVGA");
	sprintf(adi->chipset, "VM");
	sprintf(adi->serial_no, "None");
	//adi->memory = (si->ps.memory_size * 1024);
	//adi->dac_speed = si->ps.max_dac1_clock;

	return B_OK;
}
