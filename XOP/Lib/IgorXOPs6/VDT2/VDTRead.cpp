#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "VDT.h"


// Operation template: VDTRead2 /N=number:maxChars /O=number:timeOutSeconds /Q /T=string:terminator varName[100]:varNames

// Runtime param structure for VDTRead2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTRead2RuntimeParams {
	// Flag parameters.

	// Parameters for /N flag group.
	int NFlagEncountered;
	double maxChars;
	int NFlagParamsSet[1];

	// Parameters for /O flag group.
	int OFlagEncountered;
	double timeOutSeconds;
	int OFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /T flag group.
	int TFlagEncountered;
	Handle terminator;
	int TFlagParamsSet[1];

	// Main parameters.

	// Parameters for simple main group #0.
	int varNamesEncountered;
	char varNames[100][MAX_OBJ_NAME+1];		// Optional parameter.
	int varNamesParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTRead2RuntimeParams VDTRead2RuntimeParams;
typedef struct VDTRead2RuntimeParams* VDTRead2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	ReadASCIIBytes(pp, timeout, buffer, maxBytes, terminators)
	
	Reads bytes until:
		maxBytes have been read.
		A terminator has been read.
		An error occurred.
	
	If the read stops because a terminator was encountered, the terminator is not returned.
	
	The output is null terminated.
	
	Buffer must be capable of holding maxBytes+1 bytes.

	timeout is the absolute tick count by which the read must be completed or 0 for no timeout.
	
	The function result is 0 or an VDT error code.
*/
int
ReadASCIIBytes(VDTPortPtr pp, UInt32 timeout, char* buffer, VDTByteCount maxBytes, const char* terminators)
{
	int numBytesRead;
	char* p;
	VDTByteCount count;
	int err;

	numBytesRead = 0;
	p = buffer;
	while(numBytesRead < maxBytes) {
		count = 1;
		if (err = SerialRead(pp, timeout, p, &count)) {
			*p = 0;							// HR, 061018, 1.12: If error is TIME_OUT_READ, return bytes read so far.
			return err;
		}

		numBytesRead += 1;

		if (strchr(terminators,*p) != 0)	// Terminator encountered?
			break;
		
		p += 1;								// Accept this byte into the output.
	}
	
	*p = 0;
	return 0;
}

static int
IsDigit(int ch)
{
	if (ch>='0' && ch<='9')
		return 1;
	return 0;
}

/*	ParseNumber(buf)

	Skips any leading non-numeric characters.
*/
static double
ParseNumber(const char* buf)
{
	const char* p;
	double d1;

	p = buf;
	
	// Skip non-numeric characters first.
	while (*p != 0) {
		if (*p=='+' || *p=='-') {
			int next;
			next = *(p+1);
			if (IsDigit(next) || next=='.')
				break;
		}
		else {
			if (IsDigit(*p))
				break;
		}
	
		p += 1;
	}
	
	if (sscanf(p, "%lf", &d1) != 1)
		SetNaN64(&d1);					// HR, 040211, 1.02: Set to NaN if no number read.
	
	return d1;
}

/*	ExecuteVDTRead2(p)

	ExecuteVDTRead2 is called when the user invokes the VDTRead2 operation from the
	command line.

	It attempts to read data from the operations port into the string or numeric variable
	or variables specified in the input line.
	
	The syntax is:
		VDTRead2 [/T=term/N=n/O=o/Q] str1, str2, var1, var2 . . .
		
	ExecuteVDTRead2 sets the variable V_Flag, after creating it if necessary, to the
	number of items read.
*/
extern "C" int
ExecuteVDTRead2(VDTRead2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	int quiet;
	char terminators[256];
	UInt32 timeout;
	VDTByteCount maxChars;					// Max number of characters to read.
	int numItemsRead;
	char* bufPtr;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	quiet = 0;
	timeout = 0;
	strcpy(terminators, ",\t"CR_STR);		// Default terminator for read operations.
	maxChars = 255;							// Default.
	numItemsRead = 0;

	// Flag parameters.

	if (p->NFlagEncountered)
		maxChars = (VDTByteCount)p->maxChars;

	if (p->OFlagEncountered) {
		if (p->timeOutSeconds == 0)
			timeout = 0;
		else
			timeout = TickCount() + (UInt32)(60*p->timeOutSeconds);
	}

	if (p->QFlagEncountered)
		quiet = 1;

	if (p->TFlagEncountered) {
		if (err = GetCStringFromHandle(p->terminator, terminators, sizeof(terminators)-1))
			return err;
	}
	
	bufPtr = (char*)NewPtr(maxChars+1);
	if (bufPtr == NULL)
		return NOMEM;

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
				goto done;
			
			if (dataType == 0) {			// String?
				int gotTimeout;				// HR, 061018, 1.12: If error is TIME_OUT_READ, return bytes read so far.
				int gotAbort;				// HR, 061018, 1.12: If error is -1 (abort), return bytes read so far.
				
				gotTimeout = 0;
				gotAbort = 0;
				err = ReadASCIIBytes(op, timeout, bufPtr, maxChars, terminators);
				if (err == TIME_OUT_READ) {
					err = 0;
					gotTimeout = 1;
				}
				if (err == -1) {
					err = 0;
					gotAbort = 1;
				}
					
				if (err == 0) {
					err = StoreStringDataUsingVarName(varName, bufPtr, strlen(bufPtr));
					if (err==0 && gotTimeout==0 && gotAbort==0)
						numItemsRead += 1;
				}
				
				if (gotTimeout)
					err = TIME_OUT_READ;
				if (gotAbort)
					err = -1;
				
				if (err != 0)
					goto done;
			}
			else {
				double dReal, dImag;
			
				if (err = ReadASCIIBytes(op, timeout, bufPtr, maxChars, terminators))
					goto done;
				dReal = ParseNumber(bufPtr);
				numItemsRead += 1;
				
				if (dataType & NT_CMPLX) {
					if (err = ReadASCIIBytes(op, timeout, bufPtr, maxChars, terminators))
						goto done;
					dImag = ParseNumber(bufPtr);
					numItemsRead += 1;
				}
				else {
					dImag = 0;
				}
				
				if (err = StoreNumericDataUsingVarName(varName, dReal, dImag))
					goto done;
			}
		}
	}

done:						// HR, 040211, 1.02: Previously there were returns above that did not clean up properly.
	DisposePtr(bufPtr);
	
	// Set V_VDT.
	SetV_VDT(numItemsRead);

	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}

	return err;
}

int
RegisterVDTRead2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "VDTRead2 /N=number:maxChars /O=number:timeOutSeconds /Q /T=string:terminator varName[100]:varNames";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTRead2RuntimeParams), (void*)ExecuteVDTRead2, 0);
}

// Operation template: VDTReadWave2 /N=number:maxChars /O=number:timeOutSeconds /Q /R=[number:start,number:end] /T=string:terminator wave[100]:waves

// Runtime param structure for VDTReadWave2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct VDTReadWave2RuntimeParams {
	// Flag parameters.

	// Parameters for /N flag group.
	int NFlagEncountered;
	double maxChars;
	int NFlagParamsSet[1];

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

	// Parameters for /T flag group.
	int TFlagEncountered;
	Handle terminator;
	int TFlagParamsSet[1];

	// Main parameters.

	// Parameters for simple main group #0.
	int wavesEncountered;
	waveHndl waves[100];					// Optional parameter.
	int wavesParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTReadWave2RuntimeParams VDTReadWave2RuntimeParams;
typedef struct VDTReadWave2RuntimeParams* VDTReadWave2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
ReadWavePointData(VDTPortPtr op, waveHndl waveH, UInt32 timeout, IndexInt pointIndex, char* bufPtr, VDTByteCount maxChars, const char* terminators, CountInt* numItemsReadPtr)
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
		
		if (err = ReadASCIIBytes(op, timeout, bufPtr, maxChars, terminators))
			return err;

		len = (int)strlen(bufPtr);
		textH = NewHandle(len);
		if (textH == NULL)
			return NOMEM;	
		memcpy(*textH, bufPtr, len);
	
		err = MDSetTextWavePointValue(waveH, indices, textH);
		
		DisposeHandle(textH);

		if (err != 0)
			return err;
		
		*numItemsReadPtr += 1;			// HR, 061017, 1.11: Added missing asterisk.
	}
	else {
		double value[2];
	
		if (err = ReadASCIIBytes(op, timeout, bufPtr, maxChars, terminators))
			return err;
		value[0] = ParseNumber(bufPtr);
		*numItemsReadPtr += 1;			// HR, 061017, 1.11: Added missing asterisk.
		
		if (dataType & NT_CMPLX) {
			if (err = ReadASCIIBytes(op, timeout, bufPtr, maxChars, terminators))
				return err;
			value[1] = ParseNumber(bufPtr);
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
ExecuteVDTReadWave2(VDTReadWave2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	UInt32 timeout;
	int quiet;
	char terminators[256];
	VDTByteCount maxChars;					// Max number of characters to read.
	CountInt numItemsRead;
	char* bufPtr;
	waveHndl waveH;
	IndexInt startPoint, endPoint;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	WatchCursor();

	quiet = 0;
	timeout = 0;
	strcpy(terminators, ",\t"CR_STR);		// Default terminator for read operations.
	maxChars = 255;							// Default.
	numItemsRead = 0;

	// Flag parameters.

	if (p->NFlagEncountered)
		maxChars = (VDTByteCount)p->maxChars;

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

	if (p->TFlagEncountered) {
		if (err = GetCStringFromHandle(p->terminator, terminators, sizeof(terminators)-1))
			return err;
	}
	
	bufPtr = (char*)NewPtr(maxChars+1);
	if (bufPtr == NULL)
		return NOMEM;

	// Main parameters.

	if (p->wavesEncountered) {
		int* paramsSet;
		waveHndl* whp;
		IndexInt pointIndex;
		int waveIndex;
		
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
				
				if (err = ReadWavePointData(op, waveH, timeout, pointIndex, bufPtr, maxChars, terminators, &numItemsRead))
					break;
				
				if (pointIndex==startPoint)
					WaveHandleModified(waveH);	// Tell Igor wave was modified.
				
				whp += 1;
			}
			
			if (err != 0)
				break;
		}
	}

	DisposePtr(bufPtr);
	
	// Set V_Flag.
	SetV_VDT((double)numItemsRead);

	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}

	return err;
}

int
RegisterVDTReadWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "VDTReadWave2 /N=number:maxChars /O=number:timeOutSeconds /Q /R=[number:start,number:end] /T=string:terminator wave[100]:waves";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTReadWave2RuntimeParams), (void*)ExecuteVDTReadWave2, 0);
}
