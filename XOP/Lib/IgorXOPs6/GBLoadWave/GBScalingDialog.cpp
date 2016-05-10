/*	GBScalingDialog.c

	Implements the scaling subdialog accessed via the main GBLoadWave dialog.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "GBLoadWave.h"

#ifdef WINIGOR
	#include "resource.h"
#endif

// Global Variables

// Equates

#define DIALOG_TEMPLATE_ID 1261

enum {								// These are both Macintosh item numbers and Windows item IDs.
	OK_BUTTON=1,
	CANCEL_BUTTON,
	SCALING_FORMULA_TEXT,
	SCALING_OFFSET_TITLE,
	SCALING_OFFSET_TEXT,
	SCALING_MULTIPLIER_TITLE,
	SCALING_MULTIPLIER_TEXT
};

// Structures

struct DialogStorage {				// See InitDialogStorage for a discussion of this structure.
	double* offsetPtr;				// Where to get input value, where to store output value.
	double* multiplierPtr;			// Where to get input value, where to store output value.
	int offsetNoGood;
	int multiplierNoGood;
};
typedef struct DialogStorage DialogStorage;
typedef struct DialogStorage *DialogStoragePtr;

/*	InitDialogStorage(dsp, offset, multiplier)

	We use a DialogStorage structure to store working values during the dialog.
	In a Macintosh application, the fields in this structure could be local variables
	in the main dialog routine. However, in a Windows application, they would have
	to be globals. By using a structure like this, we are able to avoid using globals.
	Also, routines that access these fields (such as this one) can be used for
	both platforms.
*/
static int
InitDialogStorage(DialogStorage* dsp, double* offsetPtr, double* multiplierPtr)
{
	MemClear(dsp, sizeof(DialogStorage));
	dsp->offsetPtr = offsetPtr;
	dsp->multiplierPtr = multiplierPtr;
	return 0;
}

static void
DisposeDialogStorage(DialogStorage* dsp)
{
	// Does nothing for this dialog.
}

/*	UpdateDialogItems(theDialog, dsp)

	Shows, hides, enables, disables dialog items as necessary.
*/
static void
UpdateDialogItems(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	if (dsp->offsetNoGood || dsp->multiplierNoGood)
		DisableDControl(theDialog, OK_BUTTON);
	else
		EnableDControl(theDialog, OK_BUTTON);
}

/*	InitDialogSettings(theDialog, dsp)

	Called when the dialog is first displayed to initialize all items.
*/
static int
InitDialogSettings(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	int err;
	
	err = 0;
	
	InitPopMenus(theDialog);
	
	do {
		SetDDouble(theDialog, SCALING_OFFSET_TEXT, dsp->offsetPtr);
		SetDDouble(theDialog, SCALING_MULTIPLIER_TEXT, dsp->multiplierPtr);
		SelEditItem(theDialog, SCALING_OFFSET_TEXT);
		
		UpdateDialogItems(theDialog, dsp);
	} while(0);
	
	if (err != 0)
		KillPopMenus(theDialog);

	return err;
}

static void
ShutdownDialogSettings(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	KillPopMenus(theDialog);
}

/*	ProcessOK(theDialog, dsp)
	
	Called when the user presses OK to carry out the user-requested actions.
*/
static void
ProcessOK(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	GetDDouble(theDialog, SCALING_OFFSET_TEXT, dsp->offsetPtr);
	GetDDouble(theDialog, SCALING_MULTIPLIER_TEXT, dsp->multiplierPtr);
}

/*	HandleItemHit(theDialog, itemID, dsp)
	
	Called when the item identified by itemID is hit.
	Carries out any actions necessitated by the hit.
*/
static void
HandleItemHit(XOP_DIALOG_REF theDialog, int itemID, DialogStorage* dsp)
{
	int selItem;
	double d1;
	
	if (ItemIsPopMenu(theDialog, itemID))
		GetPopMenu(theDialog, itemID, &selItem, NULL);
	
	switch(itemID) {
		case SCALING_OFFSET_TEXT:
			dsp->offsetNoGood = GetDDouble(theDialog, SCALING_OFFSET_TEXT, &d1);
			break;
		
		case SCALING_MULTIPLIER_TEXT:
			dsp->multiplierNoGood = GetDDouble(theDialog, SCALING_MULTIPLIER_TEXT, &d1);
			break;
		
		case OK_BUTTON:
			ProcessOK(theDialog, dsp);
			break;
	}

	UpdateDialogItems(theDialog, dsp);
}

#ifdef MACIGOR			// Macintosh-specific code [

/*	GBScalingDialog()

	GBScalingDialog is called when user clicks the Scaling button in the
	GBLoadWave dialog.
	
	offsetPtr and multiplierPtr are used as both inputs and outputs.
	If the user cancels, they are not changed.
	
	Returns 0 if user clicks OK, -1 if user clicks cancel, or an error code.
*/
int
GBScalingDialog(double* offsetPtr, double* multiplierPtr)
{
	DialogPtr theDialog;
	DialogStorage ds;
	short itemHit;
	GrafPtr savePort;
	int err;
	
	if (err = InitDialogStorage(&ds, offsetPtr, multiplierPtr))
		return err;
	
	theDialog = GetXOPDialog(DIALOG_TEMPLATE_ID);
	savePort = SetDialogPort(theDialog);
	
	if (err = InitDialogSettings(theDialog, &ds)) {
		DisposeDialogStorage(&ds);
		DisposeXOPDialog(theDialog);
		SetPort(savePort);
		return err;
	}
	
	ShowDialogWindow(theDialog);
	do {
		DoXOPDialog(&itemHit);
		switch (itemHit) {
			default:
				HandleItemHit(theDialog, itemHit, &ds);
				break;
		}
	} while (itemHit!=OK_BUTTON && itemHit!=CANCEL_BUTTON);
	
	ShutdownDialogSettings(theDialog, &ds);

	DisposeDialogStorage(&ds);

	DisposeXOPDialog(theDialog);
	SetPort(savePort);
	
	if (itemHit == OK_BUTTON)
		return 0;
	return -1;			// Cancel.
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
			// Position nicely relative to the main dialog.
			PositionWinDialogWindow(theDialog, GetNextWindow(theDialog, GW_HWNDNEXT));
			
			dsp = (DialogStoragePtr)lParam;
			if (InitDialogSettings(theDialog, dsp) != 0) {
				EndDialog(theDialog, IDCANCEL);				// Should never happen.
				return FALSE;
			}

			SetFocus(GetDlgItem(theDialog, SCALING_OFFSET_TEXT));
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

/*	GBScalingDialog()

	GBScalingDialog is called when user clicks the Scaling button in the
	GBLoadWave dialog.
	
	offsetPtr and multiplierPtr are used as both inputs and outputs.
	If the user cancels, they are not changed.
	
	Returns 0 if user clicks OK, -1 if user clicks cancel, or an error code.
*/
int
GBScalingDialog(double* offsetPtr, double* multiplierPtr)
{
	DialogStorage ds;
	int result;
	
	if (result = InitDialogStorage(&ds, offsetPtr, multiplierPtr))
		return result;

	result = (int)DialogBoxParam(XOPModule(), MAKEINTRESOURCE(DIALOG_TEMPLATE_ID), IgorClientHWND(), DialogProc, (LPARAM)&ds);

	DisposeDialogStorage(&ds);

	if (result == OK_BUTTON)
		return 0;
	return -1;					// Cancel.
}

#endif					// Windows-specific code ]