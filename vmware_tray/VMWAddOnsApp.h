/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef VMW_APP_H
#define VMW_APP_H

#include <Application.h>


class VMWAddOnsApp : public BApplication {
public:
					VMWAddOnsApp();

	virtual void	ReadyToRun();
};

#endif
