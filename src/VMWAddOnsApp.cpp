/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWAddOns.h"
#include "VMWAddOnsApp.h"
#include "VMWAddOnsTray.h"
#include "vmwbackdoor.h"

#include <stdio.h>

#include <Alert.h>
#include <Deskbar.h>
#include <Roster.h>

#define PERSISTENT_TRAY

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
	
	if (VMCheckVirtual() == B_OK) {
#ifdef PERSISTENT_TRAY
		entry_ref ref;
		be_roster->FindApp(APP_SIG, &ref);
		db.AddItem(&ref);
#else
		db.AddItem(new VMWAddOnsTray);
#endif
	} else {
		(new BAlert(TRAY_NAME,
			"Unable to access the VMW backdoor.\n"
			"You are probably not running this program in a VMWare virtual machine.",
			"Quit", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
	}
	
	// We're done
	be_app->Lock();
	be_app->Quit();
}

int main()
{
	VMWAddOnsApp *app = new VMWAddOnsApp();
	app->Run();	
	//puts("hello,world");
}
