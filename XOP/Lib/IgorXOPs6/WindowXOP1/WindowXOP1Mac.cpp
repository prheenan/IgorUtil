#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "WindowXOP1.h"

// Control command IDs. These IDs are local to the window.
enum {
	kClickMeButtonID = 1
};

void
GetHourMinuteSecond(int* h, int* m, int* s)
{
	DateTimeRec date;
	
	GetTime(&date);
	*h = date.hour;
	*m = date.minute;
	*s = date.second;
}

/*	DoXOPContextualHelp()

	Update Igor Tips window (Macintosh) or status line (Windows).
	This works with Igor Pro Carbon or later. Does nothing with older versions of Igor.
*/
void
DoXOPContextualHelp(WindowPtr theWindow)
{
	ControlRef theControl;
	SInt16 controlPart;
	Rect controlRect;
	UInt32 commandID;
	Point mouse;
	char message[256];
	GrafPtr savePort;
	
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	
	*message = 0;
	
	GetMouse(&mouse);
	theControl = FindControlUnderMouse(mouse, theWindow, &controlPart);
	if (theControl != NULL) {
		GetControlBounds(theControl, &controlRect);
		if (GetControlCommandID(theControl, &commandID) != 0)
			commandID = 0;						// Should not happen.
		switch(commandID) {
			case kClickMeButtonID:
				strcpy(message, "This contextual help is displayed by the XOPSetContextualHelpMessage routine.");
				break;
		}
	}
	
	if (*message != 0)
		XOPSetContextualHelpMessage(theWindow, message, &controlRect);
	
	SetPort(savePort);
}

/*	DrawXOPWindow(wPtr)

	DrawXOPWindow draws the contents of WindowXOP1's window.
*/
void
DrawXOPWindow(WindowPtr theWindow)
{
	int hour, minute, second;
	char notice[256];
	unsigned char temp[256];
	Rect portRect;
	GrafPtr thePort, savePort;
	
	GetPort(&savePort);
	thePort = GetWindowPort(theWindow);
	SetPort(thePort);
	GetHourMinuteSecond(&hour, &minute, &second);
	GetPortBounds(thePort, &portRect);
	EraseRect(&portRect);
	DrawControls(theWindow);
	sprintf(notice, "The time is: %02d:%02d:%02d", hour, minute, second);
	MoveTo(15,25);
	CopyCStringToPascal(notice, temp);
	DrawString(temp);
	if (theWindow == GetActiveWindowRef())
		strcpy(notice, "Active");
	else
		strcpy(notice, "Inactive");
	MoveTo(15,40);
	CopyCStringToPascal(notice, temp);
	DrawString(temp);
	SetPort(savePort);
}

/*	DisplayWindowXOP1Message(theWindow, message1, message2)

	Used to display a message when user selects menu item or clicks button.
*/
void
DisplayWindowXOP1Message(WindowPtr theWindow, const char* message1, const char* message2)
{
	UInt32 ticks;
	Rect r;
	unsigned char temp[256];
	GrafPtr thePort, savePort;
	
	GetPort(&savePort);
	thePort = GetWindowPort(theWindow);
	SetPort(thePort);
	GetPortBounds(thePort, &r);
	r.bottom = (r.top+r.bottom) / 2;			// Avoid erasing the button
	EraseRect(&r);
	MoveTo(15,25);
	CopyCStringToPascal(message1, temp);
	DrawString(temp);
	MoveTo(15,40);
	CopyCStringToPascal(message2, temp);
	DrawString(temp);
	QDFlushPortBuffer(thePort, NULL);
	SetPort(savePort);
		
	ticks = TickCount();						// Let momentous notice be seen.
	while (TickCount() < ticks+120)				// A Mac tick is about 1/60 of a second.
		;
}

void
XOPWindowClickMac(WindowPtr theWindow, EventRecord* ep)
{
	WindowPtr whichWindow;
	ControlRef theControl;
	Point where;
	int what;
	GrafPtr thePort, savePort;
	
	GetPort(&savePort);
	thePort = GetWindowPort(theWindow);
	SetPort(thePort);
	
	what = FindWindow(ep->where, &whichWindow);
	switch(what) {
		case inContent:
			where = ep->where;
			GlobalToLocal(&where);
			what = FindControl(where, theWindow, &theControl);
			switch(what) {
				case kControlButtonPart:
					what = TrackControl(theControl, where, NULL);
					switch(what) {
						case kControlButtonPart:
							DisplayWindowXOP1Message(theWindow, "Thank you for clicking", "WindowXOP1's button.");
							break;
					}
					break;
			}
			break;	
	}

	SetPort(savePort);
}

WindowPtr
CreateXOPWindow(void)
{
	WindowPtr theWindow;
	ControlHandle theControl;
	
	theWindow = GetXOPWindow(XOP_WIND, NIL, (WindowPtr)-1L);
	if (theWindow != NULL) {
		Rect bounds;
		SetRect(&bounds, 55, 60, 135, 80);
		theControl = NewControl(theWindow, &bounds, "\pClick Me", 1, 0, 0, 1, pushButProc, 0L);
		if (theControl != NULL)
			SetControlCommandID(theControl, kClickMeButtonID);
	}
	return theWindow;
}

void
DestroyXOPWindow(WindowPtr theWindow)
{
	TellIgorWindowStatus(theWindow, WINDOW_STATUS_ABOUT_TO_KILL, 0);
	KillControls(theWindow);
	DisposeWindow(theWindow);
	TellIgorWindowStatus(theWindow, WINDOW_STATUS_DID_KILL, 0);	// This message was added in Igor Pro 6.04.
}
