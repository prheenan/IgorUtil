// XOP1.cpp -- A sample Igor external operation
         
#include "XOPStandardHeaders.h"    // Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "XOP1.h"
         
/*	DoWave(waveHandle)

	Adds 1 to the wave.
*/
static void
DoWave(waveHndl waveH)
{
	float *fDataPtr;	// Pointer to single-precision floating point wave data.
	double *dDataPtr;	// Pointer to double-precision floating point wave data.
	CountInt numPoints;
	IndexInt point;
	
	numPoints = WavePoints(waveH);					// Number of points in wave.
	switch (WaveType(waveH)) {
		case NT_FP32:
			fDataPtr = (float*)WaveData(waveH);		// DEREFERENCE - we must not cause heap to scramble.
			for (point = 0; point < numPoints; point++) {
				*fDataPtr += 1.0;
				fDataPtr += 1;
			}
			break;

		case NT_FP64:
			dDataPtr = (double*)WaveData(waveH);		// DEREFERENCE - we must not cause heap to scramble.
			for (point = 0; point < numPoints; point++) {
				*dDataPtr += 1.0;
				dDataPtr += 1;
			}
			break;
	}
}

/*	XOP1(waveH)

	Carries out operation described at top of file.
	Returns 0 if everything allright or error code otherwise.
*/
static int
XOP1(waveHndl waveH)			// Handle to data structure describing wave.
{
	switch (WaveType(waveH)) {
		case NT_FP32:				// Single precision floating point.
		case NT_FP64:				// Double precision floating point.
			break;
		case TEXT_WAVE_TYPE:			// Don't handle text waves.
			return NO_TEXT_OP;
		default:				// Don't handle complex or integer for now.
			return NT_FNOT_AVAIL;
	}
	
	DoWave(waveH);
	return 0;
}

#pragma pack(2)			// All structures passed to Igor are two-byte aligned.
struct XOP1RuntimeParams {	// We receive this structure from Igor when our operation is invoked.
	// Flag parameters (none).
	
	// Main params.
	
	// wave
	int waveEncountered;
	waveHndl waveH;
	int main1ParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;		// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;		// 1 if called from a macro, 0 otherwise.
};
typedef struct XOP1RuntimeParams XOP1RuntimeParams;
typedef struct XOP1RuntimeParams* XOP1RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	DoXOP1Operation(operationInfo)

	DoXOP1Operation is called when the user invokes the XOP1 operation from the
	command line, from a macro or from a user function.
	
	It returns 0 if everything went OK or error code otherwise.
*/
extern "C" int
ExecuteXOP1(XOP1RuntimeParamsPtr p)
{
	waveHndl waveH;				// Handle to wave's data structure.
	int result;
	
	// Get parameters.
	if (p->main1ParamsSet[0] == 0)
		return NOWAV;			// Wave parameter was not specified.
	waveH = p->waveH;
	if (waveH == NULL)
		return NOWAV;			// User specified a non-existent wave.

	// Do the operation.
	result = XOP1(waveH);
	if (result == 0)
		WaveHandleModified(waveH);	// Tell Igor to update wave in graphs/tables.

	return result;
}

static int
RegisterXOP1(void)		// Called at startup to register this operation with Igor.
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the XOP1RuntimeParams structure as well.
	cmdTemplate = "XOP1 wave";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(XOP1RuntimeParams), (void*)ExecuteXOP1, 0);
}

static int
RegisterOperations(void)	// Register any operations with Igor.
{
	int result;
	
	// Register XOP1 operation.
	if (result = RegisterXOP1())
		return result;
	
	// There are no more operations added by this XOP.
		
	return 0;
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP for all
	messages after the INIT message.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;
	
	switch (GetXOPMessage()) {
		// We don't need to handle any messages for this XOP.
	}
	SetXOPResult(result);
}

/*	XOPMain(ioRecHandle)

	This is the initial entry point through which the Igor calls XOP.
	
	XOPMain does any necessary initialization and then sets the XOPEntry field of the
	ioRecHandle to the address to be called for future messages.
*/
HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)
{
	int result;
	
	XOPInit(ioRecHandle);			// Do standard XOP initialization.
	SetXOPEntry(XOPEntry);			// Set entry point for future calls.
	
	if (igorVersion < 620) {		// Requires Igor Pro 6.20 or later.
		SetXOPResult(OLD_IGOR);		// OLD_IGOR is defined in XOP1.h and there are corresponding error strings in XOP1.r and XOP1WinCustom.rc.
		return EXIT_FAILURE;
	}

	if (result = RegisterOperations()) {
		SetXOPResult(result);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}

    Home Products User Resources Support Order News Search 

