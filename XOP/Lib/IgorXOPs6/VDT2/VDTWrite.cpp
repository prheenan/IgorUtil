#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

// Operation template: VDTWrite2 /O=number:timeOutSeconds /Q string:str

// Runtime param structure for VDTWrite2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTWrite2RuntimeParams {
	// Flag parameters.

	// Parameters for /O flag group.
	int OFlagEncountered;
	double timeOutSeconds;
	int OFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int strEncountered;
	Handle str;
	int strParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTWrite2RuntimeParams VDTWrite2RuntimeParams;
typedef struct VDTWrite2RuntimeParams* VDTWrite2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteVDTWrite2(VDTWrite2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int quiet;
	VDTByteCount len;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	timeout = 0;
	quiet = 0;

	// Flag parameters.

	if (p->OFlagEncountered) {
		if (p->timeOutSeconds == 0)
			timeout = 0;
		else
			timeout = TickCount() + (UInt32)(60*p->timeOutSeconds);
	}

	if (p->QFlagEncountered)
		quiet = 1;

	// Main parameters.
	
	if (p->str == NULL)
		return USING_NULL_STRVAR;
	
	len = GetHandleSize(p->str);	
	err = SerialWrite(op, timeout, *p->str, &len);

	SetV_VDT(err==0);

	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;
	}

	return err;
}

int
RegisterVDTWrite2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTWrite2RuntimeParams structure as well.
	cmdTemplate = "VDTWrite2 /O=number:timeOutSeconds /Q string:str";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTWrite2RuntimeParams), (void*)ExecuteVDTWrite2, 0);
}


// Operation template: VDTWriteWave2 /F={string:numericFormat[, string:leader, string:separator, string:terminator]} /O=number:timeOutSeconds /Q /R=[number:start,number:end] wave[100]:waves

// Runtime param structure for VDTWriteWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTWriteWave2RuntimeParams {
	// Flag parameters.

	// Parameters for /F flag group.
	int FFlagEncountered;
	Handle numericFormat;
	Handle leader;							// Optional parameter.
	Handle separator;						// Optional parameter.
	Handle terminator;						// Optional parameter.
	int FFlagParamsSet[4];

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

	// Main parameters.

	// Parameters for simple main group #0.
	int wavesEncountered;
	waveHndl waves[100];					// Optional parameter.
	int wavesParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTWriteWave2RuntimeParams VDTWriteWave2RuntimeParams;
typedef struct VDTWriteWave2RuntimeParams* VDTWriteWave2RuntimeParamsPtr;
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

/*	GetNumericFormatInfo(numericFormat, isIntegerFormatPtr, isUnsignedFormatPtr)
	
	The last character of numericFormat is assumed to be the conversion character.
	
	If the format does not end with a valid conversion character, returns BAD_NUMERIC_FORMAT.
	Otherwise, returns 0.
*/
static int
GetNumericFormatInfo(const char* numericFormat, int* isIntegerFormatPtr, int* isUnsignedFormatPtr)
{
	int len;

	*isIntegerFormatPtr = 0;
	*isUnsignedFormatPtr = 0;
	
	len = (int)strlen(numericFormat);
	if (len == 0)
		return BAD_NUMERIC_FORMAT;
		
	switch(numericFormat[len-1]) {
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			break;
	
		case 'd':
		case 'i':
		case 'c':
			*isIntegerFormatPtr = 1;
			break;
		
		case 'o':
		case 'x':
		case 'X':
		case 'u':
			*isIntegerFormatPtr = 1;
			*isUnsignedFormatPtr = 0;
			break;
		
		default:
			return BAD_NUMERIC_FORMAT;
	}
	
	return 0;
}

static void
SPrintFormatted(const char* numericFormat, int isIntegerFormat, int isUnsignedFormat, double value, char* output)
{
	if (isIntegerFormat) {
		if (isUnsignedFormat)
			sprintf(output, numericFormat, (unsigned long)value);
		else
			sprintf(output, numericFormat, (long)value);
	}
	else {
		sprintf(output, numericFormat, value);
	}
}

/*	GetWavePointData(waveH, pointIndex, numericFormat, isIntegerFormat, isUnsignedFormat, separator, bufPtr, bytesInBufferPtr, numNumbersWrittenPtr)

	numericFormat is the format for one particular number, for example, "%g" or "%15g".
	
	separator is text to write after each element, for example tab.

	bufPtr points to the start of the output line buffer.
	
	bytesInBufferPtr points to the number of bytes in the output line buffer. This routine
	updates that number.
*/
static int
GetWavePointData(
	waveHndl waveH,
	IndexInt pointIndex,
	const char* numericFormat,
	int isIntegerFormat, int isUnsignedFormat,
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
		Handle textH;

		textH = NewHandle(0);
		if (textH == NULL)
			return NOMEM;	
	
		err = MDGetTextWavePointValue(waveH, indices, textH);
		if (err == 0) {
			VDTByteCount len;
			len = GetHandleSize(textH);
			err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, *textH, len);
			DisposeHandle(textH);
			if (err == 0) {
				len = strlen(separator);
				err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, separator, len);
			}
		}

		if (err != 0)
			return err;
		
		numNumbersWrittenPtr += 1;
	}
	else {
		double value[2];
		int isComplex;
		char buffer[64];
		VDTByteCount len;
		
		isComplex = (dataType & NT_CMPLX) != 0;

		if (err = MDGetNumericWavePointValue(waveH, indices, value))
			return err;

		SPrintFormatted(numericFormat, isIntegerFormat, isUnsignedFormat, value[0], buffer);
		len = strlen(buffer);
		if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, buffer, len))
			return err;
		len = strlen(separator);
		if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, separator, len))
			return err;
		numNumbersWrittenPtr += 1;
		
		if (dataType & NT_CMPLX) {
			SPrintFormatted(numericFormat, isIntegerFormat, isUnsignedFormat, value[1], buffer);
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
ExecuteVDTWriteWave2(VDTWriteWave2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int quiet;
	char numericFormat[16];
	char leader[256];
	char separator[256];
	char terminator[256];
	waveHndl waveH;
	IndexInt startPoint, endPoint;
	CountInt numNumbersWritten;
	int isIntegerFormat, isUnsignedFormat;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	// Flag parameters.
	timeout = 0;
	quiet = 0;
	strcpy(numericFormat, "%g");
	*leader = 0;
	strcpy(separator, "\t");
	strcpy(terminator, CR_STR);

	if (p->FFlagEncountered) {
		if (p->FFlagParamsSet[0]) {
			if (err = GetCStringFromHandle(p->numericFormat, numericFormat, sizeof(numericFormat)-1))
				return err;
		}

		if (p->FFlagParamsSet[1]) {
			if (err = GetCStringFromHandle(p->leader, leader, sizeof(leader)-1))
				return err;
		}

		if (p->FFlagParamsSet[2]) {
			if (err = GetCStringFromHandle(p->separator, separator, sizeof(separator)-1))
				return err;
		}

		if (p->FFlagParamsSet[3]) {
			if (err = GetCStringFromHandle(p->terminator, terminator, sizeof(terminator)-1))
				return err;
		}
	}

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

	if (err = GetNumericFormatInfo(numericFormat, &isIntegerFormat, &isUnsignedFormat))
		return err;

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
				
				if (err = GetWavePointData(waveH, pointIndex, numericFormat, isIntegerFormat, isUnsignedFormat, separator, bufPtr, &bytesInBuffer, &numNumbersWritten))
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
RegisterVDTWriteWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTWriteWave2RuntimeParams structure as well.
	cmdTemplate = "VDTWriteWave2 /F={string:numericFormat[, string:leader, string:separator, string:terminator]} /O=number:timeOutSeconds /Q /R=[number:start,number:end] wave[100]:waves";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTWriteWave2RuntimeParams), (void*)ExecuteVDTWriteWave2, 0);
}
