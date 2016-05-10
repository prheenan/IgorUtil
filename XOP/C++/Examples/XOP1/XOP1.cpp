// XOP1.cpp -- A sample Igor external operation
         
#include "XOPStandardHeaders.h"    // Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "XOP1.h"
         
/*	DoWave(waveHandle)

	Adds 1 to the wave.
*/
static void
DoWave(waveHndl waveH)
{
        // Pointer to single-precision floating point wave data.
	float *fDataPtr;
	// Pointer to double-precision floating point wave data.
	double *dDataPtr;	
	CountInt numPoints;
	IndexInt point;
	// Number of points in wave.
	numPoints = WavePoints(waveH);					
	switch (WaveType(waveH)) {
		case NT_FP32:
    		        // DEREFERENCE - we must not cause heap to scramble.
			fDataPtr = (float*)WaveData(waveH);		
			for (point = 0; point < numPoints; point++) {
				*fDataPtr += 1.0;
				fDataPtr += 1;
			}
			break;

		case NT_FP64:
		        // DEREFERENCE - we must not cause heap to scramble.
			dDataPtr = (double*)WaveData(waveH);		
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
		case NT_FP32:
		  // Single precision floating point.
		case NT_FP64:
		  // Double precision floating point.
			break;
		case TEXT_WAVE_TYPE:
		  // Don't handle text waves.
			return NO_TEXT_OP;
		default:
		  // Don't handle complex or integer for now.
			return NT_FNOT_AVAIL;
	}
	
	DoWave(waveH);
	return 0;
}

// All structures passed to Igor are two-byte aligned.
#pragma pack(2)

struct XOP1RuntimeParams {
  // We receive this structure from Igor when our operation is invoked.
	// Flag parameters (none).
	
	// Main params.
	
	// wave
	int waveEncountered;
	waveHndl waveH;
	int main1ParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;
  // 1 if called from a user function, 0 otherwise.
	int calledFromMacro;
  // 1 if called from a macro, 0 otherwise.
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
  // Handle to wave's data structure.
	waveHndl waveH;				
	int result;
	
	// Get parameters.
	if (p->main1ParamsSet[0] == 0)
	  // Wave parameter was not specified.
		return NOWAV;			
	waveH = p->waveH;
	if (waveH == NULL)
	  // User specified a non-existent wave.
		return NOWAV;			

	// Do the operation.
	result = XOP1(waveH);
	if (result == 0)
	  // Tell Igor to update wave in graphs/tables.
		WaveHandleModified(waveH);	

	return result;
}

static int
// Called at startup to register this operation with Igor.
RegisterXOP1(void)		
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the
	// XOP1RuntimeParams structure as well.
	cmdTemplate = "XOP1 wave";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList,
				 runtimeStrVarList, sizeof(XOP1RuntimeParams),
				 (void*)ExecuteXOP1, 0);
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
	
	XOPMain does any necessary initialization and then sets the XOPEntry 
	field of the ioRecHandle to the address to be called for future messages
*/
HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)
{
	int result;

	// Do standard XOP initialization.
	XOPInit(ioRecHandle);
	// Set entry point for future calls.
	SetXOPEntry(XOPEntry);			
	
	if (igorVersion < 620) {
	  // Requires Igor Pro 6.20 or later.
	  // OLD_IGOR is defined in XOP1.h and there are corresponding error
	  // strings in XOP1.r and XOP1WinCustom.rc.
		SetXOPResult(OLD_IGOR);		
		return EXIT_FAILURE;
	}

	if (result = RegisterOperations()) {
		SetXOPResult(result);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}


