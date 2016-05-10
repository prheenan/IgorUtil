#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "NIGPIB.h"

// Operation template: GPIBWrite2/Q string:str

// Runtime param structure for GPIBWrite2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBWriteRuntimeParams {
	// Flag parameters.

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
typedef struct GPIBWriteRuntimeParams GPIBWriteRuntimeParams;
typedef struct GPIBWriteRuntimeParams* GPIBWriteRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteGPIBWrite2(GPIBWriteRuntimeParamsPtr p)
{
	int quiet;
	int len;
	int err = 0;

	if (err = CheckActiveDevice())
		return err;

	WatchCursor();

	quiet = 0;

	// Flag parameters.

	if (p->QFlagEncountered)
		quiet = 1;

	// Main parameters.
	
	if (p->str == NULL)
		return USING_NULL_STRVAR;
	
	len = (int)GetHandleSize(p->str);
	NIGPIB_ibwrt(gActiveDevice, *p->str, len);
	err = IBErr(1);
	
	SetV_Flag(err==0);

	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;
	}

	return err;
}

int
RegisterGPIBWrite2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "GPIBWrite2/Q string:str";
	runtimeNumVarList = "V_flag;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBWriteRuntimeParams), (void*)ExecuteGPIBWrite2, 0);
}


// Operation template: GPIBWriteWave2 /F={string:numericFormat[, string:leader, string:separator, string:terminator]} /R=[number:start,number:end] /Q wave[100]:wave

// Runtime param structure for GPIBWriteWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBWriteWaveRuntimeParams {
	// Flag parameters.

	// Parameters for /F flag group.
	int FFlagEncountered;
	Handle numericFormat;
	Handle leader;						// Optional parameter.
	Handle separator;					// Optional parameter.
	Handle terminator;					// Optional parameter.
	int FFlagParamsSet[4];

	// Parameters for /R flag group.
	int RFlagEncountered;
	double start;
	double end;
	int RFlagParamsSet[2];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int waveEncountered;
	waveHndl waves[100];
	int waveParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct GPIBWriteWaveRuntimeParams GPIBWriteWaveRuntimeParams;
typedef struct GPIBWriteWaveRuntimeParams* GPIBWriteWaveRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	For increased speed, we write an entire line at at time. We need to create a line
	buffer that must be large enough to hold the largest line we will ever run into.
	Since the user can write text waves which can be of any length, we have no foolproof
	way to know how big the line buffer must be, so we pick an arbitrary but very large number.
*/
#define MAX_LINE_LENGTH 100000

static int
AddBytesToBuffer(char* bufPtr, int* bytesInBufferPtr, const char* bytes, int len)
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
			sprintf(output, numericFormat, (unsigned int)value);
		else
			sprintf(output, numericFormat, (int)value);
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
	int* bytesInBufferPtr,
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
		int len;

		textH = NewHandle(0);
		if (textH == NULL)
			return NOMEM;	
	
		err = MDGetTextWavePointValue(waveH, indices, textH);
		if (err == 0) {
			len = (int)GetHandleSize(textH);
			err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, *textH, len);
			DisposeHandle(textH);

			if (err == 0) {
				len = (int)strlen(separator);
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
		int len;
		
		isComplex = (dataType & NT_CMPLX) != 0;

		if (err = MDGetNumericWavePointValue(waveH, indices, value))
			return err;

		SPrintFormatted(numericFormat, isIntegerFormat, isUnsignedFormat, value[0], buffer);
		len = (int)strlen(buffer);
		if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, buffer, len))
			return err;
		len = (int)strlen(separator);
		if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, separator, len))
			return err;
		numNumbersWrittenPtr += 1;
		
		if (dataType & NT_CMPLX) {
			SPrintFormatted(numericFormat, isIntegerFormat, isUnsignedFormat, value[1], buffer);
			len = (int)strlen(buffer);
			if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, buffer, len))
				return err;
			len = (int)strlen(separator);
			if (err = AddBytesToBuffer(bufPtr, bytesInBufferPtr, separator, len))
				return err;
			numNumbersWrittenPtr += 1;
		}
	}
	
	return 0;
}

extern "C" int
ExecuteGPIBWriteWave2(GPIBWriteWaveRuntimeParamsPtr p)
{
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

	if (err = CheckActiveDevice())
		return err;

	WatchCursor();

	// Flag parameters.
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

	if (p->waveEncountered) {
		int* paramsSet;
		waveHndl* whp;
		IndexInt pointIndex;
		int waveIndex;
		char* bufPtr;
		int bytesInBuffer;
		int len;
		
		bufPtr = (char*)NewPtr(MAX_LINE_LENGTH+1);
		if (bufPtr == NULL)
			return NOMEM;
		
		for(pointIndex=startPoint; pointIndex<=endPoint; pointIndex+=1) {
			paramsSet = p->waveParamsSet;
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
			bytesInBuffer -= (int)strlen(separator);
			if (bytesInBuffer < 0)						// Should never happen.
				bytesInBuffer = 0;
			
			len = (int)strlen(terminator);
			if (len > 0) {
				if (err = AddBytesToBuffer(bufPtr, &bytesInBuffer, terminator, len))
					break;
			}
				
			NIGPIB_ibwrt(gActiveDevice, bufPtr, bytesInBuffer);
			if (err = IBErr(1))
				break;
		}
		
		DisposePtr(bufPtr);
	}
	
	SetV_Flag((double)numNumbersWritten);
	
	if (quiet) {
		if (err == TIME_OUT_WRITE)
			err = 0;
	}

	return err;
}

int
RegisterGPIBWriteWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "GPIBWriteWave2 /F={string:numericFormat[, string:leader, string:separator, string:terminator]} /R=[number:start,number:end] /Q wave[100]:wave";
	runtimeNumVarList = "V_flag;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBWriteWaveRuntimeParams), (void*)ExecuteGPIBWriteWave2, 0);
}
