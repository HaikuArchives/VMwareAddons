/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWAddOnsApp.h"

#include <stdio.h>

#include <Alert.h>
#include <Deskbar.h>
#include <Roster.h>

#include "VMWAddOns.h"
#include "VMWAddOnsTray.h"

//#define PERSISTENT_TRAY

VMWAddOnsApp::VMWAddOnsApp()
	: BApplication(APP_SIG)
{
}

VMWAddOnsApp::~VMWAddOnsApp()
{
}

void VMWAddOnsApp::ReadyToRun()
{
	BDeskbar db;
	
	// Avoid multiplying the trays...
	db.RemoveItem(TRAY_NAME);

#ifdef PERSISTENT_TRAY
	entry_ref ref;
	be_roster->FindApp(APP_SIG, &ref);
	db.AddItem(&ref);
#else
	db.AddItem(new VMWAddOnsTray);
#endif
	
	// We're done
	be_app->Lock();
	be_app->Quit();
}

int main()
{
	VMWAddOnsApp *app = new VMWAddOnsApp();
	app->Run();	
}
