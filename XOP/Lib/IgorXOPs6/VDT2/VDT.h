// Equates for VDT XOP
 
#define VDT_VERSION 3					// Version number stored in VDT resources and files.
										// Version changed from 2 to 3 for VDT 2.0, 9/97.
// VDT custom error codes.

#define CANT_OPEN_WINDOW 1 + FIRST_XOP_ERR
#define INVALID_BAUD 2 + FIRST_XOP_ERR
#define INVALID_DATABITS 3 + FIRST_XOP_ERR
#define INVALID_STOPBITS 4 + FIRST_XOP_ERR
#define INVALID_PARITY 5 + FIRST_XOP_ERR
#define INVALID_HANDSHAKE 6 + FIRST_XOP_ERR
#define INVALID_ECHO 7 + FIRST_XOP_ERR
#define INVALID_PORT 8 + FIRST_XOP_ERR
#define INVALID_BUFFER 9 + FIRST_XOP_ERR
#define PORT_BUSY 10 + FIRST_XOP_ERR
#define CANT_INIT 11 + FIRST_XOP_ERR
#define NO_BUFFER 12 + FIRST_XOP_ERR
#define NO_PORT 13 + FIRST_XOP_ERR
#define OP_IN_PROGRESS 14 + FIRST_XOP_ERR
#define BAD_NUMERIC_FORMAT 15 + FIRST_XOP_ERR
#define AVAILABLE16 16 + FIRST_XOP_ERR
#define AVAILABLE17 17 + FIRST_XOP_ERR
#define OLD_IGOR 18 + FIRST_XOP_ERR
#define NO_BINARY_WAVE_OPERATIONS_ON_TEXT_WAVES 19 + FIRST_XOP_ERR
#define BAD_CHANNEL_NUMBER 20 + FIRST_XOP_ERR
#define BAD_STATUS_CODE 21 + FIRST_XOP_ERR

#define VDT_ERR_BREAK 22 + FIRST_XOP_ERR
#define VDT_ERR_NO_PARALLEL_DEVICE 23 + FIRST_XOP_ERR
#define VDT_ERR_FRAME 24 + FIRST_XOP_ERR
#define VDT_ERR_IO_ERROR 25 + FIRST_XOP_ERR
#define VDT_ERR_MODE_NOT_SUPPORTED 26 + FIRST_XOP_ERR
#define VDT_ERR_PARALLEL_DEVICE_OUT_OF_PAPER 27 + FIRST_XOP_ERR
#define VDT_ERR_OVERRUN_ERROR 28 + FIRST_XOP_ERR
#define VDT_ERR_PARALLEL_TIMEOUT 29 + FIRST_XOP_ERR
#define VDT_ERR_INPUT_BUFFER_OVERRUN 30 + FIRST_XOP_ERR
#define VDT_ERR_PARITY_ERROR 31 + FIRST_XOP_ERR
#define VDT_ERR_OUTPUT_BUFFER_OVERFLOW 32 + FIRST_XOP_ERR
#define VDT_ERR_UNKNOWN_COMM_ERR 33 + FIRST_XOP_ERR

#define VDT_ERR_UNKNOWN_PORT_ERR 34 + FIRST_XOP_ERR
#define VDT_ERR_NO_OPERATIONS_PORT_SELECTED 35 + FIRST_XOP_ERR
#define VDT_ERR_NO_TERMINAL_PORT_SELECTED 36 + FIRST_XOP_ERR
#define VDT_ERR_EXPECTED_PORTNAME 37 + FIRST_XOP_ERR
#define VDT_ERR_EXPECTED_PORTNAME_OR_NONE 38 + FIRST_XOP_ERR
#define VDT_ERR_PORT_NOT_AVAILABLE 39 + FIRST_XOP_ERR
#define VDT_ERR_INVALID_TERMINAL_EOL 40 + FIRST_XOP_ERR
#define VDT_ERR_CANT_OPEN_PORT 41 + FIRST_XOP_ERR
#define VDT_ERR_PORT_IN_USE 42 + FIRST_XOP_ERR

#define VDT_ERR_BAUD_NOT_SUPPORTED_ON_THIS_PLATFORM 43 + FIRST_XOP_ERR

#define VDT_ERR_UNIX_ERROR 44 + FIRST_XOP_ERR
#define VDT_ERR_SYSTEM_ERROR 45 + FIRST_XOP_ERR

#define BAD_BINARY_TYPE 46 + FIRST_XOP_ERR

#define READ_TOO_LONG 47 + FIRST_XOP_ERR
#define WRITE_TOO_LONG 48 + FIRST_XOP_ERR

#ifdef IGOR64
	typedef SInt64 VDTByteCount;
	#define VDTByteCountIs64Bits 1
#else
	typedef int VDTByteCount;
	#define VDTByteCountIs64Bits 0
#endif

// VDT data structures.

struct VDTPort {
	char name[MAX_OBJ_NAME+1];			// Name of port. e.g. "Modem" (Macintosh), "COM1" (Windows).
	char inputChan[64];					// Name of input channel. e.g. "/dev/cu.xxx" (Mac OS X), "COM1" (Windows).
	char outputChan[64];				// Name of output channel. e.g. "/dev/cu.xxx" (Mac OS X), "COM1" (Windows).

	short portIsOpen;
	
	#ifdef MACIGOR
		int fileDescriptor;				// Unix file descriptor for the serial port.
		char* savedTermiosPtr;			// Used to save the termios structure so it can be restored when we close the port.
	#endif
	#ifdef WINIGOR
		HANDLE commH;					// Handle to communications resource.
		OVERLAPPED ai;					// Used for asynchronous input.
		OVERLAPPED ao;					// Used for asynchronous output.
		char* localBuffer;				// Used in local buffering scheme in VDTIOWin.c.
		int charsInLocalBuffer;
	#endif
	unsigned short baud;				// 0, 1, ... see bauds array.
	unsigned short parity;				// 0 = no parity, 1 = odd parity, 2 = even parity.
	unsigned short stopbits;			// 0 = 1 stop bit, 1 = 2 stop bits.
	unsigned short databits;			// 0 = 7 bits, 1 = 8 bits.
	unsigned short echo;				// 0 = off, 1 = on.
	unsigned short inShake;				// Input handshake: 0 = NONE, 1 = CTS, 2 = XON.
	unsigned short outShake;			// Output handshake: 0 = NONE, 1 = CTS, 2 = XON.
	unsigned short terminalEOL;			// Terminal end-of-line: 0 = CR, 1 = LF, 2 = CRLF.
	unsigned short disableRTS;			// Non-zero to disable RTS.

	short terminalOp;					// Dumb terminal operation in progress, if any. See terminal operation #defines.
	char lastTerminalChar;				// Used to convert CRLF to CR in dumb terminal mode.
	char reserved1;

	int errorStatus[2];					// Cumulative error status for channel 0 (input) and channel 1 (output). Used by VDTGetSerialStatus.					// Used to accumulate status by VDTGetSerialStatus.

	int inputBufferSize;				// Desired size in inputBuffer in bytes.
	
	// These are used by the VDT Settings dialog to keep track of ports changed, opened and closed in the dialog.
	short portSettingsChangedInDialog;
	short portOpenedInDialog;
	short portClosedInDialog;
	short selectedInDialog;				// This port was the last selected in the VDT dialog.

	struct VDTPort* nextPort;			// Pointer to next VDTPort in linked list.
};
typedef struct VDTPort VDTPort;
typedef VDTPort *VDTPortPtr;

// Set structure alignment because the this structure is stored on disk
// in preferences and must be consistent across compilers and versions.
#pragma pack(2)
struct VDTPortSettings {				// Settings stored in preferences and experiments for each port.
	char name[MAX_OBJ_NAME+1];			// Name of port. e.g. "Modem" (Macintosh), "COM1" (Windows).
	char inputChan[MAX_OBJ_NAME+1];		// Name of input channel. e.g. ".aIn" (Macintosh), "COM1" (Windows).
	char outputChan[MAX_OBJ_NAME+1];	// Name of output channel. e.g. ".aOut" (Macintosh), "COM1" (Windows).
	short portIsOpen;
	unsigned short baud;				// 0, 1, ... see bauds array.
	unsigned short parity;				// 0 = no parity, 1 = odd parity, 2 = even parity.
	unsigned short stopbits;			// 0 = 1 stop bit, 1 = 2 stop bits.
	unsigned short databits;			// 0 = 7 bits, 1 = 8 bits.
	unsigned short echo;				// 0 = off, 1 = on.
	unsigned short inShake;				// Input handshake: 0 = NONE, 1 = CTS, 2 = XON.
	unsigned short outShake;			// Output handshake: 0 = NONE, 1 = CTS, 2 = XON.
	unsigned short terminalEOL;			// Terminal end-of-line: 0 = CR, 1 = LF, 2 = CRLF.
	unsigned short selectedInDialog;	// True if this was the last port selected in VDT Settings dialog popup menu.
	SInt32 inputBufferSize;				// Size of serial input buffer.
	SInt32 spare2[16];
};
typedef struct VDTPortSettings VDTPortSettings;
typedef VDTPortSettings *VDTPortSettingsPtr;
#pragma pack()	// Restore default structure alignment

// Set structure alignment because the this structure is stored on disk
// in preferences and must be consistent across compilers and versions.
#pragma pack(2)
struct VDTSettings {					// Settings stored for VDT as a whole.
	short version;						// Version number of this structure.
	short useExpSettings;				// True if settings should stick to experiment.
	short spare1;
	char opPortName[MAX_OBJ_NAME+1];	// Name of port used for command line operations.
	char termPortName[MAX_OBJ_NAME+1];	// Name of port used for terminal operations.
	short winState;						// Bit 0: 0=hidden, 1=visible. Bit 1: 0=normal, 1=minimized. Bit 1 is used on Windows only.
	Rect winRect;						// Location of VDT window.
	SInt32 spare2[16];
	short numPortSettings;				// Number of VDTPortSettings field in the following array.
	VDTPortSettings ps[1];				// Start of zero or more VDTPortSettings fields.
};
typedef struct VDTSettings VDTSettings;
typedef VDTSettings *VDTSettingsPtr;
typedef VDTSettingsPtr *VDTSettingsHandle;
#pragma pack()	// Restore default structure alignment

// Terminal operation #defines.
#define SENDFILEOP 1					// VDT is sending a file.
#define RECEIVEFILEOP 2					// VDT is receiving a file.
#define SENDTEXTOP 3					// VDT is sending window text.

// VDT miscellaneous equates.

#define VDT_WIND XOP_WIND				// Resource ID of VDT window resource.

#define VDT_MENUID 100					// Resource IDs of VDT menu resources.
#define VDT_TERMINAL_PORT_MENUID 101	// These must agree with resources in VDT.r.
#define VDT_OPERATIONS_PORT_MENUID 102

enum {
	VDT_ITEM_OPEN_VDT_WINDOW=1,
	VDT_ITEM_SEPARATOR_1,
	VDT_ITEM_SETTINGS_DIALOG,
	VDT_ITEM_SEPARATOR_2,
	VDT_ITEM_TERMINAL_PORT_MENU,		// Start of dumb-terminal-related items. See XSM1 resource in VDT.r if you change this.
	VDT_ITEM_SAVEFILE,
	VDT_ITEM_INSERTFILE,
	VDT_ITEM_SENDFILE,
	VDT_ITEM_RECEIVEFILE,
	VDT_ITEM_SENDTEXT,
	VDT_ITEM_SEPARATOR_3,
	VDT_ITEM_OPERATIONS_PORT_MENU,		// Start of command line operations items. See XSM1 resource in VDT.r if you change this.
	VDT_ITEM_SEPARATOR_4,
	VDT_ITEM_HELP
};

#define OP_SENDFILE 1
#define OP_RECEIVEFILE 2
#define OP_SENDTEXT 3

#define MINWINWIDTH 100
#define MINWINHEIGHT 100

#define MINBUFSIZE 32
#define MAXBUFSIZE 100000000			// Must be < 2GB because we store buffer size in int.

#define VDTSET_ID 1100

// Miscellaneous strings resource.

#define VDT_MISCSTR_ID 1102

#define SEND_VDT_TEXT 1
#define SEND_SELECTED_TEXT 2
#define STOP_SENDING_TEXT 3
#define SEND_FILE 4
#define STOP_SENDING_FILE 5
#define RECEIVE_FILE 6
#define STOP_RECEIVING_FILE 7
#define ERR_OPENING_FILE 8

// Prototypes.

// In VDT.c.
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);
void VDTAlert(char *message, int strID, int itemNum);
void ErrorAlert(int errorCode);
void ExplainPortError(const char* preamble, int err);
int CheckPort(VDTPortPtr pp, int doAlert);
int VDTNoCharactersSelected(void);
void UpdateStatusArea(void);

// In VDTMac.c
#ifdef MACIGOR
	int HandleVDTKeyMac(Handle TU, EventRecord* eventPtr);
#endif

// In VDTWin.c
#ifdef WINIGOR
	void SubclassVDTWindow(HWND hwnd);
	void UnsubclassVDTWindow(HWND hwnd);
#endif

// In VDTTerminal.c
VDTPortPtr VDTGetTerminalPortPtr(void);
void VDTSetTerminalPortPtr(VDTPortPtr pp);
int VDTGetOpenAndCheckTerminalPortPtr(VDTPortPtr* ppp, int displayDialogIfPortSelectedButNotOpenable);
void VDTSendTerminalChar(VDTPortPtr tp, char ch);
void VDTAbortTerminalOperation(void);
void VDTSendFile(void);
void VDTReceiveFile(void);
void VDTSendText(void);
void VDTTerminalIdle(void);

// In VDTIO.c.
void ClearErrorStatus(VDTPortPtr pp);
int GetErrorStatus(VDTPortPtr pp, int channelNumber);
void AccumulateErrorStatus(VDTPortPtr pp, int channelNumber, int status);
VDTByteCount GotChars(VDTPortPtr pp);
int ReadChars(VDTPortPtr pp, UInt32 timeout, char *buffer, VDTByteCount *lengthPtr);
void KillVDTIO(VDTPortPtr pp);
VDTPortPtr IndexedVDTPort(VDTPortPtr firstVDTPortPtr, int index);
VDTPortPtr FindVDTPort(VDTPortPtr firstVDTPortPtr, const char* name);
void SetCommSettingsFieldsToDefault(VDTPortPtr pp);
void SetVDTPortFieldsToDefault(const char* name, const char* inputChan, const char* outputChan, VDTPortPtr pp);
int VDTPortIsOpen(VDTPortPtr pp);
int OpenVDTPort(const char* name, VDTPortPtr* ppp);
int GetNumberedVDTPort(int portNumber, VDTPortPtr* ppp);
int TranslatePortName(char* portName);
int CloseVDTPort(const char* name);
int InitVDT(void);
void CloseAllVDTPorts(void);
int SetTestMode(int testMode);
int SerialRead(VDTPortPtr pp, UInt32 timeout, char* buf, VDTByteCount* countPtr);
int SerialWrite(VDTPortPtr pp, UInt32 timeout, char* buf, VDTByteCount* countPtr);
void DumpTestBuffer(void);

// In VDTOperations.c.
int RegisterVDTGetPortList2(void);
int RegisterVDTOpenPort2(void);
int RegisterVDTClosePort2(void);
int RegisterVDTTerminalPort2(void);
int RegisterVDTOperationsPort2(void);
int RegisterVDTOperationsPort2(void);
int RegisterVDT2(void);
int RegisterVDTGetStatus2(void);
VDTPortPtr VDTGetOperationsPortPtr(void);
void VDTSetOperationsPortPtr(VDTPortPtr pp);
void SetV_VDT(double number);
int VDTGetOpenAndCheckOperationsPortPtr(VDTPortPtr* ppp, int setTerminalPortOffLineIfConflict, int displayDialogIfPortSelectedButNotOpenable);

// In VDTRead.c
int ReadASCIIBytes(VDTPortPtr pp, UInt32 timeout, char* buffer, VDTByteCount maxBytes, const char* terminators);
int RegisterVDTRead2(void);
int RegisterVDTReadWave2(void);

// In VDTReadBinary.c
int RegisterVDTReadBinary2(void);
int RegisterVDTReadBinaryWave2(void);

// In VDTReadHex.c
int RegisterVDTReadHex2(void);
int RegisterVDTReadHexWave2(void);

// In VDTWrite.c
int RegisterVDTWrite2(void);
int RegisterVDTWriteWave2(void);

// In VDTWriteBinary.c
int RegisterVDTWriteBinary2(void);
int RegisterVDTWriteBinaryWave2(void);

// In VDTWriteHex.c
int RegisterVDTWriteHex2(void);	
int RegisterVDTWriteHexWave2(void);

// In VDTDialog.c.
int BaudRateToBaudCode(int baudRate, int* baudCodePtr);
int BaudCodeToBaudRate(int baudCode, int* baudRatePtr);
int ValidateBaudCode(int baudCode);
int VDTSettingsDialog(int useExpSettingsIn, int* useExpSettingsOutPtrPtr);

// In VDTIO_OSX.c or VDTIOWin.c.
void WaitForSerialOutputToFinish(VDTPortPtr pp);
void KillSerialIO(VDTPortPtr pp);
int WriteCharsAsync(VDTPortPtr pp, char *buffer, VDTByteCount length);
int WriteAsyncResult(VDTPortPtr pp, int* donePtr);
int WriteChars(VDTPortPtr pp, UInt32 timeout, char *buffer, VDTByteCount *lengthPtr);
VDTByteCount NumCharsInSerialInputBuffer(VDTPortPtr pp);
int ReadCharsSync(VDTPortPtr pp, UInt32 timeout, VDTByteCount numCharsToRead, VDTByteCount* numCharsReadPtr, char* buffer);
int GetSerialStatus(VDTPortPtr pp, int channel, int* cumErrs, int* xOffSent, int* readPending, int* writePending, int* ctsHold, int* xOffHold);
int GetIndexedPortNameAndChannelInfo(int index, char name[MAX_OBJ_NAME+1], char inputChan[MAX_OBJ_NAME+1], char outputChan[MAX_OBJ_NAME+1]);
int CloseSerialDrivers(VDTPortPtr pp);
int SetInputBufferSize(VDTPortPtr pp, int bufSize);
int SetCommPortSettings(VDTPortPtr pp);
int OpenCommPort(VDTPortPtr pp);
int CloseCommPort(VDTPortPtr pp);
