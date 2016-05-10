/*	WindowXOP1.c -- A sample Igor external operation

	WindowXOP1 illustrates how an XOP can add a window to Igor and how to handle
	various events in the window.

	WindowXOP1 adds a menu item to the Misc menu. When you select this menu
	item, WindowXOP1 opens a small window with a button.
	
	If you click the button, WindowXOP1 displays a message.
	
	If you click the close box, WindowXOP1 hides its window.
	
	HR, 980309:
		Previously, when you closed the window, I made WindowXOP1 transient
		and therefore Igor discarded it from memory. This causes complications
		on Windows having to do with unregistering the window class. Also, most
		XOP programmers will probably want their XOP to remain resident. Therefore,
		I have changed WindowXOP1 to remain resident.
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "WindowXOP1.h"

// Global Variables
XOP_WINDOW_REF gTheWindow = NULL;

static void
EnableOrDisableXOPMenuItem(void)
{
	MenuHandle menuH;
	int itemNumber;
	int enable;
	
	itemNumber = ResourceToActualItem(MISCID, 1);	// Find the actual item number for our menu item.
	menuH = GetMenuHandle(MISCID);					// Get menu handle for IGOR's Misc menu.
	enable = IsXOPWindowActive(gTheWindow) || !IsWindowVisible(gTheWindow);
	if (enable)
		EnableItem(menuH, itemNumber);
	else
		DisableItem(menuH, itemNumber);
}

/*	XOPMenuItem()

	XOPMenuItem is called when user selects XOPs menu item, if it has one.
*/
static int
XOPMenuItem(void)
{
	if (!IsWindowVisible(gTheWindow))
		ShowAndActivateXOPWindow(gTheWindow);
	DisplayWindowXOP1Message(gTheWindow, "Thank you for selecting", "WindowXOP1's menu item.");
	return 0;
}

/*	XOPIdle()

	XOPIdle() is called periodically by the host application when nothing else is going on.
	It allows the XOP to perform periodic, ongoing tasks.
*/
static void
XOPIdle(void)
{
	UInt32 ticks;									// Current tick count.
	
	static UInt32 lastTicks = 0;					// Ticks last time XOP idled.
	
	if (!IsWindowVisible(gTheWindow))
		return;
	
	#ifdef MACIGOR
		ticks = TickCount();						// Find current ticks.
		if (ticks < lastTicks+60)					// Update every second.
			return;
	#endif
	
	#ifdef WINIGOR
		ticks = GetTickCount();						// Find current ticks.
		if (ticks < lastTicks+1000)					// Update every second.
			return;
	#endif
	
	DrawXOPWindow(gTheWindow);
	lastTicks = ticks;
}

/*	WindowMessage()

	WindowMessage() handles any window message (update, activate, click, etc).
	It returns true if the message was a window message or false otherwise.

	Windows XOPs do not receive messages such as UPDATE, ACTIVATE and CLICK from Igor.
	Instead, they receive WM_PAINT and WM_MDIACTIVATE messages directly from the Windows OS.
*/
static int
WindowMessage(void)
{
	XOPIORecParam item0;							// Item from IO list.
	int message;
	
	message = GetXOPMessage();
	
	if ((message & XOPWINDOWCODE) == 0)
		return 0;
		
	item0 = GetXOPItem(0);
	
	switch (message) {
		#ifdef MACIGOR				// [
			case UPDATE:
				{
					WindowPtr wPtr;
					wPtr = (WindowPtr)item0;
					BeginUpdate(wPtr);
					DrawXOPWindow(wPtr);
					EndUpdate(wPtr);
				}
				break;
			
			case ACTIVATE:
				{
					WindowPtr wPtr;
					int modifiers;
					
					wPtr = (WindowPtr)item0;
					DrawXOPWindow(wPtr);
					
					modifiers = GetXOPItem(1);
					if (modifiers & activeFlag)
						TellIgorWindowStatus(wPtr, WINDOW_STATUS_ACTIVATED, 0);
					else
						TellIgorWindowStatus(wPtr, WINDOW_STATUS_DEACTIVATED, 0);
				}
				break;

			case CLICK:
				{
					WindowPtr wPtr;
					EventRecord* ePtr;
					wPtr = (WindowPtr)item0;
					ePtr = (EventRecord*)GetXOPItem(1);
					XOPWindowClickMac(wPtr, ePtr);
				}
				break;
		#endif						// MACIGOR ]
			
		case CLOSE:									// Click in close box.
			HideAndDeactivateXOPWindow(gTheWindow);
			TellIgorWindowStatus(gTheWindow, WINDOW_STATUS_DID_HIDE, 0);
			break;
			
		case XOP_HIDE_WINDOW:						// Added in Igor Pro 6.
			{
				XOP_WINDOW_REF windowRef;
				windowRef = (XOP_WINDOW_REF)GetXOPItem(0);
				HideXOPWindow(windowRef);
				TellIgorWindowStatus(windowRef, WINDOW_STATUS_DID_HIDE, 0);
			}
			break;
			
		case XOP_SHOW_WINDOW:						// Added in Igor Pro 6.
			{
				XOP_WINDOW_REF windowRef;
				windowRef = (XOP_WINDOW_REF)GetXOPItem(0);
				ShowXOPWindow(windowRef);
				TellIgorWindowStatus(windowRef, WINDOW_STATUS_DID_SHOW, 0);
			}
			break;
		
		case NULLEVENT:				// This is not sent on Windows. Instead, similar processing is done in response to the WM_MOUSEMOVE message.
			ArrowCursor();
			DoXOPContextualHelp(gTheWindow);		// Update Igor Tips window (Macintosh) or status line (Windows).
			break;
	}												// Ignore other window messages.
	return 1;
}

/*	XOPQuit()

	Called to clean thing up when XOP is about to be disposed.
	This happens when Igor is quitting.
*/
static void
XOPQuit(void)
{	
	if (gTheWindow != NULL) {
		TellIgorWindowStatus(gTheWindow, WINDOW_STATUS_ABOUT_TO_KILL, 0);
		DestroyXOPWindow(gTheWindow);
		gTheWindow = NULL;
	}
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP for all messages after the
	INIT message.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;
	
	if (WindowMessage())							// Handle all messages related to XOP window.
		return;

	switch (GetXOPMessage()) {
		case CLEANUP:								// XOP about to be disposed of.
			XOPQuit();								// Do any necessary cleanup.
			break;
		
		case IDLE:									// Time to perform idle functions.
			XOPIdle();
			break;
		
		case MENUITEM:								// XOPs menu item selected.
			result = XOPMenuItem();
			break;
		
		case MENUENABLE:							// Enable/disable XOPs menu item.
			EnableOrDisableXOPMenuItem();
			if (IsXOPWindowActive(gTheWindow))			// HR, 080226: Added this test.
				SetIgorMenuItem(CLOSE, 1, "Hide", 0);	// Set the Close item in the Windows menu to say "Hide".
			break;
	}

	SetXOPResult(result);
}

/*	XOPMain(ioRecHandle)

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.
	
	XOPMain does any necessary initialization and then sets the XOPEntry field of the
	ioRecHandle to the address to be called for future messages.
*/

HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)			// The use of XOPMain rather than main means this XOP requires Igor Pro 6.20 or later
{	
	XOPInit(ioRecHandle);					// Do standard XOP initialization.
	
	SetXOPEntry(XOPEntry);					// Set entry point for future calls.
	SetXOPType(RESIDENT | IDLES);			// Specify XOP to stick around and to receive IDLE messages.

	if (igorVersion < 620) {
		SetXOPResult(OLD_IGOR);				// OLD_IGOR is defined in WindowXOP1.h and there are corresponding error strings in WindowXOP1.r and WindowXOP1WinCustom.rc.
		return EXIT_FAILURE;
	}

	#ifdef WINIGOR
	{
		int err;
		if (err = CreateXOPWindowClass()) {
			SetXOPResult(err);				// Should not happen.
		return EXIT_FAILURE;
		}
	}
	#endif

	// Load WindowXOP1's window.
	gTheWindow = CreateXOPWindow();
	if (gTheWindow == NULL) {
		XOPNotice("WindowXOP1 was unable to create a window"CR_STR);
		SetXOPResult(-1);					// Tells Igor initialization failed. Igor will remove XOP from memory.
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
