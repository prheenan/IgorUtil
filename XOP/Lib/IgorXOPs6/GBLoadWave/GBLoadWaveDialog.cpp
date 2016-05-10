/*	GBLoadWaveDialog.c -- dialog handling routines for the GBLoadWave XOP.

*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "GBLoadWave.h"

#ifdef WINIGOR
	#include "resource.h"
#endif

// Global Variables

// Equates
#define DIALOG_TEMPLATE_ID 1260

enum {								// These are both Macintosh item numbers and Windows item IDs.
	DOIT_BUTTON=1,
	CANCEL_BUTTON,					// Cancel button ID must be 2 on Windows for else the escape key won't work.
	TOCMD_BUTTON,
	TOCLIP_BUTTON,
	CMD_BOX,
	HELP_BUTTON,
	
	INPUT_FILE_BOX,					// Item 7
	
	INPUT_TYPE_POPUP_TITLE,
	INPUT_TYPE_POPUP,
	FLOAT_FORMAT_POPUP_TITLE,
	FLOAT_FORMAT_POPUP,
	PATH_POPUP_TITLE,
	PATH_POPUP,
	FILE_BUTTON,
	FILE_NAME,
	FILE_UNDERLINE,
	
	SKIP_BYTES_TITLE,				// Item 17
	SKIP_BYTES_TEXT,
	NUMBER_ARRAYS_TITLE,
	NUMBER_ARRAYS_TEXT,
	NUMBER_POINTS_TITLE,
	NUMBER_POINTS_TEXT,
	
	BYTE_ORDER_POPUP_TITLE,
	BYTE_ORDER_POPUP,
	POINTS_INTERLEAVED,
	
	OUTPUT_WAVES_BOX,				// Item 26
	OVERWRITE_WAVES,
	BASE_NAME_TITLE,
	BASE_NAME_TEXT,
	SCALING_CHECKBOX,
	OUTPUT_TYPE_POPUP_TITLE,
	OUTPUT_TYPE_POPUP
};


// Structures

struct DialogStorage {						// See InitDialogStorage for a discussion of this structure.
	char filePath[MAX_PATH_LEN+1];			// Room for full file specification.
	int pointOpenFileDialog;				// Determines when we point the open file dialog at a particular directory.
	double offset;							// Output data = (input data + offset) * multiplier.
	double multiplier;
};
typedef struct DialogStorage DialogStorage;
typedef struct DialogStorage *DialogStoragePtr;

static void
SaveDialogPrefs(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	GBLoadInfoHandle gbHandle;
	int itemNumber;
	int int1;
	double d1;
	char temp[256];

	/* save dialog settings for next time */
	gbHandle = (GBLoadInfoHandle)NewHandle((BCInt)sizeof(GBLoadInfo));
	if (gbHandle != NULL) {
		MemClear((char *)(*gbHandle), (BCInt)sizeof(GBLoadInfo));
		(*gbHandle)->version = GBLoadInfo_VERSION;
		
		GetPopMenu(theDialog, INPUT_TYPE_POPUP, &itemNumber, temp);
		(*gbHandle)->inputDataTypeItemNumber = itemNumber;
		GetPopMenu(theDialog, OUTPUT_TYPE_POPUP, &itemNumber, temp);
		(*gbHandle)->outputDataTypeItemNumber = itemNumber;

		GetPopMenu(theDialog, FLOAT_FORMAT_POPUP, &itemNumber, temp);
		(*gbHandle)->floatFormatItemNumber = itemNumber;
		
		// (*gbHandle)->filterStr is no longer used.
		
		GetPopMenu(theDialog, PATH_POPUP, &itemNumber, temp);
		if (strlen(temp) < sizeof((*gbHandle)->symbolicPathName))
			strcpy((*gbHandle)->symbolicPathName, temp);
		
		if (GetDDouble(theDialog, SKIP_BYTES_TEXT, &d1) == 0)
			(*gbHandle)->preambleBytes = d1;

		if (GetDInt(theDialog, NUMBER_ARRAYS_TEXT, &int1) == 0)
			(*gbHandle)->numArrays = int1;

		if (GetDDouble(theDialog, NUMBER_POINTS_TEXT, &d1) == 0)
			(*gbHandle)->arrayPoints = d1;

		GetPopMenu(theDialog, BYTE_ORDER_POPUP, &itemNumber, temp);
		(*gbHandle)->bytesSwapped = itemNumber > 1;
		
		(*gbHandle)->interleaved = GetCheckBox(theDialog, POINTS_INTERLEAVED);
		(*gbHandle)->overwrite = GetCheckBox(theDialog, OVERWRITE_WAVES);
		(*gbHandle)->scalingEnabled = GetCheckBox(theDialog, SCALING_CHECKBOX);
		
		(*gbHandle)->offset = dsp->offset;
		(*gbHandle)->multiplier = dsp->multiplier;

		if (GetDText(theDialog, BASE_NAME_TEXT, temp) < sizeof((*gbHandle)->baseName))
			strcpy((*gbHandle)->baseName, temp);
		
		SaveXOPPrefsHandle((Handle)gbHandle);		// Does nothing prior to IGOR Pro 3.1.
		DisposeHandle((Handle)gbHandle);
	}
}

static void
GetDialogPrefs(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	GBLoadInfoHandle gbHandle;
	char temp[256];

	GetXOPPrefsHandle((Handle*)&gbHandle);			// Returns null handle prior to IGOR Pro 3.1.

	if (gbHandle && (*gbHandle)->version==GBLoadInfo_VERSION) {
		SetPopItem(theDialog, INPUT_TYPE_POPUP, (*gbHandle)->inputDataTypeItemNumber);
		SetPopItem(theDialog, OUTPUT_TYPE_POPUP, (*gbHandle)->outputDataTypeItemNumber);
		SetPopItem(theDialog, FLOAT_FORMAT_POPUP, (*gbHandle)->floatFormatItemNumber);
		
		strcpy(temp, (*gbHandle)->symbolicPathName);
		SetPopMatch(theDialog, PATH_POPUP, temp);
		
		// (*gbHandle)->filterStr) is no longer used.
		
		sprintf(temp, "%lld", (SInt64)(*gbHandle)->preambleBytes);
		SetDText(theDialog, SKIP_BYTES_TEXT, temp);
	
		if ((*gbHandle)->numArrays) {
			sprintf(temp, "%d", (int)(*gbHandle)->numArrays);
			SetDText(theDialog, NUMBER_ARRAYS_TEXT, temp);
		}
	
		if ((*gbHandle)->arrayPoints) {
			sprintf(temp, "%lld", (SInt64)(*gbHandle)->arrayPoints);
			SetDText(theDialog, NUMBER_POINTS_TEXT, temp);
		}
		
		SetPopItem(theDialog, BYTE_ORDER_POPUP, (*gbHandle)->bytesSwapped ? 2:1);
		
		SetCheckBox(theDialog, POINTS_INTERLEAVED, (int)(*gbHandle)->interleaved);
		SetCheckBox(theDialog, OVERWRITE_WAVES, (int)(*gbHandle)->overwrite);
		SetCheckBox(theDialog, SCALING_CHECKBOX, (int)(*gbHandle)->scalingEnabled);
		
		strcpy(temp, (*gbHandle)->baseName);
		SetDText(theDialog, BASE_NAME_TEXT, temp);
		
		dsp->offset = (*gbHandle)->offset;
		dsp->multiplier = (*gbHandle)->multiplier;
	}
	
	if (gbHandle != NULL)
		DisposeHandle((Handle)gbHandle);
}

/*	InitDialogStorage(dsp)

	We use a DialogStorage structure to store working values during the dialog.
	In a Macintosh application, the fields in this structure could be local variables
	in the main dialog routine. However, in a Windows application, they would have
	to be globals. By using a structure like this, we are able to avoid using globals.
	Also, routines that access these fields (such as this one) can be used for
	both platforms.
*/
static int
InitDialogStorage(DialogStorage* dsp)
{
	MemClear(dsp, sizeof(DialogStorage));
	dsp->offset = 0.0;
	dsp->multiplier = 1.0;
	return 0;
}

static void
DisposeDialogStorage(DialogStorage* dsp)
{
	// Does nothing for this dialog.
}

static int
InitDialogPopups(XOP_DIALOG_REF theDialog)
{
	int err;

	err = 0;
	do {
		const char* dataTypeStr;
		dataTypeStr = "Double float;Single float;32 bit signed integer;16 bit signed integer;8 bit signed integer;32 bit unsigned integer;16 bit unsigned integer;8 bit unsigned integer";
		
		if (err = CreatePopMenu(theDialog, PATH_POPUP, PATH_POPUP_TITLE, "_none_", 1))
			break;
		FillPathPopMenu(theDialog, PATH_POPUP, "*", "", 1);
		
		if (err = CreatePopMenu(theDialog, INPUT_TYPE_POPUP, INPUT_TYPE_POPUP_TITLE, dataTypeStr, 1))
			break;
		
		if (err = CreatePopMenu(theDialog, OUTPUT_TYPE_POPUP, OUTPUT_TYPE_POPUP_TITLE, dataTypeStr, 1))
			break;
		
		if (err = CreatePopMenu(theDialog, FLOAT_FORMAT_POPUP, FLOAT_FORMAT_POPUP_TITLE, "IEEE;VAX", 1))
			break;
		
		if (err = CreatePopMenu(theDialog, BYTE_ORDER_POPUP, BYTE_ORDER_POPUP_TITLE, "High byte first;Low byte first", 1))
			break;
	} while(0);
	
	return err;
}

/*	UpdateDialogItems(theDialog, dsp)

	Shows, hides, enables, disables dialog items as necessary.
*/
static void
UpdateDialogItems(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	int enableInterleave;
	int itemNumber;
	int int1;
	char temp[256];

	GetPopMenu(theDialog, INPUT_TYPE_POPUP, &itemNumber, temp);
	if (itemNumber > 2) {			/* Not a floating point format? */
		HideDialogItem(theDialog, FLOAT_FORMAT_POPUP);
		HideDialogItem(theDialog, FLOAT_FORMAT_POPUP_TITLE);
	}
	else {
		ShowDialogItem(theDialog, FLOAT_FORMAT_POPUP);
		ShowDialogItem(theDialog, FLOAT_FORMAT_POPUP_TITLE);
	}
	
	enableInterleave = GetDInt(theDialog, NUMBER_ARRAYS_TEXT, &int1)==0 && int1>1;
	HiliteDControl(theDialog, POINTS_INTERLEAVED, enableInterleave);
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
		if (err = InitDialogPopups(theDialog))
			break;
		
		SetDInt(theDialog, SKIP_BYTES_TEXT, 0);
		SetDInt(theDialog, NUMBER_ARRAYS_TEXT, 1);
		SetDText(theDialog, NUMBER_POINTS_TEXT, "auto");
		SetDText(theDialog, BASE_NAME_TEXT, "wave");
		SetDText(theDialog, FILE_NAME, "");
	
		SetCheckBox(theDialog, POINTS_INTERLEAVED, 0);
		SetCheckBox(theDialog, OVERWRITE_WAVES, 0);
		SetCheckBox(theDialog, SCALING_CHECKBOX, 0);
	
		GetDialogPrefs(theDialog, dsp);
		
		UpdateDialogItems(theDialog, dsp);
	} while(0);
	
	if (err)
		KillPopMenus(theDialog);
	
	return err;
}

static void
ShutdownDialogSettings(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	SaveDialogPrefs(theDialog, dsp);
	KillPopMenus(theDialog);
}

/*	SymbolicPathPointsToFolder(symbolicPathName, folderPath)

	symbolicPathName is the name of an Igor symbolic path.

	folderPath is a full path to a folder. This is a native path with trailing
	colon (Mac) or backslash (Win).
	
	Returns true if the symbolic path refers to the folder specified by folderPath.
*/
static int
SymbolicPathPointsToFolder(const char* symbolicPathName, const char* folderPath)
{
	char symbolicPathDirPath[MAX_PATH_LEN+1];	// This is a native path with trailing colon (Mac) or backslash (Win).
	
	if (GetPathInfo2(symbolicPathName, symbolicPathDirPath))
		return 0;		// Error getting path info.
	
	return CmpStr(folderPath, symbolicPathDirPath) == 0;
}

static int
DataTypeItemNumberToDataTypeCode(int dataTypeItemNumber)
{
	int result;
	
	switch(dataTypeItemNumber) {
		case 1:
			result = NT_FP64;
			break;
		case 2:
			result = NT_FP32;
			break;
		case 3:
			result = NT_I32;
			break;
		case 4:
			result = NT_I16;
			break;
		case 5:
			result = NT_I8;
			break;
		case 6:
			result = NT_I32 | NT_UNSIGNED;
			break;
		case 7:
			result = NT_I16 | NT_UNSIGNED;
			break;
		case 8:
			result = NT_I8 | NT_UNSIGNED;
			break;
		default:
			result = NT_FP64;
			break;	
	}
	return result;
}

/*	GetCmd(theDialog, dsp, cmd)

	Generates a GBLoadWave command based on the dialog's items.
	
	Returns 0 if the command is OK or non-zero if it is not valid.
	Returns -1 if cmd contains error message that should be shown in dialog.
*/
static int
GetCmd(XOP_DIALOG_REF theDialog, DialogStorage* dsp, char cmd[MAXCMDLEN+1])
{
	char temp[256+2];
	char fullFilePath[MAX_PATH_LEN+1];
	char fileDirPath[MAX_PATH_LEN+1];
	char fileName[MAX_FILENAME_LEN+1];
	int pathItemNumber, useSymbolicPath;
	char symbolicPathName[MAX_OBJ_NAME+1];
	int overwrite;
	int inputTypeItemNumber, outputTypeItemNumber, floatFormatItemNumber, byteOrderItemNumber;
	int inputTypeCode, outputTypeCode;
	
	*cmd = 0;
	
	if (*dsp->filePath == 0)
		return 1;
	
	strcpy(fullFilePath, dsp->filePath);
	if (GetDirectoryAndFileNameFromFullPath(fullFilePath, fileDirPath, fileName) != 0) {
		sprintf(cmd, "*** GetDirectoryAndFileNameFromFullPath Error ***");
		return -1;		// Should never happen.
	}
	
	strcpy(cmd, "GBLoadWave");

	/*	We use a symbolic path if the user has chosen one and if it points to the
		folder containing the file that the user selected.
	*/
	useSymbolicPath = 0;
	GetPopMenu(theDialog, PATH_POPUP, &pathItemNumber, symbolicPathName);
	if (pathItemNumber > 1) {					// Item 1 is "_none_".
		if (SymbolicPathPointsToFolder(symbolicPathName, fileDirPath)) {
			strcat(cmd, "/P=");
			strcat(cmd, symbolicPathName);
			useSymbolicPath = 1;
		}
	}

	if (overwrite = GetCheckBox(theDialog, OVERWRITE_WAVES))
		strcat(cmd, "/O");
		
	GetPopMenu(theDialog, BYTE_ORDER_POPUP, &byteOrderItemNumber, temp);
	if (byteOrderItemNumber > 1)					// Low byte first?
		strcat(cmd, "/B");
		
	if (GetCheckBox(theDialog, POINTS_INTERLEAVED))
		strcat(cmd, "/V");
	
	GetDText(theDialog, BASE_NAME_TEXT, temp);
	if (*temp) {
		if (overwrite) {
			strcat(cmd, "/N=");
			strcat(cmd, temp);
		}
		else {
			if (strcmp(temp, "wave")) {			/* /A=wave is default */
				strcat(cmd, "/A=");
				strcat(cmd, temp);
			}
		}
	}
	
	GetPopMenu(theDialog, INPUT_TYPE_POPUP, &inputTypeItemNumber, temp);
	GetPopMenu(theDialog, OUTPUT_TYPE_POPUP, &outputTypeItemNumber, temp);
	inputTypeCode = DataTypeItemNumberToDataTypeCode(inputTypeItemNumber);
	outputTypeCode = DataTypeItemNumberToDataTypeCode(outputTypeItemNumber);
	sprintf(temp, "/T={%d,%d}", inputTypeCode, outputTypeCode);
	strcat(cmd, temp);
	
	if (inputTypeCode==NT_FP64 || inputTypeCode==NT_FP32) {
		GetPopMenu(theDialog, FLOAT_FORMAT_POPUP, &floatFormatItemNumber, temp);
		if (floatFormatItemNumber==2)
			strcat(cmd, "/J=2");
	}
	
	if (GetCheckBox(theDialog, SCALING_CHECKBOX)) {
		sprintf(temp, "/Y={%g, %g}", dsp->offset, dsp->multiplier);
		strcat(cmd, temp);
	}
	
	GetDText(theDialog, SKIP_BYTES_TEXT, temp);
	{
		SInt64 skipBytes;

		if (sscanf(temp, " %lld", &skipBytes) <= 0)		/* number not valid ? */
			return(SKIP_BYTES_TEXT);
		if (skipBytes < 0)								/* number not valid ? */
			return(SKIP_BYTES_TEXT);
		if (skipBytes) {
			sprintf(temp, "/S=%lld", skipBytes);
			strcat(cmd, temp);
		}
	}
	
	GetDText(theDialog, NUMBER_ARRAYS_TEXT,temp);
	if (CmpStr(temp, "AUTO")) {
		int numArrays;
		if (sscanf(temp, " %d", &numArrays) <= 0)		/* number not valid ? */
			return(NUMBER_ARRAYS_TEXT);
		if (numArrays < 1)								/* number not valid ? */
			return(NUMBER_ARRAYS_TEXT);
		sprintf(temp, "/W=%d", numArrays);
		strcat(cmd, temp);
	}
	
	GetDText(theDialog, NUMBER_POINTS_TEXT,temp);
	if (CmpStr(temp, "AUTO")) {
		SInt64 numPoints;
		if (sscanf(temp, " %lld", &numPoints) <= 0)		/* number not valid ? */
			return(NUMBER_POINTS_TEXT);
		if (numPoints < 2)								/* number not valid ? */
			return(NUMBER_POINTS_TEXT);
		sprintf(temp, "/U=%lld", numPoints);
		strcat(cmd, temp);
	}
	
	// Add file name or full path to the command.
	{
		char* p1;

		if (useSymbolicPath) {
			p1 = fileName;
		}
		else {
			/*	In generating Igor commands, we always use Macintosh-style paths,
				even when running on Windows. This avoids a complication presented
				by the fact that backslash is an escape character in Igor.
			*/
			WinToMacPath(fullFilePath);
			p1 = fullFilePath;
		}
		
		{
			int length;
			
			/*	HR, 090403, 1.64: This handles escaping backslashes if the path is a
				Windows UNC path (\\server\share).
			*/
			length = (int)strlen(fullFilePath) + 1;		// Include null terminator.
			if (EscapeSpecialCharacters(fullFilePath, length, fullFilePath, sizeof(fullFilePath), &length)) {
				strcpy(cmd, "*** Path is too long to fit in command buffer ***");
				return -1;
			}			
		}
		
		if (strlen(cmd) + strlen(p1) > MAXCMDLEN) {	/* command too long ? */
			strcpy(cmd, "*** Command is too long to fit in command buffer ***");
			return -1;
		}
		
		strcat(cmd, " \"");
		strcat(cmd, p1);
		strcat(cmd, "\"");
	}
	
	return(0);
}

/*	ShowCmd(theDialog, dsp, cmd)

	Displays GBLoadWave cmd in dialog cmd box.
	Returns 0 if cmd is OK, non-zero otherwise.
	
	Also, enables or disables buttons based on cmd being OK or not.
*/
static int
ShowCmd(XOP_DIALOG_REF theDialog, DialogStorage* dsp, char cmd[MAXCMDLEN+1])
{
	int result, enable;

	result = GetCmd(theDialog, dsp, cmd);
	if (result) {
		*cmd = 0;
		enable = 0;
	}
	else {
		enable = 1;
	}
	
	HiliteDControl(theDialog, DOIT_BUTTON, enable);
	HiliteDControl(theDialog, TOCLIP_BUTTON, enable);
	HiliteDControl(theDialog, TOCMD_BUTTON, enable);
	
	DisplayDialogCmd(theDialog, CMD_BOX, cmd);

	return result;
}

/*	GetFileToLoad(theDialog, pointOpenFileDialog, fullFilePath)

	Displays open file dialog. Returns 0 if the user clicks Open or -1 if
	the user cancels.
	
	If the user clicks Open, returns via fullFilePath the full native path
	to the file to be loaded.
	
	If pointOpenFileDialog is true then the open file dialog will initially
	display the directory associated with the symbolic path selected in the
	path popup menu.
*/
static int
GetFileToLoad(XOP_DIALOG_REF theDialog, int pointOpenFileDialog, char fullFilePath[MAX_PATH_LEN+1])
{
	char symbolicPathName[MAX_OBJ_NAME+1];
	char initialDir[MAX_PATH_LEN+1];
	int pathItemNumber;
	int result;
	
	// Assume that the last path shown in the open file dialog is what we want to show.
	*initialDir = 0;
	
	// If the user just selected a symbolic path, show the corresponding folder in the open file dialog.
	if (pointOpenFileDialog) {
		GetPopMenu(theDialog, PATH_POPUP, &pathItemNumber, symbolicPathName);
		if (pathItemNumber > 1)					// Item 1 is "_none_".
			GetPathInfo2(symbolicPathName, initialDir);
	}
	
	// Display the open file dialog.
	result = XOPOpenFileDialog("Looking for a general binary file", "", NULL, initialDir, fullFilePath);

	#ifdef WINIGOR				// We need to reactivate the parent dialog.
		SetWindowPos(theDialog, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	#endif
	
	// Set the dialog FILE_NAME item.
	if (result == 0) {
		char fileDirPath[MAX_PATH_LEN+1];
		char fileName[MAX_FILENAME_LEN+1];
		
		if (GetDirectoryAndFileNameFromFullPath(fullFilePath, fileDirPath, fileName) != 0)
			strcpy(fileName, "***Error***");				// Should never happen.

		SetDText(theDialog, FILE_NAME, fileName);
	}
	
	return result;
}

/*	HandleItemHit(theDialog, itemID, dsp)
	
	Called when the item identified by itemID is hit.
	Carries out any actions necessitated by the hit.
*/
static void
HandleItemHit(XOP_DIALOG_REF theDialog, int itemID, DialogStorage* dsp)
{
	int selItem;
	
	if (ItemIsPopMenu(theDialog, itemID))
		GetPopMenu(theDialog, itemID, &selItem, NULL);
	
	switch(itemID) {
		case INPUT_TYPE_POPUP:
		case OUTPUT_TYPE_POPUP:
		case FLOAT_FORMAT_POPUP:
		case BYTE_ORDER_POPUP:
			// Nothing more needs to be done for these popup.
			break;

		case FILE_BUTTON:
			GetFileToLoad(theDialog, dsp->pointOpenFileDialog, dsp->filePath);
			dsp->pointOpenFileDialog = 0;
			break;

		case PATH_POPUP:
			dsp->pointOpenFileDialog = 1;		// Flag that we want to point open file dialog at a particular directory.
			break;

		case POINTS_INTERLEAVED:
		case OVERWRITE_WAVES:
			ToggleCheckBox(theDialog, itemID);
			break;
		
		case SCALING_CHECKBOX:					// Displays scaling subdialog.
			if (ToggleCheckBox(theDialog, SCALING_CHECKBOX)) {
				SetDialogBalloonHelpID(-1);									// Turn off dialog balloons for subdialog (NOP on Windows).
				if (GBScalingDialog(&dsp->offset, &dsp->multiplier) != 0)	// User clicked cancel?
					SetCheckBox(theDialog, SCALING_CHECKBOX, 0);
				SetDialogBalloonHelpID(DIALOG_TEMPLATE_ID);					// Restore dialog balloons (NOP on Windows. Now also a NOP under Carbon.).
				#ifdef WINIGOR					// We need to reactivate the parent dialog.
					SetWindowPos(theDialog, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				#endif
			}
			break;
		
		case HELP_BUTTON:
			// This does nothing prior to Igor Pro 3.13B03.
			XOPDisplayHelpTopic("GBLoadWave Help", "GBLoadWave XOP[The Load General Binary Dialog]", 1);
			break;

		case DOIT_BUTTON:
		case TOCMD_BUTTON:
		case TOCLIP_BUTTON:
			{
				char cmd[MAXCMDLEN+1];
				
				GetCmd(theDialog, dsp, cmd);
				switch(itemID) {
					case DOIT_BUTTON:
						FinishDialogCmd(cmd, 1);
						break;
					case TOCMD_BUTTON:
						FinishDialogCmd(cmd, 2);
						break;
					case TOCLIP_BUTTON:
						FinishDialogCmd(cmd, 3);
						break;
				}
			}
			break;
	}

	UpdateDialogItems(theDialog, dsp);
}

#ifdef MACIGOR			// Macintosh-specific code [

/*	GBLoadWaveDialog()

	GBLoadWaveDialog is called when user selects 'Load General Binary...'
	from 'Load Waves' submenu.
	
	Returns 0 if OK, -1 for cancel or an error code.
*/
int
GBLoadWaveDialog(void)
{
	DialogStorage ds;
	DialogPtr theDialog;
	GrafPtr savePort;
	short itemHit;
	char cmd[MAXCMDLEN+1];
	int err;
	
	if (err = InitDialogStorage(&ds))
		return -1;

	theDialog = GetXOPDialog(DIALOG_TEMPLATE_ID);
	savePort = SetDialogPort(theDialog);
	SetDialogBalloonHelpID(DIALOG_TEMPLATE_ID);					// NOP on Windows. Now also a NOP under Carbon.

	// As of Carbon 1.3, this call messes up the activation state of the Do It button. This appears to be a Mac OS bug.
	SetDialogDefaultItem(theDialog, DOIT_BUTTON);
	SetDialogCancelItem(theDialog, CANCEL_BUTTON);
	SetDialogTracksCursor(theDialog, 1);
	
	if (err = InitDialogSettings(theDialog, &ds)) {
		DisposeDialogStorage(&ds);
		DisposeXOPDialog(theDialog);
		SetPort(savePort);
		return err;
	}

	ShowCmd(theDialog, &ds, cmd);
	SelEditItem(theDialog, SKIP_BYTES_TEXT);
	
	ShowDialogWindow(theDialog);

	do {
		DoXOPDialog(&itemHit);
		switch(itemHit) {
			default:
				HandleItemHit(theDialog, itemHit, &ds);
				break;
		}
		ShowCmd(theDialog, &ds, cmd);
	} while (itemHit<DOIT_BUTTON || itemHit>TOCLIP_BUTTON);
	
	ShutdownDialogSettings(theDialog, &ds);

	DisposeDialogStorage(&ds);
	
	DisposeXOPDialog(theDialog);
	SetPort(savePort);

	SetDialogBalloonHelpID(-1);										/* reset resource ID for balloons */

	return itemHit==CANCEL_BUTTON ? -1 : 0;
}

#endif					// Macintosh-specific code ]


#ifdef WINIGOR			// Windows-specific code [

static INT_PTR CALLBACK
DialogProc(HWND theDialog, UINT msgCode, WPARAM wParam, LPARAM lParam)
{
	char cmd[MAXCMDLEN+1];
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
			
			ShowCmd(theDialog, dsp, cmd);

			SetFocus(GetDlgItem(theDialog, PATH_POPUP));
			result = FALSE; // Tell Windows not to set the input focus			
			break;
		
		case WM_COMMAND:
			switch(itemID) {
				case DOIT_BUTTON:
				case TOCMD_BUTTON:
				case TOCLIP_BUTTON:
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
					ShowCmd(theDialog, dsp, cmd);
					break;
			}
			break;
	}
	return result;
}

/*	GBLoadWaveDialog()

	GBLoadWaveDialog is called when user selects 'Load General Binary...'
	from 'Load Waves' submenu.
	
	Returns 0 if OK, -1 for cancel or an error code.
*/
int
GBLoadWaveDialog(void)
{
	DialogStorage ds;
	int result;
	
	if (result = InitDialogStorage(&ds))
		return result;

	result = (int)DialogBoxParam(XOPModule(), MAKEINTRESOURCE(DIALOG_TEMPLATE_ID), IgorClientHWND(), DialogProc, (LPARAM)&ds);

	DisposeDialogStorage(&ds);

	if (result != CANCEL_BUTTON)
		return 0;
	return -1;					// Cancel.
}

#endif					// Windows-specific code ]
