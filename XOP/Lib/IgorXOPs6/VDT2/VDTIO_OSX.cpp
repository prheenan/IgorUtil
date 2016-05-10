// VDTIO_OSX.c -- OSX-specific routines for VDT XOP.

/*	To Do
	
	Make WriteCharsAsync actually work asynchronously?
	
	If I send 1024 characters to the Mac, VDTGetStatus reports 1020
	characters in the input buffer. If I then do a VDT KillIO, VDTGetStatus
	reports 0 characters. If I then transmit more characters, VDTGetStatus
	still reports 0 characters. It is as if the overflow causes the input
	port to cease to function.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <CoreFoundation/CoreFoundation.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/IOBSD.h>

#include "VDT.h"

// Test routines

static void
DumpTermios(struct termios* ts)
{
	char buf[256];
	int i;
	
	XOPNotice("Termios structure dump:"CR_STR);
	
	sprintf(buf, "\tc_iflag = %0X"CR_STR, (unsigned int)ts->c_iflag);
	XOPNotice(buf);
	
	sprintf(buf, "\tc_oflag = %0X"CR_STR, (unsigned int)ts->c_oflag);
	XOPNotice(buf);
	
	sprintf(buf, "\tc_cflag = %0X"CR_STR, (unsigned int)ts->c_cflag);
	XOPNotice(buf);
	
	sprintf(buf, "\tc_lflag = %0X"CR_STR, (unsigned int)ts->c_lflag);
	XOPNotice(buf);
	
	sprintf(buf, "\tc_ispeed = %d, c_ospeed = %d"CR_STR, (int)ts->c_ispeed, (int)ts->c_ospeed);
	XOPNotice(buf);
	
	for(i=0; i<NCCS; i+=1) {
		sprintf(buf, "\tc_cc[%d] = %0X"CR_STR, i, (unsigned int)ts->c_cc[i]);
		XOPNotice(buf);	
	}
}

static void
TestCFMakeRaw(void)
{
	struct termios settings;
	
	MemClear(&settings, sizeof(settings));
	cfmakeraw(&settings);
	DumpTermios(&settings);
	
	memset(&settings, 0xFF, sizeof(settings));
	cfmakeraw(&settings);
	DumpTermios(&settings);
}

static int
StdError(void)		// Returns value from "errno" global.
{
	return errno;
}

/*	SetTermiosToDefault(struct termios* ts)

	Sets ts structure to desired default state. See the comments for cfmakeraw
	for an explanation.
*/
static void
SetTermiosToDefault(struct termios* ts)
{
	ts->c_iflag = IGNPAR | INPCK;
	ts->c_oflag = 0;
	ts->c_cflag = CS8 | CSTOPB | CREAD | CLOCAL;
	ts->c_lflag = 0;

	ts->c_ispeed = ts->c_ospeed = B9600;

	// Disable all special treatment for control characters.
	memset(ts->c_cc, _POSIX_VDISABLE, sizeof(ts->c_cc));

	ts->c_cc[VSTART] = 17;					// Standard XON character
	ts->c_cc[VSTOP] = 19;					// Standard XOFF character
	
	/*	We set the timeout conditions for read calls to not wait for characters
		and not wait for timeout. This means that, when we call read, it will return
		the number of characters we ask for or the number that are available in the
		input buffer, which could be zero. We do this because VDT never calls read
		unless it has already determined that the characters it wants to read are
		in the input buffer.
	*/
	ts->c_cc[VMIN] = 0;						// read returns with whatever characters are in input buffer
	ts->c_cc[VTIME] = 0;					// read does not wait for characters to arrive
}


static void
PrintUnixErrMessage(const char* preamble)	// errno is assumed to be set by a previously called system routine.
{
	char message[512];
	const char* errorStr;
	int error;
	
	error = StdError();
	errorStr = strerror(error);

	sprintf(message, "VDT error: %s. [ Unix error %d: \"%s\" ]"CR_STR, preamble, error, errorStr);
	XOPNotice(message);
}

static int
GetPortTermiosSettings(int fileDescriptor, const char* portName, struct termios* ts)
{
	// Get the current portSettings and save them so we can restore the default settings later.
	if (tcgetattr(fileDescriptor, ts) == -1) {
		char message[256];
		sprintf(message, "Error getting attributes for port \"%s\"", portName);
  		PrintUnixErrMessage(message);
		return VDT_ERR_UNIX_ERROR;
	}
	return 0;
}

static int
SetPortTermiosSettings(int fileDescriptor, const char* portName, struct termios* ts)
{
	if (tcsetattr(fileDescriptor, TCSANOW, ts) == -1) {
		char message[256];
		sprintf(message, "Error setting attributes for port \"%s\"", portName);
  		PrintUnixErrMessage(message);
		return VDT_ERR_UNIX_ERROR;
	}
	return 0;
}

static int
GetPortHandshakeLines(int fileDescriptor, const char* portName, int* handshakePtr)
{
	// To read the state of the modem lines, use the following ioctl.
	// See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
	if (ioctl(fileDescriptor, TIOCMGET, handshakePtr) == -1) {
		char message[256];
		sprintf(message, "Error getting handshake lines for port \"%s\"", portName);
  		PrintUnixErrMessage(message);
		return VDT_ERR_UNIX_ERROR;
	}

	return 0;
}

static int
SetPortHandshakeLines(int fileDescriptor, const char* portName, int handshake)
{
	if (ioctl(fileDescriptor, TIOCMSET, &handshake) == -1) {
		char message[256];
		sprintf(message, "Error setting handshake lines for port \"%s\"", portName);
  		PrintUnixErrMessage(message);
		return VDT_ERR_UNIX_ERROR;
	}
	return 0;
}

/* *** Low Level IO Routines *** */

void
WaitForSerialOutputToFinish(VDTPortPtr pp)
{
	if (VDTPortIsOpen(pp)) {
		if (tcdrain(pp->fileDescriptor) == -1)
			PrintUnixErrMessage("WaitForSerialOutputToFinish failed");
	}
}

void
KillSerialIO(VDTPortPtr pp)
{
	if (VDTPortIsOpen(pp)) {
		if (tcflush(pp->fileDescriptor,TCIOFLUSH) == -1)
			PrintUnixErrMessage("KillSerialIO could not flush serial port");
	}
}

/*	WriteCharsAsync(pp, buffer, length)

	Writes length characters from buffer asynchronously to specified output port.
	
	NOTE
		On OS X, the characters are actually written synchronously.
		That is, this routine does not return until the characters have been written.
		This is because when we opened the port we cleared the O_NONBLOCK flag.
		
		However "written" just means that the POSIX I/O component of the operating
		system has accepted the characters. It does NOT mean that they have actually
		been transmitted. Furthermore, we have no way to know if or when they are
		actually transmitted.
		
		This has ramifications. See "NOTE ON MAC OS X SERIAL WRITE" below.
*/
int
WriteCharsAsync(VDTPortPtr pp, char *buffer, VDTByteCount length)	/* BUFFER MUST BE NON-RELOCATABLE */
{
	VDTByteCount numBytesWritten;
	
	numBytesWritten = write(pp->fileDescriptor, buffer, length);
	if (numBytesWritten != length)
		return VDT_ERR_UNKNOWN_COMM_ERR;
	return 0;
}

/*	WriteAsyncResult(pp, donePtr)

	Returns via donePtr the truth that transmission is finished.

	Function result is 0 if OK or non-zero error code.
*/
int
WriteAsyncResult(VDTPortPtr pp, int* donePtr)
{
	/*	This just says that we have transmitted the characters to the POSIX I/O component
		of the OS. We have not way to know when the actual serial transmission is finished.
	*/
	*donePtr = 1;
	return 0;
}

int
WriteChars(							/* writes characters to specified output port */
	VDTPortPtr pp,
	UInt32 timeout,
	char *buffer,
	VDTByteCount *lengthPtr)
{
	int done;
	int result;
	
	if (result = WriteCharsAsync(pp, buffer, *lengthPtr))	// Start write
		return result;

	/*	NOTE ON MAC OS X SERIAL WRITE
		We have no way to ensure that characters have actually been transmitted other than
		to call tcdrain. If, for some reason, the characters can never be transmitted
		(e.g., we are using hardware handshaking and the handshake lines do not permit
		 transmission), then we will hang here and the user will have to force-quit Igor.
		
		This problem could be avoided if the OS provided a practical way to determine
		when the serial transmission is actually finished.
	*/
	WaitForSerialOutputToFinish(pp);						// Calls tcdrain.
	
	do {													// Now check for abort or timeout
		result = WriteAsyncResult(pp, &done);
		if (result || done)								
			return result;
		result = CheckAbort(timeout);						// Check for cmd-dot or timeout
		if (result) {
			KillSerialIO(pp);								// Abort output
			return(result == -1 ? -1:TIME_OUT_WRITE);
		}
	} while (TRUE);
}

VDTByteCount
NumCharsInSerialInputBuffer(VDTPortPtr pp)
{
	size_t numberOfBytesAvailable;

	if (ioctl(pp->fileDescriptor, FIONREAD, &numberOfBytesAvailable) == -1) {
		static UInt32 lastTimeWePrintedThisMessage = 0;
		
		if (TickCount() > lastTimeWePrintedThisMessage+10*60) {
			PrintUnixErrMessage("NumCharsInSerialInputBuffer got error from ioctl");
			lastTimeWePrintedThisMessage = TickCount();
		}
		numberOfBytesAvailable = 0;
	}
	return numberOfBytesAvailable;
}

int
ReadCharsSync(VDTPortPtr pp, UInt32 timeout, VDTByteCount numCharsToRead, VDTByteCount* numCharsReadPtr, char* buffer)
{
	VDTByteCount numBytesRemainingToBeRead, numBytesInInputBuffer;
	VDTByteCount numBytesToReadThisTime, numBytesReadThisTime;

	*numCharsReadPtr = 0;
	
	numBytesRemainingToBeRead = numCharsToRead;
	
	while(numBytesRemainingToBeRead > 0)  {
		// Wait till there are some bytes to read in the input buffer.
		numBytesInInputBuffer = NumCharsInSerialInputBuffer(pp);
		if (numBytesInInputBuffer > 0) {
			numBytesToReadThisTime = numBytesInInputBuffer;
			if (numBytesToReadThisTime > numBytesRemainingToBeRead)
				numBytesToReadThisTime = numBytesRemainingToBeRead;
 		 	numBytesReadThisTime = read(pp->fileDescriptor, buffer+*numCharsReadPtr, numBytesToReadThisTime);
			if (numBytesReadThisTime ==  -1)  {
		  		PrintUnixErrMessage("ReadCharsSync got error from read");
		  		return VDT_ERR_UNIX_ERROR;
		  	}
			*numCharsReadPtr += numBytesReadThisTime;
			numBytesRemainingToBeRead -= numBytesReadThisTime;
		}
		else {
			// No bytes in the input buffer.
			int result;
			result = CheckAbort(timeout);						// Check for cmd-dot or timeout
			if (result) {
				KillSerialIO(pp);								// Abort output
				return(result == -1 ? -1:TIME_OUT_READ);
			}
		}
	}
	
	return 0;
}

int
GetSerialStatus(VDTPortPtr pp, int channel, int* cumErrs, int* xOffSent, int* readPending, int* writePending, int* ctsHold, int* xOffHold)
{
	int handshake;
	int err;
	
	if (err = GetPortHandshakeLines(pp->fileDescriptor, pp->name, &handshake))
		return err;
	
	*cumErrs = 0;									// Not supported on OS X.
	*xOffSent = 0;									// Not supported on OS X.
	*readPending = 0;								// Not supported on OS X.
	*writePending = 0;								// Not supported on OS X.
	*ctsHold = (handshake & TIOCM_CTS) != 0;
	*xOffHold = 0;									// Not supported on OS X.
		
	return 0;
}


/* *** Initialization and Setup *** */

// Returns an iterator across all known serial ports. Caller is responsible for
// releasing the iterator when iteration is complete.
static int
GetSerialPortIterator(io_iterator_t* iterator)
{
	kern_return_t kernResult; 
	mach_port_t masterPort;
	CFMutableDictionaryRef classesToMatch;
	char message[256];
	
	kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (kernResult != KERN_SUCCESS) {
		sprintf(message, "Error: IOMasterPort returned %d."CR_STR, kernResult);
		XOPNotice(message);
		return VDT_ERR_SYSTEM_ERROR;
	}
	
	// Serial devices are instances of class IOSerialBSDClient
	classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
	if (classesToMatch == NULL) {
		sprintf(message, "Error: IOServiceMatching returned a NULL dictionary."CR_STR);
		XOPNotice(message);
		return VDT_ERR_SYSTEM_ERROR;
	}
	
	CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
	
	kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, iterator);	// This consumes classesToMatch.	
	if (kernResult != KERN_SUCCESS) {
		sprintf(message, "Error: IOServiceGetMatchingServices returned %d."CR_STR, kernResult);
		XOPNotice(message);
		return VDT_ERR_SYSTEM_ERROR;
	}
	
	return 0;
}

/*	ListSerialPorts(portList, inDriverList, outDriverList, maxLen, numNamesInListPtr)

	portList, inDriverList and outDriverList are pointers to character arrays that can
	each hold maxLen characters plus a null terminator.
	
	On output, each character array contains a semicolon-separated list of names.
	Typically this will be:
		portList: 		USA28X1P1.1;USA28X1P2.2;
		inDriverList:	/dev/cu.USA28X1P1.1;/dev/cu.USA28X1P2.2;
		outDriverList:	/dev/cu.USA28X1P1.1;/dev/cu.USA28X1P2.2;
	
	[	The name USA28X1P1.1 is the default name for a serial port added by a Keyspan
	 	USA-28X USB-to-Serial converter.
	  
	 	/dev/cu.USA28X1P1.1 is the Unix path to the file that represents the
	 	corresponding serial port.
	]
	
	On output, *numNamesInListPtr contains the number of names added to each of the lists.
	
	Function result is 0 if OK or error code.
	
	If there are too many ports to fit in the character arrays, the function result is
	STR_TOO_LONG and *numNamesInListPtr contains the number of names added before
	there was not enough room.
*/
static int
ListSerialPorts(char* portList, char* inDriverList, char* outDriverList, int* numNamesInListPtr, int maxLen)
{
	io_iterator_t serialPortIterator;
	io_registry_entry_t serialService;
	char message[256];
	int result;

	*numNamesInListPtr = 0;

	MemClear(portList, maxLen+1);
	MemClear(inDriverList, maxLen+1);
	MemClear(outDriverList, maxLen+1);
	
	if (result = GetSerialPortIterator(&serialPortIterator))
		return result;

	while (serialService = IOIteratorNext(serialPortIterator)) {
		CFStringRef portNameAsCFString;
		char portName[256];
		CFStringRef bsdPathAsCFString;
		char bsdPath[256];

		// Get port name.
		*portName = 0;
		portNameAsCFString = (CFStringRef)IORegistryEntryCreateCFProperty(serialService, CFSTR(kIOTTYDeviceKey), kCFAllocatorDefault, 0);
		if (portNameAsCFString != NULL)
			CFStringGetCString(portNameAsCFString, portName, sizeof(portName), kCFStringEncodingASCII);
		CFRelease(portNameAsCFString);
			
		// Get path to Unix device representing serial port. Usually "/dev/cu.xxx" for serial port xxx.
		*bsdPath = 0;
		bsdPathAsCFString = (CFStringRef)IORegistryEntryCreateCFProperty(serialService, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
		if (bsdPathAsCFString != NULL)
			CFStringGetCString(bsdPathAsCFString, bsdPath, sizeof(bsdPath), kCFStringEncodingASCII);
		CFRelease(bsdPathAsCFString);

		IOObjectRelease(serialService);
		
		if (*portName != 0) {				// We got a port name?
			// Make sure name and path fit in our structure

			int portNameLength, bsdPathLength;
			
			portNameLength = strlen(portName);
			bsdPathLength = strlen(bsdPath);
		
			if (bsdPathLength >= sizeof(((VDTPortPtr)(Ptr)0)->inputChan)) {
				sprintf(message, "VDT error: The path \"%s\" is too long so the port named \"%s\" will not be available."CR_STR, bsdPath, portName);
				XOPNotice(message);
				*bsdPath = 0;
			}
			if (strchr(bsdPath,';') != NULL) {	// We can not handle a path that contains a semicolon.
				sprintf(message, "VDT error: The path \"%s\" contains a semicolon so the port named \"%s\" will not be available."CR_STR, bsdPath, portName);
				XOPNotice(message);
				*bsdPath = 0;
			}
		
			if (portNameLength >= sizeof(((VDTPortPtr)(Ptr)0)->name)) {
				sprintf(message, "VDT error: The port name \"%s\" is too long so the port will not be available."CR_STR, portName);
				XOPNotice(message);
				*portName = 0;
			}
			if (strchr(portName,';') != NULL) {	// We can not handle a port name that contains a semicolon.
				sprintf(message, "VDT error: The port name \"%s\" contains a semicolon so the port will not be available."CR_STR, portName);
				XOPNotice(message);
				*portName = 0;
			}

			if (*portName!=0 && *bsdPath!=0) {	// Still good?
				if (strlen(inDriverList) + bsdPathLength + 1 > maxLen) {
					// Can't fit BSD path so bail out.
					XOPNotice("VDT error: The list of port paths is too long."CR_STR);
					result = STR_TOO_LONG;
					break;
				}

				if (strlen(portList) + portNameLength + 1 > maxLen) {
					// Can't fit port name so bail out.
					XOPNotice("VDT error: The list of port names is too long."CR_STR);
					result = STR_TOO_LONG;
					break;
				}

				strcat(inDriverList, bsdPath);
				strcat(inDriverList, ";");

				strcat(outDriverList, bsdPath);
				strcat(outDriverList, ";");

				strcat(portList, portName);
				strcat(portList, ";");

				*numNamesInListPtr += 1;
			}
		}
	}
	
	IOObjectRelease(serialPortIterator);
	
	return result;
}

/*	GetIndexedPortNameAndChannelInfo(index, name[MAX_OBJ_NAME+1], inputChan[MAX_OBJ_NAME+1], outputChan[MAX_OBJ_NAME+1])

	Returns port name and channel names for indexed port whether port is open or not.
	index starts from 0.

	Returns 0 if OK, -1 if index out of range.
*/
int
GetIndexedPortNameAndChannelInfo(int index, char name[MAX_OBJ_NAME+1], char inputChan[MAX_OBJ_NAME+1], char outputChan[MAX_OBJ_NAME+1])
{
	static int initialized = 0;
	static int numPortsInList = 0;
	static char portList[1024];
	static char inDriverList[1024];
	static char outDriverList[1024];
	char* port;
	int portLen;
	char* inDriver;
	int inDriverLen;
	char* outDriver;
	int outDriverLen;
	char* p;

	// This makes a list of all available ports the first time we are called.
	if (!initialized) {
		ListSerialPorts(portList, inDriverList, outDriverList, &numPortsInList, sizeof(portList)-1);
		initialized = 1;
	}

	*name = 0;
	*inputChan = 0;
	*outputChan = 0;
	
	if (index >= numPortsInList)
		return -1;							// Index out of range.

	// Search through semi-colon-separated lists.
	port = portList;
	inDriver = inDriverList;
	outDriver = outDriverList;
	while(index > 0) {
		port = strchr(port, ';');
		if (port == NULL)
			return -1;						// Should never happen.
		port += 1;							// Point past semicolon.
		
		inDriver = strchr(inDriver, ';');
		if (inDriver == NULL)
			return -1;						// Should never happen.
		inDriver += 1;						// Point past semicolon.
		
		outDriver = strchr(outDriver, ';');
		if (outDriver == NULL)
			return -1;						// Should never happen.
		outDriver += 1;						// Point past semicolon.
		
		index -= 1;
	}

	p = strchr(port, ';');
	if (p == NULL)
		return -1;							// Should never happen.
	portLen = p - port;

	p = strchr(inDriver, ';');
	if (p == NULL)
		return -1;							// Should never happen.
	inDriverLen = p - inDriver;

	p = strchr(outDriver, ';');
	if (p == NULL)
		return -1;							// Should never happen.
	outDriverLen = p - outDriver;

	strncat(name, port, portLen);
	strncat(inputChan, inDriver, inDriverLen);
	strncat(outputChan, outDriver, outDriverLen);

	return 0;
}

int
CloseSerialDrivers(VDTPortPtr pp)
{
	KillSerialIO(pp);
	close(pp->fileDescriptor);
	pp->fileDescriptor = 0;

	return 0;
}

int
OpenCommPort(VDTPortPtr pp)			// Much of this is taken from Apple's SerialPortSample.
{
	int fileDescriptor = -1;
	int handshake;
	struct termios portSettings;
	const char* portName;
	char message[512];
	int err = 0;
	
	portName = pp->name;
	
	/*	Open the serial port read/write, with no controlling terminal, and don't wait for a connection.
		The O_NONBLOCK flag also causes subsequent I/O on the device to be non-blocking.
		See open(2) ("man 2 open") for details.
	*/
	fileDescriptor = open(pp->inputChan, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fileDescriptor == -1) {
		sprintf(message, "Error opening serial port \"%s\"", portName);
  		PrintUnixErrMessage(message);
		err = VDT_ERR_UNIX_ERROR;
		goto done;
	}

	/*	This prevents other processes from opening this port (except that root processes
		can still open it. See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
	*/
	if (ioctl(fileDescriptor, TIOCEXCL) == -1) {
		sprintf(message, "Error setting TIOCEXCL on \"%s\"", portName);
  		PrintUnixErrMessage(message);
		err = VDT_ERR_UNIX_ERROR;
		goto done;
	}
	
	/*	Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
		See fcntl(2) ("man 2 fcntl") for details.
	*/
	if (fcntl(fileDescriptor, F_SETFL, 0) == -1) {
		sprintf(message, "Error clearing O_NONBLOCK on \"%s\"", portName);
  		PrintUnixErrMessage(message);
		err = VDT_ERR_UNIX_ERROR;
		goto done;
	}
	
	// Get the current portSettings and save them so we can restore the default settings later.
	if (err = GetPortTermiosSettings(fileDescriptor, pp->name, &portSettings))	// This prints diagnostic if error.
		goto done;
	
	/*	Create buffer to store termios settings so they can be restored when we
		close the port. This is the Unix recommended practice.
	*/
	pp->savedTermiosPtr = (char*)NewPtr(sizeof(portSettings));
	if (pp->savedTermiosPtr == NULL) {
		err = NOMEM;
		goto done;
	}
	memcpy(pp->savedTermiosPtr, &portSettings, sizeof(portSettings));

	/*	The Apple sample code calls cfmakeraw here and then does some calls to set up
		communication parameters (baud rates, etc.).
	
		SetTermiosToDefault does all of that. It sets all of the fields of the termios
		structure to an acceptable initial state.
	*/
	SetTermiosToDefault(&portSettings);

	// Cause the new portSettings to take effect immediately.
	if (err = SetPortTermiosSettings(fileDescriptor, pp->name, &portSettings))	// This prints diagnostic if error.
		goto done;

	// To set the modem handshake lines, use the following ioctls.
	// See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
	
	// Assert Data Terminal Ready (DTR)
	if (ioctl(fileDescriptor, TIOCSDTR) == -1) {
		sprintf(message, "Error asserting DTR on \"%s\"", portName);
  		PrintUnixErrMessage(message);
		err = VDT_ERR_UNIX_ERROR;
		goto done;
	}
	
	// The Apple sample code cleared DTR here but I can't imagine why.
	
	// Set the modem lines depending on the bits set in handshake (I'm not sure why the sample code does this).
	handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
	if (err = SetPortHandshakeLines(fileDescriptor, portName, handshake))
		goto done;
	
	// Success
	pp->fileDescriptor = fileDescriptor;
	
	return 0;
	
	// Failure path
done:
	if (fileDescriptor != -1)
		close(fileDescriptor);

	return err;
}

int
SetInputBufferSize(VDTPortPtr pp, int inputBufSize)
{
	/*	This is supposed to set the size of the input buffer but the symbol
		TXSETIHOG is not defined in any header file that I can find.

		There appears to be no way to set the size of the serial port input buffer
		on OS X. What little documentation there is on this subject is vague at best.
		Therefore, the "VDT buffer" command is a NOP on OS X. It is also
		not clear what the default size of the serial port buffer is. Applications
		that require the transmission of large amounts of data (kilobytes) may overflow
		the input buffer. You might be able to prevent this by using hardware handshaking,
		but hardware handshaking may not work in all cases.
	*/

	if (inputBufSize < MINBUFSIZE)
		inputBufSize = MINBUFSIZE;
	if (inputBufSize > MAXBUFSIZE)
		inputBufSize = MAXBUFSIZE;

	// ioctl(pp->fileDescriptor, TXSETIHOG, &inputBufSize);
	
	return 0;
}

/*	SetCommPortSettings(pp)

	Sets comm port settings based on settings in VDTPort.
	Returns 0 if OK or error code.
*/
int
SetCommPortSettings(VDTPortPtr pp)
{
	struct termios portSettings;
	int err;

	if (!VDTPortIsOpen(pp))
		return 0;										// NOP if port is not open.
		
	if (err = GetPortTermiosSettings(pp->fileDescriptor, pp->name, &portSettings))	// This prints diagnostic if error.
		return err;
	
	/*	Note about XON/XOFF
	
		The basic Unix documentation for this is vague. I finally found a decent
		explanation of it at:
			http://ou800doc.caldera.com/SDK_sysprog/_TTY_Flow_Control.html

		Although most documentation shows setting the IXANY, I don't see the rationale
		for it. It makes any character restart communication, not just XON. But what is
		the point of that. XON should restart the flow.
		
		I tested XON/XOFF for both input and output and it seems to work but of
		course it works only with plain text data, not with binary data which may
		contain bytes with the XON or XOFF values.
	*/
	
	/*	Note about hardware handshaking
	
		Hardware flow control is supported via the DTR and CTS signals which should
		be cross-wired on the cable. Then enable DTR-CTS flow control on both sides.
		Hardware flow control using the RTS and DSR signals is not supported.
	*/
	
	// Output flow control
	portSettings.c_iflag &= ~(IXON | IXANY);			// Turn XON/XOFF for output off.
	portSettings.c_cflag &= ~CCTS_OFLOW;				// Turn CTS output flow control off.
	portSettings.c_cflag &= ~CDSR_OFLOW;				// Turn DSR output flow control off.
	portSettings.c_cflag &= ~CCAR_OFLOW;				// Turn DCD output flow control off.
	switch(pp->outShake) {
		case 1:											// Hardware flow control.
			portSettings.c_cflag |= CCTS_OFLOW;			// Turn CTS output flow control on.
			break;	

		case 2:											// XON/XOFF
			portSettings.c_iflag |= IXON;				// Turn output flow control on.
			break;
	}
	
	// Input flow control
	portSettings.c_iflag &= ~(IXOFF | IXANY);			// Turn XON/XOFF for input off.
	portSettings.c_cflag &= ~CRTS_IFLOW;				// Turn RTS input flow control off.
	portSettings.c_cflag &= ~CDTR_IFLOW;				// Turn DTR input flow control off.
	switch(pp->outShake) {
		case 1:											// Hardware flow control.
			portSettings.c_cflag |= CDTR_IFLOW;			// Turn DTR input flow control on.
			break;	

		case 2:											// XON/XOFF
			portSettings.c_iflag |= IXOFF;				// Turn input flow control on.
			break;
	}
	
	// Baud rate
	{
		int baudCode, baudRate;
		baudCode = ValidateBaudCode(pp->baud);
		BaudCodeToBaudRate(baudCode, &baudRate);
		portSettings.c_ispeed = portSettings.c_ospeed = baudRate;
	}
	
	// Parity
	portSettings.c_cflag &= ~PARENB;					// Turn parity off (no parity).
	switch(pp->parity) {
		case 1:											// 1 = Odd parity
			portSettings.c_cflag |= PARENB | PARODD;
			break;

		case 2:											// 2 = Even parity
			portSettings.c_cflag |= PARENB;
			break;
	}
	
	// Stop bits
	portSettings.c_cflag &= ~CSTOPB;					// Set to one stop bit.
	if (pp->stopbits != 0)
		portSettings.c_cflag |= CSTOPB;					// Set to two stop bit.
	
	// Data bits
	portSettings.c_cflag &= ~CSIZE;
	if (pp->databits != 0)
		portSettings.c_cflag |= CS8;					// 8 data bits
	else
		portSettings.c_cflag |= CS7;					// 7 data bits
	
	// DumpTermios(&portSettings);							// For testing.
	
	if (err = SetPortTermiosSettings(pp->fileDescriptor, pp->name, &portSettings))	// This prints diagnostic if error.
		return err;

	if (err = SetInputBufferSize(pp, pp->inputBufferSize))
		return err;
	
	#if 0
		// RTS

		/*	As far as I can tell, there is no RTS signal on the Macintosh
			MiniDIN-8 serial connector.
		*/

		{
			int handshake;
			if (err = GetPortHandshakeLines(pp->fileDescriptor, pp->name, &handshake))
				return err;
			if (pp->disableRTS)
				handshake &= ~TIOCM_RTS;
			else
				handshake |= TIOCM_RTS;
			if (err = SetPortHandshakeLines(pp->fileDescriptor, pp->name, handshake))
				return err;
		}
	#endif

	return 0;
}

/*	CloseCommPort(pp)

	Closes the specified port if it is open.
	Does nothing it if is not open.
	
	Function result is 0 if OK (including if port not open) or an error code.
*/
int
CloseCommPort(VDTPortPtr pp)
{
	int err;

	/*	Restore termios settings to what they were when we opened the port.
		This is the Unix recommended practice.
	*/
	if (pp->savedTermiosPtr != NULL) {
		SetPortTermiosSettings(pp->fileDescriptor, pp->name, (struct termios*)pp->savedTermiosPtr);
		DisposePtr(pp->savedTermiosPtr);
		pp->savedTermiosPtr = NULL;
	}
	
	err = CloseSerialDrivers(pp);
	return err;
}

