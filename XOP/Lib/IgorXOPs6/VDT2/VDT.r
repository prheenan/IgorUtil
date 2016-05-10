#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x13, release, 0x00, 0,			/* version bytes and country integer */
	"1.14",
	"1.14, Copyright 2002-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor 6.20 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"VDT2 unable to open window",		/* can't open window on initialization */
		/* [2] */
		"Invalid baud rate",
		/* [3] */
		"Data bits must be 7 or 8",
		/* [4] */
		"Stop bits must be 1 or 2",
		/* [5] */
		"Parity is 0 for none, 1 for odd, 2 for even",
		/* [6] */
		"Handshake is 0 for none, 1 for CTS, 2 for XON/XOFF",
		/* [7] */
		"Echo is 0 for off, 1 for on",
		/* [8] */
		"Port is 0 for modem, 1 for printer",
		/* [9] */
		"Buffer size must be at least 32 bytes",
		/* [10] */
		"The port is busy",
		/* [11] */
		"The port could not be initialized",
		/* [12] */
		"There is not enough memory for the input buffer",
		/* [13] */
		"No port is selected",
		/* [14] */
		"Another VDT2 operation is in progress",
		/* [15] */
		"The numeric format must end with a valid numeric conversion character.",
		/* [16] */
		"Not used.",
		/* [17] */
		"Not used.",
		/* [18] */
		"This version of VDT2 requires Igor version 6.20 or later",
		/* [19] */
		"Can't do VDTBinaryWaveRead2 or VDTBinaryWaveWrite2 on a text wave. Use VDTWaveRead2 or VDTWaveWrite2.",
		/* [20] */
		"The channel number must be 0 (for the input port) or 1 (for the output port).",
		/* [21] */
		"The status code must be between 0 and 6.",
		/* [22] */
		"Break detected.",
		/* [23] */
		"No parallel device is selected.",
		/* [24] */
		"A framing error occurred (check baud rate, parity, stop bits).",
		/* [25] */
		"An I/O error occurred.",
		/* [26] */
		"Mode error.",
		/* [27] */
		"Parallel device is out of paper.",
		/* [28] */
		"Overrun error - too many characters received or sent to fit in buffer.",
		/* [29] */
		"Parallel port timeout.",
		/* [30] */
		"Input overrun error - too many characters received to fit in input buffer.",
		/* [31] */
		"A parity error occurred (check baud rate, parity, stop bits).",
		/* [32] */
		"Input overrun error - too many characters sent to fit in output buffer.",
		/* [33] */
		"An communications error of unknown nature occurred.",
		/* [34] */
		"Unknown port name.",
		/* [35] */
		"No port is selected for command line operations. Commands will not work until you select a port.",
		/* [36] */
		"No port is selected for terminal operations. Terminal operations will not work until you select a port.",
		/* [37] */
		"Expected the name of an installed port (such as Printer, Modem, COM1, COM2).",
		/* [38] */
		"Expected the name of an installed port (such as Printer, Modem, COM1, COM2) or 'None'.",
		/* [39] */
		"The specified port is not available on this machine.",
		/* [40] */
		"Expect 0 (CR), 1 (LF), or 2 (CRLF) for terminalEOL.",
		/* [41] */
		"The port can't be opened. This machine may not have the specified port or may not be configured to use the port.",
		/* [42] */
		"The port appears to be in use by another program.",
		/* [43] */
		"This baud rate is not supported on Macintosh.",
		/* [44] */
		"A UNIX-related error occurred. See the history for diagnostics.",
		/* [45] */
		"A system-related error occurred. See the history for diagnostics.",
		/* [46] */
		"Bad binary data type. Codes are the same as those used for the WaveType function.",
		/* [47] */
		"Serial reads are limited to 4 billion bytes at a time.",
		/* [48] */
		"Serial writes are limited to 4 billion bytes at a time.",
	}
};

resource 'STR#' (1101) {					// Misc strings that Igor looks for.
	{
		/* [1] */
		"-1",								// This item is no longer supported by the Carbon XOP Toolkit.
		/* [2] */
		"---"		,						// This item is no longer supported by the Carbon XOP Toolkit.
		/* [3] */
		"VDT2 Help",						// Name of XOP's help file.
	}
};

/* Balloon strings for "VDT2" item */
resource 'STR#' (1110, "VDT2") {
	{
		/* [1] (used when menu item is enabled) */
		"VDT2 == ‘Very Dumb Terminal’.\n\n"
		"In addition to dumb terminal emulation, the VDT2 external operation adds command "
		"line operations to Igor for setting up serial ports and for transfering data "
		"to and from serial ports.",
		
		/* [2] (used when menu item is disabled) */
		"",
		
		/* [3] (used when menu item is checked) */
		"",
		
		/* [4] (used when menu item is marked) */
		"",
	}
};

resource 'STR#' (1102) {					// Misc strings private to VDT2.
	{
		/* [1] */
		"Send VDT2 Text",
		/* [2] */
		"Send Selected Text",				// Alternate to "Send VDT2 Text" menu item.
		/* [3] */
		"Stop Sending Text",				// Alternate to "Send VDT2 Text" menu item.
		/* [4] */
		"Send File...",
		/* [5] */
		"Stop Sending File",				// Alternate to "Send File..." menu item.
		/* [6] */
		"Receive File...",
		/* [7] */
		"Stop Receiving File",				// Alternate to "Receive File..." menu item.
		/* [8] */
		"Error opening file",
	}
};

resource 'XOPI' (1100) {
	XOP_VERSION,							// XOP protocol version.
	DEV_SYS_CODE,							// Development system information.
	0,										// Obsolete - set to zero.
	0,										// Obsolete - set to zero.
	XOP_TOOLKIT_VERSION,					// XOP Toolkit version.
};

resource 'XOPC' (1100) {					/* commands added by XOP */
	{
		"VDT2",								/* name of operation */
		XOPOp+UtilOP+compilableOp,			/* operation's category */

		"VDTRead2",
		XOPOp+UtilOP+compilableOp,

		"VDTWrite2",
		XOPOp+UtilOP+compilableOp,

		"VDTReadWave2",
		XOPOp+UtilOP+compilableOp,

		"VDTWriteWave2",
		XOPOp+UtilOP+compilableOp,

		"VDTReadBinary2",
		XOPOp+UtilOP+compilableOp,

		"VDTWriteBinary2",
		XOPOp+UtilOP+compilableOp,

		"VDTReadBinaryWave2",
		XOPOp+UtilOP+compilableOp,

		"VDTWriteBinaryWave2",
		XOPOp+UtilOP+compilableOp,

		"VDTReadHex2",
		XOPOp+UtilOP+compilableOp,

		"VDTWriteHex2",
		XOPOp+UtilOP+compilableOp,

		"VDTReadHexWave2",
		XOPOp+UtilOP+compilableOp,

		"VDTWriteHexWave2",
		XOPOp+UtilOP+compilableOp,

		"VDTGetStatus2",					// Added 9/9/96 in version 1.31.
		XOPOp+UtilOP+compilableOp,

		"VDTOpenPort2",						// Added 9/29/97 in version 2.00.
		XOPOp+UtilOP+compilableOp,

		"VDTClosePort2",					// Added 9/29/97 in version 2.00.
		XOPOp+UtilOP+compilableOp,

		"VDTTerminalPort2",					// Added 9/29/97 in version 2.00.
		XOPOp+UtilOP+compilableOp,

		"VDTOperationsPort2",				// Added 9/29/97 in version 2.00.
		XOPOp+UtilOP+compilableOp,

		"VDTGetPortList2",					// Added 10/5/97 in version 2.00.
		XOPOp+UtilOP+compilableOp,
	}
};

// Description of menu items added to built-in Igor menus.
resource 'XMI1' (1100) {
	{
		8,							// Add item to menu with ID=8 (Misc menu).
		"VDT2",						// This is text for added menu item.
		100,						// This item has a submenu with resourceID==100.
		0,							// Flags field.
	}
};

// Description of submenus added to XOP menus.
resource 'XSM1' (1100) {
	{								// 2 submenus added.
		101,						// Add submenu with resource ID 101 (Terminal Port submenu).
		100,						// To menu with resource ID 100 (main VDT2 submenu).
		5,							// To item 5 of menu.

		102,						// Add submenu with resource ID 102 (Operations Port submenu).
		100,						// To menu with resource ID 100.
		12,							// To item 12 of menu.
	}
};

resource 'MENU' (100) {				// Submenu for VDT2 XOP,
	100,							// menu ID is same as resource ID.
	textMenuProc,
	0xFFFFFFFF,
	enabled,
	"VDT2",
	{
		"Open VDT2 Window", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"VDT2 Settings...", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Terminal Port", noIcon, noKey, noMark, plain,
		"Save File...", noIcon, noKey, noMark, plain,
		"Insert File...", noIcon, noKey, noMark, plain,
		"Send File...", noIcon, noKey, noMark, plain,
		"Receive File...", noIcon, noKey, noMark, plain,
		"Send Selected Text", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Operations Port", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"VDT2 Help", noIcon, noKey, noMark, plain,
	}
};

resource 'MENU' (101) {						/* submenu for Terminal Port */
	101,									/* menu ID is same as resource ID */
	textMenuProc,
	0xffffffff,
	enabled,
	"Terminal Port",
	{
		"Modem", noIcon, noKey, noMark, plain,
		"Printer", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, check, plain,
		"Off Line", noIcon, noKey, check, plain,
	}
};

resource 'MENU' (102) {						/* submenu for Operations Port */
	102,									/* menu ID is same as resource ID */
	textMenuProc,
	0xffffffff,
	enabled,
	"Operations Port",
	{
		"Modem", noIcon, noKey, noMark, plain,
		"Printer", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, check, plain,
		"Off Line", noIcon, noKey, check, plain,
	}
};

/*	VDT2 Settings Dialog */

resource 'dlgx' (1100, "VDT2 Settings") {
	versionZero {
		kDialogFlagsUseThemeBackground | kDialogFlagsUseControlHierarchy | kDialogFlagsUseThemeControls	| kDialogFlagsHandleMovableModal
	}
};

resource 'CNTL' (1100, "Port Popup Menu", purgeable) {
	{8, 57, 28, 237},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1101, "Baud Rate Popup Menu", purgeable) {
	{132, 115, 152, 235},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1102, "Input Handshake Popup Menu", purgeable) {
	{133, 406, 153, 526},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1103, "Data Length Popup Menu", purgeable) {
	{162, 115, 182, 235},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1104, "Output Handshake Popup Menu", purgeable) {
	{162, 406, 182, 526},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1105, "Stop Bits Popup Menu", purgeable) {
	{191, 115, 211, 235},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1106, "Parity Popup Menu", purgeable) {
	{222, 115, 242, 235},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1107, "Local Echo Popup Menu", purgeable) {
	{252, 115, 272, 235},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1108, "Terminal Popup Menu", purgeable) {
	{253, 361, 273, 481},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'DLOG' (1100) {
	{56, 37, 384, 597},
	noGrowDocProc,
	invisible,
	noGoAway,
	0x0,
	1100,
	"VDT2 Settings",
	noAutoCenter					// Added 971223 because Apple changed the Rez templates.
};

resource 'DITL' (1100) {
	{	/* array DITLarray: 31 elements */
		/* [1] */
		{295, 385, 315, 455},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{294, 470, 314, 540},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{10, 20, 28, 56},
		StaticText {
			enabled,
			"Port:"
		},
		/* [4] */
		{8, 57, 28, 237},			// Port Popup Menu
		Control {
			enabled,
			1100
		},
		/* [5] */
		{10, 252, 28, 436},
		StaticText {
			disabled,
			"This port is currently open"
		},
		/* [6] */
		{10, 252, 28, 447},
		StaticText {
			disabled,
			"This port is currently closed"
		},
		/* [7] */
		{42, 57, 60, 187},
		CheckBox {
			enabled,
			"Open this port"
		},
		/* [8] */
		{42, 57, 60, 187},
		CheckBox {
			enabled,
			"Close this port"
		},
		/* [9] */
		{43, 198, 61, 500},
		StaticText {
			disabled,
			"(Port will be opened when you click OK.)"
		},
		/* [10] */
		{43, 198, 61, 500},
		StaticText {
			disabled,
			"(Port will be closed when you click OK.)"
		},
		/* [11] */
		{69, 57, 87, 325},
		CheckBox {
			enabled,
			"Use this port for terminal operations"
		},
		/* [12] */
		{97, 57, 115, 359},
		CheckBox {
			enabled,
			"Use this port for command line operations"
		},
		/* [13] */
		{133, 39, 151, 112},
		StaticText {
			enabled,
			"Baud Rate:"
		},
		/* [14] */
		{132, 115, 152, 235},			// Baud Rate Popup Menu
		Control {
			enabled,
			1101
		},
		/* [15] */
		{134, 272, 152, 403},
		StaticText {
			enabled,
			"Input Handshaking:"
		},
		/* [16] */
		{133, 406, 153, 526},			// Input Handshake Popup Menu
		Control {
			enabled,
			1102
		},
		/* [17] */
		{163, 26, 181, 112},
		StaticText {
			enabled,
			"Data Length:"
		},
		/* [18] */
		{162, 115, 182, 235},			// Data Length Popup Menu
		Control {
			enabled,
			1103
		},
		/* [19] */
		{163, 260, 181, 403},
		StaticText {
			enabled,
			"Output Handshaking:"
		},
		/* [20] */						// Output Handshake Popup Menu
		{162, 406, 182, 526},
		Control {
			enabled,
			1104
		},
		/* [21] */
		{192, 46, 210, 112},
		StaticText {
			enabled,
			"Stop Bits:"
		},
		/* [22] */
		{191, 115, 211, 235},			// Stop Bits Popup Menu
		Control {
			enabled,
			1105
		},
		/* [23] */
		{195, 285, 213, 403},
		StaticText {
			enabled,
			"Input Buffer Size:"
		},
		/* [24] */
		{196, 409, 214, 470},
		EditText {
			enabled,
			"2048"
		},
		/* [25] */
		{222, 65, 240, 112},
		StaticText {
			enabled,
			"Parity:"
		},
		/* [26] */
		{222, 115, 242, 235},			// Parity Popup Menu
		Control {
			enabled,
			1106
		},
		/* [27] */
		{253, 34, 271, 112},
		StaticText {
			enabled,
			"Local Echo:"
		},
		/* [28] */
		{252, 115, 272, 235},			// Local Echo Popup Menu
		Control {
			enabled,
			1107
		},
		/* [29] */
		{254, 262, 272, 358},
		StaticText {
			enabled,
			"Terminal EOL:"
		},
		/* [30] */
		{253, 361, 273, 481},			// Terminal Popup Menu
		Control {
			enabled,
			1108
		},
		/* [31] */
		{295, 18, 315, 221},
		CheckBox {
			enabled,
			"Save setup with experiment"
		},
		/* [32] */
		{195, 409, 230, 490},
		StaticText {
			disabled,
			"Not settable on OS X."
		}
	}
};
