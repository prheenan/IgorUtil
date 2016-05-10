#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

static void
SetTerminalPortOffLineIfConflict(void)
{
	VDTPortPtr tp;

	tp = VDTGetTerminalPortPtr();
	if (tp != NULL) {
		if (tp == VDTGetOperationsPortPtr())
			VDTSetTerminalPortPtr(NULL);				// Set terminal port to "Off Line".
	}
}

static VDTPortPtr gCurPort = NULL;		// Pointer to VDTPort used for all I/O operations when an explicit port is not specified.

VDTPortPtr
VDTGetOperationsPortPtr(void)			// Returns pointer to VDTPort for port being for command line operations, or NULL.
{
	return gCurPort;
}

void
VDTSetOperationsPortPtr(VDTPortPtr pp)	// Sets VDTPort to be used for command line operations. Can be NULL.
{
	if (pp == gCurPort)
		return;
	
	gCurPort = pp;
	UpdateStatusArea();
}

void
SetV_VDT(double number)		// This should be called only while executing an operation.
{
	SetOperationNumVar("V_VDT", number);
}

/*	VDTGetOpenAndCheckOperationsPortPtr(ppp, setTerminalPortOffLineIfConflict, displayDialogIfPortSelectedButNotOpenable)
	
	This is called when we are about to attempt to use the operations port.
	
	If no operations port is selected, it displays a dialog (if requested) returns NULL via ppp.
	
	Otherwise, if the operations port is closed, it tries to open it. If this
	results in an error it displays a dialog (if requested) and returns NULL via ppp.

	If the port was open already or was successfully opened here, it returns
	a non-NULL VDTPortPtr via ppp.
	
	If there are no errors and if setTerminalPortOffLineIfConflict is true and if the
	terminal port is the same as the operations port, it sets the terminal port to
	"Off Line".
	
	The function result is 0 if OK or an error code.
*/
int
VDTGetOpenAndCheckOperationsPortPtr(VDTPortPtr* ppp, int setTerminalPortOffLineIfConflict, int displayDialogIfPortSelectedButNotOpenable)
{
	VDTPortPtr op;
	int err;
	
	*ppp = NULL;
	
	op = VDTGetOperationsPortPtr();
	if (op == NULL) {
		if (displayDialogIfPortSelectedButNotOpenable)
			ErrorAlert(VDT_ERR_NO_OPERATIONS_PORT_SELECTED);
		return VDT_ERR_NO_OPERATIONS_PORT_SELECTED;
	}
	
	if (VDTPortIsOpen(op)) {
		*ppp = op;
		if (setTerminalPortOffLineIfConflict)
			SetTerminalPortOffLineIfConflict();
		return 0;
	}
	
	err = OpenVDTPort(op->name, ppp);
	if (err == 0) {
		if (setTerminalPortOffLineIfConflict)
			SetTerminalPortOffLineIfConflict();
		return 0;
	}
	
	*ppp = NULL;			// Port can't be opened.
	
	if (displayDialogIfPortSelectedButNotOpenable)
		ErrorAlert(err);
	
	return err;
}

/*	CheckAndTranslatePortName(portNameIn, portNameOut, allowNone)

	Returns via portNameOut a valid portName, or None if allowNone is true.
	Does translation to allow platform independence.
	
	Returns 0 if OK or error code.
*/
static int
CheckAndTranslatePortName(const char* portNameIn, char* portNameOut, int allowNone)
{
	VDTPortPtr pp;
	int errorCode;
	
	strcpy(portNameOut, portNameIn);
	
	errorCode = allowNone ? VDT_ERR_EXPECTED_PORTNAME_OR_NONE:VDT_ERR_EXPECTED_PORTNAME;

	if (CmpStr(portNameIn, "None") == 0) {
		if (!allowNone)
			return VDT_ERR_EXPECTED_PORTNAME;
		return 0;	
	}

	TranslatePortName(portNameOut);				// Do platform-related port name translation and handle other name equivalences.
	
	pp = FindVDTPort(NULL, portNameOut);
	if (pp == NULL)
		return errorCode;

	return 0;
}

static void
StorePortNameInS_VDT(int whichPort)				// 0 = terminal port, 1 = operations port.
{
	VDTPortPtr pp;
	char portName[MAX_OBJ_NAME+1];

	if (whichPort == 0)
		pp = VDTGetTerminalPortPtr();			// This may be null.
	else
		pp = VDTGetOperationsPortPtr();			// This may be null.
	
	if (pp == NULL)
		strcpy(portName, "None");
	else
		strcpy(portName, pp->name);
	
	SetOperationStrVar("S_VDT", portName);
}

// Operation template: VDTGetPortList2

// Runtime param structure for VDTGetPortList2 operation.
#pragma pack(2)			// All structures passed between Igor and XOP are two-byte aligned.
struct VDTGetPortList2RuntimeParams {
	// Flag parameters.

	// Main parameters.

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTGetPortList2RuntimeParams VDTGetPortList2RuntimeParams;
typedef struct VDTGetPortList2RuntimeParams* VDTGetPortList2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteVDTGetPortList2(VDTGetPortList2RuntimeParamsPtr p)
{
	char list[1024];
	char name[MAX_OBJ_NAME+1];
	char inputChan[MAX_OBJ_NAME+1];
	char outputChan[MAX_OBJ_NAME+1];
	int nameLength, listLength;
	int index;
	int err = 0;
	
	*list = 0;
	listLength = 0;
	index = 0;
	while(1) {
		if (GetIndexedPortNameAndChannelInfo(index, name, inputChan, outputChan) != 0)
			break;
		
		nameLength = (int)strlen(name);
		if (listLength + nameLength + 1 >= sizeof(list))
			return STR_TOO_LONG;
		
		sprintf(list+listLength, "%s;", name);
		listLength += nameLength + 1;
		
		index += 1;
	}
	
	SetOperationStrVar("S_VDT", list);
	
	return err;
}

int
RegisterVDTGetPortList2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTGetPortList2RuntimeParams structure as well.
	cmdTemplate = "VDTGetPortList2";
	runtimeNumVarList = "";
	runtimeStrVarList = "S_VDT;";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTGetPortList2RuntimeParams), (void*)ExecuteVDTGetPortList2, 0);
}


// Operation template: VDTOpenPort2 name:portName

// Runtime param structure for VDTOpenPort2 operation.
#pragma pack(2)			// All structures passed between Igor and XOP are two-byte aligned.
struct VDTOpenPort2RuntimeParams {
	// Flag parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int portNameEncountered;
	char portName[MAX_OBJ_NAME+1];
	int portNameParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTOpenPort2RuntimeParams VDTOpenPort2RuntimeParams;
typedef struct VDTOpenPort2RuntimeParams* VDTOpenPort2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteVDTOpenPort2(VDTOpenPort2RuntimeParamsPtr p)
{
	VDTPortPtr pp;
	char portNameOut[MAX_OBJ_NAME+1];
	int err = 0;

	if (err = CheckAndTranslatePortName(p->portName, portNameOut, 0))
		return err;
	
	err = OpenVDTPort(portNameOut, &pp);

	return err;
}

int
RegisterVDTOpenPort2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTOpenPort2RuntimeParams structure as well.
	cmdTemplate = "VDTOpenPort2 name:portName";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTOpenPort2RuntimeParams), (void*)ExecuteVDTOpenPort2, 0);
}

// Operation template: VDTClosePort2 name:portName

// Runtime param structure for VDTClosePort2 operation.
#pragma pack(2)			// All structures passed between Igor and XOP are two-byte aligned.
struct VDTClosePort2RuntimeParams {
	// Flag parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int portNameEncountered;
	char portName[MAX_OBJ_NAME+1];
	int portNameParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTClosePort2RuntimeParams VDTClosePort2RuntimeParams;
typedef struct VDTClosePort2RuntimeParams* VDTClosePort2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteVDTClosePort2(VDTClosePort2RuntimeParamsPtr p)
{
	char portNameOut[MAX_OBJ_NAME+1];
	int err = 0;

	if (err = CheckAndTranslatePortName(p->portName, portNameOut, 0))
		return err;

	err = CloseVDTPort(portNameOut);

	return err;
}

int
RegisterVDTClosePort2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTClosePort2RuntimeParams structure as well.
	cmdTemplate = "VDTClosePort2 name:portName";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTClosePort2RuntimeParams), (void*)ExecuteVDTClosePort2, 0);
}

// Operation template: VDTTerminalPort2 [name:portName]

// Runtime param structure for VDTTerminalPort2 operation.
#pragma pack(2)			// All structures passed between Igor and XOP are two-byte aligned.
struct VDTTerminalPort2RuntimeParams {
	// Flag parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int portNameEncountered;
	char portName[MAX_OBJ_NAME+1];			// Optional parameter.
	int portNameParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTTerminalPort2RuntimeParams VDTTerminalPort2RuntimeParams;
typedef struct VDTTerminalPort2RuntimeParams* VDTTerminalPort2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteVDTTerminalPort2(VDTTerminalPort2RuntimeParamsPtr p)
{
	VDTPortPtr pp;
	char portNameOut[MAX_OBJ_NAME+1];
	int err = 0;

	StorePortNameInS_VDT(0);							// Store name of terminal port.

	if (p->portNameEncountered) {
		if (err = CheckAndTranslatePortName(p->portName, portNameOut, 1))
			return err;
		pp = FindVDTPort(NULL, portNameOut);			// NULL if setting port to None (offline).
		VDTSetTerminalPortPtr(pp);
	}

	return err;
}

int
RegisterVDTTerminalPort2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTTerminalPort2RuntimeParams structure as well.
	cmdTemplate = "VDTTerminalPort2 [name:portName]";
	runtimeNumVarList = "";
	runtimeStrVarList = "S_VDT;";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTTerminalPort2RuntimeParams), (void*)ExecuteVDTTerminalPort2, 0);
}

// Operation template: VDTOperationsPort2 [name:portName]

// Runtime param structure for VDTOperationsPort2 operation.
#pragma pack(2)			// All structures passed between Igor and XOP are two-byte aligned.
struct VDTOperationsPort2RuntimeParams {
	// Flag parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int portNameEncountered;
	char portName[MAX_OBJ_NAME+1];			// Optional parameter.
	int portNameParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTOperationsPort2RuntimeParams VDTOperationsPort2RuntimeParams;
typedef struct VDTOperationsPort2RuntimeParams* VDTOperationsPort2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteVDTOperationsPort2(VDTOperationsPort2RuntimeParamsPtr p)
{
	VDTPortPtr pp;
	char portNameOut[MAX_OBJ_NAME+1];
	int err = 0;

	StorePortNameInS_VDT(1);							// Store name of operations port.

	if (p->portNameEncountered) {
		if (err = CheckAndTranslatePortName(p->portName, portNameOut, 1))
			return err;
		pp = FindVDTPort(NULL, portNameOut);			// NULL if setting port to None (offline).
		VDTSetOperationsPortPtr(pp);
	}

	return err;
}

int
RegisterVDTOperationsPort2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTOperationsPort2RuntimeParams structure as well.
	cmdTemplate = "VDTOperationsPort2 [name:portName]";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTOperationsPort2RuntimeParams), (void*)ExecuteVDTOperationsPort2, 0);
}

// Operation template: VDT2 /P=name:portName abort, baud=number:baud, buffer=number:bufferSize, databits=number:databits, echo=number:echo, in=number:inputHandshake, killio, line=number:online, out=number:outputHandshake, parity=number:parity, port=number:port, stopbits=number:stopbits, terminalEOL=number:terminalEOL, rts=number:rts, testMode=number:testMode, DumpTestBuffer

// Runtime param structure for VDT2 operation.
#pragma pack(2)			// All structures passed between Igor and XOP are two-byte aligned.
struct VDT2RuntimeParams {
	// Flag parameters.

	// Parameters for /P flag group.
	int PFlagEncountered;
	char portName[MAX_OBJ_NAME+1];
	int PFlagParamsSet[1];

	// Main parameters.

	// Parameters for abort keyword group.
	int abortEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for baud keyword group.
	int baudEncountered;
	double baud;
	int baudParamsSet[1];

	// Parameters for buffer keyword group.
	int bufferSizeEncountered;
	double bufferSize;
	int bufferSizeParamsSet[1];

	// Parameters for databits keyword group.
	int databitsEncountered;
	double databits;
	int databitsParamsSet[1];

	// Parameters for echo keyword group.
	int echoEncountered;
	double echo;
	int echoParamsSet[1];

	// Parameters for in keyword group.
	int inEncountered;
	double inputHandshake;
	int inParamsSet[1];

	// Parameters for killio keyword group.
	int killioEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for line keyword group.
	int lineEncountered;
	double online;
	int lineParamsSet[1];

	// Parameters for out keyword group.
	int outEncountered;
	double outputHandshake;
	int outParamsSet[1];

	// Parameters for parity keyword group.
	int parityEncountered;
	double parity;
	int parityParamsSet[1];

	// Parameters for port keyword group.
	int portEncountered;
	double port;
	int portParamsSet[1];

	// Parameters for stopbits keyword group.
	int stopbitsEncountered;
	double stopbits;
	int stopbitsParamsSet[1];

	// Parameters for terminalEOL keyword group.
	int terminalEOLEncountered;
	double terminalEOL;
	int terminalEOLParamsSet[1];

	// Parameters for rts keyword group.
	int rtsEncountered;
	double rts;
	int rtsParamsSet[1];

	// Parameters for testMode keyword group.
	int testModeEncountered;
	double testMode;
	int testModeParamsSet[1];

	// Parameters for DumpTestBuffer keyword group.
	int DumpTestBufferEncountered;
	// There are no fields for this group because it has no parameters.

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDT2RuntimeParams VDT2RuntimeParams;
typedef struct VDT2RuntimeParams* VDT2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	ExecuteVDT2()

	ExecuteVDT2 is called when the user invokes the VDT2 operation from the command line.
	
	VDT2 applies to the port specified by the /P=<port> flag or, if there is no,
	/P flag, to the currently-selected operations port.
*/
extern "C" int
ExecuteVDT2(VDT2RuntimeParamsPtr p)
{
	VDTPortPtr fp;					// References the port specified by /P flag if not NULL.
	VDTPortPtr pp;
	char portNameOut[MAX_OBJ_NAME+1];
	int commSettingsChanged;
	int err = 0;

	*portNameOut = 0;
	fp = NULL;
	pp = NULL;
	commSettingsChanged = 0;

	if (p->PFlagEncountered) {
		if (err = CheckAndTranslatePortName(p->portName, portNameOut, 0))
			return err;
		fp = FindVDTPort(NULL, portNameOut);			// NULL if setting port to None (offline).
		if (fp == NULL)
			return VDT_ERR_EXPECTED_PORTNAME;
	}

	// Main parameters.
	
	if (p->abortEncountered)
		VDTAbortTerminalOperation();

	if (p->testModeEncountered) {
		if (err = SetTestMode((int)p->testMode))
			return err;
	}

	if (p->DumpTestBufferEncountered)
		DumpTestBuffer();

	// This sets the operations port. We do this before any other keywords that act on a particular port.
	if (p->portEncountered) {
		VDTPortPtr op;
		if (err = GetNumberedVDTPort((int)p->port, &op))		// Find port to which num refers.
			return err;
		VDTSetOperationsPortPtr(op);
	}

	/*	For all of the remaining keywords, we must have a valid port. It can be
		explicitly specified by /P or, if not explicitly specified, we use the
		designated operations port.
	*/
	if (fp != NULL) {
		pp = fp;								// Port specified by explicit /P=<portName> flag.
	}
	else {
		VDTPortPtr op;
		op = VDTGetOperationsPortPtr();				// Get current operations port pointer - may be null.
		if (op == NULL)
			return VDT_ERR_NO_OPERATIONS_PORT_SELECTED;
		if (op->terminalOp)							// If terminal operation in progress,
			return OP_IN_PROGRESS;					// Can't mess with port while operation is in progress. Do abort first.
		pp = op;
	}

	if (p->killioEncountered)
		KillVDTIO(pp);

	if (p->bufferSizeEncountered) {
		if (p->bufferSize>=MINBUFSIZE || p->bufferSize>MAXBUFSIZE) {
			if (err = SetInputBufferSize(pp, (int)p->bufferSize))
				return err;
		}
		else {
			return INVALID_BUFFER;
		}
	}

	if (p->baudEncountered) {
		int baudCode;
		if (err = BaudRateToBaudCode((int)p->baud, &baudCode))
			return err;
		pp->baud = baudCode;
		commSettingsChanged = 1;
	}

	if (p->databitsEncountered) {
		switch((int)p->databits) {
			case 7:
				pp->databits = 0;
				break;
			case 8:
				pp->databits = 1;
				break;
			default:
				return INVALID_DATABITS;
				break;
		}
		commSettingsChanged = 1;
	}

	if (p->echoEncountered) {
		if (p->echo>=0 && p->echo<=1)
			pp->echo = (unsigned short)p->echo;
		else
			return INVALID_ECHO;
	}

	if (p->inEncountered) {
		if (p->inputHandshake>=0 && p->inputHandshake<=2)
			pp->inShake = (unsigned short)p->inputHandshake;
		else
			return INVALID_HANDSHAKE;
		commSettingsChanged = 1;
	}

	if (p->lineEncountered) {
		if (p->online) {
			/*	HR, 9/20/97:
				Because VDT can now open multiple ports, the command "VDT line=1" is now a NOP.
				This is necessary because VDT has no way to know which port to make the terminal
				port.
			*/
		}
		else {
			VDTSetTerminalPortPtr(NULL);				// Set terminal port to "Off Line".
		}
	}

	if (p->outEncountered) {
		if (p->outputHandshake>=0 && p->outputHandshake<=2)
			pp->outShake = (unsigned short)p->outputHandshake;
		else
			return INVALID_HANDSHAKE;
		commSettingsChanged = 1;
	}

	if (p->parityEncountered) {
		// Parameter: p->parity
		if (p->parity>=0 && p->parity<=2)
			pp->parity = (unsigned short)p->parity;
		else
			return INVALID_PARITY;
		commSettingsChanged = 1;
	}

	if (p->stopbitsEncountered) {
		switch((int)p->stopbits) {
			case 1:
				pp->stopbits = 0;
				break;
			case 2:
				pp->stopbits = 1;
				break;
			default:
				return INVALID_STOPBITS;
				break;
		}
		commSettingsChanged = 1;
	}

	if (p->terminalEOLEncountered) {
		if (p->terminalEOL>=0 && p->terminalEOL<=2)
			pp->terminalEOL = (unsigned short)p->terminalEOL;
		else
			return VDT_ERR_INVALID_TERMINAL_EOL;
	}

	if (p->rtsEncountered)
		pp->disableRTS = p->rts==0;

	if (err==0 && commSettingsChanged) {
		KillVDTIO(pp);
		err = SetCommPortSettings(pp);
	}

	return err;
}

int
RegisterVDT2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDT2RuntimeParams structure as well.
	cmdTemplate = "VDT2 /P=name:portName abort, baud=number:baud, buffer=number:buffer, databits=number:databits, echo=number:echo, in=number:inputHandshake, killio, line=number:online, out=number:outputHandshake, parity=number:parity, port=number:port, stopbits=number:stopbits, terminalEOL=number:terminalEOL, rts=number:rts, testMode=number:testMode, DumpTestBuffer";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDT2RuntimeParams), (void*)ExecuteVDT2, 0);
}

// Operation template: VDTGetStatus2 number:channelNumber, number:whichStatus, number:options

// Runtime param structure for VDTGetStatus2 operation.
#pragma pack(2)			// All structures passed between Igor and XOP are two-byte aligned.
struct VDTGetStatus2RuntimeParams {
	// Flag parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int channelNumberEncountered;
	double channelNumber;
	int channelNumberParamsSet[1];

	// Parameters for simple main group #1.
	int whichStatusEncountered;
	double whichStatus;
	int whichStatusParamsSet[1];

	// Parameters for simple main group #2.
	int optionsEncountered;
	double options;
	int optionsParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct VDTGetStatus2RuntimeParams VDTGetStatus2RuntimeParams;
typedef struct VDTGetStatus2RuntimeParams* VDTGetStatus2RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	VDTGetStatus()

	The main purpose of this routine is to facilitate debugging when things
	are going wrong, especially to check for serial port overrun errors.
	
	The operation has three numeric parameters. It sets the IGOR variable V_VDT
	to a value, depending on the parameters. The parameters are:
		channelNumber:		0 for the input channel; 1 for the output channel.
		whichStatus:		Specifies what kind of status information you want.
		options				This has different meanings for different status codes.

	See the VDT Help file for details.
			
	The Macintosh SerStatus call resets the cumErrs each time you call it.
	However, VDT maintains its own copy and resets the copy only at certain times.
	See the VDT Help file for details.
	
	Added 9/9/96 in version 1.31.
*/
extern "C" int
ExecuteVDTGetStatus2(VDTGetStatus2RuntimeParamsPtr p)
{
	VDTPortPtr op;
	int channelNumber;
	int whichStatus;
	int options;
	CountInt val;
	int cumErrs, xOffSent, readPending, writePending, ctsHold, xOffHold;
	int err = 0;

	if (err = VDTGetOpenAndCheckOperationsPortPtr(&op, 1, 0))	// Make sure port is selected and open it if necessary.
		return err;

	// Main parameters.

	channelNumber = (int)p->channelNumber;
	if (channelNumber<0 || channelNumber>1)
		return BAD_CHANNEL_NUMBER;

	whichStatus = (int)p->whichStatus;
	if (whichStatus<0 || whichStatus>6)
		return BAD_STATUS_CODE;

	options = (int)p->options;
	
	if (whichStatus != 0) {
		err = GetSerialStatus(op, channelNumber, &cumErrs, &xOffSent, &readPending, &writePending, &ctsHold, &xOffHold);
		if (err)
			return err;
		AccumulateErrorStatus(op, channelNumber, cumErrs);
	}

	switch(whichStatus) {
		case 0:
			val = GotChars(op);
			break;
		
		case 1:
			val = GetErrorStatus(op, channelNumber);
			if (options == 1)
				ClearErrorStatus(op);
			break;

		case 2:
			val = xOffSent;
			break;

		case 3:
			val = readPending;
			break;

		case 4:
			val = writePending;
			break;

		case 5:
			val = ctsHold;
			break;

		case 6:
			val = xOffHold;
			break;
	}

	SetV_VDT((double)val);

	return err;
}

int
RegisterVDTGetStatus2(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the VDTGetStatus2RuntimeParams structure as well.
	cmdTemplate = "VDTGetStatus2 number:channelNumber, number:whichStatus, number:options";
	runtimeNumVarList = "V_VDT;";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(VDTGetStatus2RuntimeParams), (void*)ExecuteVDTGetStatus2, 0);
}

