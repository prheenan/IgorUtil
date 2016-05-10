#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

// Operation template: VDTRead2Hex2 /L /O=number:timeOutSeconds /Q /W varName[100]:varNames

// Runtime param structure for VDTRead2Hex2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTReadHex2RuntimeParams {
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
	int varNamesEncountered;
	char varNames[100][MAX_OBJ_NAME+1];		// Optional parameter.
	int varNamesParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTReadHex2RuntimeParams VDTReadHex2RuntimeParams;
typedef struct VDTReadHex2RuntimeParams* VDTReadHex2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
HexNibbleToDecimal(int nibble)		// Assumed to be a valid hex nibble ('0'-'9' or 'A'-'F' or 'a'-'f').
{
	int value;
	
	if (nibble >= 'a')
		nibble -= 'a' - 'A';
	
	if (nibble >= 'A')
		value = nibble - 'A' + 10;
	else
		value = nibble - '0';
	
	return value;
}

static int
Hex2Int(const char* p, int numChars)	// nbytes is the number of hex digits: 2, 4, or 8
{
	int val1, val2;
	int bytesRemaining;
	
	val2 = 0;
	bytesRemaining = numChars;
	while (bytesRemaining--) {
		val2 <<= 4;
		val1 = HexNibbleToDecimal(*p++);
		val2 += val1;
	}
	switch (numChars) {
		case 2:					// 2 hex digits == 1 signed byte
			return (char)val2;
			break;
		case 4:					// 4 hex digits == 2 signed bytes == 1 signed word
			return (short)val2;
			break;
		default:				// 8 hex digits == 4 signed bytes == 1 signed int
			return val2;
	}
	return 0;		// This is only to keep the GNU compiler happy.
}

static unsigned int
Hex2IntUnsigned(const char* p, int numChars)	// numChars is the number of hex digits: 2, 4, or 8
{
	unsigned int val1, val2;
	int bytesRemaining;
	
	val2 = 0;
	bytesRemaining = numChars;
	while (bytesRemaining--) {
		val2 <<= 4;
		val1 = HexNibbleToDecimal(*p++);
		val2 += val1;
	}
	switch (numChars) {
		case 2:					// 2 hex digits == 1 unsigned byte
			return (unsigned char)val2;
			break;
		case 4:					// 4 hex digits == 2 unsigned bytes == 1 unsigned word
			return (unsigned short)val2;
			break;
		default:				// 8 hex digits == 4 unsigned bytes == 1 unsigned int
			return val2;
	}
	return 0;		// This is only to keep the GNU compiler happy.
}

static double
ParseHexNumber(const char* buffer, int numChars, int doUnsigned)	// Contains just the hex data without any garbage.
{
	double result;
	
	if (doUnsigned) {
		result = Hex2IntUnsigned(buffer, numChars);
		return result;
	}
	
	result = Hex2Int(buffer, numChars);
	return result;
}

extern "C" int
ExecuteVDTReadHex2(VDTReadHex2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	int quiet;
	UInt32 timeout;
	int numItemsRead;
	char buffer[32];
	int numChars;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	quiet = 0;
	timeout = 0;
	numItemsRead = 0;
	numChars = 2;						// Two nibbles for one hex byte (0x21)

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

	if (p->varNamesEncountered) {
		int* paramsSet;
		char* varName;
		int dataType;
		int i;
		
		paramsSet = p->varNamesParamsSet;

		for(i=0; i<100; i+=1) {
			if (paramsSet[i] == 0)
				break;						// No more parameters.

			// When called from a user function, varName actually points to binary data.
			varName = (char*)p->varNames[i];

			if (err = VarNameToDataType(varName, &dataType))
				return err;
			
			if (dataType == 0) {			// String?
				return EXPECTED_NUM_VAR_OR_NVAR;
			}
			else {
				double dReal, dImag;
			
				if (err = ReadASCIIBytes(op, timeout, buffer, numChars, ""))
					return err;
				dReal = ParseHexNumber(buffer, numChars, 1);
				numItemsRead += 1;
				
				if (dataType & NT_CMPLX) {
					if (err = ReadASCIIBytes(op, timeout, buffer, numChars, ""))
						return err;
					dImag = ParseHexNumber(buffer, numChars, 1);
					numItemsRead += 1;
				}
				else {
					dImag = 0;
				}
				
				if (err = StoreNumericDataUsingVarName(varName, dReal, dImag))
					return err;
			}
		}
	}
	
	SetV_VDT(numItemsRead);

	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}

	return err;
}

int
RegisterVDTReadHex2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTReadHex2RuntimeParams structure as well.
	cmdTemplate = "VDTReadHex2 /L /O=number:timeOutSeconds /Q /W varName[100]:varNames";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTReadHex2RuntimeParams), (void*)ExecuteVDTReadHex2, 0);
}


// Operation template: VDTReadHexWave2 /L /O=number:timeOutSeconds /Q /R=[number:start,number:end] /W wave[100]:waves

// Runtime param structure for VDTReadHexWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTReadHexWave2RuntimeParams {
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
typedef struct VDTReadHexWave2RuntimeParams VDTReadHexWave2RuntimeParams;
typedef struct VDTReadHexWave2RuntimeParams* VDTReadHexWave2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
ReadHexWavePointData(VDTPortPtr op, waveHndl waveH, int doUnsigned, UInt32 timeout, IndexInt pointIndex, int numChars, CountInt* numItemsReadPtr)
{
	int dataType;
	CountInt indices[MAX_DIMENSIONS];
	char buffer[32];
	int err = 0;
	
	MemClear(indices, sizeof(indices));
	indices[ROWS] = pointIndex;
	
	dataType = WaveType(waveH);
	if (dataType == 0) {			// String?
		return EXPECTED_NUM_WAVE;
	}
	else {
		double value[2];
	
		if (err = ReadASCIIBytes(op, timeout, buffer, numChars, ""))
			return err;
		value[0] = ParseHexNumber(buffer, numChars, doUnsigned);
		*numItemsReadPtr += 1;			// HR, 061017, 1.11: Added missing asterisk.
		
		if (dataType & NT_CMPLX) {
			if (err = ReadASCIIBytes(op, timeout, buffer, numChars, ""))
				return err;
			value[1] = ParseHexNumber(buffer, numChars, doUnsigned);
			*numItemsReadPtr += 1;		// HR, 061017, 1.11: Added missing asterisk.
		}
		else {
			value[1] = 0;
		}
		
		if (err = MDSetNumericWavePointValue(waveH, indices, value))
			return err;
	}
	
	return 0;
}

extern "C" int
ExecuteVDTReadHexWave2(VDTReadHexWave2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	int quiet;
	UInt32 timeout;
	CountInt numItemsRead;
	int numChars;
	IndexInt startPoint, endPoint;
	waveHndl waveH;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	quiet = 0;
	timeout = 0;
	numItemsRead = 0;
	numChars = 2;						// Two nibbles for one hex byte (0x21)
	numItemsRead = 0;

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

	if (p->wavesEncountered) {
		int* paramsSet;
		waveHndl* whp;
		IndexInt pointIndex;
		int waveIndex;
		int doUnsigned;
		
		for(pointIndex=startPoint; pointIndex<=endPoint; pointIndex+=1) {
			paramsSet = p->wavesParamsSet;
			whp = &p->waves[0];

			for(waveIndex=0; waveIndex<100; waveIndex+=1) {
				if (paramsSet[waveIndex] == 0)
					break;						// No more parameters.

				waveH = *whp;
				if (waveH == NULL) {
					// Should never happen.
					err = NULL_WAVE_OP;
					break;
				}
				
				doUnsigned = (WaveType(waveH) & NT_UNSIGNED) != 0;
				if (err = ReadHexWavePointData(op, waveH, doUnsigned, timeout, pointIndex, numChars, &numItemsRead))
					break;

				if (pointIndex == startPoint)
					WaveHandleModified(waveH);	// Tell Igor wave was modified.
				
				whp += 1;
			}
			
			if (err != 0)
				break;
		}
	}
	
	SetV_VDT((double)numItemsRead);

	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}

	return err;
}

int
RegisterVDTReadHexWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTReadHexWave2RuntimeParams structure as well.
	cmdTemplate = "VDTReadHexWave2 /L /O=number:timeOutSeconds /Q /R=[number:start,number:end] /W wave[100]:waves";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTReadHexWave2RuntimeParams), (void*)ExecuteVDTReadHexWave2, 0);
}
