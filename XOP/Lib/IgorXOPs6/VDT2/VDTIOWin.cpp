/*	VDTIOWin.c -- Windows specific routines for VDT XOP.

	HR, 10/4/97:
		The Windows SDK Communications documentation leads me to believe that
		the same routines that drive the serial ports should also work for the
		parallel port. However, I have not been able to get the parallel port
		to work. I was able to send characters to a parallel port printer but I
		was not able to establish communications via LPT1 between two machines
		running VDT. A search of Microsoft documentation did not produce any
		illumination. Therefore, I commented out "LPT1;LPT2;" in the ListCommPorts
		routine.
		
	HR, 991207:
		Several users want to write to the parallel port so I decided to uncomment
		the "LPT1;LPT2;" and create an experimental version, even though I don't
		know how to read from a parallel port.
		
	HR, 021107:
		I am unable to get parallel ports to work at all on Windows 2000.
		I get an error from SetCommTimeouts when I try to use LPT1.
		Therefore, I am removing support for parallel ports.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

static void ClearCharsInLocalBuffer(VDTPortPtr pp);

/* *** Low Level IO Routines *** */

/*	CommErrorFlagsToErrorCode(errorFlags, defaultErrorCode)

	errorFlags is the code returned by ClearCommError.
	This function returns a standard IGOR or VDT error code.
	If none of the bits in errorFlags is set, it returns defaultErrorCode.
*/
static int
CommErrorFlagsToErrorCode(int errorFlags, int defaultErrorCode)
{
	if (errorFlags & CE_BREAK)
		return VDT_ERR_BREAK;
	
	if (errorFlags & CE_DNS)
		return VDT_ERR_NO_PARALLEL_DEVICE;
	
	if (errorFlags & CE_FRAME)
		return VDT_ERR_FRAME;
	
	if (errorFlags & CE_IOE)
		return VDT_ERR_IO_ERROR;
	
	if (errorFlags & CE_MODE)
		return VDT_ERR_MODE_NOT_SUPPORTED;
	
	if (errorFlags & CE_OOP)
		return VDT_ERR_PARALLEL_DEVICE_OUT_OF_PAPER;
	
	if (errorFlags & CE_OVERRUN)
		return VDT_ERR_OVERRUN_ERROR;
	
	if (errorFlags & CE_PTO)
		return VDT_ERR_PARALLEL_TIMEOUT;
	
	if (errorFlags & CE_RXOVER)
		return VDT_ERR_INPUT_BUFFER_OVERRUN;
	
	if (errorFlags & CE_RXPARITY)
		return VDT_ERR_PARITY_ERROR;
	
	if (errorFlags & CE_TXFULL)
		return VDT_ERR_OUTPUT_BUFFER_OVERFLOW;

	return defaultErrorCode;
}

static void
DisposeEventHandle(VDTPortPtr pp, int which)	// which = 0 for input event, 1 for output event.
{
	if (which == 0) {
		if (pp->ai.hEvent != NULL) {
			CloseHandle(pp->ai.hEvent);
			pp->ai.hEvent = NULL;
		}
	}
	else {
		if (pp->ao.hEvent != NULL) {
			CloseHandle(pp->ao.hEvent);
			pp->ao.hEvent = NULL;
		}
	}
}

void
WaitForSerialOutputToFinish(VDTPortPtr pp)
{
	// This is a NOP on Windows.
}

void
KillSerialIO(VDTPortPtr pp)
{
	int flags;
	
	if (pp->commH) {
		flags = PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR;
		PurgeComm(pp->commH, flags);
	}
	DisposeEventHandle(pp, 0);
	DisposeEventHandle(pp, 1);
	ClearCharsInLocalBuffer(pp);
}

/*	WriteCharsAsync(pp, buffer, length)

	Writes length characters from buffer asynchronously to specified output port.
*/
int
WriteCharsAsync(VDTPortPtr pp, char *buffer, VDTByteCount length)	/* BUFFER MUST BE NON-RELOCATABLE */
{
	int err = 0;

	static DWORD asyncBytesWritten = 0;
	
	if (length == 0)
		return 0;			// HR, 980407.

	#ifdef IGOR64
		if (length > UINT_MAX)
			return WRITE_TOO_LONG;
	#endif
	
	DisposeEventHandle(pp, 1);								// Should really not be necessary.
	pp->ao.hEvent = CreateEvent(NULL, 1, 0, NULL);			// For use with GetOverlappedResult. Event is disposed by KillSerialIO.
	if (pp->ao.hEvent == NULL) {
		err = WMGetLastError();
		return err;
	}
	
	if (WriteFile(pp->commH, buffer, (DWORD)length, &asyncBytesWritten, &pp->ao) == 0) {
		err = GetLastError();
		if (err == ERROR_IO_PENDING)						// Not really an error - just means I/O is not yet finished.
			err = 0;
		else
			err = WindowsErrorToIgorError(err);
		return err;
	}
	
	return 0;
}

/*	WriteAsyncResult(pp, donePtr)

	Returns via donePtr the truth that transmission is finished.

	Function result is 0 if OK or non-zero error code.
*/
int
WriteAsyncResult(VDTPortPtr pp, int* donePtr)
{
	DWORD bytesTransferred;
	int err;
	
	if (pp->ao.hEvent == NULL) {
		/*	If here, we were called when there was no write in progress. This happens
			when a routine (e.g., Send File) checks to make sure that nothing is being
			written before sending more.
		*/
		*donePtr = 1;
		return 0;
	}
	
	if (GetOverlappedResult(pp->commH, &pp->ao, &bytesTransferred, 0) != 0) {
		/*	Here if function "succeeded". Does that mean transmission finished?
			Documentation implies this but does not say it.

			Without this wait, Send File does not send anything but I get no errors
			from the system. I don't know why.
		*/
		if (GetOverlappedResult(pp->commH, &pp->ao, &bytesTransferred, 1) != 0) {
			err = 0;				// Normal completion.
		}	
		else {
			err = GetLastError();
			err = WindowsErrorToIgorError(err);
		}
		*donePtr = 1;
		DisposeEventHandle(pp, 1);
	}
	else {
		// Here if function "failed". But if error is ERROR_IO_INCOMPLETE, this just means there is more to send.
		err = GetLastError();
		if (err == ERROR_IO_INCOMPLETE) {		// Not really an error - just means I/O is not yet finished.
			*donePtr = 0;
			err = 0;
		}
		else {
			*donePtr = 1;
			err = WindowsErrorToIgorError(err);
			
			/*	I believe that PurgeComm is necessary to stop the WriteFile operation so that
				when we dispose pp->ao.hEvent, we are not pulling the rug out from under
				WriteFile.
			*/
			PurgeComm(pp->commH, PURGE_TXABORT | PURGE_TXCLEAR);	
			DisposeEventHandle(pp, 1);
		}
	}

	return err;
}

int
WriteChars(							/* writes characters to specified output port */
	VDTPortPtr pp,
	UInt32 timeout,					// This is the tick count before which the write must be finished or 0 if no timeout.
	char *buffer,
	VDTByteCount *lengthPtr)
{
	int done;
	int result;
	
	if (result = WriteCharsAsync(pp, buffer, *lengthPtr))	/* start write */
		return result;

	do {													/* now check for abort or timeout */
		result = WriteAsyncResult(pp, &done);
		if (result || done)								
			return result;
		result = CheckAbort(timeout);						/* check for cmd-dot or timeout */
		if (result) {
			KillSerialIO(pp);								/* abort output */
			return(result == -1 ? -1:TIME_OUT_WRITE);
		}
	} while(1);
}

/*	Input Buffering

	On Windows, there is no way for us to know how many characters are in the comm
	device's input buffer without reading them. The routines in VDTIO.c and the way
	that VDT interfaces to IGOR are predicated on the ability to know this. Therefore,
	when VDTIO.c calls NumCharsInSerialInputBuffer, we have to do an actual read with
	a comm device timeout of zero. This transfers whatever characters are available into
	a buffer local to this file. When VDTIO.c calls ReadCharsSync, we first return characters
	from our local buffer.
*/
#define LOCAL_BUFFER_SIZE 1024

static void
ClearCharsInLocalBuffer(VDTPortPtr pp)			// Called by NumCharsInSerialInputBuffer() and KillSerialIO.
{
	pp->charsInLocalBuffer = 0;
}

VDTByteCount
NumCharsInSerialInputBuffer(VDTPortPtr pp)
{
	DWORD numBytesToRead, bytesRead;
	char* p;
	int err;
	
	if (pp->localBuffer == NULL) {
		ClearCharsInLocalBuffer(pp);
		pp->localBuffer = (char*)NewPtr(LOCAL_BUFFER_SIZE);
		if (pp->localBuffer == NULL)
			return 0;
	}
	
	if (pp->charsInLocalBuffer >= LOCAL_BUFFER_SIZE)
		return pp->charsInLocalBuffer;			// Can't hold any more local characters.
	
	/*	Note: We assume that SetCommTimeouts has been called so that ReadFile returns
		immediately with whatever characters have already been read.
	*/
	pp->ai.Offset = 0;
	pp->ai.OffsetHigh = 0;
	pp->ai.hEvent = CreateEvent(NULL, 1, 0, NULL);		// Manual reset event required for ReadFile.
	if (pp->ai.hEvent == NULL) {
		err = GetLastError();		// For debugging only.
		return 0;
	}
	
	p = pp->localBuffer + pp->charsInLocalBuffer;					// Where to put the next byte.
	numBytesToRead = LOCAL_BUFFER_SIZE - pp->charsInLocalBuffer;	// The amount of space we have left.
	if (ReadFile(pp->commH, p, numBytesToRead, &bytesRead, &pp->ai) == 0) {
		err = GetLastError();		// For debugging only.
		DisposeEventHandle(pp, 0);
		return 0;
	}
	
	// This waits till ReadFile is finished which should be immediately.
	if (GetOverlappedResult(pp->commH, &pp->ai, &bytesRead, 1) == 0) {
		err = GetLastError();		// For debugging only.
		DisposeEventHandle(pp, 0);
		return 0;
	}	
	
	DisposeEventHandle(pp, 0);
	
	pp->charsInLocalBuffer += bytesRead;
	return pp->charsInLocalBuffer;
}

int
ReadCharsSync(VDTPortPtr pp, UInt32 timeout, VDTByteCount numCharsToRead, VDTByteCount* numCharsReadPtr, char* buffer)
{
	int chunkSize=0;
	int numCharsToLeftRead;
	int result=0;
	
	*numCharsReadPtr = 0;
	numCharsToLeftRead = (int)numCharsToRead;

	while (numCharsToLeftRead > 0) {						// Want more chars?
		if (result = CheckAbort(timeout)) {					// Check for cmd-dot or timeout.
			result = result == -1 ? -1:TIME_OUT_READ;
			break;
		}
		
		if (NumCharsInSerialInputBuffer(pp) > 0) {			// This reads characters if they are available.
			chunkSize = numCharsToLeftRead < pp->charsInLocalBuffer ? numCharsToLeftRead:pp->charsInLocalBuffer;
			memcpy(buffer, pp->localBuffer, chunkSize);
			pp->charsInLocalBuffer -= chunkSize;
			memmove(pp->localBuffer, pp->localBuffer+chunkSize, pp->charsInLocalBuffer);	/* shift localBuffer */
			buffer += chunkSize;
			*numCharsReadPtr += chunkSize;
			numCharsToLeftRead -= chunkSize;
		}
	}

	return result;
}

/*	GetSerialStatus()

	Windows does not support querying the error status of the input channel and the output channel
	independently is. Regardless of the value of the channelNumber parameter, you get the
	combined error status of both channels.
	
	*xOff and *xOffHold will always be zero because Windows provides no way of determining
	the correct values.
	
	The bits of the cumErrs output have the following meanings:
	
	Bit 0:	Input buffer overrun.					Supported on Macintosh and Windows.
	Bit 3:	The break signal was asserted.			"
	Bit 4:	A parity error occurred.				"
	Bit 5:	A hardware overrun error occurred.		"
	Bit 6:	A framing error occurred.				"
	Bit 8:	Parallel device not selected.			Supported on Windows only.
	Bit 9:	General I/O error.						"
	Bit 10: Requested mode not supported.			"
	Bit 11: Parallel device out of paper.			"
	Bit 12: Paralled device timed out.				"
	Bit 13: Output buffer overrun.					"
*/
int
GetSerialStatus(VDTPortPtr pp, int channel, int* cumErrs, int* xOff, int* readPending, int* writePending, int* ctsHold, int* xOffHold)
{
	COMSTAT comStat;
	DWORD errorFlags;
	DWORD commModemStatus;
	int doneWriting;
	int err;

	if (GetCommModemStatus(pp->commH, &commModemStatus) == 0) {
		err = WMGetLastError();
		return err;	
	}

	*cumErrs = 0;
	if (ClearCommError(pp->commH, &errorFlags, &comStat) == 0) {
		err = WMGetLastError();
		return err;
	}
	if (errorFlags & CE_RXOVER)
		*cumErrs |= 1;
	if (errorFlags & CE_BREAK)
		*cumErrs |= 8;
	if (errorFlags & CE_RXPARITY)
		*cumErrs |= 16;
	if (errorFlags & CE_OVERRUN)		// I assume that this is a hardware buffer overrun and CE_RXOVER is a software
		*cumErrs |= 32;					//  buffer overrun. Windows documentation is not clear on this.
	if (errorFlags & CE_FRAME)
		*cumErrs |= 64;
	if (errorFlags & CE_DNS)
		*cumErrs |= 256;
	if (errorFlags & CE_IOE)
		*cumErrs |= 512;
	if (errorFlags & CE_MODE)
		*cumErrs |= 1024;
	if (errorFlags & CE_OOP)
		*cumErrs |= 2048;
	if (errorFlags & CE_PTO)
		*cumErrs |= 4096;
	if (errorFlags & CE_TXFULL)
		*cumErrs |= 8192;

	*xOff = 0;							// Not supported on Windows because we have no way to determine this.

	*readPending = 0;					// We always do synchronous reads.

	WriteAsyncResult(pp, &doneWriting);
	*writePending = doneWriting == 0;

	*ctsHold = (commModemStatus & MS_CTS_ON) ? 1:0;

	*xOffHold = 0;						// Not supported on Windows because we have no way to determine this.
	
	return 0;
}


/* *** Initialization and Setup *** */

/*	ListCommPorts(portList, maxLen, numNamesInListPtr)

	portList, is a pointer to character arrays that can hold maxLen characters
	plus a null terminator.
	
	On output, portList contains a semicolon-separated list of names.
	
	On output, *numNamesInListPtr contains the number of names added to the list.
	
	Function result is 0 if OK or error code.
	
	If there are too many ports to fit in the list, the function result is
	STR_TOO_LONG and *numNamesInListPtr contains the number of names added before
	there was not enough room.

	HR, 030515, 2.20
		Previously this was hardballed to COM1..COM4. Users asked for access
		to more ports.
		
		I found out how to determine what ports are actually available on the
		machine but the code works only for Windows NT or later.

		So now it will return COM1..COM8 on Windows 95/98/ME. On NT/2000/XP
		or later, it will return that actually installed COM ports.
*/
static int
ListCommPorts(char* portList, int* numNamesInListPtr, int maxLen)
{

	MemClear(portList, maxLen+1);
	*numNamesInListPtr = 0;

	/*	This code is based on http://www.systech.com/support/ntapp/enumeratingports.htm
		
		I could not find out how to do this from Microsoft documentation.

		Different, more complicated code is required for Windows 95/98/ME.
		Therefore, we use this code for Windows NT and later only.
	*/
	{
		OSVERSIONINFO vi;
		
		vi.dwOSVersionInfoSize = sizeof(vi);
		GetVersionEx(&vi);

		if (vi.dwPlatformId >= VER_PLATFORM_WIN32_NT) {		// Windows NT or later?
			DWORD resVal;
			HKEY hSerialCommKey = NULL;
			DWORD i, status;
			DWORD keyType, nameBufSize, dataBufSize;
			UCHAR valueKeyName[64], valueKeyData[64];

			// First - Get access to the registry key we need
			resVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hSerialCommKey);

			if (resVal == ERROR_SUCCESS) {
				// Read all the registry entries under this key.

				*portList = 0;

				for(i = 0; ; i++) {
					nameBufSize = sizeof(valueKeyName);
					dataBufSize = sizeof(valueKeyData);
					status = RegEnumValue(hSerialCommKey, i, (CHAR *)valueKeyName, &nameBufSize,
								NULL, &keyType, (UCHAR *)valueKeyData, &dataBufSize);

					if (status != ERROR_SUCCESS)
						break;								// We hit the end of the list.
					
					if ((int)strlen(portList) + (int)strlen((char*)valueKeyData) + 1 <= maxLen) {
						strcat(portList, (char*)valueKeyData);
						strcat(portList, ";");
					}
				}
				
				*numNamesInListPtr = i;

				RegCloseKey(hSerialCommKey);
			 }
		}
	}

	if (*numNamesInListPtr == 0) {
		// We are running on Windows 95/98/ME or the code above failed for some reason.
		strcpy(portList, "COM1;COM2;COM3;COM4;COM5;COM6;COM7;COM8;");
		*numNamesInListPtr = 8;
	}

	#if 0		// See note above from 991207.
		{
			char parallelPortList[64];

			strcpy(parallelPortList, "LPT1;LPT2;");
			if (strlen(portList) + strlen(parallelPortList) <= maxLen) {
				strcat(portList, parallelPortList);
				*numNamesInListPtr += 2;
			}
		}
	#endif

	return 0;	
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
	static char portList[256];
	char* port;
	int portLen;
	char* p;

	// This makes a list of all available ports the first time we are called.
	if (!initialized) {
		ListCommPorts(portList, &numPortsInList, sizeof(portList)-1);
		initialized = 1;
	}

	*name = 0;
	*inputChan = 0;
	*outputChan = 0;
	
	if (index >= numPortsInList)
		return -1;							// Index out of range.

	// Search through semi-colon-separated lists.
	port = portList;
	while(index > 0) {
		port = strchr(port, ';');
		if (port == NULL)
			return -1;						// Should never happen.
		port += 1;							// Point past semicolon.
		
		index -= 1;
	}

	p = strchr(port, ';');
	if (p == NULL)
		return -1;							// Should never happen.
	portLen = (int)(p - port);

	strncat(name, port, portLen);
	strncat(inputChan, port, portLen);
	strncat(outputChan, port, portLen);

	return 0;
}

int
CloseSerialDrivers(VDTPortPtr pp)
{
	int err;

	KillSerialIO(pp);
	if (pp->commH) {
		if (CloseHandle(pp->commH) == 0) {
			err = WMGetLastError();
			return err;
		}
		pp->commH = NULL;
	}
	return 0;
}

int
SetInputBufferSize(VDTPortPtr pp, int inputBufSize)			// This aborts any I/O in progress.
{
	DWORD numCommPropBytes;
	LPCOMMPROP lpCommProp;
	int outputBufSize;
	
	if (inputBufSize < MINBUFSIZE)
		inputBufSize = MINBUFSIZE;
	if (inputBufSize > MAXBUFSIZE)
		inputBufSize = MAXBUFSIZE;
	
	if (pp->commH == NULL) {								// Port not open? (Can't use VDTPortIsOpen here because it is not set until initialization of the port is finished but this routine is called before that.)
		pp->inputBufferSize = inputBufSize;
		return 0;
	}

	numCommPropBytes = sizeof(COMMPROP);
	numCommPropBytes += sizeof(MODEMDEVCAPS);				// HR, 031212: See Microsoft Knowledge Base Article 162136.
	lpCommProp = (LPCOMMPROP)malloc(numCommPropBytes);

	pp->inputBufferSize = inputBufSize;						// This is actually just a suggestion to SetupComm.
	MemClear(lpCommProp, numCommPropBytes);					// HR, 031212: Added this.
	if (GetCommProperties(pp->commH, lpCommProp)) {
		if (lpCommProp->dwCurrentRxQueue == inputBufSize)
			goto done;										// No change needed.
		outputBufSize = lpCommProp->dwCurrentTxQueue;
	}
	else {
		outputBufSize = 1024;	
	}
	SetupComm(pp->commH, inputBufSize, outputBufSize);		// This aborts any I/O in progress.
	MemClear(lpCommProp, numCommPropBytes);					// HR, 031212: Added this.
	if (GetCommProperties(pp->commH, lpCommProp))
		pp->inputBufferSize = lpCommProp->dwCurrentRxQueue;

done:
	free(lpCommProp);

	return 0;
}

/*	SetCommPortSettings(pp)

	Sets comm port settings based on settings in VDTPort.
	Returns 0 if OK or error code.
*/
int
SetCommPortSettings(VDTPortPtr pp)
{
	DCB commState;
	int XonLim, XoffLim;
	int baudCode, baudRate;
	int err;
	
	if (!VDTPortIsOpen(pp))
		return 0;										// NOP if port is not open.
	
	// We do this first because we need an accurate reading of the size of the input buffer for XonLim/XoffLim calculations.
	if (err = SetInputBufferSize(pp, pp->inputBufferSize))		// This can change pp->inputBufferSize if driver does not honor our request.
		return err;

	commState.DCBlength = sizeof(commState);
	if (GetCommState(pp->commH, &commState) == 0) {
		err = WMGetLastError();
		return err;
	}
	
	// These only have an effect if XOFF/XON input handshaking is selected.
	XoffLim = 1 * pp->inputBufferSize / 5;				// Send XOFF when buffer is 1/5 free (4/5 full).
	XonLim = 1 * pp->inputBufferSize / 2;				// Send XON when buffer is 1/2 free (1/2 full).
	
	baudCode = ValidateBaudCode(pp->baud);
	BaudCodeToBaudRate(baudCode, &baudRate);
	commState.BaudRate = baudRate;
	commState.fBinary = 1;
	commState.fParity = pp->parity != 0;
	commState.fOutxCtsFlow = pp->outShake == 1;
	commState.fOutxDsrFlow = 0;
	commState.fDtrControl = pp->inShake == 1 ? DTR_CONTROL_HANDSHAKE : DTR_CONTROL_ENABLE;
	commState.fDsrSensitivity = 0;
	commState.fTXContinueOnXoff = 1;				// Documentation on this is confusing.
	commState.fOutX = pp->outShake == 2;
	commState.fInX = pp->inShake == 2;
	commState.fErrorChar = 0;
	commState.fNull = 0;
	commState.fRtsControl = pp->disableRTS ? RTS_CONTROL_DISABLE : RTS_CONTROL_ENABLE;
	commState.fAbortOnError = 0;
	commState.XonLim = XonLim;
	commState.XoffLim = XoffLim;
	commState.ByteSize = pp->databits ? 8:7;
	switch(pp->parity) {
		case 1:
			commState.Parity = ODDPARITY;
			break;
		case 2:
			commState.Parity = EVENPARITY;
			break;
		default:
			commState.Parity = NOPARITY;
			break;
	}
	commState.StopBits = pp->stopbits ? TWOSTOPBITS:ONESTOPBIT;
	commState.XonChar = 17;							// Control-Q.
	commState.XoffChar = 19;						// Control-S.
	// commState.ErrorChar = <unchanged>;			// Leave this unchanged. We don't care about it.
	// commState.EofChar = <unchanged>;				// Leave this unchanged. We don't care about it.
	// commState.EvtChar = <unchanged>;				// Leave this unchanged. We don't care about it.
	if (SetCommState(pp->commH, &commState) == 0) {
		err = WMGetLastError();
		return err;
	}
	
	return 0;
}

int
OpenCommPort(VDTPortPtr pp)
{
	COMMTIMEOUTS to;
	char portName[64];
	int err;
	
	// HR, 030516: For ports beyond COM9, you have to use special syntax (e.g., \\.\COM10).
	strcpy(portName, pp->name);
	if (strlen(portName) > 4)
		sprintf(portName, "\\\\.\\%s", pp->name);

	pp->commH = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (pp->commH == INVALID_HANDLE_VALUE) {
		err = GetLastError();
		switch(err) {
			case ERROR_FILE_NOT_FOUND:
				err = VDT_ERR_CANT_OPEN_PORT;
				break;

			case ERROR_ACCESS_DENIED:
				err = VDT_ERR_PORT_IN_USE;
				break;
			
			default:
				err = WindowsErrorToIgorError(err);
				break;
		}
		return err;
	}
	
	/*	This combination of values specifies that a read is to return immediately with
		whatever characters have already been received, even if zero. This is assumed in
		the NumCharsInSerialInputBuffer routine.
	*/
	to.ReadIntervalTimeout = MAXDWORD;
	to.ReadTotalTimeoutConstant = 0;
	to.ReadTotalTimeoutMultiplier = 0;

	// These values mean that there is no timeout on write. We handle timeout with our own routines instead.
	to.WriteTotalTimeoutConstant = 0;
	to.WriteTotalTimeoutMultiplier = 0;
	
	if (SetCommTimeouts(pp->commH, &to) == 0) {
		CloseHandle(pp->commH);
		pp->commH = NULL;
		err = WMGetLastError();
		return err;
	}
	
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
	
	err = CloseSerialDrivers(pp);
	if (pp->localBuffer != NULL) {
		DisposePtr(pp->localBuffer);
		pp->localBuffer = NULL;
	}

	return err;
}
