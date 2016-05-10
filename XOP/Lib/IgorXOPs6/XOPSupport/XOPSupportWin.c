/*	This file contains routines that are Windows-specific.
	This file is used only when compiling for Windows.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

/*	XOPModule()

	Returns XOP's module handle.
	
	You will need this HMODULE if your XOP needs to get resources from its own
	executable file using the Win32 FindResource and LoadResource routines. It is
	also needed for other Win32 API routines. 
	
	Thread Safety: XOPModule is thread-safe. It can be called from any thread.
*/
HMODULE
XOPModule(void)
{
	HMODULE hModule;
	
	hModule = GetXOPModule(XOPRecHandle);
	return hModule;
}

/*	IgorModule(void)

	Returns Igor's HINSTANCE.
	
	You will probably never need this.	
	
	Thread Safety: IgorModule is thread-safe. It can be called from any thread.
*/
HMODULE
IgorModule(void)
{
	HMODULE hModule;
	
	hModule = GetIgorModule();
	return hModule;
}

/*	IgorClientHWND(void)

	Returns Igor's MDI client window.
	
	Some Windows calls require that you pass an HWND to identify the owner of a
	new window or dialog. An example is MessageBox. You must pass IgorClientHWND()
	for this purpose.
	
	Thread Safety: IgorClientHWND is thread-safe. It can be called from any thread.
*/
HWND
IgorClientHWND(void)
{
	HWND hwnd;
	
	hwnd = GetIgorClientHWND();
	return hwnd;
}

void
debugstr(const char *text)			// Emulates Macintosh debugstr.
{
	DebugBreak();					// Break into debugger.
}

/*	SendWinMessageToIgor(hwnd, iMsg, wParam, lParam, beforeOrAfter)

	This is for Windows XOPs only.
	
	You must call this twice from your window procedure - once before you process
	the message and once after. You must do this for every message that you
	receive.
	
	This allows Igor to do certain housekeeping operations that are needed so
	that your window will fit cleanly into the Igor environment.
	
	If the result from SendWinMessageToIgor is non-zero, you should skip processing
	of the message. For example, Igor returns non-zero for click and key-related
	messages while an Igor procedure is running.

	To help you understand why this is necessary, here is a description of what
	Igor does with these messages as of this writing.
	
	NOTE: Future versions of Igor may behave differently, so you must send every
	message to Igor, once before you process it and once after.

	WM_CREATE
		Before: Allocates memory used so that XOP window appears in the Windows menu
				and can respond to user actions like Ctrl-E (send behind) and Ctrl-W
				(close).
		After:	Sets a flag saying that the XOP window is ready to interact with Igor.
	
	WM_DESTROY:
		Before:	Nothing.
		After:	Deallocates memory allocated by WM_CREATE.
	
	WM_MDIACTIVATE (when XOP window is being activated only)
		Before:	Compiles procedure window if necessary.
		After:	Sets Igor menu bar (e.g., removes "Graph" menu from Igor menu bar).
	
	Once Igor has processed the WM_CREATE message (after you have processed it),
	Igor may send messages, such as MENUITEM, MENUENABLE, CUT, and COPY, to your
	XOPEntry routines.
	
	Igor does not send the following messages to Windows XOPs, because these kinds
	of matters are handled through the standard Windows OS messages:
		ACTIVATE, UPDATE, GROW, CLICK, KEY, DRAGGED
	
	Thread Safety: SendWinMessageToIgor is not thread-safe.
*/
int
SendWinMessageToIgor(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam, int beforeOrAfter)
{
	HMODULE hModule;
	int result;
	
	hModule = XOPModule();
	result = HandleXOPWinMessage(hModule, hwnd, iMsg, wParam, lParam, beforeOrAfter);
	return result;
}


/*	PositionWinDialogWindow(theDialog, refWindow)

	Positions the dialog nicely relative to the reference window.

	If refWindow is NULL, it uses the Igor MDI client window. You should pass
	NULL for refWindow unless this is a second-level dialog that you want to
	position nicely relative to the first-level dialog. In that case, pass the
	HWND for the first-level dialog.
	
	Thread Safety: PositionWinDialogWindow is not thread-safe.
*/
void
PositionWinDialogWindow(HWND theDialog, HWND refWindow)
{
	WINDOWPLACEMENT wp;
	RECT childRECT, refRECT;
	int width, height;
	
	if (refWindow == NULL)
		refWindow = IgorClientHWND();
	GetWindowRect(refWindow, &refRECT);

	wp.length = sizeof(wp);
	GetWindowPlacement(theDialog, &wp);
	
	childRECT = wp.rcNormalPosition;
	width = childRECT.right - childRECT.left;
	height = childRECT.bottom - childRECT.top;
	
	childRECT.top = refRECT.top + 20;
	childRECT.bottom = childRECT.top + height;
	childRECT.left = (refRECT.left + refRECT.right)/2 - width/2;
	childRECT.right = childRECT.left + width;
	
	#if 0
	{
		/*	HR, 2013-02-26, XOP Toolkit 6.30: This did not work with multiple monitors
			if Igor was on the second monitor because, among other things, GetSystemMetrics
			returns information about the main screen only. There is no simple solution
			for this so I have removed this check altogether.
		*/

		// Make sure window remains on screen.
	
		int screenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);

		if (childRECT.left < 0) {
			childRECT.left = 0;
			childRECT.right = width;
		}
		if (childRECT.right > screenWidth) {
			childRECT.right = screenWidth;
			childRECT.left = screenWidth - width;
		}
		if (childRECT.bottom > screenHeight) {
			childRECT.bottom = screenHeight;
			childRECT.top = screenHeight - height;
		}
	}
	#endif

	wp.flags = 0;
	wp.rcNormalPosition = childRECT;
	SetWindowPlacement(theDialog, &wp);
}

/*	IsWinDialogItemHitMessage(theDialog, itemID, notificationMessage)
	
	This routine is used to determine if the notification message received
	for a particular dialog item constitutes a "hit" on that item. By "hit",
	we mean an action that signals a change or possible change of the value of
	the item.
	
	notificationMessage is the notification message received by the dialog procedure
	for the item.
	
	Return 1 if the notification message signals a hit on the dialog item, 0 if not.
	
	This routine is Windows-specific.
	
	Thread Safety: IsWinDialogItemHitMessage is not thread-safe.
*/
int
IsWinDialogItemHitMessage(HWND theDialog, int itemID, int notificationMessage)
{
	HWND hwnd;
	char className[32];
	
	extern int gSetDTextInProgress;		// See SetDText in XOPDialogsWin.c for an explanation of this.
	
	hwnd = GetDlgItem(theDialog, itemID);
	if (hwnd == NULL)
		return 0;						// Should not happen.
	
	if (GetClassName(hwnd, className, sizeof(className)) == 0)
		return 0;						// Should not happen.
	
	if (CmpStr(className, "BUTTON")==0 || CmpStr(className, "RADIOBUTTON")==0 || CmpStr(className, "CHECKBOX")==0) {
		if (notificationMessage == BN_CLICKED)
			return 1;
		return 0;
	}
		
	if (CmpStr(className, "COMBOBOX") == 0) {
		// This indicates that the user dropped the menu, may have made a selection change, and then closed it.
		if (notificationMessage==CBN_CLOSEUP)
			return 1;
			
		// This indicates that the user made a selection change while the menu was closed, using the arrow keys.
		if (notificationMessage==CBN_SELCHANGE && SendMessage(hwnd, CB_GETDROPPEDSTATE, 0, 0)==0)
			return 1;
		return 0;
	}
		
	if (CmpStr(className, "EDIT") == 0) {
		if (gSetDTextInProgress == 0) {				// See SetDText for an explanation.
			if (notificationMessage == EN_CHANGE)
				return 1;
		}
		return 0;
	}
	
	return 0;
}

/*	HideDialogItem(theDialog, itemID)

	Hides the dialog item whose ID is itemID in the dialog specified
	by theDialog.
	
	This routine is analogous to a MacOS routine of the same name.
	
	Thread Safety: HideDialogItem is not thread-safe.
*/
void
HideDialogItem(HWND theDialog, int itemID)
{
	HWND hwnd;
	
	hwnd = GetDlgItem(theDialog, itemID);
	if (hwnd == NULL)
		return;
	ShowWindow(hwnd, SW_HIDE);
}

/*	ShowDialogItem(theDialog, itemID)

	Shows (unhides) the dialog item whose ID is itemID in the dialog specified
	by theDialog.
	
	This routine is analogous to a MacOS routine of the same name.
	
	Thread Safety: ShowDialogItem is not thread-safe.
*/
void
ShowDialogItem(HWND theDialog, int itemID)
{
	HWND hwnd;
	
	hwnd = GetDlgItem(theDialog, itemID);
	if (hwnd == NULL)
		return;
	ShowWindow(hwnd, SW_SHOW);
}
