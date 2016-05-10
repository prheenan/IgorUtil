#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

// Operation template: VDTWriteHex2 /L /O=number:timeOutSeconds /Q /W number[100]:numbers

// Runtime param structure for VDTWriteHex2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTWriteHex2RuntimeParams {
	// Flag parameters.

	// Parameters for /L flag group.
	int LFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /O flag group.
	int OFlagEncountered;
	double timeOutSeconds;
	int OFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /W flag group.
	int WFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int numbersEncountered;
	double numbers[100];					// Optional parameter.
	int numbersParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTWriteHex2RuntimeParams VDTWriteHex2RuntimeParams;
typedef struct VDTWriteHex2RuntimeParams* VDTWriteHex2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	GetHexBytesToWrite(val, numChars, buffer)

	Given a value, val, generates hex text representing that value.
	numChars is the number of hex nibbles to generate.
	buffer is the place to put the text and is assumed to be at least numChars long.
	
	When writing byte data (numChars == 2), valid values of val are -128 to 255.
	When writing short data (numChars == 4), valid values of val are -37268 to 65535.
	When writing int data (numChars == 8), valid values of val are -2147483648 to 4294967295.

	If val is not in the valid range, the result is undefined.
*/
static void
GetHexBytesToWrite(double val, int numChars, char* buffer)
{
	unsigned int uval;
	
	if (val >= 0) {
		uval = (unsigned int)val;
	}
	else {
		int ival = (int)val;
		switch(numChars) {
			case 2:
				uval = 0x100U + ival;
				break;
			case 4:
				uval = 0x10000U + ival;
				break;
			case 8:
				uval = 0xFFFFFFFFU + ival + 1;
				break;
		}
	}
	sprintf(buffer, "%*.*X", numChars, numChars, uval);
}

static int
WriteNumberAsHex(VDTPortPtr op, UInt32 timeout, int numChars, double val)
{
	char buffer[32];
	VDTByteCount len;
	int err;
	
	GetHexBytesToWrite(val, numChars, buffer);
	len = strlen(buffer);
	err = SerialWrite(op, timeout, buffer, &len);
	return err;
}

extern "C" int
ExecuteVDTWriteHex2(VDTWriteHex2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int quiet;
	int numChars;
	CountInt numItemsWritten;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	timeout = 0;
	numChars = 2;						// Two nibbles for one hex byte (0x21).
	quiet = 0;
	numItemsWritten = 0;

	// Flag parameters.

	if (p->LFlagEncountered)
		numChars = 8;					// Eight nibbles for four hex bytes (0x87654321).

	if (p->OFlagEncountered) {
		if (p->timeOutSeconds == 0)
			timeout = 0;
		else
			timeout = TickCount() + (UInt32)(60*p->timeOutSeconds);
	}

	if (p->QFlagEncountered)
		quiet = 1;

	if (p->WFlagEncountered)
		numChars = 4;					// Four nibbles for two hex bytes (0x4321).

	// Main parameters.

	if (p->numbersEncountered) {
		int* paramsSet = p->numbersParamsSet;
		double d1;
		int i;

		for(i=0; i<100; i++) {
			if (paramsSet[i] == 0)
				break;		// No more parameters.

			d1 = p->numbers[i];

			if (err = WriteNumberAsHex(op, timeout, numChars, d1))
				break;

			numItemsWritten += 1;
		}
	}

	SetV_VDT((double)numItemsWritten);
	
	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;	
	}

	return err;
}

int
RegisterVDTWriteHex2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTWriteHex2RuntimeParams structure as well.
	cmdTemplate = "VDTWriteHex2 /L /O=number:timeOutSeconds /Q /W number[100]:numbers";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTWriteHex2RuntimeParams), (void*)ExecuteVDTWriteHex2, 0);
}


// Operation template: VDTWriteHexWave2 /F={string:leader, string:separator, string:terminator} /L /O=number:timeOutSeconds /Q /R=[number:start,number:end] /W wave[100]:waves

// Runtime param structure for VDTWriteHexWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTWriteHexWave2RuntimeParams {
	// Flag parameters.

	// Parameters for /F flag group.
	int FFlagEncountered;
	Handle leader;
	Handle separator;
	Handle terminator;
	int FFlagParamsSet[3];

	// Parameters for /L flag group.
	int LFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /O flag group.
	int OFlagEncountered;
	double timeOutSeconds;
	int OFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /R flag group.
	int RFlagEncountered;
	double start;
	double end;
	int RFlagParamsSet[2];

	// Parameters for /W flag group.
	int WFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int wavesEncountered;
	waveHndl waves[100];					// Optional parameter.
	int wavesParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTWriteHexWave2RuntimeParams VDTWriteHexWave2RuntimeParams;
typedef struct VDTWriteHexWave2RuntimeParams* VDTWriteHexWave2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	For increased speed, we write an entire line at at time. We need to create a line
	buffer that must be large enough to hold the largest line we will ever run into.
	Since the user can write text waves which can be of any length, we have no foolproof
	way to know how big the line buffer must be, so we pick an arbitrary but very large number.
*/
#define MAX_LINE_LENGTH 100000

static int
AddBytesToBuffer(char* bufPtr, VDTByteCount* bytesInBufferPtr, const char* bytes, VDTByteCount len)
{
	if (*bytesInBufferPtr + len > MAX_LINE_LENGTH)
		return STR_TOO_LONG;
	
	memcpy(bufPtr + *bytesInBufferPtr, bytes, len);
	
	*bytesInBufferPtr += len;
	return 0;
}

/*	GetWavePointData(waveH, pointIndex, numChars, separator, bufPtr, bytesInBufferPtr, numNumbersWrittenPtr)
	
	numChars is the number of hex nibbles per data point.
	
	separator is text to write after each element, for example tab.

	bufPtr points to the start of the output line buffer.
	
	bytesInBufferPtr points to the number of bytes in the output line buffer. This routine
	updates that number.
*/
static int
GetWavePointData(
	waveHndl waveH,
	IndexInt pointIndex,
	int numChars,
	const char* separator,
	char* bufPtr,
	VDTByteCount* bytesInBufferPtr,
	CountInt* numNumbersWrittenPtr)
{
	int dataType;
	CountInt indices[MAX_DIMENSIONS];
	int err = 0;
	
	MemClear(indices, sizeof(indices));
	indices[ROWS] = pointIndex;
	
	dataType = WaveType(waveH);
	if (dataType == 0) {			// String?
		return EXPECTED_NUM_WAVE;
	}
	else {
		double value[2];
		int isComplex;
		char buffer[64];
		VDTByteCount len;
		
		isComplex = (dataType & NT_CMPLX) != 0;

		if (err = MDGetNumericWavePointValue(waveH, indices, value))
			return err;

		GetHexBytesToWrite(value[0], numChars, buffer);
		len = strlen(buffer);
		if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, buffer, len))
			return err;
		len = strlen(separator);
		if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, separator, len))
			return err;
		numNumbersWrittenPtr += 1;
		
		if (dataType & NT_CMPLX) {
			GetHexBytesToWrite(value[1], numChars, buffer);
			len = strlen(buffer);
			if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, buffer, len))
				return err;
			len = strlen(separator);
			if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, separator, len))
				return err;
			numNumbersWrittenPtr += 1;
		}
	}
	
	return 0;
}

extern "C" int
ExecuteVDTWriteHexWave2(VDTWriteHexWave2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int quiet;
	int numChars;
	char leader[256];
	char separator[256];
	char terminator[256];
	IndexInt startPoint, endPoint;
	CountInt numNumbersWritten;
	waveHndl waveH;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	timeout = 0;
	quiet = 0;
	numChars = 2;						// Two nibbles for one hex byte (0x21).
	*leader = 0;
	*separator = 0;
	*terminator = 0;

	// Flag parameters.

	if (p->FFlagEncountered) {
		if (p->FFlagParamsSet[0]) {
			if (err = GetCStringFromHandle(p->leader, leader, sizeof(leader)-1))
				return err;
		}

		if (p->FFlagParamsSet[1]) {
			if (err = GetCStringFromHandle(p->separator, separator, sizeof(separator)-1))
				return err;
		}

		if (p->FFlagParamsSet[2]) {
			if (err = GetCStringFromHandle(p->terminator, terminator, sizeof(terminator)-1))
				return err;
		}
	}

	if (p->LFlagEncountered)
		numChars = 8;					// Eight nibbles for four hex bytes (0x87654321).

	if (p->OFlagEncountered) {
		if (p->timeOutSeconds == 0)
			timeout = 0;
		else
			timeout = TickCount() + (UInt32)(60*p->timeOutSeconds);
	}

	if (p->QFlagEncountered)
		quiet = 1;
		
	if (p->RFlagEncountered) {
		startPoint = (IndexInt)p->start;
		endPoint = (IndexInt)p->end;
	}
	else {
		startPoint = 0;
		waveH = p->waves[0];
		if (waveH == NULL)
			return NULL_WAVE_OP;
		endPoint = WavePoints(waveH) - 1;
	}

	if (p->WFlagEncountered)
		numChars = 4;					// Four nibbles for two hex bytes (0x4321).

	// Main parameters.

	numNumbersWritten = 0;

	if (p->wavesEncountered) {
		int* paramsSet;
		waveHndl* whp;
		IndexInt pointIndex;
		int waveIndex;
		char* bufPtr;
		VDTByteCount bytesInBuffer;
		int len;
		
		bufPtr = (char*)NewPtr(MAX_LINE_LENGTH+1);
		if (bufPtr == NULL)
			return NOMEM;
		
		for(pointIndex=startPoint; pointIndex<=endPoint; pointIndex+=1) {
			paramsSet = p->wavesParamsSet;
			whp = &p->waves[0];

			bytesInBuffer = 0;
			
			// Add leader.
			len = (int)strlen(leader);
			if (len > 0) {
				if (err = AddBytesToBuffer(bufPtr, &bytesInBuffer, leader, len))
					break;
			}

			// Get the data for an entire line.
			for(waveIndex=0; waveIndex<100; waveIndex+=1) {
				if (paramsSet[waveIndex] == 0)
					break;						// No more parameters.

				waveH = *whp;
				if (waveH == NULL) {
					// Should never happen.
					err = NULL_WAVE_OP;
					break;
				}
				
				if (err = GetWavePointData(waveH, pointIndex, numChars, separator, bufPtr, &bytesInBuffer, &numNumbersWritten))
					break;
				
				whp += 1;
			}
			
			if (err != 0)
				break;
				
			// Remove last separator which needs to be replaced by the terminator.
			bytesInBuffer -= strlen(separator);
			if (bytesInBuffer < 0)						// Should never happen.
				bytesInBuffer = 0;
			
			len = (int)strlen(terminator);
			if (len > 0) {
				if (err = AddBytesToBuffer(bufPtr, &bytesInBuffer, terminator, len))
					break;
			}
				
			if (err = SerialWrite(op, timeout, bufPtr, &bytesInBuffer))
				break;
		}
		
		DisposePtr(bufPtr);
	}

	SetV_VDT((double)numNumbersWritten);
	
	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;	
	}

	return err;
}

int
RegisterVDTWriteHexWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTWriteHexWave2RuntimeParams structure as well.
	cmdTemplate = "VDTWriteHexWave2 /F={string:leader, string:separator, string:terminator} /L /O=number:timeOutSeconds /Q /R=[number:start,number:end] /W wave[100]:waves";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTWriteHexWave2RuntimeParams), (void*)ExecuteVDTWriteHexWave2, 0);
}
