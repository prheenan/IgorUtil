#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "NIGPIB.h"

static int
NeedToSwapBytes(int lowByteFirst)		// Returns truth data is in the wrong order for this platform.
{
	#ifdef XOP_LITTLE_ENDIAN			// HR, 091029, 1.05: Fixed to work correctly on Intel Macintosh
		return lowByteFirst == 0;		// Data is big-endian. Need to swap for little-endian processor.
	#else
		return lowByteFirst != 0;		// Data is little-endian. Need to swap for big-endian processor.
	#endif
}

// Operation template: GPIBReadBinary2 /B /TYPE=number:dataType /Q /S=number:strLen /T=string:terminators /Y={number:offset, number:multiplier} varName[100]:varName

// Runtime param structure for GPIBReadBinary2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBReadBinaryRuntimeParams {
	// Flag parameters.

	// Parameters for /B flag group.
	int BFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /TYPE flag group.
	int TYPEFlagEncountered;
	double dataType;
	int TYPEFlagParamsSet[1];

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

	// Parameters for /Y flag group.
	int YFlagEncountered;
	double offset;
	double multiplier;
	int YFlagParamsSet[2];

	// Main parameters.

	// Parameters for simple main group #0.
	int varNameEncountered;
	char varNames[MAX_OBJ_NAME+1][100];
	int varNameParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct GPIBReadBinaryRuntimeParams GPIBReadBinaryRuntimeParams;
typedef struct GPIBReadBinaryRuntimeParams* GPIBReadBinaryRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	ReadBinaryBytes(device, buffer, maxBytes, terminators, numBytesReadPtr)
	
	Reads bytes until:
		maxBytes have been read.
		A terminator has been read if *terminator!=0.
		The EOS character as defined in NI-488 driver setup was read.
		The END line was asserted.
		An error occurred.
	
	If the read stops because a terminator was encountered, the terminator is not returned
	and not included in *numBytesReadPtr.
	
	The output is NOT null terminated.
	
	Buffer must be capable of holding maxBytes bytes.
	
	The function result is 0 or an NIGPIB error code.
*/
static int
ReadBinaryBytes(int device, char* buffer, BCInt maxBytes, const char* terminators, BCInt* numBytesReadPtr)
{
	int status;
	
	*numBytesReadPtr = 0;
	
	if (*terminators == 0) {
		status = NIGPIB_ibrd(device, buffer, maxBytes);
		if (status & ERR)
			return IBErr(0);
		*numBytesReadPtr = ibcnt;
	}
	else {
		int numBytesRead;
		char* p;

		numBytesRead = 0;
		p = buffer;
		while(numBytesRead < maxBytes) {
			status = NIGPIB_ibrd(device, p, 1);
			if (status & ERR)
				return IBErr(0);

			if (strchr(terminators,*p) != 0)	// Terminator encountered?
				break;

			numBytesRead += 1;
			
			p += 1;								// Accept this byte into the output.

			if (status & END)					// END or EOS detected (we have no way to know which occurred).
				break;
		}
		*numBytesReadPtr = numBytesRead;
	}

	return 0;
}

static int
ReadBinaryIntoStringVar(int device, char* bufPtr, BCInt stringLength, const char* terminators, const char* varName)
{
	BCInt numBytesRead;
	int err;
	
	if (err = ReadBinaryBytes(device, bufPtr, stringLength, terminators, &numBytesRead))
		return err;

	if (err = StoreStringDataUsingVarName(varName, bufPtr, numBytesRead))
		return err;	
	
	return 0;		
}

static int
ReadBinaryIntoNumVar(int device, int lowByteFirst, int numBytesPerValue, int incomingDataFormat, int isComplex, int doScale, double offset, double multiplier, const char* varName)
{
	char buffer[32];
	double dReal, dImag;
	BCInt numBytesRead;
	int err;
	
	if (err = ReadBinaryBytes(device, buffer, numBytesPerValue, (char*)"", &numBytesRead))
		return err;
	if (NeedToSwapBytes(lowByteFirst))
		FixByteOrder(buffer, numBytesPerValue, 1);
	if (ConvertData(buffer, &dReal, 1, numBytesPerValue, incomingDataFormat, 8, IEEE_FLOAT) == 1)
		return BAD_BINARY_TYPE;
	if (doScale)
		ScaleData(NT_FP64, &dReal, &offset, &multiplier, 1);

	dImag = 0.0;
	if (isComplex) {
		if (err = ReadBinaryBytes(device, buffer, numBytesPerValue, (char*)"", &numBytesRead))
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
ExecuteGPIBReadBinary2(GPIBReadBinaryRuntimeParamsPtr p)
{
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

	if (err = CheckActiveDevice())
		return err;

	WatchCursor();

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

	if (p->TYPEFlagEncountered)
		dataType = (int)p->dataType;

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

	if (p->varNameEncountered) {
		int* paramsSet;
		char* varName;
		int varDataType;
		int i;

		paramsSet = p->varNameParamsSet;
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
			
				if (err = ReadBinaryIntoStringVar(gActiveDevice, bufPtr, stringLength, terminators, varName))
					goto done;

				numItemsRead += 1;
			}
			else {
				if (readStrings) {
					err = EXPECTED_VARNAME;
					goto done;
				}
			
				if (err = ReadBinaryIntoNumVar(gActiveDevice, lowByteFirst, numBytesPerValue, incomingDataFormat, isComplex, doScale, offset, multiplier, varName))
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
	
	SetV_Flag(numItemsRead);
	
	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}
	
	return err;
}

int
RegisterGPIBReadBinary2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "GPIBReadBinary2 /B /TYPE=number:dataType /Q /S=number:strLen /T=string:terminators /Y={number:offset, number:multiplier} varName[100]:varName";
	runtimeNumVarList = "V_Flag;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBReadBinaryRuntimeParams), (void*)ExecuteGPIBReadBinary2, 0);
}

// Operation template: GPIBReadBinaryWave2 /B /TYPE=number:dataType /Q /Y={number:offset, number:multiplier} wave[100]:waves

// Runtime param structure for GPIBReadBinaryWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBReadBinaryWaveRuntimeParams {
	// Flag parameters.

	// Parameters for /B flag group.
	int BFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /TYPE flag group.
	int TYPEFlagEncountered;
	double dataType;
	int TYPEFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

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
typedef struct GPIBReadBinaryWaveRuntimeParams GPIBReadBinaryWaveRuntimeParams;
typedef struct GPIBReadBinaryWaveRuntimeParams* GPIBReadBinaryWaveRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
ReadBinaryIntoNumWave(int device, int lowByteFirst, int incomingBytesPerValue, int incomingDataFormat, int doScale, double offset, double multiplier, waveHndl waveH)
{
	int waveType, waveBytesPerValue, waveDataFormat, waveIsComplex;
	CountInt numWavePoints, totalBytesToRead;
	CountInt numWaveValues;							// 1 value for real wave, 2 for complex.
	void* waveDataPtr;
	void* incomingDataPtr;
	int allocatedBuffer;
	BCInt numBytesRead;
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

	err = ReadBinaryBytes(device, (char*)incomingDataPtr, totalBytesToRead, (char*)"", &numBytesRead);
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
ExecuteGPIBReadBinaryWave2(GPIBReadBinaryWaveRuntimeParamsPtr p)
{
	int lowByteFirst;
	int dataType, incomingBytesPerValue, incomingDataFormat, isComplex;
	int quiet;
	double offset, multiplier;
	int doScale;
	int numWavesRead;
	int err = 0;

	if (err = CheckActiveDevice())
		return err;

	WatchCursor();

	lowByteFirst = 0;
	dataType = 8;				// Default is signed byte. This uses the same values as the WaveType function.
	quiet = 0;
	offset = 0.0;
	multiplier = 1.0;
	doScale = 0;
	numWavesRead = 0;

	if (p->BFlagEncountered)
		lowByteFirst = 1;

	if (p->TYPEFlagEncountered) {
		dataType = (int)p->dataType;
		dataType &= ~NT_CMPLX;		// The wave determines if we expect complex data or not.
	}

	if (p->QFlagEncountered)
		quiet = 1;

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

			if (err = ReadBinaryIntoNumWave(gActiveDevice, lowByteFirst, incomingBytesPerValue, incomingDataFormat, doScale, offset, multiplier, waveH))
				goto done;
			numWavesRead += 1;
		}
	}

done:
	SetV_Flag(numWavesRead);
	
	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}
	
	return err;
}

int
RegisterGPIBReadBinaryWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the GPIBReadBinaryWaveRuntimeParams structure as well.
	cmdTemplate = "GPIBReadBinaryWave2 /B /TYPE=number:dataType /Q /Y={number:offset, number:multiplier} wave[100]:waves";
	runtimeNumVarList = "V_Flag;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBReadBinaryWaveRuntimeParams), (void*)ExecuteGPIBReadBinaryWave2, 0);
}
