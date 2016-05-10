/*	VDTMac.c

	Macintosh-specific support for VDT.c.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "VDT.h"

#define LEFT_ARROW_KEY_CODE 0x7B		// Second byte of message field of EventRecord.
#define RIGHT_ARROW_KEY_CODE 0x7C
#define UP_ARROW_KEY_CODE 0x7E
#define DOWN_ARROW_KEY_CODE 0x7D
#define PAGE_UP_KEY_CODE 0x74
#define PAGE_DOWN_KEY_CODE 0x79
#define HOME_KEY_CODE 0x73
#define END_KEY_CODE 0x77
#define FORWARD_DELETE_KEY_CODE 0x75
#define HELP_KEY_CODE 0x72

static int
IsSendableKey(int keyCode)
{
	switch(keyCode) {
		case LEFT_ARROW_KEY_CODE:
		case RIGHT_ARROW_KEY_CODE:
		case UP_ARROW_KEY_CODE:
		case DOWN_ARROW_KEY_CODE:
		case PAGE_UP_KEY_CODE:
		case PAGE_DOWN_KEY_CODE:
		case HOME_KEY_CODE:
		case END_KEY_CODE:
		case FORWARD_DELETE_KEY_CODE:
		case HELP_KEY_CODE:
			return 0;					// Don't send these characters.
	}
	
	return 1;
}
	
/*	HandleVDTKeyMac(TU, eventPtr)

	Handles a keystroke event by sending the key to the VDT window and/or by
	sending the key to the serial port.
	
	Returns true if the key was sent to the VDT window.
	
	"Online" means that the chosen terminal port is not "None".
	
	The key is sent to the VDT window if we are online and local echo mode is on
	or if we are not online.
	
	The key is sent to the serial port if the terminal port is online
	and if the key is a sendable key (i.e., not page up, page down, etc.)
*/
int
HandleVDTKeyMac(Handle TU, EventRecord* eventPtr)
{
	VDTPortPtr tp;
	int terminalIsOnline;
	int sentKeyToVDTWindow;
	
	sentKeyToVDTWindow = 0;

	terminalIsOnline = VDTGetTerminalPortPtr() != NULL;
	tp = NULL;
	if (terminalIsOnline) {
		VDTGetOpenAndCheckTerminalPortPtr(&tp, 1);		// tp may be null.
		if (tp == NULL)
			return 0;									// We are online but can't open the port.
	}
	if ((tp!=NULL && tp->echo) || !terminalIsOnline) {
		TUKey(TU, eventPtr);
		sentKeyToVDTWindow = 1;
	}
	if (tp!=NULL && terminalIsOnline) {
		int keyCode;
		char ch;

		ch = eventPtr->message & 0xff;
		keyCode = (eventPtr->message >> 8) & 0xff;
		if (IsSendableKey(keyCode))						// Don't send special characters.
			VDTSendTerminalChar(tp, ch);
	}

	return sentKeyToVDTWindow;
}
