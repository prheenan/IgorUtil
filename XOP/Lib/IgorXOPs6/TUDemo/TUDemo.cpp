/*	TUDemo.c -- for testing and demonstrating the Igor XOP Toolkit "Text Utility" callbacks.
	
	Change History
	
	5/11/94
		Used CloseWindow instead of DisposeWindow.
		Compiled version 1.01.
	
	10/14/96
		Changed GetXOPWindow to not store window record as a global and changed
		CloseWindow back to DisposeWindow. This is because the wStorage parameter
		to GetXOPWindow is not supported under Windows.
	
	1/28/98
		Changed to use XMI1 resource to specify XOP's menu rather than STR# resource,
		which is not supported on Windows.
	
	020919
		Revamped to use Operation Handler so that the TUDemo operation
		can be called from a user function.
	
	091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "TUDemo.h"

#ifdef MACIGOR
	#define TUDEMO_SUBMENU 100			// Menu resource ID is 100.
#endif
#ifdef WINIGOR
	#include "resource.h"				// Get definition of IDR_MENU1.
	#define TUDEMO_SUBMENU IDR_MENU1	// Menu resource ID is IDR_MENU1. IDR_MENU1 is defined by VC++ resource editor or manually if you are not using VC++.
#endif

// Global Variables
static XOP_WINDOW_REF gTheWindow = NULL;
static Handle theTU = NULL;
#ifdef MACIGOR
	static CGrafPtr gThePort = NULL;
#endif

static int
SetStatusArea(const char* message)
{
	return TUSetStatusArea(theTU, message, TU_ERASE_STATUS_WHEN_ANYTHING_HAPPENS, -1);
}

static void
TUDemoDocInfo(void)			// Writes TUDemo doc info to Igor history.
{
	TUDocInfo di;
	TULoc startLoc, endLoc;
	char buf[256];
	
	di.version = TUDOCINFO_VERSION;
	if (TUGetDocInfo(theTU, &di) == 0) {
		sprintf(buf, "Number of paragraphs = %d, permission = %d"CR_STR, di.paragraphs, di.permission);
		XOPNotice(buf);
	}
	
	if (TUGetSelLocs(theTU, &startLoc, &endLoc) == 0) {
		sprintf(buf, "startLoc = (%d,%d), endLoc = (%d,%d)"CR_STR,
				startLoc.paragraph, startLoc.pos, endLoc.paragraph, endLoc.pos);
		XOPNotice(buf);
	}
	
	SetStatusArea("Doc info written to history");
}

static int
TUDemoInsertText(void)
{
	OSType fileTypes = 'TEXT';
	int result;

	result = TUSFInsertFile(theTU, "File to insert in TUDemo window:", &fileTypes, 1);
	if (result == 0)
		SetStatusArea("Text inserted in document");
	return result;
}

static int
TUDemoSaveText(void)
{
	int result;

	result = TUSFWriteFile(theTU, "Save TUDemo text as:", 'TEXT', TRUE);
	if (result == 0)
		SetStatusArea("Text saved in file");
	return result;
}

static void
DumpCharsToHistory(Ptr text, int totalNumChars)
{
	int numCharsInLine, numCharsToDump, offset;
	char buf[256];
	int maxChars;
	int prefixLen;
	char* p;

	numCharsToDump = totalNumChars;
	if (numCharsToDump > 1024)
		numCharsToDump = 1024;

	offset = 0;
	strcpy(buf, ">> ");
	prefixLen = (int)strlen(buf);

	while (offset < numCharsToDump) {
		p = strchr(text + offset, 0x0D);
		if (p)						// Found a CR ?
			numCharsInLine = (int)(p - (text + offset));		// Does not include CR.
		else
			numCharsInLine = numCharsToDump - offset;
		maxChars = (int)(sizeof(buf) - prefixLen - 1 - 1);		// 1 for CR, one for null.
		if (numCharsInLine > maxChars)
			numCharsInLine = maxChars;
		strncpy(buf+prefixLen, text + offset, numCharsInLine);
		buf[prefixLen+numCharsInLine] = 0;
		strcat(buf, CR_STR);
		XOPNotice(buf);
		offset += numCharsInLine + (p != NULL);
	}
	
	if (totalNumChars > numCharsToDump)
		XOPNotice("And so on . . ."CR_STR);

	SetStatusArea("Text written to history");
}

static void
TUDemoFetchSelectedTextTest(void)
{
	Handle h;
	int numSelectedChars;
	char message[256];
	int result;

	*message = 0;
	
	if (result = TUFetchSelectedText(theTU, &h, NULL, 0)) {
		sprintf(message, "Error %d fetching text", result);
	}
	else {
		numSelectedChars = (int)GetHandleSize(h);
		if (numSelectedChars == 0) {
			strcpy(message, "There are no characters selected"CR_STR);
			XOPNotice(message);
		}
		else {
			DumpCharsToHistory(*h, numSelectedChars);
		}
		DisposeHandle(h);
		strcpy(message, "Selected text written to history");
	}

	SetStatusArea(message);
}

static void
WaitTicks(UInt32 numTicksToWait)
{
	UInt32 ticks;

	ticks=TickCount();
	while (TickCount() < ticks + numTicksToWait)
		;
}

static void
TUDemoSetSelectionTest(void)		// This just moves the selection around.
{
	TULoc startLoc, endLoc;
	int i, numIterations;

	numIterations = 5;
	
	if (TUGetSelLocs(theTU, &startLoc, &endLoc) == 0) {
		for (i = 0; i < numIterations; i++) {
			TUSetSelLocs(theTU, &startLoc, &startLoc, 0);
			#ifdef __QUICKDRAWAPI__						// QDFlushPortBuffer requires Mac OS X SDK 10.6 or earlier
				QDFlushPortBuffer(gThePort, NULL);		// Mac OS X buffers the window.
			#endif
			WaitTicks(15);
			TUSetSelLocs(theTU, &startLoc, &endLoc, 0);
			#ifdef __QUICKDRAWAPI__						// QDFlushPortBuffer requires Mac OS X SDK 10.6 or earlier
				QDFlushPortBuffer(gThePort, NULL);		// Mac OS X buffers the window.
			#endif
			WaitTicks(15);
		}
	}
}

/*	TUDemoGetAllTextTest()
	
	Tests the TUSelectAll and TUFetchSelectedText callback.
*/
static void
TUDemoGetAllTextTest(void)
{
	Handle h;
	int numCharsInHandle;
	char buf[256];
	char message[256];
	int err;
	
	h = NULL;
	*message = 0;
	
	err = TUFetchText2(theTU, NULL, NULL, &h, NULL, 0);
	if (err != 0) {
		GetIgorErrorMessage(err, message);
		strcat(message, CR_STR);
		XOPNotice(message);
	}
	else {
		numCharsInHandle = (int)GetHandleSize(h);
		sprintf(buf, "Number of characters in document = %d"CR_STR, numCharsInHandle);
		XOPNotice(buf);
		strcpy(message, buf);
		DumpCharsToHistory(*h, numCharsInHandle);
	}

	if (h != NULL)
		DisposeHandle(h);
	
	SetStatusArea(message);
}

// Operation: TUDemo status

// Runtime param structure for TUDemo operation.
#pragma pack(2)			// All structures passed between Igor and XOP are two-byte aligned.
struct TUDemoRuntimeParams {
	// Flag parameters.

	// Main parameters.

	// Parameters for status keyword group.
	int statusEncountered;
	// There are no fields for this group because it has no parameters.

	// These are postamble fields that Igor sets.
	int calledFromFunction;						// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;						// 1 if called from a macro, 0 otherwise.
};
typedef struct TUDemoRuntimeParams TUDemoRuntimeParams;
typedef struct TUDemoRuntimeParams* TUDemoRuntimeParamsPtr;
#pragma pack()			// Reset structure alignment to default.

extern "C" int
ExecuteTUDemo(TUDemoRuntimeParamsPtr p)
{
	int err = 0;

	if (p->statusEncountered)
		TUDemoDocInfo();

	return err;
}

static int
RegisterTUDemo(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "TUDemo status";
	runtimeNumVarList = "";
	runtimeStrVarList = "";

	/*	We pass NULL here instead of ExecuteTUDemo because we want the operation to be called via
		the EXECUTE_OPERATION message, not directly. This is because we want to be able to unload
		the XOP from memory.
	*/
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(TUDemoRuntimeParams), NULL, 0);
}

/*	XOPMenuItem()

	XOPMenuItem is called when user selects an XOP menu item, if it has one.
	The menuID is the first thing in the IO list and the itemID is in the second thing.
*/
static int
XOPMenuItem(void)
{
	int menuID, itemID, xopSubMenuID;
	int result=0;
	
	menuID = (int)GetXOPItem(0);							// ID of menu user selected.
	itemID = (int)GetXOPItem(1);

	xopSubMenuID = ResourceToActualMenuID(TUDEMO_SUBMENU);	// ID of TUDemo sub menu.
	
	if (menuID == xopSubMenuID) {
		switch (itemID) {
			case TUDemo_Open:
				ShowAndActivateXOPWindow(gTheWindow);
				break;
			
			case TUDemo_Status:
				TUDemoDocInfo();
				break;
				
			case TUDemo_Insert_Text:
				result = TUDemoInsertText();
				break;
				
			case TUDemo_Save_Text:
				result = TUDemoSaveText();
				break;
				
			case TUDemo_GetAllText_Test:
				TUDemoGetAllTextTest();
				break;
				
			case TUDemo_FetchSelectedText_Test:
				TUDemoFetchSelectedTextTest();
				break;
				
			case TUDemo_SetSelection_Test:
				TUDemoSetSelectionTest();
				break;
				
			case TUDemo_Quit:					// Mark VDT as ready to be disposed.
				SetXOPType(TRANSIENT);			// Igor will call with CLEANUP message.
				break;
		}
	}
	return(result);
}

/*	XOPNew()

	XOPNew() is called when a new document is opened in the host application.
	The XOP should reset itself to its default settings.
*/
static void
XOPNew(void)
{
	TUSelectAll(theTU);
	TUDelete(theTU);
	XOPModified = FALSE;			// XOPModified is defined in XOPSupport.c.
}

/*	XOPIdle()

	XOPIdle() is called periodically by the host application when nothing else is going on.
	It allows the XOP to perform periodic, ongoing tasks.
*/
static void
XOPIdle(void)
{
	TUIdle(theTU);
}

/*	WindowMessage(message)

	WindowMessage() handles any window message (update, activate, click, etc).
	Returns an error code or 0 if no error.
*/
static int
WindowMessage(int message)
{	
	XOPIORecParam item0, item1;					// Items from IO list.
	int result=0;
			
	item0 = GetXOPItem(0);						// Get first parameter if any.
	item1 = GetXOPItem(1);						// Get second parameter if any.
	
	switch (message) {
		/*	Igor sends these messages to Macintosh XOPs only. For TU windows on
			Windows, Igor handles them internally.
		*/
		#ifdef MACIGOR				// [
		{
			EventRecord *eventPtr;
			Rect *rectPtr;
			
			case ACTIVATE:
				TUActivate(theTU, item1&activeFlag);
				break;
			case UPDATE:
				TUUpdate(theTU);
				break;
			case KEY:
				eventPtr = (EventRecord *)item1;
				TUKey(theTU, eventPtr);
				XOPModified = TRUE;
				break;
			case GROW:
				TUGrow(theTU, item1);
				break;
			case SETGROW:
				rectPtr = (Rect *)item1;
				rectPtr->left= MIN_WIN_WIDTH;
				rectPtr->top= MIN_WIN_HEIGHT;
				break;
			case CLICK:
				eventPtr = (EventRecord *)item1;
				TUClick(theTU, eventPtr);
				break;
			case NULLEVENT:
				eventPtr = (EventRecord *)item1;
				TUNull(theTU, eventPtr);
				break;
		}
		#endif						// ]
		case CLOSE:
			HideAndDeactivateXOPWindow(gTheWindow);
			break;
		case COPY:
			TUCopy(theTU);
			break;
		case CUT:
			TUCut(theTU);
			XOPModified = TRUE;
			break;
		case PASTE:
			TUPaste(theTU);
			XOPModified = TRUE;
			break;
		case CLEAR:
			TUClear(theTU);
			XOPModified = TRUE;
			break;
		case FIND:
			TUFind(theTU, (int)item1);
			break;
		case DISPLAYSELECTION:
			TUDisplaySelection(theTU);
			break;
		case REPLACE:
			TUReplace(theTU);
			XOPModified = TRUE;
			break;
		case INDENTLEFT:
			TUIndentLeft(theTU);
			XOPModified = TRUE;
			break;
		case INDENTRIGHT:
			TUIndentRight(theTU);
			XOPModified = TRUE;
			break;
		case UNDO:
			TUUndo(theTU);
			break;
		case SELECT_ALL:
			TUSelectAll(theTU);
			break;
		case PAGESETUP:
			TUPageSetupDialog(theTU);
			break;
		case PRINT:
			TUPrint(theTU);
			break;
		case INSERTFILE:
			result = TUDemoInsertText();
			break;
		case SAVEFILE:
			result = TUDemoSaveText();
			break;
		case MOVE_TO_PREFERRED_POSITION:
			TUMoveToPreferredPosition(theTU);
			break;
		case MOVE_TO_FULL_POSITION:
			TUMoveToFullSizePosition(theTU);
			break;
		case RETRIEVE:
			TURetrieveWindow(theTU);
			break;
	}
	return(result);
}

static void
PrepareTUDemoMenuForQuit(void)							// Set menu to dormant state.
{
	MenuHandle menuHandle;
	int i;

	menuHandle = ResourceMenuIDToMenuHandle(TUDEMO_SUBMENU);
	if (menuHandle == NULL)
		return;											// Should never happen.

	for(i = 1; i <= TUDemo_NumberOfMenuItems; i++) {
		if (i == TUDemo_Open)
			EnableItem(menuHandle, i);
		else
			DisableItem(menuHandle, i);
	
	}
}

/*	XOPQuit()

	Called to clean thing up when XOP is about to be disposed.
*/
static void
XOPQuit(void)
{	
	PrepareTUDemoMenuForQuit();
	TUDispose(theTU);							// Discard text document and window.
}

/*	XOPMenuEnable()

	Sets XOPs menu items according to current conditions.
	Before calling us with the MENUENABLE message, Igor disables all
	built-in Igor menu items that are controlled by the front window.
	We can then re-enable those that we want.
	We also need to set our own menu items to the desired state.
*/
static void
XOPMenuEnable(void)
{
	MenuHandle menuHandle;
	char text[32];
	int isActive;
	int i;
	
	isActive = IsXOPWindowActive(gTheWindow);
	if (isActive) {								// If XOP is front window.
		TUFixEditMenu(theTU);
		TUFixFileMenu(theTU);
		SetIgorMenuItem(CLOSE, 1, "Hide", 0);
		
		// This is here mainly to demonstrate the use of the SetIgorMenuItem() callback.
		{
			TULoc startLoc, endLoc;
			int hasSelection = 0;
			if (TUGetSelLocs(theTU,&startLoc,&endLoc) == 0)
				hasSelection = !(startLoc.paragraph==endLoc.paragraph && startLoc.pos==endLoc.pos);
			if (hasSelection)
				strcpy(text, "Print Selected Text");
			else
				strcpy(text, "Print TUDemo Window");
			SetIgorMenuItem(PRINT, 1, text, 0);
		}
		
		SetIgorMenuItem(MOVE_TO_PREFERRED_POSITION, 1, NULL, 0);
		SetIgorMenuItem(MOVE_TO_FULL_POSITION, 1, NULL, 0);
		SetIgorMenuItem(RETRIEVE, 1, NULL, 0);
	}

	menuHandle = ResourceMenuIDToMenuHandle(TUDEMO_SUBMENU);
	if (menuHandle == NULL)
		return;											// Should never happen.

	for(i = 1; i <= TUDemo_NumberOfMenuItems; i++) {
		EnableItem(menuHandle, i);
	}
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP when the message
	specified by the host is other than INIT.
*/
extern "C" void
XOPEntry(void)
{	
	int message;
	XOPIORecResult result = 0;
	
	message = GetXOPMessage();
	if (message & XOPWINDOWCODE) {
		result = WindowMessage(message);			// If window message, we're done.
	}
	else {
		switch (message) {
			case EXECUTE_OPERATION:
				{
					void* params;
					params = (void*)GetXOPItem(1);
					result = ExecuteTUDemo((TUDemoRuntimeParamsPtr)params);
				}
				break;
			case CLEANUP:							// XOP about to be disposed of.
				XOPQuit();
				break;
			case IDLE:								// Time to perform idle functions.
				XOPIdle();
				break;
			case NEW:								// New experiment -- init settings.
				XOPNew();
				break;
			case LOADSETTINGS:						// Loading experiment -- load settings.
				break;
			case SAVESETTINGS:						// Saving experiment -- save settings.
				break;
			case MODIFIED:							// Saving experiment -- save settings.
				result = XOPModified;
				break;
			case MENUITEM:							// XOPs menu item selected.
				result = XOPMenuItem();
				break;
			case MENUENABLE:						// Enable/disable XOPs menu item.
				XOPMenuEnable();
				break;
		}
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
XOPMain(IORecHandle ioRecHandle)		// The use of XOPMain rather than main means this XOP requires Igor Pro 6.20 or later
{
	Rect winRect;
	int err;
	
	XOPInit(ioRecHandle);				// Do standard XOP initialization.
	SetXOPEntry(XOPEntry);				// Set entry point for future calls.
	SetXOPType(RESIDENT | IDLES);		// Specify XOP to stick around and to receive IDLE messages.

	if (igorVersion < 620) {			// Requires Igor 6.20 or later.
		SetXOPResult(OLD_IGOR);			// OLD_IGOR is defined in TUDemo.h and there are corresponding error strings in TUDemo.r and TUDemoWinCustom.rc.
		return EXIT_FAILURE;
	}
	
	if (err = RegisterTUDemo()) {
		SetXOPResult(err);
		return EXIT_FAILURE;
	}
		
	// Create XOP TU record and window.
	winRect.left = 5;
	winRect.right = 500;
	winRect.top = 160;
	winRect.bottom = 260;
	err = TUNew2("TUDemo", &winRect, &theTU, &gTheWindow);	// TUNew2 was added in Igor Pro 3.13.
	if (err != 0) {
		SetXOPResult(err);
		return EXIT_FAILURE;
	}
	ShowAndActivateXOPWindow(gTheWindow);

	#ifdef MACIGOR
		gThePort = GetWindowPort(gTheWindow);
	#endif
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
