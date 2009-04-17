/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_STATUS_WINDOW_H
#define VMW_STATUS_WINDOW_H

#include <Button.h>
#include <File.h>
#include <StatusBar.h>
#include <View.h>
#include <Window.h>

class VMWAddOnsStatusWindow : public BWindow {
public:
					VMWAddOnsStatusWindow();
	virtual			~VMWAddOnsStatusWindow();
	virtual void	MessageReceived(BMessage* message);
	BStatusBar*		progress_bar;

	bool			cancelled;

private:
	BView*			status_view;
	BButton*		stop_button;
	
	thread_id th;
};

#define RESET_PROGRESS 'rePr'
#define UPDATE_PROGRESS 'upPr'

#endif
