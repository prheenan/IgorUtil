/*	VDTIO.c -- does the real work for VDT XOP.

	HR, 2/17/94
		Changed CloseVDTIO to close both the input port AND the output port.
	
	HR, 8/24/94
		Set routines that require 68K assembly to compile under MPW for 68K only,
		not under THINK or PowerPC compilers. These are the hex IO routines,
		VDTReadHex, VDTWriteHex, VDTReadHexWave and VDTWriteHexWave.
	
	HR, 1/12/96 - 1/13/96
		Added up error result to InitVDT.
		Fixed some problems that left locked wave handles if error or user aborted a transfer.
		Changed SetInputBufferSize to clip serial buffer size to 32767, the max that the Mac supports.
		Followed new Apple recommendations:
			Open the output driver first, then the input driver.
			On closing serial driver - see CloseDrivers().
			On calling SerReset. We now call it on the output driver instead of the input driver.
			Used Control call to set handshaking instead of old SerHShake. This should make DTR signal work when user chooses hardware handshaking.
		Finalized version 1.22 which works with Igor Pro 1.24 or later.
	
	HR, 1/14/96 - 1/15/96
		Updated for Igor Pro 3.0. Added support for data folders.
		Finalized version 3.00 which requires Igor Pro 3.0 or later.
	
	HR, 9/9/96 for version 1.31
		Fixed timeout in binary operations. VDTGetBinaryFlags was treating the timeout
		parameter as a number of ticks when it should be a number of seconds.
		
	HR, 9/11/97, for version 2.0
		Split out Macintosh and Windows-specific routines into separate files.
		
	HR, 9/13/97, for version 2.0
		Started major overhaul to allow communicating via multiple ports and once and
		also to clean up some ugly code from the late 80's.
		
	HR, 9/14/97, for version 2.0
		Split out routines that implement VDT command line operations into VDTOperations.c.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

static int gTestMode = 0;
static Handle gTestDataH = NULL;				// Contains data written by serial operations while test mode is in effect.

void
ClearErrorStatus(VDTPortPtr pp)
{
	MemClear(pp->errorStatus, sizeof(pp->errorStatus));
}

int
GetErrorStatus(VDTPortPtr pp, int channelNumber)
{
	return pp->errorStatus[channelNumber];
}

void
AccumulateErrorStatus(VDTPortPtr pp, int channelNumber, int status)
{
	pp->errorStatus[channelNumber] |= status;
}

static void
DisposeBuffers(VDTPortPtr pp)
{
	/*	This was used for Mac OS 9 where we had to allocate the serial
		buffer ourselves. On Mac OS X we can't allocate a serial buffer
		and on Windows the OS allocates it so this routine is now vestigial.
	*/
}

static int
AllocateBuffers(VDTPortPtr pp)
{
	DisposeBuffers(pp);

	/*	This was used for Mac OS 9 where we had to allocate the serial
		buffer ourselves. On Mac OS X we can't allocate a serial buffer
		and on Windows the OS allocates it so this routine is now vestigial.
	*/

	return 0;
}

VDTByteCount
GotChars(VDTPortPtr pp)								/* returns number of characters available */						
{
	VDTByteCount length;
	
	length = NumCharsInSerialInputBuffer(pp);
	return length;
}

int
ReadChars(									// Reads specified number of chars into buffer.
	VDTPortPtr pp,
	UInt32 timeout, 						// Absolute ticks by which read must be done or 0 for no timeout.
	char *buffer,							// Returns error code from read operation.
	VDTByteCount *lengthPtr)				// Returns number of chars read via lengthPtr.
{
	VDTByteCount numCharsRead;
	int result=0;
	
	numCharsRead = 0;
	
	if (*lengthPtr) {
		VDTByteCount count;
		count = *lengthPtr;
		result = ReadCharsSync(pp, timeout, count, &count, buffer);
		if (result==0 && count!=*lengthPtr) {
			// If here, ReadCharsSync returned before completing the read. This should never happen.
			XOPNotice("BUG: VDT ReadChars"CR_STR);
		}
		// numCharsRead += *lengthPtr;
		numCharsRead += count;				// HR, 040211, 1.02: This is more accurate in case of error.
	}
	
	*lengthPtr = numCharsRead;
	return result;
}


/* *** Initialization and Shutdown *** */

static VDTPortPtr gFirstVDTPortPtr = NULL;			// Pointer to first VDTPort in linked list.
	
/*	IndexedVDTPort(firstVDTPortPtr, index)

	Returns a pointer to the indexth VDTPort in the linked list or NULL.
	index starts from 0.
	
	firstVDTPortPtr is either a pointer to the start of a linked list of
	VDTPort structures or NULL to use gFirstVDTPortPtr.
*/
VDTPortPtr
IndexedVDTPort(VDTPortPtr firstVDTPortPtr, int index)
{
	VDTPortPtr pp;
	
	if (firstVDTPortPtr == NULL)
		firstVDTPortPtr = gFirstVDTPortPtr;
	
	pp = firstVDTPortPtr;
	while(pp != NULL) {
		if (index == 0)
			return pp;
		pp = pp->nextPort;
		index -= 1;
	}

	return NULL;
}
	
/*	FindVDTPort(firstVDTPortPtr, name)

	Returns a pointer to the named VDTPort in the linked list or NULL.
	index starts from 0.
	
	firstVDTPortPtr is either a pointer to the start of a linked list of
	VDTPort structures or NULL to use gFirstVDTPortPtr.
*/
VDTPortPtr
FindVDTPort(VDTPortPtr firstVDTPortPtr, const char* name)
{
	VDTPortPtr pp;
	
	if (firstVDTPortPtr == NULL)
		firstVDTPortPtr = gFirstVDTPortPtr;
	
	pp = firstVDTPortPtr;
	while(pp != NULL) {
		if (CmpStr(pp->name, (char*)name) == 0)
			return pp;
		pp = pp->nextPort;
	}

	return NULL;
}

/*	AddVDTPort(pp)

	pp points to an initialized VDTPort structure in the heap.
	This adds it to the linked list of open ports and takes over the memory
	pointed to by pp.
*/
static int
AddVDTPort(VDTPortPtr pp)
{
	if (FindVDTPort(gFirstVDTPortPtr, pp->name) != NULL) {
		XOPNotice("BUG: AddVDTPort"CR_STR);			// Trying to add a port that is already in the list.
		return 0;
	}
	
	if (gFirstVDTPortPtr == NULL) {
		gFirstVDTPortPtr = pp;
	}
	else {
		VDTPortPtr tt;
		tt = gFirstVDTPortPtr;
		while (tt->nextPort != NULL)
			tt = tt->nextPort;
		tt->nextPort = pp;
	}
	pp->nextPort = NULL;							// Should not be necessary.
	
	return 0;
}

/*	DisposeVDTPort(VDTPortPtr pp)

	Disposes the memory occupied by pp.
	
	NOTE: This assumes that the comm port has already been closed and that any
	buffers referenced by pp have been disposed.
*/
static int
DisposeVDTPort(VDTPortPtr pp)
{
	if (FindVDTPort(gFirstVDTPortPtr, pp->name) == NULL) {
		XOPNotice("BUG: DisposeVDTPort"CR_STR);		// Trying to dispose a port that is not in the list.
		return 0;
	}
	
	// Remove from linked list.
	if (pp == gFirstVDTPortPtr) {
		gFirstVDTPortPtr = pp->nextPort;
	}
	else {
		VDTPortPtr tt;
		if (tt = gFirstVDTPortPtr) {
			while (tt->nextPort != pp)
				tt = tt->nextPort;
			tt->nextPort = pp->nextPort;
		}
	}
	
	DisposePtr((Ptr)pp);
	return 0;
}

void
SetCommSettingsFieldsToDefault(VDTPortPtr pp)		// Just sets fields. Does not actually set up the port.
{
	pp->baud = 5;									// 9600 baud.
	pp->parity = 0;									// No parity.
	pp->stopbits = 1;								// 2 stop bits.
	pp->databits = 1;								// 8 data bits.
	pp->outShake = 0;								// Output flow control - none.	HR, 981118, 2.11: Replaced == with =. This was asymptomatic.
	pp->inShake = 0;								// Input flow control - none.	HR, 981118, 2.11: Replaced == with =. This was asymptomatic.	
	pp->echo = 1;
	pp->terminalEOL = 0;							// Terminal EOL - CR.			HR, 981118, 2.11: Replaced == with =. This was asymptomatic.
}

void
SetVDTPortFieldsToDefault(const char* name, const char* inputChan, const char* outputChan, VDTPortPtr pp)	// Just sets fields. Does not actually set up the port.
{
	MemClear(pp, sizeof(VDTPort));
	strcpy(pp->name, name);
	strcpy(pp->inputChan, inputChan);
	strcpy(pp->outputChan, outputChan);
	pp->inputBufferSize = 4096;
	pp->echo = 1;
	SetCommSettingsFieldsToDefault(pp);
}

int
VDTPortIsOpen(VDTPortPtr pp)
{
	if (pp == NULL) {
		XOPNotice("BUG: VDTPortIsOpen\015");
		return 0;
	}
	return pp->portIsOpen;
}

/*	OpenVDTPort(name, ppp)

	Opens the named port.
	Returns VDTPortPtr via VDTPortPtr if OK or NULL if error.
	Function result is 0 if OK or error code if the port could not be opened.
	
	If the port is already open, this returns a VDTPortPtr via ppp and returns 0
	as the function result but does not do anything else (such as initialize the port
	since it has already been initialized).
*/
int
OpenVDTPort(const char* name, VDTPortPtr* ppp)
{
	VDTPortPtr pp;
	int err;
	
	pp = FindVDTPort(gFirstVDTPortPtr, name);
	*ppp = pp;
	if (pp == NULL)
		return VDT_ERR_UNKNOWN_PORT_ERR;
	if (VDTPortIsOpen(pp))
		return 0;							// Port already open.
	
	if (err = AllocateBuffers(pp))
		return err;
	
	if (err = OpenCommPort(pp)) {
		DisposeBuffers(pp);
		return err;
	}
	
	pp->portIsOpen = 1;						// Needs to be here because SetCommPortSettings checks it.
	
	if (err = SetCommPortSettings(pp)) {
		DisposeBuffers(pp);
		CloseCommPort(pp);
		return err;
	}

	return 0;
}

/*	GetNumberedVDTPort(portNumber, ppp)

	Numbered ports are those ports that can be opened via the VDT port=<p> command.
	
	On Macintosh, the standard ports are:
		0: Modem port
		1: Printer port
	
	On Windows, the standard ports are:
		0: COM1
		1: COM1
		2: COM2
*/
int
GetNumberedVDTPort(int portNumber, VDTPortPtr* ppp)
{
	char name[MAX_OBJ_NAME+1];
	
	*ppp = NULL;
	
	#ifdef MACIGOR
		switch(portNumber) {
			case 0:
				strcpy(name, "Modem");			// Modem port.
				break;
			
			case 1:
				strcpy(name, "Printer");		// Printer port.
				break;
			
			default:
				return INVALID_PORT;
		}
		
		/*	HR, 971223: This takes care of translating into non-English port names.
			See comments from 971223 in TranslatePortName.
		*/
		TranslatePortName(name);
	#endif
	
	#ifdef WINIGOR
		switch(portNumber) {
			case 0:
			case 1:
				strcpy(name, "COM1");
				break;
			
			case 2:
				strcpy(name, "COM2");
				break;
			
			default:
				return INVALID_PORT;
		}
	#endif
	
	*ppp = FindVDTPort(NULL, name);
	if (*ppp == NULL)
		return VDT_ERR_PORT_NOT_AVAILABLE;
	return 0;
}

/*	TranslatePortName(portName)

	Does platform-related port name translation. This is used so that commands or experiments
	created on one platform will work on another.
	
	Also translates "Printer" or "Modem" into "Printer-Modem" if we are running on a Macintosh
	that has only one serial port named "Printer-Modem".
	
	Returns true if a translation was made, false otherwise.
*/
int
TranslatePortName(char* portName)
{
	char modemStr[MAX_OBJ_NAME+1], printerStr[MAX_OBJ_NAME+1];
	int didTranslation;
	
	strcpy(modemStr, "Modem");
	strcpy(printerStr, "Printer");
	
	didTranslation = 0;
	
	#ifdef MACIGOR
		if (CmpStr(portName, "COM1")==0 || CmpStr(portName, "COM3")==0) {
			strcpy(portName, modemStr);
			didTranslation = 1;
		}
		if (CmpStr(portName, "COM2")==0 || CmpStr(portName, "COM4")==0) {
			strcpy(portName, printerStr);
			didTranslation = 1;
		}
		
		/*	HR, 971223: This takes care of translating the English names "Modem" and "Printer"
			into actual port names, which will be different in other languages. For example,
			on a French system, the names are "Port modem" and "Port imprimante".
		*/
		if (CmpStr(portName, modemStr)==0 && FindVDTPort(NULL, modemStr)==NULL) {
			VDTPortPtr pp;
			pp = IndexedVDTPort(NULL, 0);			// Map "Modem" to first port.
			if (pp != NULL) {
				strcpy(portName, pp->name);
				didTranslation = 1;
			}
		}
		if (CmpStr(portName, printerStr)==0 && FindVDTPort(NULL, printerStr)==NULL) {
			VDTPortPtr pp;
			pp = IndexedVDTPort(NULL, 1);			// Map "Printer" to second port.		
			if (pp == NULL)							// However, on Powerbook, there is no second port so
				pp = IndexedVDTPort(NULL, 0);		// map "Printer" to first port.
			if (pp != NULL) {
				strcpy(portName, pp->name);
				didTranslation = 1;
			}
		}
		
		// HR, 971223: Removed this because the code I added above takes care of it.
		#ifdef NOT_DEFINED
			if (FindVDTPort(NULL, portName) == NULL) {			// No such port?
				if (CmpStr(portName, printerStr)==0 || CmpStr(portName, modemStr)==0) {
					if (FindVDTPort(NULL, "Printer-Modem") != NULL) {
						strcpy(portName, "Printer-Modem");
						didTranslation = 1;
					}
				}
			}
		#endif
	#endif
	
	#ifdef WINIGOR
		if (CmpStr(portName, modemStr)==0) {
			strcpy(portName, "COM1");
			didTranslation = 1;
		}
		if (CmpStr(portName, printerStr)==0) {
			strcpy(portName, "COM2");
			didTranslation = 1;
		}
	#endif
	
	return didTranslation;
}

/*	CloseVDTPort(name)

	Closes the named port if it is open.
	Does nothing it if is not open.
	
	Function result is 0 if OK (including if port not open) or an error code.
*/
int
CloseVDTPort(const char* name)
{
	VDTPortPtr pp;
	int err;
	
	pp = FindVDTPort(gFirstVDTPortPtr, name);
	if (pp == NULL)
		return VDT_ERR_UNKNOWN_PORT_ERR;
	
	if (VDTPortIsOpen(pp) == 0) {
		DisposeBuffers(pp);				// HR, 980321: The port can have an input buffer allocated even if it is not open.
		return 0;						// Port not open.
	}

	#if NOT_DEFINED	
		// I think it is OK to have a closed port designated as terminal or operations port.
		if (VDTGetOperationsPortPtr() == pp)
			VDTSetOperationsPortPtr(NULL);
		
		if (VDTGetTerminalPortPtr() == pp)
			VDTSetTerminalPortPtr(NULL);
	#endif
	
	err = CloseCommPort(pp);
	if (err != 0)
		return err;

	DisposeBuffers(pp);

	#if NOT_DEFINED	
		// I think it is OK to have a closed port designated as terminal or operations port.
		if (VDTGetOperationsPortPtr() == NULL)
			VDTSetOperationsPortPtr(gFirstVDTPortPtr);			// This will be NULL if there are no more open ports.
	#endif
	
	pp->portIsOpen = 0;
	
	return 0;
}

int
InitVDT(void)
{
	VDTPortPtr pp;
	int index;
	char name[MAX_OBJ_NAME+1];
	char inputChan[MAX_OBJ_NAME+1];
	char outputChan[MAX_OBJ_NAME+1];
	int err;
	
	err = 0;

	gTestMode = 0;
	
	// This creates master linked list of all known ports.
	index = 0;
	while(1) {
		if (GetIndexedPortNameAndChannelInfo(index, name, inputChan, outputChan) != 0)
			break;
			
		pp = (VDTPortPtr)NewPtr(sizeof(VDTPort));
		if (pp == NULL) {
			err = NOMEM;
			break;
		}
		SetVDTPortFieldsToDefault(name, inputChan, outputChan, pp);
		
		if (err = AddVDTPort(pp))				// If no error, AddVDTPort takes over the memory pointed to by pp.
			break;
		
		index += 1;
	}
	if (err != 0) {
		while(gFirstVDTPortPtr != NULL)
			DisposeVDTPort(gFirstVDTPortPtr);
		return err;
	}
	
	return 0;
}

void
KillVDTIO(VDTPortPtr pp)
{
	KillSerialIO(pp);
	ClearErrorStatus(pp);							// HR, 9/9/96.
}

/*	CloseAllVDTPorts()

	This is called only when VDT is quitting. It closes all serial ports
	and disposes memory used to keep track of them.
	
	HR, 2/17/94
		Modified this according to the New Inside Mac "Serial Driver" chapter.
		Formerly, it was recommended to close only the input port and the output
		port was supposed to be automatically closed. Now, it is recommended
		to close both.
*/
void
CloseAllVDTPorts(void)				/* closes any open IO port and does cleanup */
{
	VDTPortPtr pp, tt;
	
	pp = gFirstVDTPortPtr;
	while (pp) {
		tt = pp->nextPort;
		KillVDTIO(pp);
		CloseVDTPort(pp->name);
		pp = tt;
	}
	
	while(gFirstVDTPortPtr != NULL)				// HR, 980321: Added this to stop leak.
		DisposeVDTPort(gFirstVDTPortPtr);
}

static int
NullTerminateHandle(Handle h)					// Adds null terminator to handle.
{
	VDTByteCount numBytes;
	
	numBytes = GetHandleSize(h);
	SetHandleSize(h, numBytes+1);
	if (MemError())
		return NOMEM;
	(*h)[numBytes] = 0;
	
	return 0;
}

int
SetTestMode(int testMode)
{
	gTestMode = testMode;
	return 0;
}

/*	SerialRead(pp, timeout, buf, countPtr)
	
	If test mode is off, just calls regular ReadChars.
	
	If test mode is on, returns data from gTestDataH handle via buf.
	
	timeout is the absolute tick count by which the read must be completed or 0 for no timeout.
	
	Returns error code.
	
	HR, 061019, 1.12: The count of characters to read is in *countPtr and the count of characters
	actually read is returned in countPtr.
*/
int
SerialRead(VDTPortPtr pp, UInt32 timeout, char* buf, VDTByteCount* countPtr)
{
	VDTByteCount numBytesInHandle;
	VDTByteCount numBytesToTransfer;
	
	if (gTestMode == 0)
		return ReadChars(pp, timeout, buf, countPtr);
	
	if (gTestDataH == NULL)
		numBytesInHandle = 0;
	else
		numBytesInHandle = GetHandleSize(gTestDataH);
	
	numBytesToTransfer = *countPtr;
	if (numBytesToTransfer > numBytesInHandle)
		numBytesToTransfer = numBytesInHandle;
		
	if (numBytesToTransfer <= 0)
		return TIME_OUT_READ;	
	
	memcpy(buf, *gTestDataH, numBytesToTransfer);
	
	memmove(*gTestDataH, *gTestDataH+numBytesToTransfer, numBytesInHandle-numBytesToTransfer);
	SetHandleSize(gTestDataH, numBytesInHandle-numBytesToTransfer);
	
	return 0;
}

/*	SerialWrite(pp, timeout, buf, countPtr)
	
	If test mode is off, just calls regular WriteChars.
	
	If test mode is on, stores data in gTestDataH handle.
	
	timeout is the absolute tick count by which the write must be completed or 0 for no timeout.
	
	Returns status.
	
	HR, 061019, 1.12: The number of bytes to write is in *countPtr and the number of bytes
	actually written is returned in *countPtr.
*/
int
SerialWrite(VDTPortPtr pp, UInt32 timeout, char* buf, VDTByteCount* countPtr)
{
	VDTByteCount numBytesInHandle;
	
	if (gTestMode == 0)
		return WriteChars(pp, timeout, buf, countPtr);
	
	if (gTestDataH == NULL) {
		gTestDataH = NewHandle(0L);
		if (gTestDataH == NULL)
			return NOMEM;
	}

	numBytesInHandle = GetHandleSize(gTestDataH);
	SetHandleSize(gTestDataH, numBytesInHandle + *countPtr);
	if (MemError())
		return NOMEM;
	
	memcpy(*gTestDataH + numBytesInHandle, buf, *countPtr);
	return 0;
}

static void
DumpTestBufferAsText(void)
{
	VDTByteCount numBytesInHandle;
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
	VDTByteCount numBytesInHandle;
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

void
DumpTestBuffer(void)
{
	char temp[256];
	VDTByteCount numBytesInHandle;
	char* p1;
	int i, isText;
	
	if (gTestDataH == NULL) {
		XOPNotice("VDT2 Test Buffer is NULL."CR_STR);
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

