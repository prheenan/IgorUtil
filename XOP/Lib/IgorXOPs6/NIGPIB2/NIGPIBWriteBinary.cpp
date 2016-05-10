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

// Operation template: GPIBWriteBinary2 /B /Q /TYPE=number:dataType number[100]:numbers

// Runtime param structure for GPIBWriteBinary2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBWriteBinaryRuntimeParams {
	// Flag parameters.

	// Parameters for /B flag group.
	int BFlagEncountered;
	// There are no fields for this group because it has no parameters.

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
typedef struct GPIBWriteBinaryRuntimeParams GPIBWriteBinaryRuntimeParams;
typedef struct GPIBWriteBinaryRuntimeParams* GPIBWriteBinaryRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
WriteNumberAsBinary(int device, int lowByteFirst, int destBytesPerValue, int destDataFormat, double d1)
{
	char buffer[32];
	int err;
	
	if (ConvertData(&d1, buffer, 1, 8, IEEE_FLOAT, destBytesPerValue, destDataFormat) == 1)
		return BAD_BINARY_TYPE;
	
	if (NeedToSwapBytes(lowByteFirst))
		FixByteOrder(buffer, destBytesPerValue, 1);
	
	NIGPIB_ibwrt(device, buffer, destBytesPerValue);
	err = IBErr(1);
	return err;
}

extern "C" int
ExecuteGPIBWriteBinary2(GPIBWriteBinaryRuntimeParamsPtr p)
{
	int lowByteFirst;
	int quiet;
	int dataType, destBytesPerValue, destDataFormat, isComplex;
	int numItemsWritten;
	int err = 0;

	if (err = CheckActiveDevice())
		return err;

	WatchCursor();

	lowByteFirst = 0;
	dataType = 8;				// Default is signed byte. This uses the same values as the WaveType function.
	quiet = 0;
	numItemsWritten = 0;

	if (p->BFlagEncountered)
		lowByteFirst = 1;

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

			if (err = WriteNumberAsBinary(gActiveDevice, lowByteFirst, destBytesPerValue, destDataFormat, d1))
				break;
			numItemsWritten += 1;
		}
	}
	
done:
	SetV_Flag(numItemsWritten);
	
	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;	
	}

	return err;
}

int
RegisterGPIBWriteBinary2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the GPIBWriteBinaryRuntimeParams structure as well.
	cmdTemplate = "GPIBWriteBinary2 /B /Q /TYPE=number:dataType number[100]:numbers";
	runtimeNumVarList = "V_Flag;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBWriteBinaryRuntimeParams), (void*)ExecuteGPIBWriteBinary2, 0);
}

// Operation template: GPIBWriteBinaryWave2 /B /Q /TYPE=number:dataType wave[100]:waves

// Runtime param structure for GPIBWriteBinaryWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBWriteBinaryWaveRuntimeParams {
	// Flag parameters.

	// Parameters for /B flag group.
	int BFlagEncountered;
	// There are no fields for this group because it has no parameters.

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
typedef struct GPIBWriteBinaryWaveRuntimeParams GPIBWriteBinaryWaveRuntimeParams;
typedef struct GPIBWriteBinaryWaveRuntimeParams* GPIBWriteBinaryWaveRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
WriteWaveAsBinary(int device, int lowByteFirst, int destBytesPerValue, int destDataFormat, waveHndl waveH)
{
	int waveType, waveBytesPerValue, waveDataFormat, waveIsComplex;
	CountInt numWavePoints;
	BCInt totalBytesToWrite;
	CountInt numWaveValues;					// 1 value for real wave, 2 for complex.
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

	NIGPIB_ibwrt(device, (char*)destDataPtr, totalBytesToWrite);
	err = IBErr(1);

done:
	if (allocatedBuffer)
		DisposePtr((char*)destDataPtr);
	return err;
}

extern "C" int
ExecuteGPIBWriteBinaryWave2(GPIBWriteBinaryWaveRuntimeParamsPtr p)
{
	int lowByteFirst;
	int quiet;
	int dataType, destBytesPerValue, destDataFormat, isComplex;
	int numWavesWritten;
	int err = 0;

	if (err = CheckActiveDevice())
		return err;

	WatchCursor();

	lowByteFirst = 0;
	dataType = 8;						// Default is signed byte. This uses the same values as the WaveType function.
	quiet = 0;
	numWavesWritten = 0;

	if (p->BFlagEncountered)
		lowByteFirst = 1;

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

			if (err = WriteWaveAsBinary(gActiveDevice, lowByteFirst, destBytesPerValue, destDataFormat, waveH))
				break;
			numWavesWritten += 1;
		}
	}
	
done:
	SetV_Flag(numWavesWritten);
	
	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;	
	}

	return err;
}

int
RegisterGPIBWriteBinaryWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the GPIBWriteBinaryWaveRuntimeParams structure as well.
	cmdTemplate = "GPIBWriteBinaryWave2 /B /Q /TYPE=number:dataType wave[100]:waves";
	runtimeNumVarList = "V_Flag;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBWriteBinaryWaveRuntimeParams), (void*)ExecuteGPIBWriteBinaryWave2, 0);
}
