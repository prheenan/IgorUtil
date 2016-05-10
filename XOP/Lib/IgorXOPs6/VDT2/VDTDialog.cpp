/*	VDTDialog.c

	9/15/97 for version 2.0
		Split out dialog-related routines into VDTDialog.c.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

#ifdef WINIGOR
	#include "resource.h"
#endif

// Global Variables


// Equates

#define DIALOG_TEMPLATE_ID 1100

enum {				// These are both Macintosh item numbers and Windows item IDs.
	OK_BUTTON=1,
	CANCEL_BUTTON,
	PORT_TITLE,
	PORT_POPUP,
	PORT_IS_OPEN_STATTEXT,
	PORT_IS_CLOSED_STATTEXT,
	OPEN_PORT_CHECKBOX,
	CLOSE_PORT_CHECKBOX,
	PORT_WILL_BE_OPENED_STATTEXT,
	PORT_WILL_BE_CLOSED_STATTEXT,
	USE_PORT_FOR_TERMINAL_CHECKBOX,
	USE_PORT_FOR_OPERATIONS_CHECKBOX,
	BAUD_RATE_TITLE,
	BAUD_RATE_POPUP,
	INPUT_HANDSHAKE_TITLE,
	INPUT_HANDSHAKE_POPUP,
	DATA_LENGTH_TITLE,
	DATA_LENGTH_POPUP,
	OUTPUT_HANDSHAKE_TITLE,
	OUTPUT_HANDSHAKE_POPUP,
	STOP_BITS_TITLE,
	STOP_BITS_POPUP,
	INPUT_BUFFER_SIZE_TITLE,
	INPUT_BUFFER_SIZE_TEXT,
	PARITY_TITLE,
	PARITY_POPUP,
	LOCAL_ECHO_TITLE,
	LOCAL_ECHO_POPUP,
	TERMINAL_EOL_TITLE,
	TERMINAL_EOL_POPUP,
	SAVE_SETUP_WITH_EXP_CHECKBOX,
	INPUT_BUFFER_LIMITATION_MESSAGE		// "Input buffer is not settable on OS X".
};


// Structures

struct DialogStorage {		// See InitDialogStorage for a discussion of this structure.
	// These fields are initialized by VDTSettingsDialog.
	int useExpSettingsIn;				// Initial state of Use Experiment Settings checkbox.
	int* useExpSettingsOutPtr;			// On return, state of Use Experiment Settings checkbox is stored here.
	int expSettings;					// State of the Use Experiment Settings checkbox during dialog execution.
	VDTPortPtr firstVDTPortPtr;			// Pointer the first VDTPort in linked list. This is a linked list created just for use during the dialog.
	int numberOfPortsInList;			// Number of ports in the list started by firstVDTPortPtr.
	VDTPortPtr pp;						// Pointer the true VDTPort (not dialog copy) currently being set.
	VDTPortPtr dd;						// Pointer the dialog copy of VDTPort currently being set.
	VDTPort port;						// The dialog code takes settings from here and stores them here.
	VDTPortPtr portToUseForTerminal;	// The real VDTPortPtr for the terminal port or NULL if terminal port is None.
	VDTPortPtr portToUseForOperations;	// The real VDTPortPtr for the operations port or NULL if terminal port is None.
};
typedef struct DialogStorage DialogStorage;
typedef struct DialogStorage *DialogStoragePtr;


// Baud Rate Utilities

struct BaudRateInfo {
	int baudCode;				// Integer code used internally and in VDT dialog settings structure.
	int baudRate;				// e.g., 9600
	int supportedOnMac;			// True if supported on Macintosh.
	int supportedOnWin;			// True if supported on Windows.
};
typedef struct BaudRateInfo BaudRateInfo;
typedef struct BaudRateInfo* BaudRateInfoPtr;

/*	HR, 990415, 2.12: Removed support on Windows for 128000 and 256000 baud in favor
	of supporting the more widely used 115200 and 230400 rates. I decided that it
	was fairly unlikely that someone relies on those non-standard rates.
	
	On Macintosh, rates higher than 57600 are supported only by relatively
	recent operating system software, such as Mac OS 8 or later. Also, rates
	higher than 57600 will be reliable only on fast machines. See TN1018.
	
	On Windows, I believe that standard PC hardware supports rates no higher
	than 57600. Specialized hardware may support higher rates. See Knowledge
	Base article Q99026.
	
	HR, 090612, 1.14: Added support for 128000 and 256000 baud. They do not appear
	to work on Mac OS X but I have marked it as supported anyway in case there is
	some hardware that supports it.
*/
static struct BaudRateInfo baudRateInfo[]=
{
	// Baud Code		// Baud Rate		// Supported on Mac		// Supported on Win
	{0,					300,				1,						1},
	{1,					600,				1,						1},
	{2,					1200,				1,						1},
	{3,					2400,				1,						1},
	{4,					4800,				1,						1},
	{5,					9600,				1,						1},
	{6,					19200,				1,						1},
	{7,					38400,				1,						1},
	{8,					57600,				1,						1},
	{9,					115200,				1,						1},
	{10,				128000,				1,						1},		// HR, 090612, 1.14: Added this.
	{11,				230400,				1,						1},
	{12,				256000,				1,						1},		// HR, 090612, 1.14: Added this.
	{-1,				-1,					0,						0}		// Marks the end of the table.
};

/*	GetBaudRateMenuItemString(str)

	Returns a string to be used in the VDT dialog baud popup menu.
*/
static void
GetBaudRateMenuItemString(char str[256])
{
	BaudRateInfoPtr brp;
	int supported;
	char temp[32];

	*str = 0;
	brp = baudRateInfo;
	while(brp->baudCode >= 0) {
		#ifdef MACIGOR
			supported = brp->supportedOnMac;
		#endif
		#ifdef WINIGOR
			supported = brp->supportedOnWin;
		#endif
		if (supported) {
			sprintf(temp, "%d;", brp->baudRate);
			strcat(str, temp);
		}
		brp += 1;	
	}
}

/*	BaudRateToBaudCode(baudRate, baudCodePtr)
	
	baudRate is an input (e.g., 9600).
	
	Returns via baudCodePtr the integer baud code associated with the rate.
	
	If the rate is supported on the current platform, the function result
	is zero. If the rate is not supported,the function result is a non-zero
	error code.
*/
int
BaudRateToBaudCode(int baudRate, int* baudCodePtr)
{
	BaudRateInfoPtr brp;
	int supported;

	*baudCodePtr = 0;							// Init in case of an error below.
	
	brp = baudRateInfo;
	while(brp->baudCode >= 0) {
		if (brp->baudRate == baudRate)
			break;
		brp += 1;	
	}

	if (brp->baudCode < 0)
		return INVALID_BAUD;					// This baud rate was not found in the table.
	
	*baudCodePtr = brp->baudCode;

	#ifdef MACIGOR
		supported = brp->supportedOnMac;
	#endif
	#ifdef WINIGOR
		supported = brp->supportedOnWin;
	#endif
	if (supported)
		return 0;
	return VDT_ERR_BAUD_NOT_SUPPORTED_ON_THIS_PLATFORM;
}

/*	BaudCodeToBaudRate(baudCode, baudRatePtr)
	
	baudCode is a small integer code representing a baud rate.
	
	Returns via baudRatePtr the actual baud rate associated with the code.
	
	If the code is supported on the current platform, the function result
	is zero. If the code is not supported,the function result is a non-zero
	error code.
*/
int
BaudCodeToBaudRate(int baudCode, int* baudRatePtr)
{
	BaudRateInfoPtr brp;
	int supported;

	*baudRatePtr = 9600;						// Init in case of an error below.
	
	brp = baudRateInfo;
	while(brp->baudCode >= 0) {
		if (brp->baudCode == baudCode)
			break;
		brp += 1;	
	}

	if (brp->baudCode < 0)
		return INVALID_BAUD;					// This baud rate was not found in the table.
	
	*baudRatePtr = brp->baudRate;

	#ifdef MACIGOR
		supported = brp->supportedOnMac;
	#endif
	#ifdef WINIGOR
		supported = brp->supportedOnWin;
	#endif
	if (supported)
		return 0;
	return VDT_ERR_BAUD_NOT_SUPPORTED_ON_THIS_PLATFORM;
}

/*	ValidateBaudCode(baudCode)

	Returns baudCode if it is a valid baud code for the current platform.
	If not, it returns the code for 9600 baud.
*/
int
ValidateBaudCode(int baudCode)
{
	int baudRate;
	
	if (BaudCodeToBaudRate(baudCode, &baudRate) != 0)
		baudCode = 5;
	return baudCode;
}


static void
DisposeVDTPortLinkedList(VDTPortPtr firstVDTPortPtr)
{
	VDTPortPtr dd, tt;

	dd = firstVDTPortPtr;
	while (dd != NULL) {
		tt = dd->nextPort;
		DisposePtr((Ptr)dd);
		dd = tt;	
	}
}

static void
CopyVDTPortFieldsOrGetDefaults(VDTPortPtr in, VDTPortPtr out)
{
	if (in != NULL) {
		VDTPortPtr nextPort;
		nextPort = out->nextPort;
		memcpy(out, in, sizeof(VDTPort));
		out->nextPort = nextPort;			// Never copy nextPort fields because this would mix up the dialog linked list with the main linked list.
	}
	else {
		SetVDTPortFieldsToDefault("-None-", "Input", "Output", out);	// Use dummy values when no ports exist.
	}
}

static int
BuildVDTPortLinkedList(VDTPortPtr* firstVDTPortPtrPtr, int* numberOfPortsInListPtr)
{
	VDTPortPtr firstVDTPortPtr;
	VDTPortPtr pp, dd;
	int index, numberOfPortsInList;
	
	firstVDTPortPtr = *firstVDTPortPtrPtr = NULL;
	numberOfPortsInList = *numberOfPortsInListPtr = 0;
	
	index = 0;
	while(1) {
		pp = IndexedVDTPort(NULL, index);
		if (pp == NULL)
			break;						// No more ports exist.
		
		// Make a copy of VDTPort structure.
		dd = (VDTPortPtr)NewPtr(sizeof(VDTPort));
		if (dd == NULL) {
			DisposeVDTPortLinkedList(firstVDTPortPtr);
			return NOMEM;
		}
		CopyVDTPortFieldsOrGetDefaults(pp, dd);
		dd->nextPort = NULL;								// Don't mix linked lists.
		
		// These should already be zero but just in case.
		dd->portSettingsChangedInDialog = 0;
		dd->portOpenedInDialog = 0;
		dd->portClosedInDialog = 0;
		
		// Add to linked list.
		if (firstVDTPortPtr == NULL) {
			firstVDTPortPtr = dd;
		}
		else {
			VDTPortPtr tt;
			tt = firstVDTPortPtr;
			while(tt->nextPort != NULL)
				tt = tt->nextPort;
			tt->nextPort = dd;
		}
		numberOfPortsInList += 1;
		index += 1;
	}
	
	*numberOfPortsInListPtr = numberOfPortsInList;
	*firstVDTPortPtrPtr = firstVDTPortPtr;
	return 0;
}

/*	ClosePortsMarkedForClosing(firstVDTPortPtr)

	Called when the user clicks OK.
	
	Closes communications ports that the user specified to be closed.
	
	Returns 0 if OK or an error code.
*/
static int
ClosePortsMarkedForClosing(VDTPortPtr firstVDTPortPtr)
{
	VDTPortPtr dd;
	int firstErr, err;
	
	firstErr = 0;
	dd = firstVDTPortPtr;
	while(dd != NULL) {
		if (dd->portClosedInDialog) {
			err = CloseVDTPort(dd->name);
			if (err != 0) {
				char temp[128];
				sprintf(temp, "While closing %s port, the following error occurred: ", dd->name);
				ExplainPortError(temp, err);
				if (firstErr == 0)
					firstErr = err;
			}
		}
		dd = dd->nextPort;
	}

	return firstErr;
}

/*	OpenPortsMarkedForOpening(firstVDTPortPtr)

	Called when the user clicks OK.
	
	Opens communications ports that the user specified to be opened.
	
	Returns 0 if OK or an error code.
*/
static int
OpenPortsMarkedForOpening(VDTPortPtr firstVDTPortPtr)
{
	VDTPortPtr pp, dd;
	int firstErr, err;
	
	firstErr = 0;
	dd = firstVDTPortPtr;
	while(dd != NULL) {
		if (dd->portOpenedInDialog && !dd->portClosedInDialog) {
			err = OpenVDTPort(dd->name, &pp);
			if (err != 0) {
				char temp[128];
				sprintf(temp, "While opening %s port, the following error occurred: ", dd->name);
				ExplainPortError(temp, err);
				if (firstErr == 0)
					firstErr = err;
			}
		}
		dd = dd->nextPort;
	}

	return firstErr;
}

/*	UpdateChangedPorts(firstVDTPortPtr)
	
	Called when the user clicks OK.
	
	Carries out changes to ports following changes made by the user during the dialog.
	
	Returns 0 if OK or an error code.
	
	HR, 990415, 2.12: Previously, if the port was opened or closed in the dialog,
	UpdateChangedPorts did not copy the dialog settings (dd->xxx) to the actual
	port settings (pp->xxx). I fixed this by removing tests on dd->portClosedInDialog
	and dd->portOpenedInDialog in the first if statement.
*/
static int
UpdateChangedPorts(VDTPortPtr firstVDTPortPtr)
{
	VDTPortPtr pp, dd;
	int firstErr, err;
	
	firstErr = 0;
	dd = firstVDTPortPtr;
	while(dd != NULL) {
		if (dd->portSettingsChangedInDialog) {		// HR, 990415, 2.12: See note above.
			pp = FindVDTPort(NULL, dd->name);		// Pointer to real VDTPort for this port, not dialog copy.
			if (pp == NULL) {
				err = VDT_ERR_UNKNOWN_PORT_ERR;		// Should not happen.
			}
			else {
				pp->baud = dd->baud;
				pp->parity = dd->parity;
				pp->stopbits = dd->stopbits;
				pp->databits = dd->databits;
				pp->echo = dd->echo;
				pp->inShake = dd->inShake;
				pp->outShake = dd->outShake;
				pp->terminalEOL = dd->terminalEOL;
				err = SetInputBufferSize(pp, dd->inputBufferSize);
				if (VDTPortIsOpen(pp))
					err = SetCommPortSettings(pp);
			}
			if (err != 0) {
				char temp[128];
				sprintf(temp, "While changing settings for %s port, the following error occurred: ", dd->name);
				ExplainPortError(temp, err);
				if (firstErr == 0)
					firstErr = err;
			}
		}
		dd = dd->nextPort;
	}

	return firstErr;
}

/*	InitialPortToSelect(firstVDTPortPtr)

	Returns a VDTPortPtr that indicates which port should be selected
	when the dialog is first displayed.
	
	firstVDTPortPtr will be NULL if there are no functioning communications
	ports. In this case, it will return NULL.
*/
static VDTPortPtr
InitialPortToSelect(VDTPortPtr firstVDTPortPtr)
{
	VDTPortPtr dd;
	
	dd = firstVDTPortPtr;
	while(dd != NULL) {
		if (dd->selectedInDialog != 0)
			return dd;
		dd = dd->nextPort;	
	}
	
	return firstVDTPortPtr;			// Default to first.
}

/*	SetInitialPortToSelectForNextTime(firstVDTPortPtr)
	
	Marks the port that should be initially displayed the next time the
	dialog is invoked.
	
	firstVDTPortPtr will be NULL if there are no functioning communications
	ports.
*/
static void
SetInitialPortToSelectForNextTime(VDTPortPtr firstVDTPortPtr)
{
	VDTPortPtr dd, pp;
	
	dd = firstVDTPortPtr;
	while(dd != NULL) {
		pp = FindVDTPort(NULL, dd->name);
		if (pp != NULL)
			pp->selectedInDialog = dd->selectedInDialog;
		dd = dd->nextPort;	
	}
}

/*	FillPortPopup(theDialog)

	Fills the popup with list of the ports that we found to be installed at runtime.
*/
static void
FillPortPopup(XOP_DIALOG_REF theDialog)
{
	VDTPortPtr pp;
	int index;
	
	DeletePopMenuItems(theDialog, PORT_POPUP, 0);		// Delete all items.
	index = 0;
	do {
		pp = IndexedVDTPort(NULL, index);
		if (pp == NULL)
			break;
		FillPopMenu(theDialog, PORT_POPUP, pp->name, (int)strlen(pp->name), 10000);
		index += 1;
	} while(1);
	SetPopItem(theDialog, PORT_POPUP, 1);
}

static int
InitDialogPopups(XOP_DIALOG_REF theDialog)
{
	int err;
	
	err = 0;
	
	do {
		char baudStr[256];
		GetBaudRateMenuItemString(baudStr);			// Get list of supported baud rates for this platform.
		if (err = CreatePopMenu(theDialog, BAUD_RATE_POPUP, BAUD_RATE_TITLE, baudStr, 6))
			break;
		
		if (err = CreatePopMenu(theDialog, DATA_LENGTH_POPUP, DATA_LENGTH_TITLE, "7;8;", 2))
			break;
		
		if (err = CreatePopMenu(theDialog, STOP_BITS_POPUP, STOP_BITS_TITLE, "1;2;", 2))
			break;
		
		if (err = CreatePopMenu(theDialog, PARITY_POPUP, PARITY_TITLE, "None;Odd;Even;", 1))
			break;
		
		if (err = CreatePopMenu(theDialog, LOCAL_ECHO_POPUP, LOCAL_ECHO_TITLE, "Off;On;", 2))
			break;
		
		if (err = CreatePopMenu(theDialog, INPUT_HANDSHAKE_POPUP, INPUT_HANDSHAKE_TITLE, "None;CTS-DTR;XON-XOFF;", 1))
			break;
		
		if (err = CreatePopMenu(theDialog, OUTPUT_HANDSHAKE_POPUP, OUTPUT_HANDSHAKE_TITLE, "None;CTS-DTR;XON-XOFF;", 1))
			break;
		
		if (err = CreatePopMenu(theDialog, TERMINAL_EOL_POPUP, TERMINAL_EOL_TITLE, "CR;LF;CRLF;", 3))
			break;
		
		if (err = CreatePopMenu(theDialog, PORT_POPUP, PORT_TITLE, "", 1))
			break;
		FillPortPopup(theDialog);
	} while(0);
	
	return err;
}

/*	InitDialogStorage(dsp, useExpSettingsIn, useExpSettingsOutPtr)

	We use a DialogStorage structure to store working values during the dialog.
	In a Macintosh application, the fields in this structure could be local variables
	in the main dialog routine. However, in a Windows application, they would have
	to be globals. By using a structure like this, we are able to avoid using globals.
	Also, routines that access these fields (such as this one) can be used for
	both platforms.
*/
static int
InitDialogStorage(DialogStorage* dsp, int useExpSettingsIn, int* useExpSettingsOutPtr)
{
	int err;
	
	dsp->useExpSettingsIn = useExpSettingsIn;
	dsp->useExpSettingsOutPtr = useExpSettingsOutPtr;
	dsp->expSettings = useExpSettingsIn;
	
	dsp->portToUseForTerminal = VDTGetTerminalPortPtr();				// May be NULL.
	dsp->portToUseForOperations = VDTGetOperationsPortPtr();			// May be NULL.

	if (err = BuildVDTPortLinkedList(&dsp->firstVDTPortPtr, &dsp->numberOfPortsInList))
		return err;
	// Note that dsp->firstVDTPortPtr will be NULL if no ports exist.
	
	dsp->dd = InitialPortToSelect(dsp->firstVDTPortPtr);		// Will be NULL if dsp->firstVDTPortPtr is NULL.
	if (dsp->dd == NULL)
		dsp->pp = NULL;
	else
		dsp->pp = FindVDTPort(NULL, dsp->dd->name);
	CopyVDTPortFieldsOrGetDefaults(dsp->dd, &dsp->port);		// Uses default values if dd is NULL.

	return 0;
}

static void
DisposeDialogStorage(DialogStorage* dsp)
{
	DisposeVDTPortLinkedList(dsp->firstVDTPortPtr);
}

/*	SetOpenClosedItems(...)

	Sets static text and checkboxes that have to do with whether the currently-selected
	port is open, closed, marked for opening or marked for closing.
*/
static void
SetOpenClosedItems(XOP_DIALOG_REF theDialog, VDTPortPtr dd)
{
	int portIsOpen;

	portIsOpen = dd!=NULL && dd->portIsOpen;
	if (portIsOpen) {										// Port is open?
		HideDialogItem(theDialog, PORT_IS_CLOSED_STATTEXT);
		ShowDialogItem(theDialog, PORT_IS_OPEN_STATTEXT);
		HideDialogItem(theDialog, OPEN_PORT_CHECKBOX);
		ShowDialogItem(theDialog, CLOSE_PORT_CHECKBOX);
		if (dd->portClosedInDialog)
			ShowDialogItem(theDialog, PORT_WILL_BE_CLOSED_STATTEXT);
		else
			HideDialogItem(theDialog, PORT_WILL_BE_CLOSED_STATTEXT);
		HideDialogItem(theDialog, PORT_WILL_BE_OPENED_STATTEXT);
	}
	else {
		HideDialogItem(theDialog, PORT_IS_OPEN_STATTEXT);
		ShowDialogItem(theDialog, PORT_IS_CLOSED_STATTEXT);
		HideDialogItem(theDialog, CLOSE_PORT_CHECKBOX);
		ShowDialogItem(theDialog, OPEN_PORT_CHECKBOX);
		if (dd->portOpenedInDialog)
			ShowDialogItem(theDialog, PORT_WILL_BE_OPENED_STATTEXT);
		else
			HideDialogItem(theDialog, PORT_WILL_BE_OPENED_STATTEXT);
		HideDialogItem(theDialog, PORT_WILL_BE_CLOSED_STATTEXT);
	}
	
	SetCheckBox(theDialog, OPEN_PORT_CHECKBOX, dd!=NULL && dd->portOpenedInDialog);
	SetCheckBox(theDialog, CLOSE_PORT_CHECKBOX, dd!=NULL && dd->portClosedInDialog);
}

/*	SetTerminalAndOperationsCheckboxes(theDialog, dd, tp, op)
	
	dd identifies the port selected in the dialog. It will be NULL if no ports are available.
	
	tp identifies the port selected as terminal port. It will be NULL if no port is selected.
	
	op identifies the port selected as operations port. It will be NULL if no port is selected.
*/
static void
SetTerminalAndOperationsCheckboxes(XOP_DIALOG_REF theDialog, VDTPortPtr dd, VDTPortPtr tp, VDTPortPtr op)
{
	int tpState, opState;
	
	if (dd == NULL) {
		tpState = 0;
		opState = 0;
	}
	else {
		if (tp == NULL)
			tpState = 0;
		else
			tpState = CmpStr(tp->name, dd->name) == 0;
	
		if (op == NULL)
			opState = 0;
		else
			opState = CmpStr(op->name, dd->name) == 0;
	}
	
	SetCheckBox(theDialog, USE_PORT_FOR_TERMINAL_CHECKBOX, tpState);
	SetCheckBox(theDialog, USE_PORT_FOR_OPERATIONS_CHECKBOX, opState);
}

/*	UpdateDialogItems(theDialog, dsp)

	Sets all items in the dialog according to the state indicated by dsp.
	
	This is called when the dialog is first displayed and then when the user
	changes the current port.
*/
static void
UpdateDialogItems(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	int baudRate;
	char baudRateStr[32];
	
	SetCheckBox(theDialog, SAVE_SETUP_WITH_EXP_CHECKBOX, dsp->useExpSettingsIn);
	SetPopMatch(theDialog, PORT_POPUP, dsp->port.name);

	BaudCodeToBaudRate(dsp->port.baud, &baudRate);
	sprintf(baudRateStr, "%d", baudRate);
	SetPopMatch(theDialog, BAUD_RATE_POPUP, baudRateStr);
	SetPopItem(theDialog, DATA_LENGTH_POPUP, dsp->port.databits+1);
	SetPopItem(theDialog, STOP_BITS_POPUP, dsp->port.stopbits+1);
	SetPopItem(theDialog, PARITY_POPUP, dsp->port.parity+1);
	SetPopItem(theDialog, LOCAL_ECHO_POPUP, dsp->port.echo+1);
	SetPopItem(theDialog, INPUT_HANDSHAKE_POPUP, dsp->port.inShake+1);
	SetPopItem(theDialog, OUTPUT_HANDSHAKE_POPUP, dsp->port.outShake+1);
	SetPopItem(theDialog, TERMINAL_EOL_POPUP, dsp->port.terminalEOL+1);
	SetDInt(theDialog, INPUT_BUFFER_SIZE_TEXT, (int)dsp->port.inputBufferSize);
	#ifdef MACIGOR
		SelEditItem(theDialog, INPUT_BUFFER_SIZE_TEXT);
	#endif

	SetOpenClosedItems(theDialog, &dsp->port);
	SetTerminalAndOperationsCheckboxes(theDialog, &dsp->port, dsp->portToUseForTerminal, dsp->portToUseForOperations);
}

/*	InitDialogSettings(theDialog, dsp)

	Called when the dialog is first displayed to initialize all items.
*/
static int
InitDialogSettings(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	int err;
	
	err = 0;

	#ifdef MACIGOR
		// The size of the input buffer is not settable on OS X.
		HideDialogItem(theDialog, INPUT_BUFFER_SIZE_TEXT);
		ShowDialogItem(theDialog, INPUT_BUFFER_LIMITATION_MESSAGE);
	#endif
	
	InitPopMenus(theDialog);
	
	do {
		if (err = InitDialogPopups(theDialog))
			break;
		
		UpdateDialogItems(theDialog, dsp);
	} while(0);
	
	if (err != 0)
		KillPopMenus(theDialog);

	return err;
}

static void
ShutdownDialogSettings(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	SetInitialPortToSelectForNextTime(dsp->firstVDTPortPtr);
	KillPopMenus(theDialog);
}

static void
SetCurrentPort(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	int selItem;
	
	GetPopMenu(theDialog, PORT_POPUP, &selItem, NULL);			// Get number of selected item.
	
	if (dsp->dd != NULL) {
		CopyVDTPortFieldsOrGetDefaults(&dsp->port, dsp->dd);	// Copy settings back to old selected port.
		dsp->dd->selectedInDialog = 0;
	}
	dsp->dd = IndexedVDTPort(dsp->firstVDTPortPtr, selItem-1);	// Get new selected port (may be the same port).
	if (dsp->dd == NULL) {
		dsp->pp = NULL;
	}
	else {
		dsp->dd->selectedInDialog = 1;
		dsp->pp = FindVDTPort(NULL, dsp->dd->name);
	}
	CopyVDTPortFieldsOrGetDefaults(dsp->dd, &dsp->port);		// Uses default values if dsp->dd is NULL.
	UpdateDialogItems(theDialog, dsp);
}

/*	ProcessOK(theDialog, dsp)
	
	Called when the user presses OK to carry out the user-requested actions.
*/
static void
ProcessOK(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	#ifdef MACIGOR
		#pragma unused(theDialog)
	#endif
	
	if (dsp->dd != NULL)
		CopyVDTPortFieldsOrGetDefaults(&dsp->port, dsp->dd);	// Copy settings back to selected port.
	ClosePortsMarkedForClosing(dsp->firstVDTPortPtr);
	OpenPortsMarkedForOpening(dsp->firstVDTPortPtr);
	UpdateChangedPorts(dsp->firstVDTPortPtr);

	VDTSetOperationsPortPtr(dsp->portToUseForOperations);		// OK if NULL.
	VDTSetTerminalPortPtr(dsp->portToUseForTerminal);			// OK if NULL.
		
	*dsp->useExpSettingsOutPtr = dsp->expSettings;
}

/*	HandleItemHit(theDialog, itemID, dsp)
	
	Called when the item identified by itemID is hit.
	Carries out any actions necessitated by the hit.
*/
static void
HandleItemHit(XOP_DIALOG_REF theDialog, int itemID, DialogStorage* dsp)
{
	int selItem;
	char itemText[256];
	int baudRate, baudCode;
	int intVal;
	
	if (ItemIsPopMenu(theDialog, itemID))
		GetPopMenu(theDialog, itemID, &selItem, itemText);
	
	switch(itemID) {
		case PORT_POPUP:
			SetCurrentPort(theDialog, dsp);
			break;

		case OPEN_PORT_CHECKBOX:
			dsp->port.portOpenedInDialog = ToggleCheckBox(theDialog, OPEN_PORT_CHECKBOX);
			SetOpenClosedItems(theDialog, &dsp->port);
			break;

		case CLOSE_PORT_CHECKBOX:
			dsp->port.portClosedInDialog = ToggleCheckBox(theDialog, CLOSE_PORT_CHECKBOX);
			SetOpenClosedItems(theDialog, &dsp->port);
			break;
	
		case USE_PORT_FOR_TERMINAL_CHECKBOX:
			if (ToggleCheckBox(theDialog, USE_PORT_FOR_TERMINAL_CHECKBOX))
				dsp->portToUseForTerminal = dsp->pp;		// May be NULL.
			else
				dsp->portToUseForTerminal = NULL;			// No port selected as terminal port.
			break;
		
		case USE_PORT_FOR_OPERATIONS_CHECKBOX:
			if (ToggleCheckBox(theDialog, USE_PORT_FOR_OPERATIONS_CHECKBOX))
				dsp->portToUseForOperations = dsp->pp;		// May be NULL.
			else
				dsp->portToUseForOperations = NULL;			// No port selected as operations port.
			break;

		case BAUD_RATE_POPUP:
			baudRate = atoi(itemText);
			BaudRateToBaudCode(baudRate, &baudCode);
			dsp->port.baud = baudCode;
			dsp->port.portSettingsChangedInDialog = 1;
			break;

		case DATA_LENGTH_POPUP:
			dsp->port.databits = selItem - 1;
			dsp->port.portSettingsChangedInDialog = 1;
			break;

		case STOP_BITS_POPUP:
			dsp->port.stopbits = selItem - 1;
			dsp->port.portSettingsChangedInDialog = 1;
			break;

		case PARITY_POPUP:
			dsp->port.parity = selItem - 1;
			dsp->port.portSettingsChangedInDialog = 1;
			GetPopMenu(theDialog, itemID, &selItem, NULL);
			break;
		
		case LOCAL_ECHO_POPUP:
			dsp->port.echo = selItem - 1;
			dsp->port.portSettingsChangedInDialog = 1;
			break;
		
		case INPUT_HANDSHAKE_POPUP:
			dsp->port.inShake = selItem - 1;
			dsp->port.portSettingsChangedInDialog = 1;
			break;
		
		case OUTPUT_HANDSHAKE_POPUP:
			dsp->port.outShake = selItem - 1;
			dsp->port.portSettingsChangedInDialog = 1;
			break;
		
		case TERMINAL_EOL_POPUP:
			dsp->port.terminalEOL = selItem - 1;
			dsp->port.portSettingsChangedInDialog = 1;
			break;

	   case INPUT_BUFFER_SIZE_TEXT:
		   if (GetDInt(theDialog, INPUT_BUFFER_SIZE_TEXT, &intVal) == 0) {
				if (intVal < MINBUFSIZE)
					intVal = MINBUFSIZE;
				if (intVal > MAXBUFSIZE)
					intVal = MAXBUFSIZE;
			   dsp->port.inputBufferSize = intVal;
			   dsp->port.portSettingsChangedInDialog = 1;
		   }
		   break;
	   
	   case SAVE_SETUP_WITH_EXP_CHECKBOX:
		   dsp->expSettings = ToggleCheckBox(theDialog, SAVE_SETUP_WITH_EXP_CHECKBOX);
		   break;
		
		case OK_BUTTON:
			ProcessOK(theDialog, dsp);
			break;
	}
}


#ifdef MACIGOR			// Macintosh-specific code [

/*	VDTSettingsDialog(useExpSettingsIn, useExpSettingsOutPtr)

	VDTSettingsDialog is called when user selects Terminal Settings from the VDT submenu.

	The function result is 0 if the user clicks OK or -1 if the user clicks cancel
	or a non-zero error code.
	
	useExpSettingsIn is the initial state of the "Use Experiment Settings" setting.
	On output, if the user clicks OK, *useExpSettingsOutPtr is the new state.
	
	This routine creates a copy of the linked list of VDTPort structures so that
	we can change the fields but not really change anything unless the user cancels.
	The user can also open and close ports through the dialog.
*/
int
VDTSettingsDialog(int useExpSettingsIn, int* useExpSettingsOutPtr)
{
	DialogStorage ds;
	DialogPtr theDialog;
	short itemHit;
	CGrafPtr savePort;
	int err;
	
	ArrowCursor();

	*useExpSettingsOutPtr = useExpSettingsIn;
	if (err = InitDialogStorage(&ds, useExpSettingsIn, useExpSettingsOutPtr))
		return err;
	
	theDialog = GetXOPDialog(DIALOG_TEMPLATE_ID);
	savePort = SetDialogPort(theDialog);
	SetDialogBalloonHelpID(DIALOG_TEMPLATE_ID);			// Set resource ID for Macintosh help balloons.
	
	SetDialogDefaultItem(theDialog, OK_BUTTON);
	SetDialogCancelItem(theDialog, CANCEL_BUTTON);
	SetDialogTracksCursor(theDialog, 1);
	
	if (err = InitDialogSettings(theDialog, &ds)) {		// This calls InitPopMenus and calls KillPopMenus in the event of an error.
		DisposeDialogStorage(&ds);
		DisposeXOPDialog(theDialog);
		SetPort(savePort);
		return err;
	}
	
	ShowWindow(GetDialogWindow(theDialog));
	do {
		XOPDialog(NULL, &itemHit);
		switch (itemHit) {
			default:
				HandleItemHit(theDialog, itemHit, &ds);
				break;
		}
	} while (itemHit!=OK_BUTTON && itemHit!=CANCEL_BUTTON);

	SetDialogBalloonHelpID(-1);					// Reset resource ID for Macintosh balloons.
	
	ShutdownDialogSettings(theDialog, &ds);

	DisposeDialogStorage(&ds);
	
	DisposeXOPDialog(theDialog);
	SetPort(savePort);

	return itemHit==OK_BUTTON ? 0:-1;			// 0 for OK, -1 for cancel.
}

#endif					// Macintosh-specific code ]


#ifdef WINIGOR			// Windows-specific code [

static INT_PTR CALLBACK
DialogProc(HWND theDialog, UINT msgCode, WPARAM wParam, LPARAM lParam)
{
	int itemID, notificationMessage;
	BOOL result; 						// Function result

	static DialogStoragePtr dsp;

	result = FALSE;
	itemID = LOWORD(wParam);						// Item, control, or accelerator identifier.
	notificationMessage = HIWORD(wParam);
	
	switch(msgCode) {
		case WM_INITDIALOG:
			PositionWinDialogWindow(theDialog, NULL);		// Position nicely relative to Igor MDI client.
			
			dsp = (DialogStoragePtr)lParam;
			if (InitDialogSettings(theDialog, dsp) != 0) {
				EndDialog(theDialog, IDCANCEL);				// Should never happen.
				return FALSE;
			}

			SetFocus(GetDlgItem(theDialog, PORT_POPUP));
			result = FALSE; // Tell Windows not to set the input focus			
			break;
		
		case WM_COMMAND:
			switch(itemID) {
				case OK_BUTTON:
				case CANCEL_BUTTON:
					HandleItemHit(theDialog, itemID, dsp);
					ShutdownDialogSettings(theDialog, dsp);
					EndDialog(theDialog, itemID);
					result = TRUE;
					break;				
				
				default:
					if (!IsWinDialogItemHitMessage(theDialog, itemID, notificationMessage))
						break;					// This is not a message that we need to handle.
					HandleItemHit(theDialog, itemID, dsp);
					break;
			}
			break;
	}
	return result;
}

int
VDTSettingsDialog(int useExpSettingsIn, int* useExpSettingsOutPtr)
{
	DialogStorage ds;
	INT_PTR result;
	
	*useExpSettingsOutPtr = useExpSettingsIn;
	if (result = InitDialogStorage(&ds, useExpSettingsIn, useExpSettingsOutPtr))
		return (int)result;

	result = DialogBoxParam(XOPModule(), MAKEINTRESOURCE(DIALOG_TEMPLATE_ID), IgorClientHWND(), DialogProc, (LPARAM)&ds);

	DisposeDialogStorage(&ds);

	if (result == OK_BUTTON)
		return 0;
	return -1;					// Cancel.
}

#endif					// Windows-specific code ]