/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMWAPP_H
#define VMWAPP_H

#include <Application.h>

class VMWAddOnsApp: public BApplication {
public:
					VMWAddOnsApp();
					~VMWAddOnsApp();
	virtual void	ReadyToRun();
};

#endif
