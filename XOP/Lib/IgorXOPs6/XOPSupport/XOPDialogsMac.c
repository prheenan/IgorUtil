/*	This file contains utilities for Macintosh XOPs that create dialogs.
	Most of these routines are also available on Windows. Comments identify
	those that are Macintosh-specific.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

/*	XOPEmergencyAlert(message)
	
	This routine used by the XOP Toolkit for dire emergencies only.
	You should not need it. Use XOPOKAlert instead.
	
	Thread Safety: XOPEmergencyAlert is not thread-safe.
*/
void
XOPEmergencyAlert(const char* message)
{
	Str255 pTitle;
	Str255 pMessage;
	SInt16 itemHit;
	
	CopyCStringToPascal("Emergency", pTitle);
	CopyCStringToPascal(message, pMessage);
	StandardAlert(kAlertStopAlert, pTitle, pMessage, NULL, &itemHit);
}

/*	The following DLOG/DITL resources are in IGOR's resource fork
	but are used to implement XOP alerts.
*/
#define IGOR_OK_DLOG			1504		// Item 1 is OK.
#define IGOR_YES_NO_DLOG		1505		// Item 1 is OK. Item 2 is No.
#define IGOR_YES_NO_CANCEL_DLOG	1506		// Item 1 is Yes. Item 2 is No. Item 3 is Cancel.
#define IGOR_OK_CANCEL_DLOG		1511		// Item 1 is OK. Item 2 is Cancel.

static int
DoXOPAlert(short dlogID, const char* title, const char* message)
{
	DialogPtr theDialog;
	WindowRef theWindow;
	short hit;
	unsigned char temp[256];
	int result = 0;

	ArrowCursor();

	paramtext(message, "", "", "");

	theDialog = GetNewDialog(dlogID, NULL, (WindowPtr)-1L);		// This must access Igor's data fork which contains the DLOG resources for these dialogs.
	if (theDialog == NULL)
		return -1;
	theWindow = GetDialogWindow(theDialog);
	if (theWindow == NULL)
		return -1;
	
	CopyCStringToPascal(title, temp);
	SetWTitle(theWindow, temp);
	
	ShowDialogWindow(theDialog);
	do {
		ModalDialog(NULL, &hit);
		switch(hit) {
			case 1:						// OK or Yes.
				result = 1;
				break;
			case 2:						// No or Cancel.
				if (dlogID == IGOR_OK_CANCEL_DLOG)
					result = -1;		// Cancel result is -1.
				else
					result = 2;
				break;
			case 3:						// Cancel.
				result = -1;
				break;
		}
	} while(result == 0);
	
	DisposeDialog(theDialog);

	return result;
}

/*	XOPOKAlert(title, message)
	
	Thread Safety: XOPOKAlert is not thread-safe.
*/
void
XOPOKAlert(const char* title, const char* message)
{
	if (!CheckRunningInMainThread("XOPOKAlert"))
		return;

	DoXOPAlert(IGOR_OK_DLOG, title, message);
}

/*	XOPOKCancelAlert(title, message)

	Returns 1 for OK, -1 for cancel.
	
	Thread Safety: XOPOKCancelAlert is not thread-safe.
*/
int
XOPOKCancelAlert(const char* title, const char* message)
{
	if (!CheckRunningInMainThread("XOPOKCancelAlert"))
		return -1;

	return DoXOPAlert(IGOR_OK_CANCEL_DLOG, title, message);
}

/*	XOPYesNoAlert(title, message)

	Returns 1 for yes, 2 for no.
	
	Thread Safety: XOPYesNoAlert is not thread-safe.
*/
int
XOPYesNoAlert(const char* title, const char* message)
{
	if (!CheckRunningInMainThread("XOPYesNoAlert"))
		return 2;

	return DoXOPAlert(IGOR_YES_NO_DLOG, title, message);
}

/*	XOPYesNoCancelAlert(title, message)

	Returns 1 for yes, 2 for no, -1 for cancel.
	
	Thread Safety: XOPYesNoCancelAlert is not thread-safe.
*/
int
XOPYesNoCancelAlert(const char* title, const char* message)
{
	if (!CheckRunningInMainThread("XOPYesNoCancelAlert"))
		return -1;

	return DoXOPAlert(IGOR_YES_NO_CANCEL_DLOG, title, message);
}

/*	SetDialogBalloonHelpID(balloonHelpID)

	Sets the resource ID for the 'hdlg' resource to be used for contextual help.
	If balloonHelpID is -1, this indicates that no contextual help is to be used.
	
	Currently does nothing on Windows.
	
	Prior to Carbon, this routine called the Apple Balloon Help Manager HMSetDialogResID
	routine. With Carbon, Apple dropped support for balloon help. This routine now links
	with the contextual help functionality inside Igor Pro Carbon. It does nothing
	if running with a pre-Carbon version of Igor.
	
	Thread Safety: SetDialogBalloonHelpID is not thread-safe.
*/
void
SetDialogBalloonHelpID(int balloonHelpID)
{
	if (!CheckRunningInMainThread("SetDialogBalloonHelpID"))
		return;

	CallBack2(SET_CONTEXTUAL_HELP_DIALOG_ID, (void *)XOPRefNum(), (void *)balloonHelpID);
}

/*	GetDBox(theDialog, itemNumber, box)

	Thread Safety: GetDBox is not thread-safe.
*/
void
GetDBox(DialogPtr theDialog, int itemNumber, Rect *box)
{
	short type;
	Handle item;
	
	GetDialogItem(theDialog, itemNumber, &type, &item, box);
}

/*	InvalDBox(theDialog, itemNumber)

	Thread Safety: InvalDBox is not thread-safe.
*/
static void
InvalDBox(DialogPtr theDialog, int itemNumber)
{
	WindowRef theWindow;
	Rect box;
	
	theWindow = GetDialogWindow(theDialog);
	GetDBox(theDialog, itemNumber, &box);
	InvalWindowRect(theWindow, &box);
}

/*	HiliteDControl(theDialog, itemNumber, enable)

	Thread Safety: HiliteDControl is not thread-safe.
*/
void
HiliteDControl(DialogPtr theDialog, int itemNumber, int enable)
{
	ControlHandle controlH;
	
	if (XOPGetDialogItemAsControl(theDialog, itemNumber, &controlH) == 0) {
		if (enable)
			ActivateControl(controlH);
		else
			DeactivateControl(controlH);
	}
}

/*	DisableDControl(theDialog, itemNumber)

 	Disable checkbox or radio button.

	Thread Safety: DisableDControl is not thread-safe.
*/
void
DisableDControl(DialogPtr theDialog, int itemNumber)
{
	HiliteDControl(theDialog, itemNumber, 0);
}

/*	EnableDControl(theDialog, itemNumber)

 	Enables of radio button.

	Thread Safety: EnableDControl is not thread-safe.
*/
void
EnableDControl(DialogPtr theDialog, int itemNumber)
{
	HiliteDControl(theDialog, itemNumber, 1);
}

/*	SetRadBut(theDialog, first, last, theButton)

	Given a dialog pointer, a range of items, and a number (theButton) in that range,
	turns theButton on and turns all other buttons in range off.

	Thread Safety: SetRadBut is not thread-safe.
*/
void
SetRadBut(DialogPtr theDialog, int first, int last, int theButton)
{
	ControlHandle controlH;
	int i;
	
	for (i = first; i <= last; i++){
		if (XOPGetDialogItemAsControl(theDialog, i, &controlH) == 0)
			SetControlValue(controlH, i==theButton);
	}
}

/*	GetRadBut(theDialog, itemNumber)

 	Returns state of radio button.

	Thread Safety: GetRadBut is not thread-safe.
*/
int
GetRadBut(DialogPtr theDialog, int itemNumber)
{
	ControlHandle controlH;
	
	if (XOPGetDialogItemAsControl(theDialog, itemNumber, &controlH) == 0)
		return GetControlValue(controlH);
	
	return 0;
}

/*	GetDText(theDialog, theItem, theText)

	Gets text from text item in dialog.
	Returns the number of characters in the text.

	Thread Safety: GetDText is not thread-safe.
*/
int
GetDText(DialogPtr theDialog, int theItem, char *theText)
{
	short type;
	Handle item;
	Rect box;

	// According to Apple, this is the correct routine to use to get a handle
	// to pass to GetDialogItemText, even if embedding (kDialogFlagsUseControlHierarchy)
	// is on.
	GetDialogItem(theDialog, theItem, &type, &item, &box);

	GetDialogItemText(item, (unsigned char*)theText);
	CopyPascalStringToC((unsigned char*)theText, theText);
	return strlen(theText);
}

/*	SetDText(theDialog, theItem, theText)

	Sets text in text item in dialog.

	Thread Safety: SetDText is not thread-safe.
*/
void
SetDText(DialogPtr theDialog, int theItem, const char *theText)
{
	ControlHandle controlH;
	unsigned char temp[256];

	// According to Apple, this is the correct routine to use to get a handle
	// to pass to SetDialogItemText, when embedding (kDialogFlagsUseControlHierarchy)
	// is on.
	if (XOPGetDialogItemAsControl(theDialog, theItem, &controlH) == 0) {
		CopyCStringToPascal(theText, temp);
		SetDialogItemText((Handle)controlH, temp);
	}
}

/*	GetDInt(theDialog, theItem, theInt)

	Gets integer from text item in dialog.
	Returns zero if a number was read from text, non-zero if no number read.

	Thread Safety: GetDInt is not thread-safe.
*/
int
GetDInt(DialogPtr theDialog, int theItem, int *theInt)
{
	char temp[256];

	*theInt = 0;		// In case sscanf finds no number.
	GetDText(theDialog, theItem, temp);
	return(sscanf(temp, "%d", theInt) != 1);
}

/*	SetDInt(theDialog, theItem, theInt)

	Sets text item in dialog to integer.

	Thread Safety: SetDInt is not thread-safe.
*/
void
SetDInt(DialogPtr theDialog, int theItem, int theInt)
{
	char temp[32];
	
	sprintf(temp, "%d", theInt);
	SetDText(theDialog, theItem, temp);
}

/*	GetDInt64(theDialog, theItem, valPtr)

	Gets 64-bit integer from text item in dialog.
	Returns zero if a number was read from text, non-zero if no number read.

	Thread Safety: GetDInt64 is not thread-safe.
*/
int
GetDInt64(DialogPtr theDialog, int theItem, SInt64* valPtr)
{
	char temp[256];
	int result;
	
	*valPtr = 0;		// In case sscanf finds no number.
	GetDText(theDialog, theItem, temp);
	result = sscanf(temp, "%lld", valPtr) != 1;
	return result;
}

/*	SetDInt64(theDialog, theItem, val)

	Sets text item in dialog to specified value.

	Thread Safety: SetDInt64 is not thread-safe.
*/
void
SetDInt64(DialogPtr theDialog, int theItem, SInt64 val)
{
	char temp[32];
	
	sprintf(temp, "%lld", val);
	SetDText(theDialog, theItem, temp);
}

/*	GetDDouble(theDialog, theItem, theDouble)

	Gets 64 bit float from text item in dialog.
	Returns zero if a number was read from text, non-zero if no number read.

	Thread Safety: GetDDouble is not thread-safe.
*/
int
GetDDouble(DialogPtr theDialog, int theItem, double *theDouble)
{
	char temp[256];
	double d;
	int result;

	*theDouble = 0;		// In case sscanf finds no number.
	GetDText(theDialog, theItem, temp);
	result = sscanf(temp, "%lf", &d) != 1;
	*theDouble = d;
	return(result);
}

/*	SetDDouble(theDialog, theItem, theDouble)

	Sets text item in dialog to 64 bit float.

	Thread Safety: SetDDouble is not thread-safe.
*/
void
SetDDouble(DialogPtr theDialog, int theItem, double *theDouble)
{
	char temp[32];
	
	sprintf(temp, "%g", *theDouble);			// HR, 980402: Changed from "%lg" to "%g".
	SetDText(theDialog, theItem, temp);
}

/*	ToggleCheckBox(theDialog, itemNumber)

	Toggles state of check box and returns new value.

	Thread Safety: ToggleCheckBox is not thread-safe.
*/
int
ToggleCheckBox(DialogPtr theDialog, int itemNumber)
{
	ControlHandle controlH;
	int val=0;
	
	if (XOPGetDialogItemAsControl(theDialog, itemNumber, &controlH) == 0) {
		val = !GetControlValue(controlH);
		SetControlValue(controlH, val);
	}
	return val;
}

/*	GetCheckBox(theDialog, itemNumber)

 	Returns state of check box.

	Thread Safety: GetCheckBox is not thread-safe.
*/
int
GetCheckBox(DialogPtr theDialog, int itemNumber)
{
	ControlHandle controlH;
	int val=0;
	
	if (XOPGetDialogItemAsControl(theDialog, itemNumber, &controlH) == 0)
		val = GetControlValue(controlH);
	return val;
}

/*	SetCheckBox(theDialog, itemNumber, val)

	Sets state of check box.
	Returns the new value.

	Thread Safety: SetCheckBox is not thread-safe.
*/
int
SetCheckBox(DialogPtr theDialog, int itemNumber, int val)
{
	ControlHandle controlH;
	
	if (XOPGetDialogItemAsControl(theDialog, itemNumber, &controlH) == 0)
		SetControlValue(controlH, val);
	return val;
}

/*	SelEditItem(theDialog, itemNumber)

	Selects the entire text of the editText item specified by theItem.
	
	Prior to XOP Toolkit 4.0 (Carbon), if itemNumber was 0, this routine used
	the currently-selected edit item. This feature did not fit in with the Carbon
	way of doing things and is no longer supported. The itemNumber parameter must
	be the item number of an edit text item.

	Thread Safety: SelEditItem is not thread-safe.
*/
void
SelEditItem(DialogPtr theDialog, int itemNumber)
{
	char text[256];
	
	if (itemNumber == 0)
		return;
	
	SelectDialogItemText(theDialog, itemNumber, 0, GetDText(theDialog,itemNumber,text));
}

/*	SelMacEditItem(theDialog, itemNumber)

	This is a NOP on Windows.

	On Macintosh, selects the entire text of the edit item specified by itemID and
	sets the focus to that item. If itemID is 0, it selects the entire text for the
	current editText item.

	In Macintosh dialogs, often only edit items can have the focus. Thus, there
	are times when it is convenient for the program to set the focus to a particular
	edit item. On Windows, any item can have the focus and it is usually best to
	leave this in the control of the user. This routine achieves this by selecting
	a particular edit item on Macintosh while doing nothing on Windows.

	Thread Safety: SelMacEditItem is not thread-safe.
*/
void
SelMacEditItem(DialogPtr theDialog, int itemNumber)
{
	SelEditItem(theDialog, itemNumber);
}

/*	DrawDialogCmd(cmd, lineHeight)

	Draws command starting from current pen loc.
	This is a help routine for DisplayDialogCmd.

	Thread Safety: DrawDialogCmd is not thread-safe.
*/
static void
DrawDialogCmd(const char *cmd, int lineHeight)
{
	PenState penState;
	char temp[MAXCMDLEN+1];
	const char *p1, *p2;
	int done = FALSE;
	int h, v;
	int len;
	
	GetPenState(&penState);
	h = penState.pnLoc.h;
	v = penState.pnLoc.v;
	
	p1 = cmd;								// p1 marches through command.
	do {
		p2 = strchr(p1, 0x0d);				// Find CR.
		if (p2 == NULL) {					// No more CRs ?
			strcpy(temp, p1);
			done = TRUE;
		}
		else {
			len = p2-p1;
			strncpy(temp, p1, len);
			temp[len] = 0;
		}
		
		CopyCStringToPascal(temp, (unsigned char*)temp);
		DrawString((unsigned char*)temp);
		
		v += lineHeight;					// Move to next line.
		MoveTo(h, v);
		p1 = p2+1;							// Point to start of next line.
	} while (!done);
}

/*	DisplayDialogCmd(theDialog, dlogItemNo, cmd)

	Displays the command in an IGOR-style dialog. See GBLoadWaveDialog.c
	for an example.
	
	dlogItemNo is the item number of the dialog item in which the command
	is to be displayed. On the Macintosh, this must be a user item. On Windows,
	it must be an EDITTEXT item.

	Thread Safety: DisplayDialogCmd is not thread-safe.
*/
void
DisplayDialogCmd(DialogPtr theDialog, int dlogItemNo, const char* cmd)
{
	WindowRef theWindow;
	CGrafPtr thePort;
	Rect box;
	int font, size;
	int lineHeight;
	FontInfo info;
	RgnHandle saveClipRgnH;
	
	theWindow = GetDialogWindow(theDialog);
	thePort = GetWindowPort(theWindow);
	
	font = GetPortTextFont(thePort);		// Save text characteristics.
	size = GetPortTextSize(thePort);

	TextFont(kFontIDMonaco);
	TextSize(9);
	GetFontInfo(&info);
	lineHeight = info.ascent + info.descent + info.leading;
	
	GetDBox(theDialog, dlogItemNo, &box);
	saveClipRgnH = NewRgn();
	if (saveClipRgnH != NULL) {
		GetClip(saveClipRgnH);
		ClipRect(&box);
		InsetRect(&box, 2, 2);
		EraseRect(&box);
		if (*cmd != 0) {
			MoveTo(box.left+2, box.top + info.ascent + 2);
			DrawDialogCmd(cmd, lineHeight);
		}
		SetClip(saveClipRgnH);
		DisposeRgn(saveClipRgnH);
	}

	TextFont(font);									// Restore font, size, style.
	TextSize(size);
}

/*	SetDialogPort(theDialog)
	
	SetDialogPort sets the current GrafPort on Macintosh and does nothing on
	Windows.
	
	On Macintosh, it returns the current GrafPort before SetDialogPort was
	called. On Windows, it returns theDialog. This routine exists solely to avoid
	the need for an ifdef when you need to deal with the Macintosh current GrafPort
	in cross-platform dialog code. 

	Thread Safety: SetDialogPort is not thread-safe.
*/
CGrafPtr
SetDialogPort(DialogPtr theDialog)
{
	CGrafPtr savePort;
	
	GetPort(&savePort);
	SetPortDialogPort(theDialog);
	return savePort;
}

/*	ShowDialogWindow(theDialog)

	Makes the dialog visible.

	Thread Safety: ShowDialogWindow is not thread-safe.
*/
void
ShowDialogWindow(DialogPtr theDialog)
{
	WindowRef theWindow;
	
	theWindow = GetDialogWindow(theDialog);
	if (theWindow != NULL)
		ShowWindow(theWindow);
}


// PopMenu Routines

/*	GetDialogPopupMenuMenuHandle(theDialog, popupItemNumber, mhP)

	Thread Safety: GetDialogPopupMenuMenuHandle is not thread-safe.
*/
static int
GetDialogPopupMenuMenuHandle(DialogPtr theDialog, int popupItemNumber, MenuHandle* mhP)
{
	ControlRef controlH;
	int err;

	*mhP = NULL;
	
	if (err = XOPGetDialogItemAsControl(theDialog, popupItemNumber, &controlH))
		return err;
		
	*mhP = GetControlPopupMenuHandle(controlH);
	if (*mhP == 0)
		return -1;

	return 0;
}

/*	About Carbon Popup Menus

	These popup menu routines require a number of conditions to be true:
	
		Your dialog has an associated dlgx resource using the kDialogFlagsUseControlHierarchy
		flag making it an Appearance Manager-compliant dialog.

		Your popup menu items are defined in your dialog resources using a control (CNTL)
		dialog item.
		
		The control item references a "popup menu button" CDEF (CDEF ID = kControlPopupButtonProc).
		
		The menu ID field of the CNTL resource must be set to -12345.
*/

/*	ItemIsPopMenu(theDialog, popupItemNumber)

	Returns the truth that the item is a popup menu.
	
	If the item is in fact a popup menu, you must call CreatePopMenu before
	calling this function. CreatePopMenu attaches a menu handle to the control.
	Until that is done, we have no way to know that the control is a popup menu.

	Thread Safety: ItemIsPopMenu is not thread-safe.
*/
int
ItemIsPopMenu(DialogPtr theDialog, int popupItemNumber)
{
	MenuHandle mH;
	int err;
	
	if (popupItemNumber <= 0)		// A filter procedure might return 0 or -1 to signify a hit in no item.
		return 0;
	
	if (err = GetDialogPopupMenuMenuHandle(theDialog, popupItemNumber, &mH))
		return 0;
	
	return 1;
}

/*	SetPopMenuMax(theDialog, popupItemNumber)

	The Mac OS Control Manager needs to be told how many items are in the menu each time we change it.

	Thread Safety: SetPopMenuMax is not thread-safe.
*/
static int
SetPopMenuMax(DialogPtr theDialog, int popupItemNumber)
{
	ControlHandle controlH;
	MenuHandle mH;
	int numMenuItems;
	int err;

	if (err = XOPGetDialogItemAsControl(theDialog, popupItemNumber, &controlH))
		return err;
	
	if (err = GetDialogPopupMenuMenuHandle(theDialog, popupItemNumber, &mH))
		return err;

	numMenuItems = CountMenuItems(mH);				// Number of items in the menu.
	SetControlMaximum(controlH, numMenuItems);		// Tell the control manager.
	
	return 0;
}

/*	InitPopMenus(theDialog)

	Thread Safety: InitPopMenus is not thread-safe.
*/
void
InitPopMenus(DialogPtr theDialog)
{
	// Nothing to do for now.
}

struct XOPPopMenuInfo {
	DialogPtr theDialog;			// Dialog containing the popup menu item.
	int menuID;						// Menu ID of the popup menu.
};
typedef struct XOPPopMenuInfo XOPPopMenuInfo;
typedef struct XOPPopMenuInfo* XOPPopMenuInfoPtr;

static XOPPopMenuInfo* gPopMenuInfoP = NULL;	// Used to keep track of menu IDs available for popup controls.
#define MAX_POPUP_MENUS 50						// Max popup menu handles that we can keep track of using gPopMenuInfoP.
#define FIRST_POPUP_MENU_ID 1150

/*	CreatePopMenu(theDialog, popupItemNumber, titleItemNumber, itemList, initialItem)

	Creates a popup menu in the specified dialog.
	
	popupItemNumber is the dialog item number for the popup menu.
	On Macintosh, this must be a CNTL item representing a popup button control.
	See notes above under "About Carbon Popup Menus".
	
	titleItemNumber is the dialog item number for the static text title for the popup menu.
	Prior to XOP Toolkit 5, on Macintosh, this item was highlighted when the user
	clicked on the popup menu. As of XOP Toolkit 5, it is no longer used by must be
	present for backward compatibility.
	
	itemList is a semicolon-separated list of items to insert into the menu.
	For example, "Red;Green;Blue".
	
	initialItem is the 1-based number of the item in the popup menu that should
	be initially selected.
	
	The menu handle allocated here will be disposed when you call KillPopMenus
	at the end of the dialog.
	
	In contrast to Macintosh menu manager routines, this routine does not
	treat any characters as meta-characters.

	Thread Safety: CreatePopMenu is not thread-safe.
*/
int
CreatePopMenu(DialogPtr theDialog, int popupItemNumber, int titleItemNumber, const char* itemList, int initialItem)
{
	MenuHandle mH;
	ControlHandle controlH;
	int menuID;
	int i;
	int err;

	if (gPopMenuInfoP == NULL) {
		// gPopMenuInfoP is used by KillPopMenusAfterDialogIsKilled to keep track of which menu IDs are free for use with popup menus.
		gPopMenuInfoP = (XOPPopMenuInfo*)NewPtrClear(MAX_POPUP_MENUS*(sizeof(XOPPopMenuInfo)));	// Assume never more than MAX_POPUP_MENUS popups.
		if (gPopMenuInfoP == NULL)
			return NOMEM;
	}
	
	if (err = XOPGetDialogItemAsControl(theDialog, popupItemNumber, &controlH))
		return err;

	/*	Find a free menu ID. Menus IDs from 1100-1199 are reserved for XOPs.
		This routine uses the range 1150 to 1199 for dialog popup menus.
	*/
	mH = NULL;
	menuID = FIRST_POPUP_MENU_ID;
	for(i=0; i<MAX_POPUP_MENUS; i++) {
		if (gPopMenuInfoP[i].menuID == 0) {
			char menuName[64];
			sprintf(menuName, "Menu %d", menuID);
			{
				unsigned char temp[256];
				CopyCStringToPascal(menuName, temp);
				mH = NewMenu(menuID, temp);
			}
			if (mH == NULL)
				return NOMEM;
			break;
		}
		menuID += 1;
	}
	if (mH == NULL)					// Ran out of menu IDs.
		return -1;					// Very unlikely.
	
	FillMenuNoMeta(mH, itemList, strlen(itemList), 0);
	
	/*	Tell the control what menu handle it should use. According to Eric Schlegel from Apple,
		the menu does not need to be in the menu list. If this succeeds, the control now owns
		the menu handle and will dispose it.
	*/
	err = SetControlData(controlH, kControlMenuPart, kControlPopupButtonOwnedMenuRefTag, sizeof(mH), &mH);
	if (err!=0 || mH!=GetControlPopupMenuHandle(controlH)) {
		/*	If here, you are probably need to double-check your popupItemNumber
			or your CNTL resource or your DITL resource.
		*/
		DisposeMenu(mH);
		return err ? err:-1;		// Should never happen.
	}

	SetPopMenuMax(theDialog, popupItemNumber);
	
	// Store the menu ID so we can tell what menu ID is free the next time we create a popup menu.
	gPopMenuInfoP[menuID-FIRST_POPUP_MENU_ID].theDialog = theDialog;
	gPopMenuInfoP[menuID-FIRST_POPUP_MENU_ID].menuID = menuID;
	
	// Set the initially-selected item.
	SetPopItem(theDialog, popupItemNumber, initialItem);
	
	return 0;
}

/*	GetPopMenuHandle(theDialog, popupItemNumber)

	Thread Safety: GetPopMenuHandle is not thread-safe.
*/
MenuHandle
GetPopMenuHandle(DialogPtr theDialog, int popupItemNumber)
{
	MenuHandle mH;
	int err;
	
	if (err = GetDialogPopupMenuMenuHandle(theDialog, popupItemNumber, &mH))
		return NULL;		// Most likely a programmer error (e.g., incorrect DITL/CNTL resources) if this happens.
	
	return mH;
}

/*	GetPopMenu(theDialog,popupItemNumber,selItem,selStr)

 	Returns currently selected item number and string.
	Pass NULL for selStr if you don't care about it.

	Thread Safety: GetPopMenu is not thread-safe.
*/
void
GetPopMenu(DialogPtr theDialog, int popupItemNumber, int *selItem, char *selStr)
{
	ControlHandle controlH;
	MenuHandle mH;
	int err;
	
	*selItem = 1;
	if (selStr != NULL)
		*selStr = 0;

	if (err = XOPGetDialogItemAsControl(theDialog, popupItemNumber, &controlH))
		return;
	
	*selItem = GetControlValue(controlH);
	
	if (selStr != NULL) {
		mH = GetPopMenuHandle(theDialog, popupItemNumber);
		if (mH != NULL) {
			unsigned char temp[256];
			GetMenuItemText(mH, *selItem, temp);
			CopyPascalStringToC(temp, selStr);
		}
	}	
}

/*	SetPopMatch(theDialog, popupItemNumber, selStr)

 	Sets currently selected item to that which matches string (case insensitive).
	Returns item number or zero if there is no match.

	Thread Safety: SetPopMatch is not thread-safe.
*/
int
SetPopMatch(DialogPtr theDialog, int popupItemNumber, const char *selStr)
{
	ControlHandle controlH;
	MenuHandle mH;
	int numMenuItems;
	int i;
	unsigned char temp[256];
	char itemText[256];
	int result;
	int err;

	if (err = XOPGetDialogItemAsControl(theDialog, popupItemNumber, &controlH))
		return 0;
	
	mH = GetPopMenuHandle(theDialog, popupItemNumber);
	if (mH == NULL)
		return 0;
	
	result = 0;
	numMenuItems = CountMenuItems(mH);
	for (i=1; i<=numMenuItems; i++) {
		GetMenuItemText(mH, i, temp);
		CopyPascalStringToC(temp, itemText);
		if (CmpStr(itemText,selStr) == 0) {
			result = i;
			SetControlValue(controlH, result);
			InvalDBox(theDialog, popupItemNumber);		// This can move heap.
			break;
		}
	}
	return result;
}

/*	SetPopItem(theDialog, popupItemNumber, theItem)

 	Makes theItem the currently selected item.

	Thread Safety: SetPopItem is not thread-safe.
*/
void
SetPopItem(DialogPtr theDialog, int popupItemNumber, int theItem)
{
	ControlHandle controlH;
	MenuHandle mH;
	int numMenuItems;
	int err;

	if (err = XOPGetDialogItemAsControl(theDialog, popupItemNumber, &controlH))
		return;
	
	mH = GetPopMenuHandle(theDialog, popupItemNumber);
	if (mH == NULL)
		return;
	
	numMenuItems = CountMenuItems(mH);
	if (theItem<=0 || theItem>numMenuItems)
		return;
	
	SetControlValue(controlH, theItem);
	InvalDBox(theDialog, popupItemNumber);			// This can move heap.
}

/*	KillPopMenus(theDialog)

 	Call when exiting the dialog, before calling DisposeXOPDialog.
 	
 	The menu handles associated with popup controls are automatically deleted
 	by the controls because we use SetControlData(...kControlPopupButtonOwnedMenuRefTag...).
 	However, we need to do some bookkeeping here to keep track of which menu IDs are
 	free for use in popup menus. This bookkeeping is complicated by the possibility
 	of subdialogs.

	Thread Safety: KillPopMenus is not thread-safe.
*/
void
KillPopMenus(DialogPtr theDialog)
{
	int i;
	
	if (gPopMenuInfoP == NULL)
		return;								// Would happen if you did not create any popup menus.
	
	// Mark menu IDs used for this dialog as free so they can be reused for another dialog.
	for(i=0; i<MAX_POPUP_MENUS; i++) {
		if (gPopMenuInfoP[i].theDialog == theDialog) {	// This is the dialog for which the menu was created?
			gPopMenuInfoP[i].theDialog = NULL;
			gPopMenuInfoP[i].menuID = 0;
		}
	}

	/*	Now see if gPopMenuInfoP is no longer needed. If we just disposed a subdialog,
		it is still needed for the parent dialog.
	*/
	{
		int infoStillNeeded;
		infoStillNeeded = 0;
		for(i=0; i<MAX_POPUP_MENUS; i++) {
			if (gPopMenuInfoP[i].menuID != 0) {
				infoStillNeeded = 1;
				break;		
			}
		}
		if (!infoStillNeeded) {
			DisposePtr((Ptr)gPopMenuInfoP);
			gPopMenuInfoP = NULL;
		}
	}
}

/*	AddPopMenuItems(theDialog, popupItemNumber, itemList)

	Adds the contents of itemList to the existing dialog popup menu.
	This can be called only AFTER the popup menu item has been initialized
	by calling CreatePopMenu.
	
	itemList can be a single item ("Red") or a semicolon-separated list of items
	("Red;Green;Blue;"). The trailing semicolon is optional.
	
	In contrast to Macintosh menu manager routines, this routine does not
	treat any characters as meta-characters.

	Thread Safety: AddPopMenuItems is not thread-safe.
*/
void
AddPopMenuItems(DialogPtr theDialog, int popupItemNumber, const char* itemList)
{
	FillPopMenu(theDialog, popupItemNumber, itemList, strlen(itemList), 10000);
}

/*	FillPopMenu(theDialog, popupItemNumber, itemList, itemListLen, afterItem)

	Sets the contents of the existing dialog popup menu.
	This can be called only AFTER the popup menu item has been initialized
	by calling CreatePopMenu.
	
	itemList is a semicolon separated list of items to be put into the popup menu.
	itemListLen is the total number of characters in itemList.
	afterItem specifies where the items in itemList are to appear in the menu.
		afterItem = 0			new items appear at beginning of popup menu
		afterItem = 10000		new items appear at end of popup menu
		afterItem = item number	new items appear after specified existing item number.
	
	In contrast to Macintosh menu manager routines, this routine does not
	treat any characters as meta-characters.

	Thread Safety: FillPopMenu is not thread-safe.
*/
void
FillPopMenu(DialogPtr theDialog, int popupItemNumber, const char *itemList, int itemListLen, int afterItem)
{
	MenuHandle menuH;
	
	menuH = GetPopMenuHandle(theDialog, popupItemNumber);
	if (menuH == NULL)								// Should not happen.
		return;
	FillMenuNoMeta(menuH, itemList, itemListLen, afterItem);
	SetPopMenuMax(theDialog, popupItemNumber);
}

/*	FillWavePopMenu(theDialog, popupItemNumber, match, options, afterItem)

	Sets the contents of the existing dialog popup menu.
	This can be called only AFTER the popup menu item has been initialized
	by calling CreatePopMenu.
	
	Puts names of waves into the popup menu.
	match and options are as for the Igor WaveList() function:
		match = "*" for all waves
		options = "" for all waves
		options = "WIN: Graph0" for waves in graph0 only.
	
	afterItem is as for FillPopMenu, described above.
	
	In contrast to Macintosh menu manager routines, this routine does not
	treat any characters as meta-characters.
	
	Returns 0 if OK or an error code.

	Thread Safety: FillWavePopMenu is not thread-safe.
*/
int
FillWavePopMenu(DialogPtr theDialog, int popupItemNumber, const char *match, const char *options, int afterItem)
{
	MenuHandle menuH;
	int result;	
	
	menuH = GetPopMenuHandle(theDialog, popupItemNumber);
	if (menuH == NULL)								// Should not happen.
		return -1;
	result = FillWaveMenu(menuH, match, options, afterItem);
	SetPopMenuMax(theDialog, popupItemNumber);
	return result;
}

/*	FillPathPopMenu(theDialog, popupItemNumber, match, options, afterItem)

	Sets the contents of the existing dialog popup menu.
	This can be called only AFTER the popup menu item has been initialized
	by calling CreatePopMenu.
	
	Puts names of Igor paths into the popup menu.
	match and options are as for the Igor PathList() function:
		match = "*" for all paths
		options = "" for all paths
	
	afterItem is as for FillPopMenu, described above.
	
	In contrast to Macintosh menu manager routines, this routine does not
	treat any characters as meta-characters.

	Thread Safety: FillPathPopMenu is not thread-safe.
*/
int
FillPathPopMenu(DialogPtr theDialog, int popupItemNumber, const char *match, const char *options, int afterItem)
{
	MenuHandle menuH;
	int result;	
	
	menuH = GetPopMenuHandle(theDialog, popupItemNumber);
	if (menuH == NULL)								// Should not happen.
		return -1;
	result = FillPathMenu(menuH, match, options, afterItem);
	SetPopMenuMax(theDialog, popupItemNumber);
	return result;
}

/*	FillWindowPopMenu(theDialog, popupItemNumber, char *match, char *options, int afterItem)

	Sets the contents of the existing dialog popup menu.
	This can be called only AFTER the popup menu item has been initialized
	by calling CreatePopMenu.
	
	Puts names of Igor windows into the popup menu.
	match and options are as for the Igor WinList() function:
		match = "*" for all windows
		options = "" for all windows
		options = "WIN: 1" for all graphs		( bit 0 selects graphs)
		options = "WIN: 2" for all tables		( bit 1 selects graphs)
		options = "WIN: 4" for all layouts		( bit 2 selects graphs)
		options = "WIN: 3" for all graphs and tables
	
	afterItem is as for FillPopMenu, described above.
	
	In contrast to Macintosh menu manager routines, this routine does not
	treat any characters as meta-characters.

	Thread Safety: FillWindowPopMenu is not thread-safe.
*/
int
FillWindowPopMenu(DialogPtr theDialog, int popupItemNumber, const char *match, const char *options, int afterItem)
{
	MenuHandle menuH;
	int result;	
	
	menuH = GetPopMenuHandle(theDialog, popupItemNumber);
	if (menuH == NULL)								// Should not happen.
		return -1;
	result = FillWinMenu(menuH, match, options, afterItem);
	SetPopMenuMax(theDialog, popupItemNumber);
	return result;
}

/*	DeletePopMenuItems(theDialog, popupItemNumber, afterItem)

	Deletes the contents of the existing dialog popup menu.
	
	afterItem is 1-based. Pass 0 to delete all items.
	
	This can be called only AFTER the popup menu item has been initialized
	by calling CreatePopMenu.

	Thread Safety: DeletePopMenuItems is not thread-safe.
*/
void
DeletePopMenuItems(DialogPtr theDialog, int popupItemNumber, int afterItem)
{
	MenuHandle menuH;
	
	menuH = GetPopMenuHandle(theDialog, popupItemNumber);
	if (menuH == NULL)								// Should not happen.
		return;
	WMDeleteMenuItems(menuH, afterItem);
	SetPopMenuMax(theDialog, popupItemNumber);
}
