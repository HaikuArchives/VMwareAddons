/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef VMW_STATUS_WINDOW_H
#define VMW_STATUS_WINDOW_H

#include <Button.h>
#include <StatusBar.h>
#include <View.h>
#include <Window.h>


#define RESET_PROGRESS 'rePr'
#define UPDATE_PROGRESS 'upPr'


class VMWAddOnsStatusWindow : public BWindow {
public:
					VMWAddOnsStatusWindow();
	virtual void	MessageReceived(BMessage* message);

public:
	bool			fCancelled;

private:
	BStatusBar*		fProgressBar;
	BView*			fStatusView;
	BButton*		fStopButton;
};


#endif
