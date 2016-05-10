/*	VDT.c -- implements a dumb terminal (very dumb terminal) as an Igor XOP

	VDT is documented in the 'VDT Documentation' file on the Igor Extras disk
	and in the "VDT Help" file in Igor Pro's "More Help Files:Data Acquisition"
	folder.
	
	Change History
	
	6/10/92
		For Igor Pro 2.0 and beyond, added 'Save File' and 'Insert File' items to
		VDT menu because these items have been removed from Igor Pro.
		
	5/18/93
		Fixed Hex2Long in VDTIO.c to treat 2 and 4 character hex values as SIGNED.
		Previously, it treated them as signed but treated 8 character values as signed.
	
	3/28/94
		Changed some things to check igorVersion. Compiled VDT 1.20.
	
	5/11/94
		Used CloseWindow instead of DisposeWindow.
		Compiled version 1.21.
	
	8/24/94
		Set routines that require 68K assembly to compile under MPW for 68K only,
		not under THINK or PowerPC compilers. These are the hex IO routines,
		VDTReadHex, VDTWriteHex, VDTReadHexWave and VDTWriteHexWave.
	
	1/12/96
		Removed calls to 68K assembly so that hex I/O routines can work in PPC version.
		Added error checking after InitVDT.
		Prevented idle loop from attempting to access driver if the driver is not open.

	9/9/96 for version 1.31
		Fixed timeout in binary operations. VDTGetBinaryFlags was treating the timeout
		parameter as a number of ticks when it should be a number of seconds.
	
	4/25/97
		Changed GetXOPWindow to pass NULL for wStorage parameter as required under Windows.
		As a consequence of this, I changed CloseWindow instead of DisposeWindow.
	
	9/11/97 for version 2.0
		Reorganized and ported to Windows.
	
	9/14/97 for version 2.0
		Split out dumb-terminal-related routines into VDTTerminal.c.

	9/15/97 for version 2.0
		Split out dialog-related routines into VDTDialog.c.

	3/18/98 for version 2.1
		Revamped so that VDT for Windows can be compiled by XOP Toolkit users.
		Because of this, this version requires Igor Pro 3.13 or later.
		
	021024:
		Totally revamped using Operation Handler so that VDT operations are
		directly usable in user functions.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#define VDTMAIN
#include "VDT.h"

// Global Variables
int gSettingsModified = 0;					// True if need to save new settings.
static int gUseExpSettings = 0;				// Should VDT reset settings when experiment loaded ?.
static XOP_WINDOW_REF gVDTWindow = NULL;
static int gDisposeInProgress = 0;			// Flag used by XOPEntry.
Handle gVDTTU = NULL;


static void
ShowVDTWindow(void)
{
	if (gVDTWindow != NULL)
		ShowAndActivateXOPWindow(gVDTWindow);
}

static void
HideVDTWindow(void)
{
	if (gVDTWindow != NULL)
		HideAndDeactivateXOPWindow(gVDTWindow);
}

/*	VDTAlert(message, strID, itemNum)

	Puts up alert.
	If message is not NULL, uses that as message.
	Otherwise, gets message from STR# resource identified by strID and itemNum.
*/
void
VDTAlert(char *message, int strID, int itemNum)
{
	char alert[256];

	if (message == NULL)
		GetXOPIndString(alert, strID, itemNum);
	else
		strcpy(alert, message);
	XOPOKAlert("VDT2 Error", alert);
}

void
ErrorAlert(int errorCode)		// errorCode can be VDT error, Igor error or system error code.
{
	if (errorCode>=FIRST_XOP_ERR && errorCode<=LAST_XOP_ERR)
		VDTAlert(NULL, XOP_ERRS_ID, errorCode-FIRST_XOP_ERR);
	else
		IgorError("VDT2 Error", errorCode);
}

void
ExplainPortError(const char* preamble, int err)
{
	char errMessage[256];
	char message[256];
	
	if (err>=FIRST_XOP_ERR && err<=LAST_XOP_ERR)		// VDT error.
		GetXOPIndString(errMessage, XOP_ERRS_ID, err-FIRST_XOP_ERR);
	else
		sprintf(errMessage, "Error number %d", err);	// Igor or system error.
	strcpy(message, preamble);
	strcat(message, errMessage);
	VDTAlert(message, 0, 0);
}

int
CheckPort(VDTPortPtr pp, int doAlert)
{
	int result=0;
	
	if (pp == NULL)	{							// Couldn't open port?
		result = CANT_INIT;
	}
	else {
		if (!VDTPortIsOpen(pp))
			result = NO_PORT;
	}
	if (result && doAlert)
		VDTAlert(NULL, 1100, result-FIRST_XOP_ERR);
	return result;
}

int
VDTNoCharactersSelected(void)
{
	TULoc startLoc, endLoc;
	
	TUGetSelLocs(gVDTTU, &startLoc, &endLoc);
	return startLoc.paragraph==endLoc.paragraph && startLoc.pos==endLoc.pos;
}

static int
VDTInsertText(void)
{
	OSType fileTypes;
	
	#ifdef MACIGOR
		fileTypes = 'TEXT';			// We want to insert a file containing plain text.
	#endif
	#ifdef WINIGOR
		fileTypes = '\?\?\?\?';		// On Windows, there is no extension that covers all files with plain text, so allow any file.
	#endif

	return(TUSFInsertFile(gVDTTU, "File to insert in VDT window:", &fileTypes, 1));
}

static int
VDTSaveText(void)
{
	return(TUSFWriteFile(gVDTTU, "Save VDT text as:", 'TEXT', TRUE));		// Write TEXT file on Mac, .txt file on Windows.
}

static void
CheckPortMenuItems(int portMenuID, int quittingVDT)
{
	MenuHandle portMenuHandle;
	VDTPortPtr pp;
	char portName[MAX_OBJ_NAME+1];
	unsigned char itemText[MAX_OBJ_NAME+1];
	int i, numItems;
	
	portMenuHandle = ResourceMenuIDToMenuHandle(portMenuID);
	if (portMenuHandle == NULL)
		return;											// Should never happen.

	if (portMenuID == VDT_TERMINAL_PORT_MENUID)
		pp = VDTGetTerminalPortPtr();					// This may be null.
	else
		pp = VDTGetOperationsPortPtr();					// This may be null.
	if (pp == NULL)
		strcpy(portName, "Off Line");					// Last item in menu is always "Off Line".
	else
		strcpy(portName, pp->name);

	numItems = CountMItems(portMenuHandle);
	for(i=1; i<=numItems; i++) {
		getmenuitemtext(portMenuHandle, i, (char*)itemText);
		if (!quittingVDT && CmpStr((char*)itemText, portName)==0)
			CheckItem(portMenuHandle, i, 1);
		else
			CheckItem(portMenuHandle, i, 0);
	}
}

/*	XOPMenuItem()

	XOPMenuItem is called when user selects XOPs menu item, if it has one.
	The menuID is the first thing in the IO list and the itemID is in the second thing.
*/
static int
XOPMenuItem(void)
{
	VDTPortPtr tp;
	int menuID, itemID;
	
	// Make sure to get all XOP items before doing another callback to Igor.
	menuID = (int)GetXOPItem(0);
	itemID = (int)GetXOPItem(1);

	/*	This is another callback to Igor. It would clobber the menuID and itemID if
		we did it before calling GetXOPItem(0) and GetXOPItem(1).
	*/
	menuID = ActualToResourceMenuID(menuID);
	
	tp = NULL;
	
	if (menuID == VDT_MENUID) {					// Main menu?
		switch (itemID) {
			case VDT_ITEM_OPEN_VDT_WINDOW:
				ShowVDTWindow();
				break;
			
			case VDT_ITEM_SETTINGS_DIALOG:
				{
					int useExpSettings;
					if (VDTSettingsDialog(gUseExpSettings, &useExpSettings) != 0)
						break;
					gSettingsModified = 1;		// Mark settings to be written to preferences.
					gUseExpSettings = useExpSettings;
				}
				break;
			
			case VDT_ITEM_SAVEFILE:
				VDTSaveText();
				break;
			
			case VDT_ITEM_INSERTFILE:
				VDTInsertText();
				break;
			
			case VDT_ITEM_SENDFILE:
				VDTGetOpenAndCheckTerminalPortPtr(&tp, 1);		// tp may be null.
				if (tp == NULL)
					break;						// No terminal port is selected or other error.
				if (tp->terminalOp == OP_SENDFILE)
					VDTAbortTerminalOperation();
				else
					VDTSendFile();
				break;
			
			case VDT_ITEM_RECEIVEFILE:
				VDTGetOpenAndCheckTerminalPortPtr(&tp, 1);		// tp may be null.
				if (tp == NULL)
					break;						// No terminal port is selected or other error.
				if (tp->terminalOp == OP_RECEIVEFILE)
					VDTAbortTerminalOperation();
				else
					VDTReceiveFile();
				break;
			
			case VDT_ITEM_SENDTEXT:
				VDTGetOpenAndCheckTerminalPortPtr(&tp, 1);		// tp may be null.
				if (tp == NULL)
					break;						// No terminal port is selected or other error.
				if (tp->terminalOp == OP_SENDTEXT)
					VDTAbortTerminalOperation();
				else
					VDTSendText();
				break;
			
			case VDT_ITEM_HELP:
				XOPDisplayHelpTopic("", "VDT2 - Serial Port Support", 0);
				break;
		}
	}

	// Handle selection in Terminal Port menu or Operations Port menu.
	while (menuID==VDT_TERMINAL_PORT_MENUID || menuID==VDT_OPERATIONS_PORT_MENUID) {
		VDTPortPtr pp;
		MenuHandle portMenuHandle;
		unsigned char itemText[MAX_OBJ_NAME+1];
		
		portMenuHandle = ResourceMenuIDToMenuHandle(menuID);
		if (portMenuHandle == NULL)
			break;										// Should never happen.
		getmenuitemtext(portMenuHandle, itemID, (char*)itemText);
		pp = FindVDTPort(NULL, (char*)itemText);		// This will be NULL if user chose Off Line.
		if (menuID == VDT_TERMINAL_PORT_MENUID)
			VDTSetTerminalPortPtr(pp);					// OK if pp is NULL.
		else
			VDTSetOperationsPortPtr(pp);				// OK if pp is NULL.
		CheckPortMenuItems(menuID, 0);
		if (pp != NULL) {
			int err;
			err = OpenVDTPort((char*)itemText, &pp);	// Does nothing if port is already open.
			if (err != 0) {
				char temp[256];

				if (menuID == VDT_TERMINAL_PORT_MENUID)
					VDTSetTerminalPortPtr(NULL);
				else
					VDTSetOperationsPortPtr(NULL);

				sprintf(temp, "While trying to open the %s port", itemText);
				ExplainPortError(temp, err);
			}
		}
		gSettingsModified = 1;			// This is so that new port assignment will be written to Igor Prefs file.
		
		break;
	}

	return 0;
}

/*	XOPNew()

	XOPNew() is called when a new document is opened in the host application. The XOP should
	reset itself to its default settings.
*/
static void
XOPNew(void)
{
	TUSelectAll(gVDTTU);
	TUDelete(gVDTTU);
	XOPModified = FALSE;
}

/*	GetVDTSettingsHandle(hPtr)

	Returns via hPtr a handle containing the current VDT setup or NULL if can't get memory.
	
	Function result is 0 if OK or error code.
*/
static int
GetVDTSettingsHandle(VDTSettingsHandle* hPtr)
{
	VDTPortPtr tp, op;
	int index, numPorts, numBytes;
	VDTSettingsHandle vsHandle;
	VDTSettingsPtr vsPtr;
	VDTPortSettingsPtr psp;
	int winState;
	
	*hPtr = NULL;
	
	// Find number of open ports (may be zero).
	numPorts = 0;
	index = 0;
	while(IndexedVDTPort(NULL, index) != NULL) {
		index += 1;
		numPorts += 1;
	}
	
	numBytes = (int)(sizeof(VDTSettings) + (numPorts-1)*sizeof(VDTPortSettings));	// -1 because the VDTSettings field has one VDTPortSettings record in it.
	vsHandle = (VDTSettingsHandle)NewHandle(numBytes);
	if (vsHandle == NULL)
		return NOMEM;
	vsPtr = *vsHandle;
	MemClear(vsPtr, numBytes);

	vsPtr->version = VDT_VERSION;
	vsPtr->useExpSettings = gUseExpSettings;
	op = VDTGetOperationsPortPtr();
	if (op != NULL)
		strcpy(vsPtr->opPortName, op->name);
	tp = VDTGetTerminalPortPtr();
	if (tp != NULL)
		strcpy(vsPtr->termPortName, tp->name);

	GetXOPWindowPositionAndState(gVDTWindow, &vsPtr->winRect, &winState);
	vsPtr->winState = winState;
	
	vsPtr->numPortSettings = numPorts;
	
	for(index=0; index<numPorts; index++) {
		VDTPortPtr pp;
		pp = IndexedVDTPort(NULL, index);
		psp = &vsPtr->ps[index];
		strcpy(psp->name, pp->name);
		strcpy(psp->inputChan, pp->inputChan);
		strcpy(psp->outputChan, pp->outputChan);
		psp->baud = pp->baud;
		psp->parity = pp->parity;
		psp->stopbits = pp->stopbits;
		psp->databits = pp->databits;
		psp->echo = pp->echo;
		psp->inShake = pp->inShake;
		psp->outShake = pp->outShake;
		psp->terminalEOL = pp->terminalEOL;
		psp->inputBufferSize = pp->inputBufferSize;
		psp->selectedInDialog = pp->selectedInDialog;
	}
	
	*hPtr = vsHandle;
	return 0;
}

/*	StoreVDTSetup()

	Stores current VDT settings in the IGOR preferences file.
	
	Returns error code from writing resource.
*/
static int
StoreVDTSetup(void)
{
	VDTSettingsHandle vsHandle;
	int result;
	
	if (result = GetVDTSettingsHandle(&vsHandle))
		return result;
	
	result = SaveXOPPrefsHandle((Handle)vsHandle);		// SaveXOPPrefsHandle makes its own copy of handle.
	DisposeHandle((Handle)vsHandle);
	
	return result;
}

/*	SetVDTSetup(vsHandle, loadingExperiment)

	Sets VDT settings from contents of vsHandle.
	
	This routine does not change the open/closed status of ports.
	
	loadingExperiment is true if we are loading settings from an experiment file,
	false if we are loading settings from preferences file.
	
	If the settings come from another platform (e.g. Windows when we are running
	on Mac or vice versa), SetVDTSetup does nothing. To support cross-platform
	settings, we would need to do byte-swapping on the structure and also take
	into account the different meaning of the winRect field.
		
	Returns:
		0 if everything OK
		1 if this version of VDT doesn't know about version of settings
		2 if vsHandle was NULL
		3 if vsHandle contains byte-swapped settings.
*/
static int
SetVDTSetup(VDTSettingsHandle vsHandle, int loadingExperiment)
{
	VDTPortPtr op, tp;
	int index, numPorts;
	VDTSettingsPtr vsPtr;
	VDTPortSettingsPtr psp;
	short version;
	int winState;
	int err;

	if (vsHandle == NULL)
		return 2;
	
	version = (*vsHandle)->version;				// Version of settings structure.
	
	if ((version & 0xFF) == 0)					// Don't support cross-platform settings.
		return 3;
	
	if (version < VDT_VERSION)					// Don't support old versions.
		return 1;
	
	if (version > VDT_VERSION)					// Don't know about new versions.
		return 1;

	op = VDTGetOperationsPortPtr();
	if (op != NULL)
		KillVDTIO(op);
	VDTAbortTerminalOperation();

	vsPtr = *vsHandle;

	gUseExpSettings = vsPtr->useExpSettings;
	winState = vsPtr->winState;
	if (!loadingExperiment)					// Just launched VDT and loading prefs?
		winState &= ~1;						// Clear bit 0 - leave window hidden.
	
	SetXOPWindowPositionAndState(gVDTWindow, &vsPtr->winRect, winState);
	TUGrow(gVDTTU, -1);						// Allow TU document to adjust to change in window size.

	numPorts = vsPtr->numPortSettings;
	
	for(index=0; index<numPorts; index++) {
		VDTPortPtr pp;
		pp = IndexedVDTPort(NULL, index);
		psp = &vsPtr->ps[index];
		TranslatePortName(psp->name);		// Do platform-related port name translation and handle other name equivalences.
		pp = FindVDTPort(NULL, psp->name);
		if (pp == NULL) {
			// This port is not available now. (e.g., opening Mac experiment on PC).
			continue;
		}
		
		pp->selectedInDialog = psp->selectedInDialog;
		
		pp->baud = ValidateBaudCode(psp->baud);
		pp->parity = psp->parity;
		pp->stopbits = psp->stopbits;
		pp->databits = psp->databits;
		pp->echo = psp->echo;
		pp->inShake = psp->inShake;
		pp->outShake = psp->outShake;
		pp->terminalEOL = psp->terminalEOL;
		SetInputBufferSize(pp, psp->inputBufferSize);

		if (pp->portIsOpen) {
			if (err = SetCommPortSettings(pp)) {
				char temp[128];
				sprintf(temp, "While changing settings for %s port, the following error occurred: ", pp->name);
				ExplainPortError(temp, err);
				continue;
			}
		}
	}

	op = FindVDTPort(NULL, vsPtr->opPortName);			// NULL if port does not exist.
	VDTSetOperationsPortPtr(op);						// It is OK if NULL or the port is not open.

	tp = FindVDTPort(NULL, vsPtr->termPortName);		// NULL if port does not exist.
	VDTSetTerminalPortPtr(tp);							// It is OK if NULL or the port is not open.
	
	/*	If there is a designated terminal port, we must open it so that if any incoming
		characters will be received.
	*/
	VDTGetOpenAndCheckTerminalPortPtr(&tp, 0);
	
	return 0;
}

/*	LoadVDTSetup()

	Loads VDT settings from the IGOR preferences file.
	Then, sets up hardware accordingly.
	
	Returns 0 or error code.
*/
static int
LoadVDTSetup(void)
{
	VDTSettingsHandle vsHandle;
	int result;
	
	if (result = GetXOPPrefsHandle((Handle*)&vsHandle))
		return result;
	result = SetVDTSetup(vsHandle, 0);
	DisposeHandle((Handle)vsHandle);
	
	return result;
}

/*	XOPLoadSettings()

	XOPLoadSettings() is called when a existing experiment is opened in the IGOR
	only if the XOP has previously stored its settings in the experiment using
	XOPSaveSettings.
	
	The XOPRecHandle contains a handle to the saved settings which the XOP can use to restore
	its settings or NULL if there are no settings to restore.
	
	NOTE: the host will dispose of this handle shortly so if you want, make a copy of it.
*/
static void
XOPLoadSettings(void)
{
	VDTSettingsHandle vsHandle;
	
	vsHandle = (VDTSettingsHandle)GetXOPItem(0);			// NOTE: This handle still belongs to IGOR.
	if (vsHandle == NULL)
		return;												// We did not save settings in this experiment.
	
	gUseExpSettings = (*vsHandle)->useExpSettings;
	if (gUseExpSettings)
		SetVDTSetup(vsHandle, 1);
	gSettingsModified = FALSE;
}

/*	XOPSaveSettings()

	XOPSaveSettings() is called when the current experiment is saved in the IGOR.
	The XOP can return a handle to data to be saved in the experiment.
	If the XOP does not want to save anything it should return NULL.
		
	NOTE: The host will dispose of this handle shortly so if you want, make a copy of it.
*/
static Handle
XOPSaveSettings(void)
{
	VDTSettingsHandle vsHandle = NULL;
	
	if (gUseExpSettings) {							// Want to save settings in experiment ?
		GetVDTSettingsHandle(&vsHandle);			// vsHandle will be NULL if error.
		gSettingsModified = FALSE;
	}
	return((Handle)vsHandle);
}

/*	XOPIdle()

	XOPIdle() is called periodically by the host application when nothing else is going on.
	It allows the XOP to perform periodic, ongoing tasks.
*/
static void
XOPIdle(void)
{
	TUIdle(gVDTTU);
		
	VDTTerminalIdle();							// Handle dumb terminal chores.
}

void
UpdateStatusArea(void)
{
	VDTPortPtr tp, op;
	char termPortName[MAX_OBJ_NAME+1];
	char opPortName[MAX_OBJ_NAME+1];
	char message[256];

	tp = VDTGetTerminalPortPtr();
	if (tp == NULL)
		strcpy(termPortName, "Off line");
	else
		strcpy(termPortName, tp->name);

	op = VDTGetOperationsPortPtr();
	if (op == NULL)
		strcpy(opPortName, "Off line");
	else
		strcpy(opPortName, op->name);
	
	sprintf(message, "Terminal port: %s.  Operations port: %s.", termPortName, opPortName);
	TUSetStatusArea(gVDTTU, message, TU_ERASE_STATUS_NEVER, -1);
}

/*	WindowMessage()

	WindowMessage handles any window message (update, activate, click, etc).
	It returns TRUE if the message was a window message or FALSE otherwise.
*/
static int
WindowMessage(void)
{	
	int message;
	XOPIORecParam item0, item1;					// Items from IO list.
	
	message = GetXOPMessage();
	if ((message & XOPWINDOWCODE) == 0)
		return 0;								// Not window message.
		
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

			case UPDATE:
				TUUpdate(gVDTTU);
				break;
			
			case ACTIVATE:
				TUActivate(gVDTTU, item1&activeFlag);
				break;
			
			case KEY:
				eventPtr = (EventRecord *)item1;
				if (HandleVDTKeyMac(gVDTTU, eventPtr))
					XOPModified = TRUE;
				break;
			
			case GROW:
				TUGrow(gVDTTU, item1);
				gSettingsModified = 1;			// This is so that new window size will be written to Igor Prefs file.
				break;
			
			case WINDOW_MOVED:
				gSettingsModified = 1;			// This is so that new window position will be written to Igor Prefs file.
				break;
			
			case SETGROW:
				rectPtr = (Rect *)item1;
				rectPtr->left= MINWINWIDTH;
				rectPtr->top= MINWINHEIGHT;
				break;
			
			case CLICK:
				eventPtr = (EventRecord *)item1;
				TUClick(gVDTTU, eventPtr);
				break;
			
			case NULLEVENT:
				eventPtr = (EventRecord *)item1;
				TUNull(gVDTTU, eventPtr);
				break;
		}
		#endif 						// MACIGOR ]
		
		case CLOSE:
			HideVDTWindow();
			break;
		
		case COPY:
			TUCopy(gVDTTU);
			break;
		
		case CUT:
			TUCut(gVDTTU);
			XOPModified = TRUE;
			break;
		
		case PASTE:
			TUPaste(gVDTTU);
			XOPModified = TRUE;
			break;
		
		case CLEAR:
			TUClear(gVDTTU);
			XOPModified = TRUE;
			break;
		
		case FIND:
			TUFind(gVDTTU, (int)item1);
			break;
		
		case DISPLAYSELECTION:
			TUDisplaySelection(gVDTTU);
			break;
		
		case REPLACE:
			TUReplace(gVDTTU);
			XOPModified = TRUE;
			break;
		
		case INDENTLEFT:
			TUIndentLeft(gVDTTU);
			XOPModified = TRUE;
			break;
		
		case INDENTRIGHT:
			TUIndentRight(gVDTTU);
			XOPModified = TRUE;
			break;
		
		case UNDO:
			TUUndo(gVDTTU);
			break;
		
		case SELECT_ALL:
			TUSelectAll(gVDTTU);
			break;
		
		case PRINT:
			TUPrint(gVDTTU);
			break;
		
		case INSERTFILE:
			VDTInsertText();
			break;
		
		case SAVEFILE:
			VDTSaveText();
			break;
		
		case PAGESETUP:
			TUPageSetupDialog(gVDTTU);
			break;

		case MOVE_TO_PREFERRED_POSITION:
			TUMoveToPreferredPosition(gVDTTU);
			break;
			
		case MOVE_TO_FULL_POSITION:
			TUMoveToFullSizePosition(gVDTTU);
			break;
			
		case RETRIEVE:
			TURetrieveWindow(gVDTTU);
			break;
	}
	return 1;
}

static void
DisposeVDTWindow(void)
{
	gDisposeInProgress = 1;						// Flag used by XOPEntry.

	if (gVDTTU == NULL)
		return;
	
	#ifdef WINIGOR
		UnsubclassVDTWindow(gVDTWindow);
	#endif

	TUDispose(gVDTTU);							// Discard text window.
	gVDTTU = NULL;
}

/*	FillPortMenu(mH, includeOffLine)

	Fills the menu with list of the ports that we found to be installed at runtime.
	If includeOffLine is true, it includes a separator plus "Off Line".
*/
static void
FillPortMenu(MenuHandle menuH, int includeOffLine)
{
	VDTPortPtr pp;
	int index;
	
	WMDeleteMenuItems(menuH, 0);				// Delete all items.
	index = 0;
	do {
		pp = IndexedVDTPort(NULL, index);
		if (pp == NULL)
			break;
		appendmenu(menuH, pp->name);
		index += 1;
	} while(1);
	
	if (includeOffLine)
		appendmenu(menuH, "-");					// Append the separator.
	appendmenu(menuH, "Off Line");
}

/*	XOPMenuEnable()

	Sets XOPs menu items according to current conditions.
*/
static void
XOPMenuEnable(void)
{
	VDTPortPtr tp;
	MenuHandle vdtMenuHandle;
	int menuID, itemID;
	char text[80];
	int isActive;
	int terminalIsOnline, terminalOp;
	
	isActive = IsXOPWindowActive(gVDTWindow);
	if (isActive) {										// If XOP is front window
		TUFixEditMenu(gVDTTU);
		TUFixFileMenu(gVDTTU);
		SetIgorMenuItem(CLOSE, 1, "Hide", 0);
		SetIgorMenuItem(MOVE_TO_PREFERRED_POSITION, 1, NULL, 0);
		SetIgorMenuItem(MOVE_TO_FULL_POSITION, 1, NULL, 0);
		SetIgorMenuItem(RETRIEVE, 1, NULL, 0);
	}
	menuID = MISCID;									// Main VDT item is in Igor Misc menu.
	itemID = ResourceToActualItem(MISCID, 1);			// Ask Igor which item VDT item is.
	vdtMenuHandle = ResourceMenuIDToMenuHandle(VDT_MENUID);
	if (vdtMenuHandle == NULL)
		return;											// Should never happen.

	// VDT_ITEM_OPEN_VDT_WINDOW item is always enabled.

	// VDT_ITEM_SETTINGS_DIALOG item is always enabled.

	// VDT_ITEM_TERMINAL_PORT_MENU item is always enabled. Also, all items in the VDT Operations submenu are always enabled.
	CheckPortMenuItems(VDT_TERMINAL_PORT_MENUID, 0);
	
	tp = VDTGetTerminalPortPtr();						// This may be null.
	terminalIsOnline = tp != NULL;
	terminalOp = tp ? tp->terminalOp:0;

	if (isActive && !terminalIsOnline) {
		EnableItem(vdtMenuHandle, VDT_ITEM_SAVEFILE);
		EnableItem(vdtMenuHandle, VDT_ITEM_INSERTFILE);
	}
	else {
		DisableItem(vdtMenuHandle, VDT_ITEM_SAVEFILE);
		DisableItem(vdtMenuHandle, VDT_ITEM_INSERTFILE);
	}
	
	// Set "Send File" menu item (VDT_ITEM_SENDFILE).
	if (terminalOp == OP_SENDFILE)
		GetXOPIndString(text, VDT_MISCSTR_ID, STOP_SENDING_FILE);
	else
		GetXOPIndString(text, VDT_MISCSTR_ID, SEND_FILE);
	setmenuitemtext(vdtMenuHandle, VDT_ITEM_SENDFILE, text);
	if (tp!=NULL && (terminalOp==OP_SENDFILE || terminalOp==0))
		EnableItem(vdtMenuHandle, VDT_ITEM_SENDFILE);
	else
		DisableItem(vdtMenuHandle, VDT_ITEM_SENDFILE);
	
	// Set "Receive File" menu item (VDT_ITEM_RECEIVEFILE).
	if (terminalOp == OP_RECEIVEFILE)
		GetXOPIndString(text, VDT_MISCSTR_ID, STOP_RECEIVING_FILE);
	else
		GetXOPIndString(text, VDT_MISCSTR_ID, RECEIVE_FILE);
	setmenuitemtext(vdtMenuHandle, VDT_ITEM_RECEIVEFILE, text);
	if (tp!=NULL && (terminalOp==OP_RECEIVEFILE || terminalOp==0))
		EnableItem(vdtMenuHandle, VDT_ITEM_RECEIVEFILE);
	else
		DisableItem(vdtMenuHandle, VDT_ITEM_RECEIVEFILE);
	
	// Set "Send Text" menu item (VDT_ITEM_SENDTEXT).
	if (terminalOp == OP_SENDTEXT) {
		GetXOPIndString(text, VDT_MISCSTR_ID, STOP_SENDING_TEXT);
	}
	else {
		if (VDTNoCharactersSelected())
			GetXOPIndString(text, VDT_MISCSTR_ID, SEND_VDT_TEXT);
		else
			GetXOPIndString(text, VDT_MISCSTR_ID, SEND_SELECTED_TEXT);
	}
	setmenuitemtext(vdtMenuHandle, VDT_ITEM_SENDTEXT, text);
	if (tp!=NULL && (terminalOp==OP_SENDTEXT || terminalOp==0))
		EnableItem(vdtMenuHandle, VDT_ITEM_SENDTEXT);
	else
		DisableItem(vdtMenuHandle, VDT_ITEM_SENDTEXT);

	// VDT_ITEM_OPERATIONS_PORT_MENU item is always enabled. Also, all items in the VDT Operations submenu are always enabled.
	CheckPortMenuItems(VDT_OPERATIONS_PORT_MENUID, 0);

	// VDT_ITEM_TERMINAL_PORT_MENU item is always enabled.
	
	EnableItem(vdtMenuHandle, VDT_ITEM_HELP);
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP when the message specified by the
	host is other than INIT.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;
	
	if (gDisposeInProgress) {
		/*	If here, we got a message from IGOR while we were quitting VDT.
			We may have already disposed some VDT resources (e.g. VDT window).
			Therefore, we can't service the message. This happens because, under Windows,
			when we close the VDT window, the Windows OS sends a barrage of messages.
		*/
		return;	
	}

	if (WindowMessage())							// Handle all messages related to XOP window.
		return;

	switch (GetXOPMessage()) {
		case CLEANUP:								// XOP about to be disposed of.
			VDTAbortTerminalOperation();			// Abort any operation in progress.
			if (gSettingsModified)					// Need to write settings out ?
				StoreVDTSetup();
			CloseAllVDTPorts();
			DisposeVDTWindow();
			break;
		
		case IDLE:									// Time to perform idle functions.
			XOPIdle();
			break;
		
		case NEW:									// New experiment -- init settings.
			XOPNew();
			break;
		
		case LOADSETTINGS:							// Loading experiment -- load settings.
			XOPLoadSettings();
			break;
		
		case SAVESETTINGS:							// Saving experiment -- save settings.
			result = (XOPIORecResult)XOPSaveSettings();
			break;
		
		case MODIFIED:								// Saving experiment -- save settings.
			result = XOPModified;
			result |= gUseExpSettings && gSettingsModified;
			break;
		
		case MENUITEM:								// XOPs menu item selected.
			result = XOPMenuItem();
			break;
		
		case MENUENABLE:							// Enable/disable XOPs menu item.
			XOPMenuEnable();
			break;
	}
	SetXOPResult(result);
}

static int
RegisterOperations(void)
{
	int err;
	
	if (err = RegisterVDTGetPortList2())
		return err;

	if (err = RegisterVDTOpenPort2())
		return err;

	if (err = RegisterVDTClosePort2())
		return err;

	if (err = RegisterVDTTerminalPort2())
		return err;

	if (err = RegisterVDTOperationsPort2())
		return err;

	if (err = RegisterVDT2())
		return err;

	if (err = RegisterVDTGetStatus2())
		return err;

	if (err = RegisterVDTRead2())
		return err;

	if (err = RegisterVDTReadWave2())
		return err;

	if (err = RegisterVDTReadBinary2())
		return err;

	if (err = RegisterVDTReadBinaryWave2())
		return err;

	if (err = RegisterVDTReadHex2())
		return err;

	if (err = RegisterVDTReadHexWave2())
		return err;

	if (err = RegisterVDTWrite2())
		return err;

	if (err = RegisterVDTWriteWave2())
		return err;

	if (err = RegisterVDTWriteBinary2())
		return err;

	if (err = RegisterVDTWriteBinaryWave2())
		return err;

	if (err = RegisterVDTWriteHex2())	
		return err;

	if (err = RegisterVDTWriteHexWave2())
		return err;
	
	return 0;
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
	
	XOPInit(ioRecHandle);				// Do standard XOP initialization
	SetXOPEntry(XOPEntry);				// Set entry point for future calls.
	SetXOPType(RESIDENT | IDLES);		// Specify XOP to stick around and to receive IDLE messages.

	if (igorVersion < 620) {			// Requires Igor 6.20 or later.
		SetXOPResult(OLD_IGOR);			// OLD_IGOR is defined in VDT.h and there are corresponding error strings in VDT.r and VDTWinCustom.rc.
		return EXIT_FAILURE;
	}
	
	if (err = RegisterOperations()) {
		SetXOPResult(err);
		return EXIT_FAILURE;
	}
		
	// Create XOP TU record and window.
	winRect.left = 5;
	winRect.right = 500;
	winRect.top = 160;
	winRect.bottom = 260;
	err = TUNew2("VDT2", &winRect, &gVDTTU, &gVDTWindow);
	if (err != 0) {
		SetXOPResult(err);
		return EXIT_FAILURE;
	}

	TUSetStatusArea(gVDTTU, "", TU_ERASE_STATUS_NEVER, 300);	// Set initial status area width.
	
	if (err = InitVDT()) {
		DisposeVDTWindow();
		SetXOPResult(err);
		return EXIT_FAILURE;
	}

	if (LoadVDTSetup())							// Try to load settings from IGOR preferences file.
		gSettingsModified = TRUE;				// If no settings, flag settings to be written.

	#ifdef WINIGOR
		SubclassVDTWindow(gVDTWindow);			// We need to intercept WM_CHAR messages to send them to serial port.
	#endif
	
	/*	Fill the terminal port menu and the operations port menu with list of the ports
		that InitVDT found to be installed.
	*/
	{
		MenuHandle tmH, omH;
		
		tmH = ResourceMenuIDToMenuHandle(VDT_TERMINAL_PORT_MENUID);
		if (tmH != NULL)						// Should never be NULL.
			FillPortMenu(tmH, 1);
		
		omH = ResourceMenuIDToMenuHandle(VDT_OPERATIONS_PORT_MENUID);
		if (omH != NULL)						// Should never be NULL.
			FillPortMenu(omH, 1);
	}
	
	UpdateStatusArea();
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
