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

// Operation template: VDTWriteBinary2 /B /O=number:timeOutSeconds /Q /TYPE=number:dataType number[100]:numbers

// Runtime param structure for VDTWriteBinary2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTWriteBinary2RuntimeParams {
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

	// Main parameters.

	// Parameters for simple main group #0.
	int numbersEncountered;
	double numbers[100];					// Optional parameter.
	int numbersParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTWriteBinary2RuntimeParams VDTWriteBinary2RuntimeParams;
typedef struct VDTWriteBinary2RuntimeParams* VDTWriteBinary2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
WriteNumberAsBinary(VDTPortPtr op, UInt32 timeout, int lowByteFirst, int destBytesPerValue, int destDataFormat, double d1)
{
	char buffer[32];
	VDTByteCount len;
	int err;
	
	if (ConvertData(&d1, buffer, 1, 8, IEEE_FLOAT, destBytesPerValue, destDataFormat) == 1)
		return BAD_BINARY_TYPE;
	
	if (NeedToSwapBytes(lowByteFirst))
		FixByteOrder(buffer, destBytesPerValue, 1);
	
	len = destBytesPerValue;
	err = SerialWrite(op, timeout, buffer, &len);
	return err;
}

extern "C" int
ExecuteVDTWriteBinary2(VDTWriteBinary2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int lowByteFirst;
	int quiet;
	int dataType, destBytesPerValue, destDataFormat, isComplex;
	int numItemsWritten;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	timeout = 0;
	lowByteFirst = 0;
	dataType = 8;				// Default is signed byte. This uses the same values as the WaveType function.
	quiet = 0;
	numItemsWritten = 0;

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
		dataType &= ~NT_CMPLX;			// Complex is not supported.
	}

	if (err = NumTypeToNumBytesAndFormat(dataType, &destBytesPerValue, &destDataFormat, &isComplex))
		goto done;

	if (p->numbersEncountered) {
		int* paramsSet = p->numbersParamsSet;
		double d1;
		int i;

		for(i=0; i<100; i++) {
			if (paramsSet[i] == 0)
				break;		// No more parameters.

			d1 = p->numbers[i];

			if (err = WriteNumberAsBinary(op, timeout, lowByteFirst, destBytesPerValue, destDataFormat, d1))
				break;
			numItemsWritten += 1;
		}
	}
	
done:
	SetV_VDT(numItemsWritten);
	
	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;	
	}

	return err;
}

int
RegisterVDTWriteBinary2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTWriteBinary2RuntimeParams structure as well.
	cmdTemplate = "VDTWriteBinary2 /B /O=number:timeOutSeconds /Q /TYPE=number:dataType number[100]:numbers";
	runtimeNumVarList = "V_VDT";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTWriteBinary2RuntimeParams), (void*)ExecuteVDTWriteBinary2, 0);
}

// Operation template: VDTWriteBinaryWave2 /B /O=number:timeOutSeconds /Q /TYPE=number:dataType wave[100]:waves

// Runtime param structure for VDTWriteBinaryWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTWriteBinaryWave2RuntimeParams {
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

	// Main parameters.

	// Parameters for simple main group #0.
	int wavesEncountered;
	waveHndl waves[100];					// Optional parameter.
	int wavesParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTWriteBinaryWave2RuntimeParams VDTWriteBinaryWave2RuntimeParams;
typedef struct VDTWriteBinaryWave2RuntimeParams* VDTWriteBinaryWave2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
WriteWaveAsBinary(VDTPortPtr op, UInt32 timeout, int lowByteFirst, int destBytesPerValue, int destDataFormat, waveHndl waveH)
{
	int waveType, waveBytesPerValue, waveDataFormat, waveIsComplex;
	CountInt numWavePoints;
	VDTByteCount totalBytesToWrite;
	CountInt numWaveValues;								// 1 value per point for real wave, 2 for complex.
	void* waveDataPtr;
	void* destDataPtr;
	int allocatedBuffer;
	int err;

	waveType = WaveType(waveH);
	if (waveType == TEXT_WAVE_TYPE)
		return NO_TEXT_OP;
	if (err = NumTypeToNumBytesAndFormat(waveType, &waveBytesPerValue, &waveDataFormat, &waveIsComplex))
		return err;

	waveDataPtr = WaveData(waveH);
	numWavePoints = WavePoints(waveH);
	numWaveValues = numWavePoints * (waveIsComplex ? 2:1);
	totalBytesToWrite = destBytesPerValue*numWaveValues;
	
	// Check if buffer needed for dest data.
	destDataPtr = waveDataPtr;							// Assume we can write directly from wave.
	allocatedBuffer = 0;
	if (destBytesPerValue!=waveBytesPerValue || destDataFormat!=waveDataFormat || NeedToSwapBytes(lowByteFirst)) {	// If the source and dest formats are not identical, we need to use a temporary buffer.
		destDataPtr = NewPtr(totalBytesToWrite);
		if (destDataPtr == NULL) {
			err = NOMEM;
			goto done;
		}
		allocatedBuffer = 1;

		if (ConvertData(waveDataPtr, destDataPtr, numWaveValues, waveBytesPerValue, waveDataFormat, destBytesPerValue, destDataFormat) == 1) {
			err = BAD_BINARY_TYPE;
			goto done;
		}

		if (NeedToSwapBytes(lowByteFirst))
			FixByteOrder(destDataPtr, destBytesPerValue, numWaveValues);
	}

	err = SerialWrite(op, timeout, (char*)destDataPtr, &totalBytesToWrite);

done:
	if (allocatedBuffer)
		DisposePtr((char*)destDataPtr);

	return err;
}

extern "C" int
ExecuteVDTWriteBinaryWave2(VDTWriteBinaryWave2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int lowByteFirst;
	int quiet;
	int dataType, destBytesPerValue, destDataFormat, isComplex;
	int numWavesWritten;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	timeout = 0;
	lowByteFirst = 0;
	dataType = 8;						// Default is signed byte. This uses the same values as the WaveType function.
	quiet = 0;
	numWavesWritten = 0;

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
		dataType &= ~NT_CMPLX;			// Complex determined by the wave.
	}

	if (err = NumTypeToNumBytesAndFormat(dataType, &destBytesPerValue, &destDataFormat, &isComplex))
		goto done;

	// Main parameters.

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

			if (err = WriteWaveAsBinary(op, timeout, lowByteFirst, destBytesPerValue, destDataFormat, waveH))
				break;
			numWavesWritten += 1;
		}
	}
	
done:
	SetV_VDT(numWavesWritten);
	
	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;	
	}

	return err;
}

int
RegisterVDTWriteBinaryWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTWriteBinaryWave2RuntimeParams structure as well.
	cmdTemplate = "VDTWriteBinaryWave2 /B /O=number:timeOutSeconds /Q /TYPE=number:dataType wave[100]:waves";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTWriteBinaryWave2RuntimeParams), (void*)ExecuteVDTWriteBinaryWave2, 0);
}
