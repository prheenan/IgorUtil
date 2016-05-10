#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

static int
NeedToSwapBytes(int lowByteFirst)		// Returns truth data is in the wrong order for this platform.
{
	#ifdef XOP_LITTLE_ENDIAN
		return lowByteFirst == 0;
	#else
		return lowByteFirst != 0;
	#endif
}

// Operation template: VDTReadBinary2 /B /O=number:timeOutSeconds /Q /S=number:strLen /T=string:terminators /TYPE=number:dataType /Y={number:offset, number:multiplier} varName[100]:varNames

// Runtime param structure for VDTReadBinary2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTReadBinary2RuntimeParams {
	// Flag parameters.

	// Parameters for /B flag group.
	int BFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /O flag group.
	int OFlagEncountered;
	double timeOutSeconds;
	int OFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /S flag group.
	int SFlagEncountered;
	double strLen;
	int SFlagParamsSet[1];

	// Parameters for /T flag group.
	int TFlagEncountered;
	Handle terminators;
	int TFlagParamsSet[1];

	// Parameters for /TYPE flag group.
	int TYPEFlagEncountered;
	double dataType;
	int TYPEFlagParamsSet[1];

	// Parameters for /Y flag group.
	int YFlagEncountered;
	double offset;
	double multiplier;
	int YFlagParamsSet[2];

	// Main parameters.

	// Parameters for simple main group #0.
	int varNamesEncountered;
	char varNames[100][MAX_OBJ_NAME+1];		// Optional parameter.
	int varNamesParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTReadBinary2RuntimeParams VDTReadBinary2RuntimeParams;
typedef struct VDTReadBinary2RuntimeParams* VDTReadBinary2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	ReadBinaryBytes(op, timeout, buffer, maxBytes, terminators, numBytesReadPtr)
	
	Reads bytes until:
		maxBytes have been read.
		A terminator has been read if *terminator!=0.
		An error occurred.
	
	If the read stops because a terminator was encountered, the terminator is not returned
	and not included in *numBytesReadPtr.
	
	The output is NOT null terminated.
	
	Buffer must be capable of holding maxBytes bytes.
	
	The function result is 0 or an error code.
*/
static int
ReadBinaryBytes(VDTPortPtr op, UInt32 timeout, char* buffer, VDTByteCount maxBytes, const char* terminators, VDTByteCount* numBytesReadPtr)
{
	VDTByteCount count;
	int err = 0;
	
	*numBytesReadPtr = 0;
	
	if (*terminators == 0) {
		count = maxBytes;
		err = SerialRead(op, timeout, buffer, &count);
		*numBytesReadPtr = count;	// HR, 061019, 1.12: Do this even if timeout error so we can return partial results.
		if (err != 0)
			return err;
	}
	else {
		int numBytesRead;
		char* p;

		numBytesRead = 0;
		p = buffer;
		while(numBytesRead < maxBytes) {
			count = 1;
			if (err = SerialRead(op, timeout, p, &count))
				return err;

			if (strchr(terminators,*p) != 0)	// Terminator encountered?
				break;

			numBytesRead += 1;
			
			p += 1;								// Accept this byte into the output.
		}
		*numBytesReadPtr = numBytesRead;
	}

	return 0;
}

static int
ReadBinaryIntoStringVar(VDTPortPtr op, UInt32 timeout, char* bufPtr, VDTByteCount stringLength, const char* terminators, const char* varName)
{
	VDTByteCount numBytesRead;
	int gotTimeout, gotAbort;
	int err;
	
	gotTimeout = 0;			// HR, 061018, 1.12: If error is TIME_OUT_READ, return bytes read so far.
	gotAbort = 0;			// HR, 061018, 1.12: If error is -1 (abort), return bytes read so far.
	
	err = ReadBinaryBytes(op, timeout, bufPtr, stringLength, terminators, &numBytesRead);
	if (err == TIME_OUT_READ) {
		err = 0;
		gotTimeout = 1;
	}
	if (err == -1) {
		err = 0;
		gotAbort = 1;
	}

	if (err == 0)
		err = StoreStringDataUsingVarName(varName, bufPtr, numBytesRead);
	
	if (gotTimeout)
		err = TIME_OUT_READ;
	if (gotAbort)
		err = -1;
	
	return err;		
}

static int
ReadBinaryIntoNumVar(VDTPortPtr op, UInt32 timeout, int lowByteFirst, int numBytesPerValue, int incomingDataFormat, int isComplex, int doScale, double offset, double multiplier, const char* varName)
{
	char buffer[32];
	double dReal, dImag;
	VDTByteCount numBytesRead;
	int err;
	
	if (err = ReadBinaryBytes(op, timeout, buffer, numBytesPerValue, "", &numBytesRead))
		return err;
	if (NeedToSwapBytes(lowByteFirst))
		FixByteOrder(buffer, numBytesPerValue, 1);
	if (ConvertData(buffer, &dReal, 1, numBytesPerValue, incomingDataFormat, 8, IEEE_FLOAT) == 1)
		return BAD_BINARY_TYPE;
	if (doScale)
		ScaleData(NT_FP64, &dReal, &offset, &multiplier, 1);

	dImag = 0.0;
	if (isComplex) {
		if (err = ReadBinaryBytes(op, timeout, buffer, numBytesPerValue, "", &numBytesRead))
			return err;
		if (NeedToSwapBytes(lowByteFirst))
			FixByteOrder(buffer, numBytesPerValue, 1);
		if (ConvertData(buffer, &dImag, 1, numBytesPerValue, incomingDataFormat, 8, IEEE_FLOAT) == 1)
			return BAD_BINARY_TYPE;
		if (doScale)
			ScaleData(NT_FP64, &dImag, &offset, &multiplier, 1);
	}

	if (err = StoreNumericDataUsingVarName(varName, dReal, dImag))
		return err;	
	
	return 0;		
}

extern "C" int
ExecuteVDTReadBinary2(VDTReadBinary2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int quiet;
	int numItemsRead;
	int readStrings;
	int stringLength;
	char* bufPtr;
	char terminators[256];
	int lowByteFirst;
	int dataType, numBytesPerValue, incomingDataFormat, isComplex;
	double offset, multiplier;
	int doScale;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	timeout = 0;
	quiet = 0;
	numItemsRead = 0;
	readStrings = 0;
	stringLength = 0;
	bufPtr = NULL;
	*terminators = 0;
	lowByteFirst = 0;
	dataType = 8;				// Default is signed byte. This uses the same values as the WaveType function.
	offset = 0.0;
	multiplier = 1.0;
	doScale = 0;

	// Flag parameters.

	if (p->BFlagEncountered)
		lowByteFirst = 1;

	if (p->OFlagEncountered) {
		if (p->timeOutSeconds == 0)
			timeout = 0;
		else
			timeout = TickCount() + (UInt32)(60*p->timeOutSeconds);
	}

	if (p->QFlagEncountered)
		quiet = 1;

	if (p->SFlagEncountered) {
		stringLength = (int)p->strLen;
		readStrings = 1;
	}

	if (p->TFlagEncountered) {
		if (err = GetCStringFromHandle(p->terminators, terminators, sizeof(terminators)-1))
			goto done;
	}

	if (p->TYPEFlagEncountered)
		dataType = (int)p->dataType;

	if (p->YFlagEncountered) {
		offset = p->offset;
		multiplier = p->multiplier;
		doScale = 1;
	}
	
	if (readStrings == 0) {
		if (err = NumTypeToNumBytesAndFormat(dataType, &numBytesPerValue, &incomingDataFormat, &isComplex))
			goto done;
	}
	
	if (readStrings) {
		bufPtr = (char*)NewPtr(stringLength);
		if (bufPtr == NULL) {
			err = NOMEM;
			goto done;
		}
	}

	if (p->varNamesEncountered) {
		int* paramsSet;
		char* varName;
		int varDataType;
		int i;

		paramsSet = p->varNamesParamsSet;
		varName = (char*)p->varNames;

		for(i=0; i<100; i+=1) {
			if (paramsSet[i] == 0)
				break;						// No more parameters.

			if (err = VarNameToDataType(varName, &varDataType))
				goto done;
			
			if (varDataType == 0) {			// String?
				if (!readStrings) {
					err = EXPECTED_STRINGVARNAME;
					goto done;
				}
			
				if (err = ReadBinaryIntoStringVar(op, timeout, bufPtr, stringLength, terminators, varName))
					goto done;

				numItemsRead += 1;
			}
			else {
				if (readStrings) {
					err = EXPECTED_VARNAME;
					goto done;
				}
			
				if (err = ReadBinaryIntoNumVar(op, timeout, lowByteFirst, numBytesPerValue, incomingDataFormat, isComplex, doScale, offset, multiplier, varName))
					goto done;

				numItemsRead += 1;
				if (dataType & NT_CMPLX)
					numItemsRead += 1;
			}
			
			varName += MAX_OBJ_NAME+1;
		}
	}

done:
	if (bufPtr != NULL)
		DisposePtr(bufPtr);
	
	SetV_VDT(numItemsRead);
	
	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}
	
	return err;
}

int
RegisterVDTReadBinary2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTReadBinary2RuntimeParams structure as well.
	cmdTemplate = "VDTReadBinary2 /B /O=number:timeOutSeconds /Q /S=number:strLen /T=string:terminators /TYPE=number:dataType /Y={number:offset, number:multiplier} varName[100]:varNames";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTReadBinary2RuntimeParams), (void*)ExecuteVDTReadBinary2, 0);
}

// Operation template: VDTReadBinaryWave2 /B /O=number:timeOutSeconds /Q /TYPE=number:dataType /Y={number:offset, number:multiplier} wave[100]:waves

// Runtime param structure for VDTReadBinaryWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTReadBinaryWave2RuntimeParams {
	// Flag parameters.

	// Parameters for /B flag group.
	int BFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /O flag group.
	int OFlagEncountered;
	double timeOutSeconds;
	int OFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /TYPE flag group.
	int TYPEFlagEncountered;
	double dataType;
	int TYPEFlagParamsSet[1];

	// Parameters for /Y flag group.
	int YFlagEncountered;
	double offset;
	double multiplier;
	int YFlagParamsSet[2];

	// Main parameters.

	// Parameters for simple main group #0.
	int wavesEncountered;
	waveHndl waves[100];					// Optional parameter.
	int wavesParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTReadBinaryWave2RuntimeParams VDTReadBinaryWave2RuntimeParams;
typedef struct VDTReadBinaryWave2RuntimeParams* VDTReadBinaryWave2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
ReadBinaryIntoNumWave(VDTPortPtr op, UInt32 timeout, int lowByteFirst, int incomingBytesPerValue, int incomingDataFormat, int doScale, double offset, double multiplier, waveHndl waveH)
{
	int waveType, waveBytesPerValue, waveDataFormat, waveIsComplex;
	CountInt numWavePoints;
	VDTByteCount totalBytesToRead;
	CountInt numWaveValues;							// 1 value per point for real wave, 2 for complex.
	void* waveDataPtr;
	void* incomingDataPtr;
	int allocatedBuffer;
	VDTByteCount numBytesRead;
	int err;

	waveType = WaveType(waveH);
	
	if (waveType == TEXT_WAVE_TYPE)
		return NO_TEXT_OP;
	if (err = NumTypeToNumBytesAndFormat(waveType, &waveBytesPerValue, &waveDataFormat, &waveIsComplex))
		return err;

	waveDataPtr = WaveData(waveH);
	numWavePoints = WavePoints(waveH);
	numWaveValues = numWavePoints * (waveIsComplex ? 2:1);
	totalBytesToRead = incomingBytesPerValue*numWaveValues;
	
	// Check if buffer needed for incoming data.
	incomingDataPtr = waveDataPtr;						// Assume we can read directly into wave.
	allocatedBuffer = 0;
	if (incomingBytesPerValue > waveBytesPerValue) {	// Wave too small?
		incomingDataPtr = NewPtr(totalBytesToRead);
		if (incomingDataPtr == NULL) {
			err = NOMEM;
			goto done;
		}
		allocatedBuffer = 1;
	}

	err = ReadBinaryBytes(op, timeout, (char*)incomingDataPtr, totalBytesToRead, "", &numBytesRead);
	if (err == 0) {
		if (NeedToSwapBytes(lowByteFirst))
			FixByteOrder(incomingDataPtr, incomingBytesPerValue, numWaveValues);
		if (ConvertData(incomingDataPtr, waveDataPtr, numWaveValues, incomingBytesPerValue, incomingDataFormat, waveBytesPerValue, waveDataFormat) == 1)
			err = BAD_BINARY_TYPE;
		else
			ScaleData(waveType, waveDataPtr, &offset, &multiplier, numWaveValues);
	}

	WaveHandleModified(waveH);

done:
	if (allocatedBuffer)
		DisposePtr((char*)incomingDataPtr);

	return err;
}

extern "C" int
ExecuteVDTReadBinaryWave2(VDTReadBinaryWave2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int lowByteFirst;
	int dataType, incomingBytesPerValue, incomingDataFormat, isComplex;
	int quiet;
	double offset, multiplier;
	int doScale;
	int numWavesRead;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	timeout = 0;
	lowByteFirst = 0;
	dataType = 8;				// Default is signed byte. This uses the same values as the WaveType function.
	quiet = 0;
	offset = 0.0;
	multiplier = 1.0;
	doScale = 0;
	numWavesRead = 0;

	if (p->BFlagEncountered)
		lowByteFirst = 1;

	if (p->OFlagEncountered) {
		if (p->timeOutSeconds == 0)
			timeout = 0;
		else
			timeout = TickCount() + (UInt32)(60*p->timeOutSeconds);
	}

	if (p->QFlagEncountered)
		quiet = 1;

	if (p->TYPEFlagEncountered) {
		dataType = (int)p->dataType;
		dataType &= ~NT_CMPLX;		// The wave determines if we expect complex data or not.
	}

	if (p->YFlagEncountered) {
		offset = p->offset;
		multiplier = p->multiplier;
		doScale = 1;
	}

	if (err = NumTypeToNumBytesAndFormat(dataType, &incomingBytesPerValue, &incomingDataFormat, &isComplex))
		goto done;

	if (p->wavesEncountered) {
		int* paramsSet = p->wavesParamsSet;
		waveHndl waveH;
		int i;

		for(i=0; i<100; i++) {
			if (paramsSet[i] == 0)
				break;		// No more parameters.

			waveH = p->waves[i];
			if (waveH == NULL) {
				err = NULL_WAVE_OP;
				break;
			}

			if (err = ReadBinaryIntoNumWave(op, timeout, lowByteFirst, incomingBytesPerValue, incomingDataFormat, doScale, offset, multiplier, waveH))
				goto done;
			numWavesRead += 1;
		}
	}

done:
	SetV_VDT(numWavesRead);
	
	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}
	
	return err;
}

int
RegisterVDTReadBinaryWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTReadBinaryWave2RuntimeParams structure as well.
	cmdTemplate = "VDTReadBinaryWave2 /B /O=number:timeOutSeconds /Q /TYPE=number:dataType /Y={number:offset, number:multiplier} wave[100]:waves";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTReadBinaryWave2RuntimeParams), (void*)ExecuteVDTReadBinaryWave2, 0);
}
