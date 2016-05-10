/*	GBLoadWave.c
	
	An Igor external operation, GBLoadWave, that imports data from various binary files.
	
	See the file GBLoadWave Help for usage of GBLoadWave XOP.
	
	HR, 10/15/93
		Added code to set S_waveNames output string.
		
	HR, 1/8/96 (Update for Igor Pro 3.0)
		Allowed zero and one point waves.
		Set up to require Igor Pro 3.0 or later.
	
	HR, 2/17/96
		Prior to version 3.0, there was one flag (/L) that specified the length of
		the input data and another flag (/F) that specified the format. Now, there
		is an additional flag (/T) that specifies both at once. For backward compatibility,
		the /L and /F flags are still supported.
		
		The program uses two different ways of specifying the input data type:
			inputBytesPerPoint and inputDataFormat		/L and /F flags
		or
			inputDataType								/T flag
		
		inputBytesPerPoint is 1, 2, 4 or 8
		inputDataFormat is IEEE_FLOAT, SIGNED_INT or UNSIGNED_INT
		
		inputDataType is one of the standard Igor data types:
			NT_FP64, NT_FP32
			NT_I32, NT_I16, NT_I8
			NT_I32 | NT_UNSIGNED, NT_I16 | NT_UNSIGNED, NT_I8 | NT_UNSIGNED
		
		The NumBytesAndFormatToNumType routine converts from the first representation
		to the second representation and the NumTypeToNumBytesAndFormat routine
		converts from the second to the first.
		
		The same is true for the output data type (the type of the waves that are created).
		The type is represented in these same two ways and the same routines are used
		to convert between them.
	
	HR, 2/20/96
		Fixed a bug in version 1.20. The interleave feature worked only when the
		waves created were 4 or 8 bytes per point.
		
	The requirement that we run with Igor Pro 3.0 or later comes from two reasons.
	First, we use CreateValidDataObjectName which requires Igor Pro 3.0. Second,
	we allow zero and one point waves, which Igor Pro 2.0 does not allow.
	
	HR, 10/96
		Converted to work on Windows as well as Macintosh.
		Changed to save settings in the IGOR preferences file rather than in
		the XOP's resource fork. This requires IGOR Pro 3.10 or later.
	
	HR, 980331
		Made changes to improve platform independence.
	
	HR, 980401
		Added a new syntax for the /I flag that allows the user to provide filters
		for both Macintosh and Windows.
	
	HR, 980403
		Changed to use standard C file I/O for platform independence.
	
	HR, 010528
		Started port to Carbon.
		
	HR, 020916
		Revamped for Igor Pro 5 using Operation Handler.
		
	HR, 060127
		Tweaked for Intel Macintosh.

	HR, 080820
		Support for very big files.
		
	HR, 091001
		Modify for XOP Toolkit 6 by removing handle locking.
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"		// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "GBLoadWave.h"

// Global Variables (none).

/*	MakeAWave(waveName, flags, waveHandlePtr, points)

	MakeAWave makes a wave with specified number of points.
	It returns 0 or an error code.
	It returns a handle to the wave via waveHandlePtr.
	It returns the number of points in the wave via pointsPtr.
*/
static int
MakeAWave(char *waveName, int flags, waveHndl *waveHandlePtr, CountInt points, int dataType)
{	
	int type, overwrite;
	CountInt dimensionSizes[MAX_DIMENSIONS+1];
	
	type = dataType;
	overwrite = flags & OVERWRITE;
	MemClear(dimensionSizes, sizeof(dimensionSizes));
	dimensionSizes[0] = points;
	return MDMakeWave(waveHandlePtr, waveName, NIL, dimensionSizes, type, overwrite);
}

/*	AddWaveNameToHandle(waveName, waveNamesHandle)
	
	This routine accumulates wave names in a handle for later use in setting
	the S_waveNames file-loader variable in Igor.
	
	waveNamesHandle contains a C (null terminated) string to which we append
	the current name.
*/
static int
AddWaveNameToHandle(char* waveName, Handle waveNamesHandle)
{
	int len;
	char temp[256];
	
	len = (int)strlen(waveName);
	SetHandleSize(waveNamesHandle, GetHandleSize(waveNamesHandle) + len + 1);
	if (MemError())
		return NOMEM;

	sprintf(temp, "%s;", waveName);
	strcat(*waveNamesHandle, temp);
	
	return 0;
}

/*	FinishWave(wavH, point, points, flags, message)

	FinishWave trims unneeded points from the end of the specified wave.
	point is the number of the first unused point in the wave.
	points is the total number of points in the wave.
	flags is the flags passed to the MakeWave operation.
	message is NIL for no message at all, "" for standard message of "whatever" for custom message.
	It returns 0 or an error code.
*/
static int
FinishWave(waveHndl wavH, IndexInt point, CountInt points, int flags, const char *message)
{
	char buffer[80];
	char waveName[MAX_OBJ_NAME+1];
	int result;
	
	WaveName(wavH, waveName);					// Get wave name.
	
	/*	HR, 1/8/96
		Removed test requiring at least two points. In Igor Pro 3.0, waves
		can contain any number of points, including zero.
	*/
	
	/*	Cut back unused points at end of wave.
		This is not used in GBLoadWave. It is useful when loading text
		files which may have blank values at the end of a column.
	*/
	if (points-point) {							// Are there extra points at end of wave ?
		if (result = ChangeWave(wavH, point, WaveType(wavH)))
			return result;
	}
	
	sprintf(buffer, "%s loaded, %lld points\015", waveName, (SInt64)point);
	WaveHandleModified(wavH);

	if (message && (flags & QUIET) == 0) {		// Want message ?
		if (*message)							// Want custom message ?
			strcpy(buffer, message);
		XOPNotice(buffer);
	}
	return 0;
}

/*	TellFileType(fileMessage, fileName, flags, totalBytes)

	Shows type of file being loaded in history.
*/
static void
TellFileType(char *fileMessage, char *fileName, int flags, CountInt totalBytes)
{
	char temp[MAX_PATH_LEN + 100];
	
	if ((flags & QUIET) == 0) {
		sprintf(temp, "%s \"%s\" (%lld total bytes)"CR_STR, fileMessage, fileName, (SInt64)totalBytes);
		XOPNotice(temp);
	}
}

#define MAX_MESSAGE 80
static void
AnnounceWaves(ColumnInfoPtr caPtr, int numColumns)
{
	ColumnInfoPtr ciPtr;
	char temp[MAX_MESSAGE+1+1];		// Room for message plus null plus CR.
	int column;

	sprintf(temp, "Data length: %lld, waves: ", (SInt64)caPtr->points);
	ciPtr = caPtr;
	column = 0;
	do {
		while (strlen(temp) + strlen(ciPtr->waveName) < MAX_MESSAGE) {
			strcat(temp, ciPtr->waveName);
			strcat(temp, ", ");
			column += 1;
			ciPtr += 1;
			if (column >= numColumns)
				break;
		}
		temp[strlen(temp)-2] = 0;			// Remove last ", ".
		strcat(temp, CR_STR);
		XOPNotice(temp);
		strcpy(temp, "and: ");				// Get ready for next line.
	} while (column < numColumns);
}

/*	SwapVAXShorts(dataPtr, numBytePairs)

	In VAX data, the low byte of each word is stored before the high
	byte. This is reverse of Motorola. However, the short words are
	in the same order as Motorola. Thus, a 4 byte word is stored as:
			Motorola						VAX
		High byte of high word		Low byte of high word
		Low byte of high word		High byte of high word
		High byte of low word		Low byte of low word
		Low byte of low word		High byte of low word
	
	Note that this is not swapped in the same sense as Motorala <-> Intel
	which would be:
			Motorola						Intel
		High byte of high word		Low byte of low word
		Low byte of high word		High byte of low word
		High byte of low word		Low byte of high word
		Low byte of low word		High byte of high word
	
	The "Swap Bytes" checkbox in the GBLoadWave dialog does
	Motorala <-> Intel swapping, not Motorala <-> VAX swapping.

	Motorala <-> VAX swapping is done by this routine automatically
	if the input data is specified as VAX. The GBLoadWave dialog
	"Byte Order" popup menu is ignored.
	
	This routine does the necessary swapping.
*/
static void
SwapVAXShorts(unsigned char* dataPtr, CountInt numBytePairs)
{
	unsigned char temp;

	while(numBytePairs>0) {
		temp = *dataPtr;
		*dataPtr = *(dataPtr+1);
		dataPtr += 1;
		*dataPtr = temp;
		dataPtr += 1;
		numBytePairs -= 1;
	}
}

static void
ConvertVAX32ToIEEE32(unsigned char* dataPtr, CountInt numPoints)
{
	unsigned char* dataStartPtr;
	unsigned char exponent;
	unsigned char highByte, lowByte;
	CountInt pointsLeft;
	
	dataStartPtr = dataPtr;
	
	SwapVAXShorts(dataPtr, numPoints*2);
	
	pointsLeft = numPoints;
	while(pointsLeft>0) {
		if (*(SInt32*)dataPtr != 0) {		// 0 is the same in VAX and IEEE.
			// NOTE: This swaps the bytes of the first word.
			highByte = dataPtr[0];
			lowByte = dataPtr[1];
			exponent = ((highByte & 0x7F)<<1) | ((lowByte & 0x80)>>7);	// Extract exponent which spans two bytes.
			exponent -= 2;												// Convert from VAX to IEEE.
			dataPtr[0] = (highByte & 0x80) | ((exponent>>1)&0x7F);
			dataPtr[1] = (lowByte & 0x7F) | ((exponent&0x01)<<7);
		}
		dataPtr += 4;
		pointsLeft -= 1;
	}
	
	/*	HR, 990412, 1.43
		Previously we did not convert VAX correctly on PC. The technique now is to
		treat the data identically on both platforms except for this last step of
		reversing the byte order on PC.
		
		HR, 040121, 1.51:
		This fix was omitted from 1.50, the version of GBLoadWave that shipped with Igor
		Pro 5. It was added back for GBLoadWave 1.51/Igor Pro 5.01.
	*/
	#ifdef XOP_LITTLE_ENDIAN
		FixByteOrder(dataStartPtr, 4, numPoints);
	#endif
}

static void
ConvertVAX64ToIEEE64(unsigned char* dataPtr, CountInt numPoints)
{
	unsigned char* dataStartPtr;
	unsigned short exponent;
	unsigned char highByte, lowByte;
	CountInt pointsLeft;
	
	dataStartPtr = dataPtr;

	SwapVAXShorts(dataPtr, numPoints*4);
	
	pointsLeft = numPoints;
	while(pointsLeft>0) {
		if (*(SInt32*)dataPtr!=0 || *(SInt32*)(dataPtr+4)!=0) {			// 0 is the same in VAX and IEEE.
			highByte = dataPtr[0];
			lowByte = dataPtr[1];
			exponent = ((highByte & 0x7F)<<4) | ((lowByte & 0xF0)>>4);	// Extract exponent which spans two bytes.
			exponent -= 2;												// Convert from VAX to IEEE.
			dataPtr[0] = (highByte & 0x80) | ((exponent>>4)&0x7F);
			dataPtr[1] = (lowByte & 0x0F) | ((exponent&0x0F)<<4);
		}
		dataPtr += 8;
		pointsLeft -= 1;
	}
	
	/*	HR, 990412, 1.43
		Previously we did not convert VAX correctly on PC. The technique now is to
		treat the data identically on both platforms except for this last step of
		reversing the byte order on PC.	
		
		HR, 040121, 1.51:
		This fix was omitted from 1.50, the version of GBLoadWave that shipped with Igor
		Pro 5. It was added back for GBLoadWave 1.51/Igor Pro 5.01.
	*/
	#ifdef XOP_LITTLE_ENDIAN
		FixByteOrder(dataStartPtr, 8, numPoints);
	#endif
}

/*	ConvertFloatingPointFormats(dataPtr, inputDataType, numPoints)
	
	Does VAX -> IEEE conversion.
*/
static void
ConvertFloatingPointFormats(unsigned char* dataPtr, int inputDataType, CountInt numPoints)
{
	switch(inputDataType) {
		case NT_FP32:
			ConvertVAX32ToIEEE32(dataPtr, numPoints);
			break;
		case NT_FP64:
			ConvertVAX64ToIEEE64(dataPtr, numPoints);
			break;
	}				
}

/*	UntangleWaves(caPtr, numArrays, arrayPoints, bytesInPoint, interleaved)

	Untangles data if data in file was interleaved.
	It needs to allocate a temporary buffer to contain ALL of the loaded data.
	Returns NOMEM if it can't get enough memory.
	
	caPtr points to locked array of column info (see .h file).
	numArrays is the number of waves created.
	arrayPoints is the number of points in each wave.
	bytesInPoint is the number of bytes in a point of the wave.
	interleaved is the truth that the arrays in the file were interleaved.	
*/
static int
UntangleWaves(ColumnInfoPtr caPtr, int numArrays, CountInt arrayPoints, int bytesInPoint, int interleaved)
{
	ColumnInfoPtr ciPtr;
	CountInt point, skipBytes;
	IndexInt column;
	SInt64 totalBytes64;
	BCInt bytesInWave, totalBytes;
	Ptr buffer, srcPtr, destPtr;

	if (!interleaved || numArrays == 1)
		return 0;

	totalBytes64 = (SInt64)numArrays * arrayPoints * bytesInPoint;
	#ifdef IGOR32
		if (totalBytes64 > 4294967295LL)	// > 4GB?
			return NOMEM;					// malloc is limited to 4GB
	#endif
	
	bytesInWave = (BCInt)(arrayPoints * bytesInPoint);
	totalBytes = numArrays * bytesInWave;
	
	buffer = (Ptr)malloc(totalBytes);			// HR, 080820, 1.62: Changed from NewPtr to malloc to support allocations larger than 2 GB.
	if (buffer == NIL)
		return NOMEM;

	for (column = 0; column < numArrays; column++) {
		ciPtr = caPtr + column;
		ciPtr->dataPtr = WaveData(ciPtr->waveHandle);
		memcpy(buffer + (BCInt)(column*bytesInWave), (char *)ciPtr->dataPtr, bytesInWave);
	}
	
	skipBytes = bytesInPoint*numArrays;
	for (column = 0; column < numArrays; column++) {
		ciPtr = caPtr + column;
		destPtr = (char *)ciPtr->dataPtr;
		srcPtr = buffer + column*bytesInPoint;

		switch (bytesInPoint) {
			case 8:
				for (point = 0; point < arrayPoints; point++) {
					*(double *)destPtr = *(double *)srcPtr;
					srcPtr += skipBytes;
					destPtr += 8;
				}
				break;
			
			case 4:
				for (point = 0; point < arrayPoints; point++) {
					*(SInt32 *)destPtr = *(SInt32 *)srcPtr;
					srcPtr += skipBytes;
					destPtr += 4;
				}
				break;
			
			case 2:			// HR, 2/20/96: This was missing in version 1.20 of GBLoadWave.
				for (point = 0; point < arrayPoints; point++) {
					*(short *)destPtr = *(short *)srcPtr;
					srcPtr += skipBytes;
					destPtr += 2;
				}
				break;
			
			case 1:			// HR, 2/20/96: This was missing in version 1.20 of GBLoadWave.
				for (point = 0; point < arrayPoints; point++) {
					*(char *)destPtr = *(char *)srcPtr;
					srcPtr += skipBytes;
					destPtr += 1;
				}
				break;
		}
	}
	
	free(buffer);
	
	return 0;
}

/*	ScaleWaves(...)

	Applies the offset and scaling specified by the /Y={offset, multiplier} flags, if any.
*/
static void
ScaleWaves(ColumnInfoPtr caPtr, int numArrays, CountInt numPoints, int dataType, double* offsetPtr, double* multiplierPtr)
{
	ColumnInfoPtr ciPtr;
	void *dataPtr;
	int column;

	if (*offsetPtr==0.0 && *multiplierPtr==1.0)
		return;
	
	for (column = 0; column < numArrays; column++) {
		ciPtr = caPtr + column;
		dataPtr = WaveData(ciPtr->waveHandle);
		ScaleData(dataType, dataPtr, offsetPtr, multiplierPtr, numPoints);
	}
}

/*	LoadData(fileRef, lsp, caPtr, numColumns, waveNamesHandle)

	Loads data into waves.
	lsp is a pointer to a structure containing settings for the LoadWave operation.
	caPtr points to locked array of column info (see .h file).
	numColumns is number of "columns" (arrays of data to be loaded into waves) in file.
	The names of waves loaded are stored in waveNamesHandle for use in creating the S_waveNames output string.
*/
static int
LoadData(XOP_FILE_REF fileRef, LoadSettingsPtr lsp, ColumnInfoPtr caPtr, int numColumns, Handle waveNamesHandle)
{
	ColumnInfoPtr ciPtr;
	int column;
	CountInt numPoints;
	int isVAXFloatingPoint;
	char* buffer;
	char* readDataPtr;
	int result, err;
	
	numPoints = caPtr->points;
	
	isVAXFloatingPoint = lsp->floatingPointFormat==2 && (lsp->inputDataType==NT_FP32 || lsp->inputDataType==NT_FP64);

	/*	Here we decide if we can load the data directly into the waves
		or if we need to create an intermediate buffer. We need the buffer
		if the size of the data exceeds the size of the wave.
	*/
	buffer = NIL;
	if (lsp->inputBytesPerPoint > lsp->outputBytesPerPoint) {	// Can't read directly into wave?
		// HR, 080820, 1.62: Changed from NewPtr to malloc to support allocations larger than 2 GB.
		buffer = (char*)malloc(numPoints*lsp->inputBytesPerPoint);		// Get temporary buffer.
		if (buffer == NIL)
			return NOMEM;
	}
	
	for (column = 0; column < numColumns; column++) {
		ciPtr = caPtr + column;
		ciPtr->dataPtr = WaveData(ciPtr->waveHandle);
		if (buffer==NIL)									// Reading directly into wave.
			readDataPtr = (char*)ciPtr->dataPtr;
		else												// Reading into buffer.
			readDataPtr = buffer;
		{
			SInt64 byteCount;
			byteCount = lsp->inputBytesPerPoint*numPoints;
			result = XOPReadFile64(fileRef, byteCount, readDataPtr, NULL);
		}
		
		if (result==0) {
			// See if we need to reverse bytes.
			if (!isVAXFloatingPoint) {			// HR, 990412, 1.43: Skip normal byte swapping for VAX.
				#ifdef XOP_LITTLE_ENDIAN
					if (lsp->lowByteFirst == 0)
						FixByteOrder(readDataPtr, lsp->inputBytesPerPoint, numPoints);
				#else
					if (lsp->lowByteFirst != 0)
						FixByteOrder(readDataPtr, lsp->inputBytesPerPoint, numPoints);
				#endif
			}

			// See if we need to convert VAX floating point into IEEE.
			if (isVAXFloatingPoint)
				ConvertFloatingPointFormats((unsigned char*)readDataPtr, lsp->inputDataType, numPoints);

			// Convert from the input data type to the wave data type.
			ConvertData(readDataPtr, ciPtr->dataPtr, numPoints, lsp->inputBytesPerPoint, lsp->inputDataFormat, lsp->outputBytesPerPoint, lsp->outputDataFormat);
		}
		if (result)
			break;
	}
	
	if (buffer != NIL)
		free(buffer);

	if (XOPAtEndOfFile(fileRef))
		result = 0;
	
	// Clean up waves.
	for (column = 0; column < numColumns; column++) {
		ciPtr = caPtr + column;
		if (ciPtr->waveHandle != NIL) {
			AddWaveNameToHandle(ciPtr->waveName, waveNamesHandle);		// Keep track of waves loaded so that we can set S_waveNames.
			if (err = FinishWave(ciPtr->waveHandle, numPoints, numPoints, lsp->flags | QUIET, "")) {
				if (result == 0)
					result = err;
			}
		}
	}
	
	// See if we need to sort out the data because the data in the file was interleaved.
	if (result == 0)
		result = UntangleWaves(caPtr, lsp->numArrays, lsp->arrayPoints, lsp->outputBytesPerPoint, lsp->interleaved);
	
	// Do offset and scaling specified by /Y={offset, multiplier}, if necessary.
	if (result == 0)
		ScaleWaves(caPtr, lsp->numArrays, numPoints, lsp->outputDataType, &lsp->offset, &lsp->multiplier);
	
	return result;
}

/*	Load(fileRef, lsp, fileName, baseName, numWavesLoadedPtr, waveNamesHandle)
	
	Loads custom file and returns error code.
	fileRef is the ref number for the file's data fork.
	lsp is a pointer to a structure containing settings for the LoadWave operation.
	baseName is base name for new waves or "" to auto name.
	The names of waves loaded are stored in waveNamesHandle for use in creating the S_waveNames output string.
*/
static int
Load(XOP_FILE_REF fileRef, LoadSettingsPtr lsp, char *fileName, const char *baseName, int* numWavesLoadedPtr, Handle waveNamesHandle)
{
	int column;
	ColumnInfoHandle ciHandle;					// See GBLoadWave.h.
	ColumnInfoPtr ciPtr;
	char base[MAX_OBJ_NAME+2];					// Name if no baseName supplied.
	char temp[128];
	int suffixNum;
	int nameChanged, doOverwrite;
	SInt64 fileBytes;
	int result;
	
	*numWavesLoadedPtr = 0;
	
	// Find the total number of bytes in the file.
	if (result = XOPNumberOfBytesInFile2(fileRef, &fileBytes))
		return result;

	strcpy(temp, "General binary file load from");
	TellFileType(temp, fileName, lsp->flags, (CountInt)fileBytes);
		
	// Figure out number of waves and points/wave.
	fileBytes -= lsp->preambleBytes;
	if (lsp->arrayPoints == 0) {				// # of points in array not specified ?
		if (lsp->numArrays == 0)				// Number of waves not specified ?
			lsp->numArrays = 1;					// Assume one array.
		lsp->arrayPoints = (CountInt)(fileBytes/(lsp->inputBytesPerPoint*lsp->numArrays));
	}
	else {										// # of bytes in array was specified.
		if (lsp->numArrays == 0)				// Number of waves not specified ?
			lsp->numArrays = (int)(fileBytes/(lsp->arrayPoints*lsp->inputBytesPerPoint));	// Calculate number of arrays.
	}
	if (lsp->arrayPoints < 0)					// HR, 1/8/96: Was < 2. Now we allow zero and one point waves.
		return NO_DATA_FOUND;
	if (lsp->numArrays < 1)
		return BAD_NUM_WAVES;
	if (lsp->numArrays*lsp->arrayPoints*lsp->inputBytesPerPoint > fileBytes)
		return NOT_ENOUGH_BYTES;
	
	#ifdef IGOR32
		if (lsp->arrayPoints > 2147483647L)		// IGOR32 is limited to 2GB waves.
			return ARRAY_TOO_BIG_FOR_IGOR;
	#endif
	
	// Go to start of data.
	if (result = XOPSetFilePosition2(fileRef, lsp->preambleBytes))
		return result;

	/*	Make data structure used to keep track of each wave being made.
		Because this structure originated in an XOP that loaded text files,
		we call each wave a "column".
	*/
	ciHandle = (ColumnInfoHandle)NewHandle((BCInt)(lsp->numArrays * sizeof(ColumnInfo)));
	if (ciHandle == NIL)
		return NOMEM;
	MemClear((void *)*ciHandle, GetHandleSize((Handle)ciHandle));	
	
	// Get base name for waves.
	strcpy(base, baseName);
	if (*base == 0)	{							// No baseName supplied with the command ?
		strcpy(base, "wave");
		lsp->flags |= AUTO_NAME;
	}
		
	// Make a wave for each array in the file.
	suffixNum = 0;								// Used by CreateValidDataObjectName to create unique wave names.
	for (column = 0; column < lsp->numArrays; column++) {
		ciPtr = *ciHandle + column;
		ciPtr->points = lsp->arrayPoints;
		
		/*	In GBLoadWave, wave names are always made using a base name (e.g., "wave")
			plus a number to give something like "wave0". In other file loaders, the
			wave name might come from the file being loaded. In that case, we would
			not use a base name and would store the name from the file in ciPtr->waveName.
		*/
		if (*base != 0)			// In GBLoadWave, this will always be true.
			strcpy(ciPtr->waveName, base);

		/*	CreateValidDataObjectName takes care of any illegal or conflicting names.
			CreateValidDataObjectName requires Igor Pro 3.00 or later.
		*/
		result = CreateValidDataObjectName(NULL, ciPtr->waveName, ciPtr->waveName, &suffixNum, WAVE_OBJECT,
											1, lsp->flags & OVERWRITE, *base!=0, 1, &nameChanged, &doOverwrite);
		
		ciPtr->wavePreExisted = doOverwrite;	// Used below in case an error occurs and we want to kill any waves we've made.
		
		if (result == 0)
			result = MakeAWave(ciPtr->waveName, lsp->flags, &ciPtr->waveHandle, lsp->arrayPoints, lsp->outputDataType);
		
		if (result) {							// Couldn't make wave (probably low memory) ?
			int thisColumn;
			thisColumn = column;				// Kill waves that we just made.
			for (column = 0; column < thisColumn; column++) {
				ciPtr = *ciHandle + column;
				if (!ciPtr->wavePreExisted)		// Is this a wave that we just made?
					KillWave(ciPtr->waveHandle);
			}
			DisposeHandle((Handle)ciHandle);
			return result;
		}
	}
	
	// Load data.
	result = LoadData(fileRef, lsp, *ciHandle, lsp->numArrays, waveNamesHandle);
	if (result == 0) {
		*numWavesLoadedPtr = lsp->numArrays;
		
		// Announce waves that were made.
		if ((lsp->flags & QUIET) == 0)
			AnnounceWaves(*ciHandle, lsp->numArrays);
	}
		
	DisposeHandle((Handle)ciHandle);
	
	if (result == 0 && lsp->numArrays == 0)
		result = NO_DATA_FOUND;
	
	return result;
}

/*	LoadFile(fullFilePath, lsp, baseName, calledFromFunction)

	LoadFile loads data from the file specified by fullFilePath.
	See the discussion at the top of this file for the specifics on fullFilePath and wavename.
	
	It returns 0 if everything goes OK or an error code if not.
*/
static int
LoadFile(const char* fullFilePath, LoadSettingsPtr lsp, const char *baseName, int calledFromFunction)
{
	XOP_FILE_REF fileRef;
	int updateSave;
	int numWavesLoaded;				// Number of waves loaded from file.
	Handle waveNamesHandle;
	char fileName[MAX_FILENAME_LEN+1];
	char fileDirPath[MAX_PATH_LEN+1];
	int err;
	
	WatchCursor();
	
	if (err = GetDirectoryAndFileNameFromFullPath(fullFilePath, fileDirPath, fileName))
		return err;
	
	// Initialize file loader global output variables.
	if (err = SetFileLoaderOperationOutputVariables(calledFromFunction, fullFilePath, 0, ""))
		return err;

	// Open file.
	if (err = XOPOpenFile(fullFilePath, 0, &fileRef))
		return err;
		
	numWavesLoaded = 0;
	waveNamesHandle = NewHandle(1L);
	if (waveNamesHandle == NIL)
		return NOMEM;
	**waveNamesHandle = 0;		// We build up a C string in this handle.

	PauseUpdate(&updateSave);						// Don't update windows during this.
	err = Load(fileRef, lsp, fileName, baseName, &numWavesLoaded, waveNamesHandle);
	ResumeUpdate(&updateSave);						// Go back to previous update state.

	XOPCloseFile(fileRef);

	if (err == 0)
		err = SetFileLoaderOperationOutputVariables(calledFromFunction, fullFilePath, numWavesLoaded, *waveNamesHandle);

	DisposeHandle(waveNamesHandle);

	ArrowCursor();

	return err;
}

/*	LoadWave(lsp, baseName, symbolicPathName, fileParam, calledFromFunction)

	LoadWave does the actual work of GBLoadWave.
	
	symbolicPathName may be an empty string.
	
	fileParam may be full path, partial path, just a file name, or an empty string.

	LoadWave returns 0 if everything went alright or an error code if not.
*/
int
LoadWave(LoadSettings* lsp, const char* baseName, const char* symbolicPathName, const char* fileParam, int calledFromFunction)
{
	char symbolicPathPath[MAX_PATH_LEN+1];		// Full path to the folder that the symbolic path refers to. This is a native path with trailing colon (Mac) or backslash (Win).
	char nativeFilePath[MAX_PATH_LEN+1];			// Full path to the file to load. Native path.
	int err;

	*symbolicPathPath = 0;
	if (*symbolicPathName != 0) {
		if (err = GetPathInfo2(symbolicPathName, symbolicPathPath))
			return err;
	}

	if (GetFullPathFromSymbolicPathAndFilePath(symbolicPathName, fileParam, nativeFilePath) != 0)
		lsp->flags |= INTERACTIVE;				// Need dialog to get file name.
		
	if (!FullPathPointsToFile(nativeFilePath))	// File does not exist or path is bad?
		lsp->flags |= INTERACTIVE;				// Need dialog to get file name.

	if (lsp->flags & INTERACTIVE) {
		if (err = XOPOpenFileDialog("Looking for a general binary file", lsp->filterStr, NULL, symbolicPathPath, nativeFilePath))
			return err;
	}

	// We have the parameters, now load the data.
	err = LoadFile(nativeFilePath, lsp, baseName, calledFromFunction);
	
	return err;
}

static int
RegisterOperations(void)		// Register any operations with Igor.
{
	int result;
	
	if (result = RegisterGBLoadWave())
		return result;
	
	// There are no more operations added by this XOP.
		
	return 0;
}

/*	MenuItem()

	MenuItem() is called when XOP's menu item is selected.
	Puts up the Load General Binary dialog.
*/
static int
MenuItem(void)
{
	GBLoadWaveDialog();						// This is in GBLoadWaveDialog.c.
	return 0;
}

/*	XOPEntry()

	This is the the routine that Igor calls to pass all messages other than INIT.

	You must get the message (GetXOPMessage) and get any parameters (GetXOPItem)
	before doing any callbacks to Igor.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;
	
	switch (GetXOPMessage()) {
		case CLEANUP:								// XOP about to be disposed of.
			break;
		
		case MENUITEM:								// XOPs menu item selected.
			result = MenuItem();
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
	int result;
	
	XOPInit(ioRecHandle);				// Do standard XOP initialization.
	SetXOPEntry(XOPEntry);				// Set entry point for future calls.

	if (igorVersion < 620) {
		SetXOPResult(OLD_IGOR);			// OLD_IGOR is defined in GBLoadWave.h and there are corresponding error strings in GBLoadWave.r and GBLoadWaveWinCustom.rc.
		return EXIT_FAILURE;
	}

	if (result = RegisterOperations()) {
		SetXOPResult(result);
		return EXIT_FAILURE;
	}

	SetXOPResult(0L);
	return EXIT_SUCCESS;
}
