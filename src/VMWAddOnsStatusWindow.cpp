/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWAddOnsStatusWindow.h"

#include <stdio.h>

#include <Screen.h>

#define _H(x) static_cast<int>((x)->Frame().Height())
#define _W(x) static_cast<int>((x)->Frame().Width())
#define SPACING 8

#define STOP_OPERATION 'stOp'
#define MB (1024 * 1024)

VMWAddOnsStatusWindow::VMWAddOnsStatusWindow()
	: BWindow(BRect(0, 0, 300, 400), "Progress", B_TITLED_WINDOW, B_NOT_RESIZABLE 
		| B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	status_view = new BView(Bounds(), "cleanup view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	status_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	progress_bar = new BStatusBar(BRect(SPACING, SPACING, 200, 45), NULL, "Starting...");
	
	status_view->AddChild(progress_bar);
	
	stop_button = new BButton(BRect(0, 0, 0, 0), NULL, "Stop", new BMessage(STOP_OPERATION));
	stop_button->ResizeToPreferred();
	
	stop_button->MoveTo(2 * SPACING + _W(progress_bar), 
		progress_bar->Frame().bottom - _H(stop_button));
	
	status_view->AddChild(stop_button);
	
	status_view->ResizeTo(3 * SPACING + _W(progress_bar) + _W(stop_button), 
		2 * SPACING + _H(progress_bar));
	
	ResizeTo(_W(status_view), _H(status_view));	
	AddChild(status_view);
	cancelled = false;
	
	BScreen screen(this);
	if (screen.IsValid()) {
		MoveTo((screen.Frame().Width() - Frame().Width()) / 2,
			(screen.Frame().Height() - Frame().Height()) / 2);
	}
}

VMWAddOnsStatusWindow::~VMWAddOnsStatusWindow()
{
}

void
VMWAddOnsStatusWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case STOP_OPERATION:
			cancelled = true;
			Hide();
		break;
		
		case RESET_PROGRESS:
		{
			const char* name;
			int64 size_read;

			message->FindString("volume_name", &name);
			message->FindInt64("max_size", &size_read);
			
			off_t size = size_read;

			progress_bar->Reset((BString("Cleaning up “") << name << "”" B_UTF8_ELLIPSIS).String());
			progress_bar->SetMaxValue(size);
		}
		
		case UPDATE_PROGRESS:
			progress_bar->Update(MB);
		break;
		
		default:
			BWindow::MessageReceived(message);
		break;
	}
}
