#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "NIGPIB.h"


// Operation template: GPIBRead2/T=string/N=number/Q varName[100]:varName

// Runtime param structure for GPIBRead2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBReadRuntimeParams {
	// Flag parameters.

	// Parameters for /T flag group.
	int TFlagEncountered;
	Handle TFlagStrH;
	int TFlagParamsSet[1];

	// Parameters for /N flag group.
	int NFlagEncountered;
	double maxChars;
	int NFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int varNameEncountered;
	char varNames[100][MAX_OBJ_NAME+1];
	int varNameParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct GPIBReadRuntimeParams GPIBReadRuntimeParams;
typedef struct GPIBReadRuntimeParams* GPIBReadRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	ReadASCIIBytes(device, buffer, maxBytes, terminators)
	
	Reads bytes until:
		maxBytes have been read.
		A terminator has been read.
		The EOS character as defined in NI-488 driver setup was read.
		The END line was asserted.
		An error occurred.
	
	If the read stops because a terminator was encountered, the terminator is not returned.
	
	The output is null terminated.
	
	Buffer must be capable of holding maxBytes+1 bytes.
	
	The function result is 0 or an NIGPIB error code.
*/
static int
ReadASCIIBytes(int device, char* buffer, BCInt maxBytes, const char* terminators)
{
	int numBytesRead;
	int status;
	char* p;

	numBytesRead = 0;
	p = buffer;
	while(numBytesRead < maxBytes) {
		status = NIGPIB_ibrd(device, p, 1);
		if (status & ERR)
			return IBErr(0);

		numBytesRead += 1;

		if (strchr(terminators,*p) != 0)	// Terminator encountered?
			break;
		
		p += 1;								// Accept this byte into the output.

		if (status & END)					// END or EOS detected (we have no way to know which occurred).
			break;
	}
	
	*p = 0;
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
			if (isdigit(next) || next=='.')
				break;
		}
		else {
			if (isdigit(*p))
				break;
		}
	
		p += 1;
	}
	
	sscanf(p, "%lf", &d1);	
	
	return d1;
}

/*	ExecuteGPIBRead(p)

	ExecuteGPIBRead is called when the user invokes the GPIBRead operation from the
	command line.

	It attempts to read data from gActiveDevice into the string or numeric variable
	or variables specified in the input line.
	
	The syntax is:
		GPIBRead [/T=term/N=n/O=o/Q] str1, str2, var1, var2 . . .
		
	ExecuteGPIBRead sets the variable V_Flag, after creating it if necessary, to the
	number of items read.
*/
extern "C" int
ExecuteGPIBRead(GPIBReadRuntimeParamsPtr p)
{
	int quiet;
	char terminators[256];
	BCInt maxChars;							// Max number of characters to read.
	int numItemsRead;
	char* bufPtr;
	int err = 0;

	if (err = CheckActiveDevice())
		return err;

	WatchCursor();

	quiet = 0;
	strcpy(terminators, ",\t"CR_STR);		// Default terminator for read operations.
	maxChars = 255;							// Default.
	numItemsRead = 0;
	bufPtr = NULL;

	// Flag parameters.

	if (p->TFlagEncountered) {
		if (err = GetCStringFromHandle(p->TFlagStrH, terminators, sizeof(terminators)-1))
			return err;
	}

	if (p->NFlagEncountered)
		maxChars = (BCInt)p->maxChars;

	if (p->QFlagEncountered)
		quiet = 1;
	
	bufPtr = (char*)NewPtr(maxChars+1);
	if (bufPtr == NULL)
		return NOMEM;

	// Main parameters.

	if (p->varNameEncountered) {
		int* paramsSet;
		char* varName;
		int dataType;
		int i;
		
		paramsSet = p->varNameParamsSet;

		for(i=0; i<100; i+=1) {
			if (paramsSet[i] == 0)
				break;						// No more parameters.

			// When called from a user function, varName actually points to binary data.
			varName = (char*)p->varNames[i];

			if (err = VarNameToDataType(varName, &dataType))
				goto done;					// HR, 080704, 1.04: Was return err;
			
			if (dataType == 0) {			// String?
				if (err = ReadASCIIBytes(gActiveDevice, bufPtr, maxChars, terminators))
					goto done;				// HR, 080704, 1.04: Was return err;
			
				if (err = StoreStringDataUsingVarName(varName, bufPtr, strlen(bufPtr)))
					goto done;				// HR, 080704, 1.04: Was return err;
				numItemsRead += 1;
			}
			else {
				double dReal, dImag;
			
				if (err = ReadASCIIBytes(gActiveDevice, bufPtr, maxChars, terminators))
					goto done;				// HR, 080704, 1.04: Was return err;
				dReal = ParseNumber(bufPtr);
				numItemsRead += 1;
				
				if (dataType & NT_CMPLX) {
					if (err = ReadASCIIBytes(gActiveDevice, bufPtr, maxChars, terminators))
						goto done;			// HR, 080704, 1.04: Was return err;
					dImag = ParseNumber(bufPtr);
					numItemsRead += 1;
				}
				else {
					dImag = 0;
				}
				
				if (err = StoreNumericDataUsingVarName(varName, dReal, dImag))
					goto done;				// HR, 080704, 1.04: Was return err;
			}
		}
	}

done:
	if (bufPtr != NULL)
		DisposePtr(bufPtr);
	
	// Set V_Flag.
	SetV_Flag(numItemsRead);

	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}

	return err;
}

int
RegisterGPIBRead2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "GPIBRead2/T=string/N=number/Q varName[100]:varName";
	runtimeNumVarList = "V_flag;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBReadRuntimeParams), (void*)ExecuteGPIBRead, 0);
}

// Operation template: GPIBReadWave2 /Q /R=[number:start,number:end] /N=number:maxChars /T=string:terminator wave[100]:wave

// Runtime param structure for GPIBReadWave operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBReadWaveRuntimeParams {
	// Flag parameters.

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /R flag group.
	int RFlagEncountered;
	double start;
	double end;
	int RFlagParamsSet[2];

	// Parameters for /N flag group.
	int NFlagEncountered;
	double maxChars;
	int NFlagParamsSet[1];

	// Parameters for /T flag group.
	int TFlagEncountered;
	Handle TFlagStrH;
	int TFlagParamsSet[1];

	// Main parameters.

	// Parameters for simple main group #0.
	int waveEncountered;
	waveHndl waves[100];
	int waveParamsSet[100];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct GPIBReadWaveRuntimeParams GPIBReadWaveRuntimeParams;
typedef struct GPIBReadWaveRuntimeParams* GPIBReadWaveRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
ReadWavePointData(waveHndl waveH, IndexInt pointIndex, char* bufPtr, BCInt maxChars, const char* terminators, CountInt* numItemsReadPtr)
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
		
		if (err = ReadASCIIBytes(gActiveDevice, bufPtr, maxChars, terminators))
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
		
		numItemsReadPtr += 1;
	}
	else {
		double value[2];
	
		if (err = ReadASCIIBytes(gActiveDevice, bufPtr, maxChars, terminators))
			return err;
		value[0] = ParseNumber(bufPtr);
		numItemsReadPtr += 1;
		
		if (dataType & NT_CMPLX) {
			if (err = ReadASCIIBytes(gActiveDevice, bufPtr, maxChars, terminators))
				return err;
			value[1] = ParseNumber(bufPtr);
			numItemsReadPtr += 1;
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
ExecuteGPIBReadWave2(GPIBReadWaveRuntimeParamsPtr p)
{
	int quiet;
	char terminators[256];
	BCInt maxChars;							// Max number of characters to read.
	CountInt numItemsRead;
	char* bufPtr;
	waveHndl waveH;
	IndexInt startPoint, endPoint;
	int err = 0;

	if (err = CheckActiveDevice())
		return err;

	WatchCursor();

	quiet = 0;
	strcpy(terminators, ",\t"CR_STR);		// Default terminator for read operations.
	maxChars = 255;							// Default.
	numItemsRead = 0;
	bufPtr = NULL;

	// Flag parameters.

	if (p->QFlagEncountered)
		quiet = 1;

	if (p->NFlagEncountered)
		maxChars = (BCInt)p->maxChars;

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
		if (err = GetCStringFromHandle(p->TFlagStrH, terminators, sizeof(terminators)-1))
			return err;
	}
	
	bufPtr = (char*)NewPtr(maxChars+1);
	if (bufPtr == NULL)
		return NOMEM;

	// Main parameters.

	if (p->waveEncountered) {
		int* paramsSet;
		waveHndl* whp;
		IndexInt pointIndex;
		int waveIndex;
		
		for(pointIndex=startPoint; pointIndex<=endPoint; pointIndex+=1) {
			paramsSet = p->waveParamsSet;
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
				
				if (err = ReadWavePointData(waveH, pointIndex, bufPtr, maxChars, terminators, &numItemsRead))
					break;
					
				if (pointIndex == startPoint)
					WaveHandleModified(waveH);
				
				whp += 1;
			}
			
			if (err != 0)
				break;
		}
	}
	
	if (bufPtr != NULL)
		DisposePtr(bufPtr);
	
	// Set V_Flag.
	SetV_Flag((double)numItemsRead);

	if (quiet) {
		if (err == TIME_OUT_READ)
			err = 0;
	}

	return err;
}

int
RegisterGPIBReadWave2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "GPIBReadWave2 /Q /R=[number:start,number:end] /N=number:maxChars /T=string:terminator wave[100]:wave";
	runtimeNumVarList = "V_flag;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBReadWaveRuntimeParams), (void*)ExecuteGPIBReadWave2, 0);
}
