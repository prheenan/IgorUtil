/*	VDTTerminal.c -- implements the dumb terminal part of VDT.

	9/14/97 for version 2.0
		Created this file by splitting dumb-terminal-related routines out of
		VDT.c.
	
	3/23/98
		Changed to use standard C file I/O for platform independence.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

// Global Variables
static Handle gSendTextH = NULL;			// Contains text being sent by VDTSendText.
static VDTByteCount gCharOffset;			// Offset to next character to xmit/receive.
static VDTByteCount gCharCount;				// # of characters left to xmit/receive.
static XOP_FILE_REF gFileRef = NULL;		// Referenct to file being sent/received.
static VDTPortPtr gTermPort = NULL;			// Pointer to VDTPort used for dumb terminal operations.
extern Handle gVDTTU;						// In VDT.c.

VDTPortPtr
VDTGetTerminalPortPtr(void)				// Returns pointer to VDTPort for port being used as dumb terminal, or NULL.
{
	return gTermPort;
}

void
VDTSetTerminalPortPtr(VDTPortPtr pp)	// Sets VDTPort used as dumb terminal. Can be NULL.
{
	if (pp == gTermPort)
		return;

	gTermPort = pp;
	UpdateStatusArea();
}

/*	VDTGetOpenAndCheckTerminalPortPtr(ppp, displayDialogIfPortSelectedButNotOpenable)
	
	This is called when we are about to attempt to use the terminal port.
	
	If no terminal port is selected, it displays a dialog (if requested) returns NULL via ppp.
	
	Otherwise, if the terminal port is closed, it tries to open it. If this
	results in an error it displays a dialog (if requested) and returns NULL via ppp.

	If the port was open already or was successfully opened here, it returns
	a non-NULL VDTPortPtr via ppp.
	
	The function result is 0 if OK or an error code.
*/
int
VDTGetOpenAndCheckTerminalPortPtr(VDTPortPtr* ppp, int displayDialogIfPortSelectedButNotOpenable)
{
	VDTPortPtr tp;
	int err;
	
	*ppp = NULL;
	
	tp = VDTGetTerminalPortPtr();
	if (tp == NULL) {
		if (displayDialogIfPortSelectedButNotOpenable)
			ErrorAlert(VDT_ERR_NO_TERMINAL_PORT_SELECTED);
		return VDT_ERR_NO_TERMINAL_PORT_SELECTED;
	}
	
	if (VDTPortIsOpen(tp)) {
		*ppp = tp;
		return 0;
	}
	
	err = OpenVDTPort(tp->name, ppp);
	if (err == 0)
		return 0;
	
	*ppp = NULL;			// Port can't be opened.
	
	if (displayDialogIfPortSelectedButNotOpenable)
		ErrorAlert(err);
	
	return err;
}

void
VDTSendTerminalChar(VDTPortPtr tp, char ch)
{
	VDTByteCount length;
	char buf[4];
	int err;
	
	if (ch == CR_CHAR) {				// Need to send end-of-line?
		switch(tp->terminalEOL) {
			case 1:						// EOL is linefeed.
				length = 1;
				buf[0] = LF_CHAR;
				break;

			case 2:						// EOL is carriage return/linefeed.
				length = 2;
				buf[0] = CR_CHAR;
				buf[1] = LF_CHAR;
				break;
		
			default:					// EOL is carriage return.
				length = 1;
				buf[0] = CR_CHAR;
				break;
		}					
		err = WriteChars(tp, 0, buf, &length);
	}
	else {
		length = 1;
		err = WriteChars(tp, 0, &ch, &length);
	}
	
	if (err != 0)
		ErrorAlert(err);
}

/*	VDTAbortTerminalOperation()

	VDTAbort stops the current terminal operation which might be a file transfer,
	a file receive a text transfer or some other operation.
*/
void
VDTAbortTerminalOperation(void)
{
	VDTPortPtr tp;
	
	tp = VDTGetTerminalPortPtr();
	if (tp == NULL)
		return;
	
	tp->lastTerminalChar = 0;
	
	if (!VDTPortIsOpen(tp))
		return;
	
	switch (tp->terminalOp) {				// What operation is in progress?
		case OP_SENDFILE:
			XOPCloseFile(gFileRef);
			gFileRef = NULL;
			KillVDTIO(tp);
			break;
		
		case OP_RECEIVEFILE:
			XOPCloseFile(gFileRef);
			gFileRef = NULL;
			KillVDTIO(tp);
			break;
		
		case OP_SENDTEXT:
			if (gSendTextH != NULL) {
				DisposeHandle(gSendTextH);
				gSendTextH = NULL;
			}
			KillVDTIO(tp);
			break;
	}
	tp->terminalOp = 0;						// No operation in progress now.
}

/*	VDTSendFile()

	VDTSendFile lets the user pick a file and transmits it.
*/
void
VDTSendFile(void)
{
	VDTPortPtr tp;
	char fullFilePath[MAX_PATH_LEN+1];
	const char* fileFilterStr;
	int err;
	
	static int fileIndex = 2;		// Used on Windows. Ignored on Macintosh.
	
	#ifdef MACIGOR
		fileFilterStr = "TEXT";		// Show text files only.
	#endif
	#ifdef WINIGOR
		fileFilterStr = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
	#endif
	
	VDTGetOpenAndCheckTerminalPortPtr(&tp, 1);			// tp may be null.
	if (tp == NULL)
		return;											// No port is selected or can't open port.
	
	if (tp->terminalOp != 0)
		return;											// Terminal operation is already in progress.
	
	*fullFilePath = 0;
	if (XOPOpenFileDialog("Choose file to send", fileFilterStr, &fileIndex, "", fullFilePath) != 0)
		return;											// User cancelled or error.

	gCharOffset = 0;
	err = XOPOpenFile(fullFilePath, 0, &gFileRef);
	if (err == 0) {
		#if (VDTByteCountIs64Bits)						// See definition of VDTByteCount
			err = XOPNumberOfBytesInFile2(gFileRef, &gCharCount);
		#else
			{
				UInt32 count;
				err = XOPNumberOfBytesInFile(gFileRef, &count);
				gCharCount = count;
			}
		#endif
		if (err != 0) {
			XOPCloseFile(gFileRef);
			gFileRef = NULL;
		}
	}
	if (err != 0) {
		VDTAlert(NULL, 1102, ERR_OPENING_FILE);
		return;											// Error opening file.
	}

	tp->lastTerminalChar = 0;
	tp->terminalOp = OP_SENDFILE;
}

/*	VDTReceiveFile()

	VDTReceiveFile lets the user pick a file and receives it.
*/
void
VDTReceiveFile(void)
{
	VDTPortPtr tp;
	char fullFilePath[MAX_PATH_LEN+1];
	const char* fileFilterStr;
	int err;
	
	static int fileIndex = 1;		// Used on Windows. Ignored on Macintosh.
	
	#ifdef MACIGOR
		fileFilterStr = "";			// Ignored on Macintosh.
	#endif
	#ifdef WINIGOR
		fileFilterStr = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
	#endif
	
	VDTGetOpenAndCheckTerminalPortPtr(&tp, 1);			// tp may be null.
	if (tp == NULL)
		return;											// No port is selected or can't open port.
	
	if (tp->terminalOp != 0)
		return;											// Terminal operation is already in progress.

	*fullFilePath = 0;
	if (XOPSaveFileDialog("Name of file to create", fileFilterStr, &fileIndex, "", "txt", fullFilePath))
		return;											// User cancelled.
		
	if (err = XOPCreateFile(fullFilePath, 1, 'IGR0', 'TEXT')) {
		VDTAlert(NULL, 1102, ERR_OPENING_FILE);
		return;
	}
	
	gCharOffset = 0;
	gCharCount = 0;
	err = XOPOpenFile(fullFilePath, 1, &gFileRef);
	if (err != 0) {
		VDTAlert(NULL, 1102, ERR_OPENING_FILE);
		return;											// Error opening file.
	}

	tp->lastTerminalChar = 0;
	
	tp->terminalOp = OP_RECEIVEFILE;
}

/*	VDTSendText()

	VDTSendText transmits the selected text or the whole window if nothing selected.
*/
void
VDTSendText(void)
{
	VDTPortPtr tp;

	VDTGetOpenAndCheckTerminalPortPtr(&tp, 1);			// tp may be null.
	if (tp == NULL)
		return;											// No port is selected or can't open port.
	
	if (tp->terminalOp != 0)
		return;											// Terminal operation is already in progress.

	if (gSendTextH != NULL) {
		DisposeHandle(gSendTextH);
		gSendTextH = NULL;
	}
	gCharOffset = 0;
	gCharCount = 0;

	if (VDTNoCharactersSelected()) {					// Send all text ?
		if (TUFetchText2(gVDTTU, NULL, NULL, &gSendTextH, NULL, 0) == 0)
			gCharCount = GetHandleSize(gSendTextH);
	}
	else {
		if (TUFetchSelectedText(gVDTTU, &gSendTextH, NULL, 0) == 0)
			gCharCount = GetHandleSize(gSendTextH);
	}
	
	if (gCharCount == 0) {
		if (gSendTextH != NULL) {
			DisposeHandle(gSendTextH);
			gSendTextH = NULL;
		}
		return;
	}
	
	tp->lastTerminalChar = 0;

	tp->terminalOp = OP_SENDTEXT;
}

/*	SetEndOfLineCharacters(tp, inbuf, numInChars, outbuf, numOutCharsPtr)
	
	This routine does what is necessary so that we can treat either CR, LF, or
	CRLF as and end-of-line when receiving characters in the terminal port.
	
	On output, outbuf contains the characters to be place into the VDT window and
	*numOutCharsPtr contains the number of characters in outbuf.
	
	It uses and may change the terminal port's lastTerminalChar field.
*/
static void
SetEndOfLineCharacters(VDTPortPtr tp, char* inbuf, VDTByteCount numInChars, char* outbuf, VDTByteCount* numOutCharsPtr)
{
	char* pIn;
	char* pOut;
	int i;
	
	*numOutCharsPtr = 0;
	pIn = inbuf;
	pOut = outbuf;
	for(i=0; i<numInChars; i++) {
		if (*pIn == LF_CHAR) {
			if (tp->lastTerminalChar == CR_CHAR) {
				// Skip this linefeed because we have already handled the preceding CR.
			}
			else {
				// Replace LF with CR because IGOR deals with CR only as end-of-line.
				*pOut++ = CR_CHAR;
				*numOutCharsPtr += 1;
			}
		}
		else {
			*pOut++ = *pIn;
			*numOutCharsPtr += 1;
		}
		tp->lastTerminalChar = *pIn++;
	}
}

void
VDTTerminalIdle(void)
{
	VDTPortPtr tp;
	#define BUFLEN 256							// Size of character buffer.
	static char buffer[BUFLEN];
	VDTByteCount numChars;
	TULoc startLoc, endLoc;
	int done;
	int result;
	
	tp = VDTGetTerminalPortPtr();				// This may be null.
	if (tp == NULL)
		return;									// No port is the terminal port.
	if (!VDTPortIsOpen(tp))
		return;
	
	switch (tp->terminalOp) {					// What operation is in progress ?
		case OP_SENDFILE:
			if (result = WriteAsyncResult(tp, &done)) {
				VDTAbortTerminalOperation();
				ErrorAlert(result);
				break;
			}
			if (!done)
				break;							// Current transmission is not yet finished.
			
			// Transmission finished. Start new transmission if more left to send.
			numChars = gCharCount;
			if (numChars == 0) {				// All done ?
				// Without this on OS X the KillVDTIO call in VDTAbortTerminalOperation would interrupt the output.
				WaitForSerialOutputToFinish(tp);

				VDTAbortTerminalOperation();
				break;
			}
			if (numChars > BUFLEN)
				numChars = BUFLEN;
			
			#ifdef IGOR64
				result = XOPReadFile64(gFileRef, numChars, buffer, NULL);
			#else
				result = XOPReadFile(gFileRef, numChars, buffer, NULL);
			#endif
			if (result != 0) {					// Error reading file or eof and no more chars ?
				VDTAbortTerminalOperation();	// Abort operation next time around.
			}
			else {
				if (result = WriteCharsAsync(tp, buffer, numChars)) {
					VDTAbortTerminalOperation();
					ErrorAlert(result);
					break;
				}
				gCharOffset += numChars;
				gCharCount -= numChars;
			}
			break;
		
		case OP_RECEIVEFILE:
			if (numChars = GotChars(tp)) {
				VDTByteCount numCharsWritten;
				if (numChars > BUFLEN)
					numChars = BUFLEN;
				ReadChars(tp, 0, buffer, &numChars);
				#if (VDTByteCountIs64Bits)						// See definition of VDTByteCount
					XOPWriteFile64(gFileRef, numChars, buffer, &numCharsWritten);
				#else
					{
						UInt32 count;
						XOPWriteFile(gFileRef, numChars, buffer, &count);
						numCharsWritten = count;
					}
				#endif
				if (numCharsWritten != numChars) {
					VDTAbortTerminalOperation();	// Error writing.
					numChars = numCharsWritten;
				}
				gCharCount += numChars;
				gCharOffset += numChars;
			}
			break;
		
		case OP_SENDTEXT:
			if (result = WriteAsyncResult(tp, &done)) {
				VDTAbortTerminalOperation();
				ErrorAlert(result);
				break;
			}
			if (!done)
				break;							// Current transmission is not yet finished.

			if (gSendTextH == NULL) {			// Should not happen.
				VDTAbortTerminalOperation();
				break;
			}
			
			// Transmission finished. Start new transmission if more left to send.
			numChars = gCharCount;
			if (numChars == 0) {
				DisposeHandle(gSendTextH);
				gSendTextH = NULL;
				tp->terminalOp = 0;				// End of operation.
				break;
			}
			if (numChars > BUFLEN)
				numChars = BUFLEN;
			memcpy(buffer, *gSendTextH+gCharOffset, numChars);
			if (result = WriteCharsAsync(tp, buffer, numChars)) {
				VDTAbortTerminalOperation();
				ErrorAlert(result);
				break;
			}
			gCharOffset += numChars;
			gCharCount -= numChars;			
			break;
		
		default:								// Here if no operation in progress.
			if (numChars = GotChars(tp)) {
				char buf1[BUFLEN];
				char buf2[BUFLEN];
				
				if (numChars > BUFLEN)
					numChars = BUFLEN;
				ReadChars(tp, 0, buf1, &numChars);
				
				// This allows us to accept CR, LF, or CRLF.
				SetEndOfLineCharacters(tp, buf1, numChars, buf2, &numChars);
				if (numChars == 0)
					break;
				
				TUGetSelLocs(gVDTTU, &startLoc, &endLoc);
				if (startLoc.paragraph!=endLoc.paragraph || startLoc.pos!=endLoc.pos)
					TUSetSelLocs(gVDTTU, &startLoc, &startLoc, 0);

				TUInsert(gVDTTU, buf2, (int)numChars); 
				TUDisplaySelection(gVDTTU);
				XOPModified = TRUE;
			}
			break;
	}
}
