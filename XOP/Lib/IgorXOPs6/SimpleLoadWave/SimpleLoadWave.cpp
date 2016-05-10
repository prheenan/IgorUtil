/*	SimpleLoadWave.cpp
	
	This XOP is an example for XOP programmers who want to write
	file loader XOPs. Thus, we have kept is as simple as possible.
	
	The file is assumed to be tab- or comma-delimited. It is assumed to contain
	numeric data, optionally preceded by a row of column names. If column names
	are present, they are assumed to be on the first line, with the numeric data
	starting on the second line. If there are no column names, then the numeric
	data is assumed to start on the first line.
	
	See "SimpleLoadWave Help" for an explanation of how to use the SimpleLoadWave XOP.
	
	HR, 10/19/96:
		Changed to make CR, CRLF or LF be acceptable end-of-line markers.
		
		It does not handle LFCR which is a deviant form created by some
		whacko file translators.
	
	HR, 980427:
		Made platform-independent.
		
	HR, 020919:
		Revamped to use Operation Handler so that the SimpleLoadWave operation
		can be called from a user function.
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "SimpleLoadWave.h"


// Global Variables
static char gSeparatorsStr[] = {'\t', ',', 0x00};	// Tab or comma separates one column from the next.


/*	TellFileType(fileMessage, fullFilePath, fileLoaderFlags)

	Shows type of file being loaded in history.
*/
static void
TellFileType(const char* fileMessage, const char* fullFilePath, int fileLoaderFlags)
{
	char temp[MAX_PATH_LEN + 100 + 1];
	
	if ((fileLoaderFlags & FILE_LOADER_QUIET) == 0) {
		sprintf(temp, "%s \"%s\""CR_STR, fileMessage, fullFilePath);
		XOPNotice(temp);
	}
}

/*	FinishWave(ciPtr, fileLoaderFlags)

	Finalizes waves after load.
	
	ciPtr points to the column info for this column.
	
	It returns 0 or an error code.
*/
static int
FinishWave(ColumnInfoPtr ciPtr, int fileLoaderFlags)
{
	waveHndl waveHandle;
	char temp[256];
	CountInt numPoints;
	
	waveHandle = ciPtr->waveHandle;
	WaveHandleModified(waveHandle);
	numPoints = WavePoints(waveHandle);

	if ((fileLoaderFlags & FILE_LOADER_QUIET) == 0) {			// Want message?
		sprintf(temp, "%s loaded, %lld points"CR_STR, ciPtr->waveName, (SInt64)numPoints);
		XOPNotice(temp);
	}
	
	return 0;
}

/*	FindNumberOfRows(fileRef, buffer, bufferLength, rowsInFilePtr)

	Returns via rowsInFilePtr the number of rows in the file, starting
	from the current file position. Restores the file position when done.
	
	A row is considered to consist of 0 or more characters followed by a
	CR or LF or CRLF except that the last row does not need to have any of
	these.
	
	Returns 0 or error code.
*/
static int
FindNumberOfRows(XOP_FILE_REF fileRef, char* buffer, int bufferLength, CountInt* rowsInFilePtr)
{
	SInt64 origFPos;
	int err;
	
	*rowsInFilePtr = 0;

	if (err = XOPGetFilePosition2(fileRef, &origFPos))
		return err;
	
	while (1) {
		if (err = XOPReadLine(fileRef, buffer, bufferLength, NULL)) {
			if (XOPAtEndOfFile(fileRef))
				err = 0;
			break;
		}
		*rowsInFilePtr += 1;
	}
	
	XOPSetFilePosition2(fileRef, origFPos);			// Restore original file position.
	return err;
}

/*	FindNumberOfColumns(fileRef, buffer, bufferLength, numColumnsPtr)

	Returns via numColumnsPtr the number of columns in the file, starting
	from the current file position. Restores the file position when done.
	
	A column is considered to consist of 0 or more characters followed by a
	tab, comma, CR, LF or CRLF.
	
	Returns 0 or error code.
*/
static int
FindNumberOfColumns(XOP_FILE_REF fileRef, char* buffer, int bufferLength, int* numColumnsPtr)
{
	SInt64 origFPos;
	char* bufPtr;
	char ch;
	int err;
	
	*numColumnsPtr = 1;
	
	if (err = XOPGetFilePosition2(fileRef, &origFPos))
		return err;

	if (err = XOPReadLine(fileRef, buffer, bufferLength, NULL))
		return err;
	
	bufPtr = buffer;
	while(1) {
		ch = *bufPtr++;
		if (ch == 0)
			break;
		if (strchr(gSeparatorsStr, ch))				// Tab or comma separates one column from the next.
			*numColumnsPtr += 1;
	}

	XOPSetFilePosition2(fileRef, origFPos);			// Restore original file position.
	
	return 0;
}

/*	ReadASCIINumber(bufPtrPtr, doublePtr)

	Reads an ASCII number from the buffer.
	
	bufPtrPtr is a pointer to a variable that points to the next character
	to be read in the current line. The line is null-terminated but does
	not contain CR or LF.
	
	ReadASCIINumber returns the value via the pointer doublePtr.
	It also advances *bufPtrPtr past the number just read.
	
	It returns the 0 if everything was OK or a non-zero error code.
*/
static int
ReadASCIINumber(char** bufPtrPtr, double *doublePtr)
{
	char *bufPtr;
	double d;
	int hitColumnEnd;
	int ch;
	
	bufPtr = *bufPtrPtr;					// Points to next character to read.
	
	// See if we have the end of the column before number.
	hitColumnEnd = 0;
	while (1) {
		ch = *bufPtr;
		if (ch == ' ') {					// Skip leading spaces.
			bufPtr += 1;
			continue;
		}
		if (ch==0 || strchr(gSeparatorsStr, ch))
			hitColumnEnd = 1;
		break;								// We've found the first non-space character.
	}

	if (hitColumnEnd || sscanf(bufPtr, " %lf", &d) <= 0)
		*doublePtr = DOUBLE_NAN;
	else
		*doublePtr = d;

	// Now figure out how many characters were really needed for number.
	while (1) {								// Search for tab or comma or null-terminator.
		ch = *bufPtr;
		if (ch == 0)
			break;
		bufPtr += 1;
		if (strchr(gSeparatorsStr, ch))
			break;
	}
	
	*bufPtrPtr = bufPtr;					// Points to next character to be read.
	return 0;		
}

/*	LoadSimpleData(fileRef, buffer, bufferLength, fileLoaderFlags, caPtr, numRows, numColumns)

	Loads simple tab or comma delimited data into waves.
	caPtr points to locked array of column info (see SimpleLoadWave.h).
	numColumns is number of columns in file.
	
	File mark is assumed pointing to start of first row of data.
*/
static int
LoadSimpleData(
	XOP_FILE_REF fileRef,
	char* buffer, int bufferLength,
	int fileLoaderFlags, ColumnInfoPtr caPtr,
	CountInt numRows, CountInt numColumns)
{
	ColumnInfoPtr ciPtr;
	int column;
	IndexInt row;
	double doubleValue;
	int isDouble;
	char* bufPtr;				// Points to the next character in the current line of text.
	int err;
	
	isDouble = fileLoaderFlags & FILE_LOADER_DOUBLE_PRECISION;

	for (row = 0; row < numRows; row++) {
		if (err = XOPReadLine(fileRef, buffer, bufferLength, NULL)) {
			if (XOPAtEndOfFile(fileRef))
				err = 0;
			break;
		}
		bufPtr = buffer;
		for (column = 0; column < numColumns; column++) {
			ciPtr = caPtr + column;

			if (err = ReadASCIINumber(&bufPtr, &doubleValue))
				break;

			// Store the data.
			if (isDouble)
				((double*)ciPtr->waveData)[row] = doubleValue;	
			else
				((float*)ciPtr->waveData)[row] = (float)doubleValue;
		}
		if (err)
			break;
		SpinCursor();
	}
	
	return err;
}

/*	GetWaveNames(...)

	Stores a wave name for each column in the corresponding record of the
	array pointed to by caPtr.
	
	If the file appears to have a row of wave names and if the user has
	specified reading names from the file, then the names are read from
	the file. Otherwise, the names are generated from the base name.

	The file names are assumed to be on the first line, if they are present at all.
	
	This routine reads the names row, if there is one, leaving the file marker at
	the start of the numeric data. If there is no names row, it does not read
	anything from the file, so that the file marker will still be at the start
	of the numeric data.
	
	Returns 0 if OK or a non-zero error code.
*/
static int
GetWaveNames(
	XOP_FILE_REF fileRef,
	ColumnInfoPtr caPtr,
	char* buffer, int bufferLength,
	int numColumns,
	int fileLoaderFlags,
	int fileHasNamesRow,
	const char* baseName)
{
	ColumnInfoPtr ciPtr;
	int getNamesFromNamesRow;
	char* bufPtr;
	char ch;
	int nameSuffix;
	int column;
	int i;
	int err;
	
	getNamesFromNamesRow = fileHasNamesRow && (fileLoaderFlags & SIMPLE_READ_NAMES);
	nameSuffix = -1;
	
	err = 0;
	if (fileHasNamesRow) {
		if (err = XOPReadLine(fileRef, buffer, bufferLength, NULL))
			return err;
	}
	else {
		*buffer = 0;
	}
	bufPtr = buffer;

	for (column = 0; column < numColumns; column++) {
		ciPtr = caPtr + column;
		if (getNamesFromNamesRow) {
			for (i = 0; i < 256; i++) {					// Column name assumed never longer than 256.
				ch = *bufPtr++;
				if (ch == 0)
					break;
				if (strchr(gSeparatorsStr, ch))			// Tab or comma separates one column from the next.
					break;
				if (i < MAX_OBJ_NAME)
					ciPtr->waveName[i] = ch;
			}
			if (err)
				break;									// File manager error.
		}
		else {		// Use default names.
			if (fileLoaderFlags & FILE_LOADER_OVERWRITE) {
				sprintf(ciPtr->waveName, "%s%lld", baseName, (SInt64)column);
			}
			else {
				nameSuffix += 1;
				if (err = UniqueName2(MAIN_NAME_SPACE, baseName, ciPtr->waveName, &nameSuffix))
					return err;
			}
		}
	
		SanitizeWaveName(ciPtr->waveName, column);		// Make sure it is a proper wave name.
	}
	
	return err;
}

/*	GetWaveNameList(caPtr, numColumns, waveNamesHandle)

	Stores semicolon-separated list of wave names in waveNamesHandle with
	a null terminating character at the end. This is used to set the
	Igor variable S_waveNames via SetFileLoaderOperationOutputVariables.
*/
static int
GetWaveNameList(ColumnInfoPtr caPtr, CountInt numColumns, Handle waveNamesHandle)
{
	ColumnInfoPtr ciPtr;
	char* p;
	int waveNamesLen;
	int column;
	
	waveNamesLen = 0;
	for (column = 0; column < numColumns; column++) {
		ciPtr = caPtr + column;
		if (ciPtr->waveHandle != NULL)						// We made a wave for this column ?
			waveNamesLen += (int)strlen(ciPtr->waveName)+1;	// +1 for semicolon.
	}
	SetHandleSize(waveNamesHandle, waveNamesLen+1);			// +1 for null char at end.
	if (MemError())
		return NOMEM;

	p = *waveNamesHandle;		// DEREFERENCE -- we must not scramble heap.
	for (column = 0; column < numColumns; column++) {
		ciPtr = caPtr + column;
		if (ciPtr->waveHandle != NULL) {					// We made a wave for this column ?
			strcpy(p, ciPtr->waveName);
			p += strlen(ciPtr->waveName);
			*p++ = ';';
		}
	}
	*p = 0;						// Add null char.

	return 0;
}

/*	LoadSimple(fileRef, baseName, fileLoaderFlags, wavesLoadedPtr, waveNamesHandle)
	
	Loads a simple tab-delimited TEXT file and returns error code.
	fileRef is the file reference for the file to be loaded.
	baseName is base name for new waves.
	fileLoaderFlags contains bit flags corresonding to flags the user
	supplied to the XOP.
	
	Sets *wavesLoadedPtr to the number of waves succesfully loaded.
	Stores in waveNamesHandle a semicolon-separated list of wave names
	with a null character at the end.
	*wavesLoadedPtr is initially a handle with 0 bytes in it.
*/
static int
LoadSimple(
	XOP_FILE_REF fileRef,
	const char* baseName,
	int fileLoaderFlags,
	int* wavesLoadedPtr,
	Handle waveNamesHandle)
{
	CountInt numRows;
	int numColumns;
	int column;									// Current column.
	int fileHasNamesRow;						// Truth file has column names.
	ColumnInfoPtr caPtr;						// Pointer to an array of records, one for each column. See SimpleLoadWave.h.
	ColumnInfoPtr ciPtr;						// Pointer to the record for a specific column.
	char* buffer;
	int bufferLength;
	int err;
	
	caPtr = NULL;
	bufferLength = 20000;			// This is an arbitrary choice of maximum line length.
	buffer = (char*)NewPtr(bufferLength);
	if (buffer == NULL)
		return NOMEM;
	
	*wavesLoadedPtr = 0;

	// Find number of columns of data.
	if (err = FindNumberOfColumns(fileRef, buffer, bufferLength, &numColumns))
		goto done;

	// Determine if file has column names.
	if (err = XOPReadLine(fileRef, buffer, bufferLength, NULL))
		goto done;
	// File assumed to have column names if first char is alphabetic - a very simple-minded test.
	fileHasNamesRow = isalpha(*buffer);

	XOPSetFilePosition(fileRef, 0, -1);						// Go back to start of first row.
		
	// Make data structure used to input columns.
	caPtr = (ColumnInfoPtr)NewPtr(numColumns*sizeof(ColumnInfo));
	if (caPtr == NULL) {
		err = NOMEM;
		goto done;
	}
	MemClear((char *)caPtr, numColumns*sizeof(ColumnInfo));	
	
	err = GetWaveNames(fileRef, caPtr, buffer, bufferLength, numColumns, fileLoaderFlags, fileHasNamesRow, baseName);
	if (err)												// Error reading names ?
		goto done;
	
	// GetWaveNames leaves the file marker pointing to the numeric data.

	// Find number of rows in the file.
	if (err = FindNumberOfRows(fileRef, buffer, bufferLength, &numRows))
		goto done;
	if (numRows < 1) {
		err = NO_DATA_FOUND;
		goto done;
	}

	// Make wave for each column.
	for (column = 0; column < numColumns; column++) {
		ciPtr = caPtr + column;
		ciPtr->waveAlreadyExisted = FetchWave(ciPtr->waveName) != NULL;
		err = FileLoaderMakeWave(column, ciPtr->waveName, numRows, fileLoaderFlags, &ciPtr->waveHandle);
		if (err)
			break;		// NOTE: In this case, not all fields in caPtr are set.
	}
	
	if (err == 0) {
		// Lock wave handles and get pointers to wave data.
		for (column = 0; column < numColumns; column++) {
			ciPtr = caPtr + column;
			ciPtr->waveData = WaveData(ciPtr->waveHandle);		// ciPtr->waveData is now valid because the wave is locked.
		}
	
		// Load data.
		err = LoadSimpleData(fileRef, buffer, bufferLength, fileLoaderFlags, caPtr, numRows, numColumns);
	}
	
	if (err==0) {
		// Clean up waves.
		for (column = 0; column < numColumns; column++)
			FinishWave(caPtr + column, fileLoaderFlags);
		*wavesLoadedPtr = numColumns;
	}
	else {
		// Error occurred - kill any waves that we created.
		for (column = 0; column < numColumns; column++) {
			ciPtr = caPtr + column;
			if (ciPtr->waveHandle && !ciPtr->waveAlreadyExisted) {	// We made a wave for this column ?
				KillWave(ciPtr->waveHandle);
				ciPtr->waveHandle = NULL;
			}
		}
	}
			
	// Get semicolon-separated list of wave names used below for SetFileLoaderOperationOutputVariables.
	if (err == 0)
		err = GetWaveNameList(caPtr, numColumns, waveNamesHandle);

done:
	if (buffer != NULL)
		DisposePtr(buffer);
	if (caPtr != NULL)
		DisposePtr((Ptr)caPtr);
	
	return err;
}

/*	GetLoadFile(initialDir, fullFilePath)

	GetLoadFile puts up a standard open dialog to get the name of the file
	that the user wants to open.
	
	initialDir is a full path to the directory to initially display in the
	dialog or "" if you don't care.
	
	It returns -1 if the user cancels or 0 if the user clicks Open.
	If the user clicks Open, returns the full path via fullFilePath.
*/
static int
GetLoadFile(const char* initialDir, char* fullFilePath)
{
	#ifdef MACIGOR
		const char* filterStr = "Data Files:TEXT:.dat;All Files:****:;";
	#endif
	#ifdef WINIGOR
		const char* filterStr = "Data Files (*.dat)\0*.dat\0All Files (*.*)\0*.*\0\0";	
	#endif
	char prompt[80];
	int result;
	
	static int fileIndex = 2;		// This preserves the setting across invocations of the dialog. A more sophisticated XOP would save this as a preference.

	*fullFilePath = 0;				// Must be preset to zero because on Windows XOPOpenFileDialog passes this to GetOpenFileName which requires that it be preset.
	
	strcpy(prompt, "Looking for a tab or comma-delimited file");
	result = XOPOpenFileDialog(prompt, filterStr, &fileIndex, initialDir, fullFilePath);

	return result;
}

/*	LoadWave(calledFromFunction, flags, baseName, symbolicPathName, fileParam)

	LoadWave loads data from the tab-delimited file specified by symbolicPathName and fileParam.
	
	calledFromFunction is 1 if we were called from a user function, zero otherwise.
	
	flags contains bits set based on the command line flags that were
	used when the command was invoked from the Igor command line or from a macro.
	
	baseName is the base name to use for new waves if autonaming is enabled by fileLoaderFlags.
	
	It returns 0 if everything goes OK or an error code if not.
*/
int
LoadWave(int calledFromFunction, int fileLoaderFlags, const char* baseName, const char* symbolicPathName, const char* fileParam)
{
	char symbolicPathPath[MAX_PATH_LEN+1];		// Full path to the folder that the symbolic path refers to. This is a native path with trailing colon (Mac) or backslash (Win).
	char nativeFilePath[MAX_PATH_LEN+1];		// Full path to the file to load. Native path.
	Handle waveNamesHandle;
	int wavesLoaded;
	XOP_FILE_REF fileRef;
	int err;

	*symbolicPathPath = 0;
	if (*symbolicPathName != 0) {
		if (err = GetPathInfo2(symbolicPathName, symbolicPathPath))
			return err;
	}
	
	if (GetFullPathFromSymbolicPathAndFilePath(symbolicPathName, fileParam, nativeFilePath) != 0)
		fileLoaderFlags |= FILE_LOADER_INTERACTIVE;		// Need dialog to get file name.
		
	if (!FullPathPointsToFile(nativeFilePath))			// File does not exist or path is bad?
		fileLoaderFlags |= FILE_LOADER_INTERACTIVE;		// Need dialog to get file name.

	if (fileLoaderFlags & FILE_LOADER_INTERACTIVE) {
		if (GetLoadFile(symbolicPathPath, nativeFilePath))
			return -1;							// User cancelled.
	}
	
	TellFileType("Simple tab delimited file load from", nativeFilePath, fileLoaderFlags);
	
	if (err = SetFileLoaderOperationOutputVariables(calledFromFunction, nativeFilePath, 0, ""))	// Initialize Igor output variables.
		return err;
	
	// Open file.
	if (err = XOPOpenFile(nativeFilePath, 0, &fileRef))
		return err;
	
	waveNamesHandle = NewHandle(0L);	// Will contain semicolon-separated list of wave names.
	WatchCursor();
	err = LoadSimple(fileRef, baseName, fileLoaderFlags, &wavesLoaded, waveNamesHandle);
	XOPCloseFile(fileRef);
	ArrowCursor();
	
	// Store standard file loader output globals.
	if (err == 0)
		err = SetFileLoaderOperationOutputVariables(calledFromFunction, nativeFilePath, wavesLoaded, *waveNamesHandle);

	DisposeHandle(waveNamesHandle);
	
	return err;
}

/*	XOPMenuItem()

	XOPMenuItem is called when the XOP's menu item is selected.
*/
static int
XOPMenuItem(void)
{
	char filePath[MAX_PATH_LEN+1];				// Room for full file specification.
	int fileLoaderFlags;
	int err;

	if (GetLoadFile("", filePath))
		return 0;								// User cancelled.
	
	fileLoaderFlags = SIMPLE_READ_NAMES | FILE_LOADER_DOUBLE_PRECISION;
	err = LoadWave(0, fileLoaderFlags, "wave", "", filePath);
	return err;
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP when the message specified by the
	host is other than INIT.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;
	
	switch (GetXOPMessage()) {
		case CLEANUP:								// XOP about to be disposed of.
			break;

		case MENUITEM:								// XOPs menu item selected.
			result = XOPMenuItem();
			SetXOPType(TRANSIENT);					// XOP is done now.
			break;
	}
	
	SetXOPResult(result);
}

/*	XOPMain()

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.
	
	XOPMain does any necessary initialization and then sets the XOPEntry field of the
	XOPRecHandle to the address to be called for future messages.
*/
HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)		// The use of XOPMain rather than main means this XOP requires Igor Pro 6.20 or later
{
	int err;
	
	XOPInit(ioRecHandle);				// Do standard XOP initialization.
	SetXOPEntry(XOPEntry);				// Set entry point for future calls.

	if (igorVersion < 620) {			// Requires Igor Pro 6.20 or later.
		SetXOPResult(OLD_IGOR);			// OLD_IGOR is defined in SimpleLoadWave.h and there are corresponding error strings in SimpleLoadWave.r and SimpleLoadWaveWinCustom.rc.
		return EXIT_FAILURE;
	}
	
	if (err = RegisterSimpleLoadWave()) {
		SetXOPResult(err);
		return EXIT_FAILURE;
	}

	SetXOPResult(0);
	return EXIT_SUCCESS;
}
