// NIGPIBOperation.c -- Handles the GPIB command line operation.

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "NIGPIB.h"

// Globals.
int gActiveBoard = NO_BOARD;			// The unit descriptor used for interface clear.
int gActiveDevice = NO_DEVICE;			// The unit descriptor used for read/write.
static int gTestMode = 0;
static Handle gTestDataH = NULL;		// Contains data written by GPIB operations while test mode is in effect.

int
NullTerminateHandle(Handle h)		// Adds null terminator to handle.
{
	BCInt numBytes;
	
	numBytes = GetHandleSize(h);
	SetHandleSize(h, numBytes+1);
	if (MemError())
		return NOMEM;
	(*h)[numBytes] = 0;
	
	return 0;
}

int
Init_NIGPIB_IO(void)
{
	gActiveBoard = NO_BOARD;
	gActiveDevice = NO_DEVICE;
	gTestMode = 0;
	return 0;
}

void
Close_NIGPIB_IO(void)
{
	// Nothing to do now.
}

int
SetTestMode(int testMode)
{
	gTestMode = testMode;
	return 0;
}

/*	NIGPIB_ibrd(ud, buf, count)
	
	If test mode is off, just calls regular ibrd.
	
	If test mode is on, returns data from gTestDataH handle via buf.
	
	Sets ibcnt to number of bytes read.
	
	Returns status.
*/
int
NIGPIB_ibrd(int ud, char* buf, NICountInt count)
{
	BCInt numBytesInHandle;
	BCInt numBytesToTransfer;
	
	if (gTestMode == 0)
		return ibrd(ud, buf, count);
	
	if (gTestDataH == NULL)
		numBytesInHandle = 0;
	else
		numBytesInHandle = GetHandleSize(gTestDataH);
	
	numBytesToTransfer = count;
	if (numBytesToTransfer > numBytesInHandle)
		numBytesToTransfer = numBytesInHandle;
		
	if (numBytesToTransfer <= 0) {
		ibsta = ERR;
		iberr = -1;
		ibcnt = 0;
		return ibsta;
	}	

	memcpy(buf, *gTestDataH, numBytesToTransfer);
	ibsta = 0;
	iberr = 0;
	ibcnt = numBytesToTransfer;
	
	memmove(*gTestDataH, *gTestDataH+numBytesToTransfer, numBytesInHandle-numBytesToTransfer);
	SetHandleSize(gTestDataH, numBytesInHandle-numBytesToTransfer);
	
	return 0;
}

/*	NIGPIB_ibwrt(ud, buf, count)
	
	If test mode is off, just calls regular ibwrt.
	
	If test mode is on, stores data in gTestDataH handle.
	
	Returns status.
*/
int
NIGPIB_ibwrt(int ud, char* buf, NICountInt count)
{
	BCInt numBytesInHandle;
	
	if (gTestMode == 0)
		return ibwrt(ud, buf, count);
	
	if (gTestDataH == NULL) {
		gTestDataH = NewHandle(0L);
		if (gTestDataH == NULL) {
			ibsta = ERR;
			iberr = -1;
			return ibsta;
		}
	}

	numBytesInHandle = GetHandleSize(gTestDataH);
	SetHandleSize(gTestDataH, numBytesInHandle + count);
	if (MemError()) {
		ibsta = ERR;
		iberr = -1;
		return ibsta;
	}
	
	memcpy(*gTestDataH + numBytesInHandle, buf, count);
	return 0;
}

static void
DumpTestBufferAsText(void)
{
	BCInt numBytesInHandle;
	int linesDumped;
	char* p1;

	numBytesInHandle = GetHandleSize(gTestDataH);
	if (NullTerminateHandle(gTestDataH))
		return;
	
	p1 = *gTestDataH;
	linesDumped = 0;
	while(*p1 != 0) {
		char* p2;
		char saveChar;
		
		saveChar = 0;
		p2 = strchr(p1, CR_CHAR);
		if (p2 != NULL) {					// Found CR?
			p2 += 1;
			saveChar = *p2;
			*p2 = 0;						// Add null after CR.
		}
		XOPNotice(p1);						// Dump the line.
		if (p2 != NULL) {
			*p2 = saveChar;
		}
		else {
			XOPNotice(CR_STR);				// Buffer did not end with a CR but we add one anyway.
			break;
		}
		p1 = p2;
		
		linesDumped += 1;
		if (linesDumped >= 25)
			break;
	}	

	SetHandleSize(gTestDataH, numBytesInHandle);
}

static void
DumpTestBufferAsBinary(void)
{
	BCInt numBytesInHandle;
	int linesDumped;
	unsigned char* p1;
	char* p2;
	char buffer[256];
	int i;

	numBytesInHandle = GetHandleSize(gTestDataH);
	
	p1 = (unsigned char*)*gTestDataH;
	linesDumped = 0;
	
	for(i=0; i<numBytesInHandle; i+=16) {
		int j;

		p2 = buffer;
		
		sprintf(p2, "%02d: ", i/16);		// Line number
		p2 += strlen(p2);
		
		for(j=0; j<16; j++) {
			if (i + j >= numBytesInHandle)
				break;
		
			sprintf(p2, "%02X", *p1);
			p1 += 1;
			p2 += 2;
			*p2 = ' ';
			p2 += 1;
		}
		
		*p2 = 0;
		p2 -= 1;
		*p2 = CR_CHAR;						// Replace last space with CR.
		
		XOPNotice(buffer);
		
		linesDumped += 1;
		if (linesDumped >= 25)
			break;
	}
}

static void
DumpTestBuffer(void)
{
	char temp[256];
	BCInt numBytesInHandle;
	char* p1;
	int i, isText;
	
	if (gTestDataH == NULL) {
		XOPNotice("NIGPIB2 Test Buffer is NULL."CR_STR);
		return;
	}
	
	numBytesInHandle = GetHandleSize(gTestDataH);
	sprintf(temp, "Number of bytes in buffer: %lld"CR_STR, (SInt64)numBytesInHandle);
	XOPNotice(temp);
	
	if (numBytesInHandle == 0)
		return;
	
	// Determine if buffer contains text or binary.
	isText = 1;
	p1 = *gTestDataH;
	for(i=0; i<numBytesInHandle; i++) {
		unsigned int ch;
		ch = *(unsigned char*)p1++;
		if (ch < 0x20) {					// Less than space?
			if (ch!=CR_CHAR && ch!=LF_CHAR && ch!='\t') {
				// Found a byte below 0x20 and it is not a CR, LF or tab.
				isText = 0;
				break;
			}
		}
	}
	
	if (isText)
		DumpTestBufferAsText();
	else
		DumpTestBufferAsBinary();
}

int
CheckActiveDevice(void)		// Returns error code if the user did not specify what GPIB device he is talking to.
{
	if (gTestMode != 0)						// Don't need device if test mode is on.
		return 0;
	if (gActiveDevice == NO_DEVICE)
		return(NO_DEVICE_ERR);
	return 0;
}

static int
InterfaceClear(void)
{
	if (gActiveBoard == NO_BOARD)
		return(NO_BOARD_ERR);
	
	ibsic(gActiveBoard);
	return(IBErr(1));
}

static int
DeviceClear(void)
{
	int err;

	if (err = CheckActiveDevice())
		return err;
	
	ibclr(gActiveDevice);
	return(IBErr(1));
}

static int
ReadSerialPoll(char *buf)
{
	int err;

	if (err = CheckActiveDevice())
		return err;
	
	ibrsp(gActiveDevice, buf);
	return(IBErr(0));
}

static int
GotoLocal(void)
{
	int err;

	if (err = CheckActiveDevice())
		return err;
	
	ibloc(gActiveDevice);
	return(IBErr(1));
}

// Operation template: GPIB2/Q board=number:ud, device=number:ud, InterfaceClear, KillIO, DeviceClear, GotoLocal, ReadSerialPoll, testMode=number:mode, DumpTestBuffer

// Runtime param structure for GPIB2 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GPIBRuntimeParams {
	// Flag parameters.

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Main parameters.

	// Parameters for board keyword group.
	int boardEncountered;
	double board_ud;
	int boardParamsSet[1];

	// Parameters for device keyword group.
	int deviceEncountered;
	double device_ud;
	int deviceParamsSet[1];

	// Parameters for InterfaceClear keyword group.
	int InterfaceClearEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for KillIO keyword group.
	int KillIOEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for DeviceClear keyword group.
	int DeviceClearEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for GotoLocal keyword group.
	int GotoLocalEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for ReadSerialPoll keyword group.
	int ReadSerialPollEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for testMode keyword group.
	int testModeEncountered;
	double testMode_mode;
	int testModeParamsSet[1];

	// Parameters for DumpTestBuffer keyword group.
	int DumpTestBufferEncountered;
	// There are no fields for this group because it has no parameters.

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct GPIBRuntimeParams GPIBRuntimeParams;
typedef struct GPIBRuntimeParams* GPIBRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteGPIB2(GPIBRuntimeParamsPtr p)
{
	int quiet;
	int err = 0;

	// Flag parameters.
	
	quiet = 0;

	if (p->QFlagEncountered)
		quiet = 1;

	// Main parameters.

	if (p->boardEncountered) {
		gActiveBoard = (int)p->board_ud;		// Used for InterfaceClear, killio operation.
		ibwait(gActiveBoard, 0);				// See if board or device exists.
		if (ibsta & ERR) {
			err = NO_SUCH_BOARD;
			goto done;
		}
	}

	if (p->deviceEncountered) {
		gActiveDevice = (int)p->device_ud;		// Used for GPIB read/write operations.
		ibwait(gActiveDevice, 0);				// See if board or device exists.
		if (ibsta & ERR) {
			err = NO_SUCH_DEVICE;
			goto done;
		}
	}

	if (p->InterfaceClearEncountered) {
		if (err = InterfaceClear())
			goto done;
	}
		
	if (p->KillIOEncountered) {
		if (gTestDataH != NULL)
			SetHandleSize(gTestDataH, 0L);
		
		if (gTestMode == 0) {
			if (err = InterfaceClear())
				goto done;
		}
	}

	if (p->DeviceClearEncountered) {
		if (err = DeviceClear())
			goto done;
	}

	if (p->GotoLocalEncountered) {
		if (err = GotoLocal())
			goto done;
	}

	if (p->ReadSerialPollEncountered) {
		double d1;
		char temp[32];

		if (err = ReadSerialPoll(temp))
			goto done;
		d1 = *(unsigned char *)temp;
		SetOperationNumVar("V_spr", d1);
	}

	if (p->testModeEncountered) {
		if (err = SetTestMode((int)p->testMode_mode))
			goto done;
	}

	if (p->DumpTestBufferEncountered)
		DumpTestBuffer();

done:
	if (quiet) {
		SetV_Flag(err);
		if (err == TIME_OUT_READ || err == TIME_OUT_WRITE)
			err = 0;
	}

	return err;
}

int
RegisterGPIB2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "GPIB2/Q board=number:ud, device=number:ud, InterfaceClear, KillIO, DeviceClear, GotoLocal, ReadSerialPoll, testMode=number:mode, DumpTestBuffer";
	runtimeNumVarList = "V_flag;V_spr;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GPIBRuntimeParams), (void*)ExecuteGPIB2, 0);
}
