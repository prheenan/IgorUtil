/*	VDTWin.c

	Windows-specific support for VDT.c.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

static WNDPROC gOriginalWindowProc = NULL;

	
/*	HandleVDTKeyWin(wParam, lParam)

	Called to handle a WM_CHAR message. wParam is the character code.
	lParam is the key data.
	
	Handles a keystroke event by sending the key to the VDT window and/or by
	sending the key to the serial port.
	
	Returns true if the key was sent to the VDT window.
	
	"Online" means that the chosen terminal port is not "None".
	
	The key is sent to the VDT window if we are online and local echo mode is on
	or if we are not online.
	
	The key is sent to the serial port if the terminal port is online.
*/
static int
HandleVDTKeyWin(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	VDTPortPtr tp;
	int terminalIsOnline;
	int sentKeyToVDTWindow;
	
	sentKeyToVDTWindow = 0;

	terminalIsOnline = VDTGetTerminalPortPtr() != NULL;
	tp = NULL;
	if (terminalIsOnline) {
		VDTGetOpenAndCheckTerminalPortPtr(&tp, 1);		// tp may be null.
		if (tp == NULL)
			return 0;									// We are online but can't open the port.
	}
	if ((tp!=NULL && tp->echo) || !terminalIsOnline) {
		// Use this for CWPro2 only.
		// CallWindowProc((FARPROC)gOriginalWindowProc, hwnd, WM_CHAR, wParam, lParam);

		CallWindowProc(gOriginalWindowProc, hwnd, WM_CHAR, wParam, lParam);

		sentKeyToVDTWindow = 1;
	}
	if (tp!=NULL && terminalIsOnline)
		VDTSendTerminalChar(tp, (char)wParam);

	return sentKeyToVDTWindow;
}

/*	VDTWndProc(hwnd, uMsg, wParam, lParam)

	This is the window procedure that filters messages to the VDT window
	before passing them on to the original window procedure which is inside
	Igor. This is necessary so that we can transmit keystrokes to the serial
	port as the user types them. The original window procedure inside Igor
	merely stores the keystrokes in the TU window.
*/
static LRESULT APIENTRY 
VDTWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	switch(uMsg) {
		case WM_CHAR:
			if (HandleVDTKeyWin(hwnd, wParam, lParam))
				XOPModified = 1;
			return 0;
	}

	// Use this for CWPro2 only.
	// return CallWindowProc((FARPROC)gOriginalWindowProc, hwnd, uMsg, wParam, lParam); 

	return CallWindowProc(gOriginalWindowProc, hwnd, uMsg, wParam, lParam); 
} 

void
SubclassVDTWindow(HWND hwnd)
{
	gOriginalWindowProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)VDTWndProc); 
}

void
UnsubclassVDTWindow(HWND hwnd)
{
	if (gOriginalWindowProc != NULL) {
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)gOriginalWindowProc);
		gOriginalWindowProc = NULL;
	}
}
