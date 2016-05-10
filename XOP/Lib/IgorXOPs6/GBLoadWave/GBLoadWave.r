#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x70, release, 0x00, 0,			/* version bytes and country integer */
	"1.70",
	"1.70, Copyright 1994-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor Pro 6.20 or later)"
};

resource 'STR#' (1101) {					// Misc strings that Igor looks for.
	{
		/* [1] */
		"-1",								// This item is no longer supported by the Carbon XOP Toolkit.
		/* [2] */
		"---"		,						// This item is no longer supported by the Carbon XOP Toolkit.
		/* [3] */
		"GBLoadWave Help",					// Name of XOP's help file.
	}
};

// Description of menu items added to built-in Igor menus.
resource 'XMI1' (1100) {
	{
		50,									// Add item to menu with ID=50 (Load Waves menu).
		"Load General Binary File...",		// This is text for added menu item.
		0,									// This item has no submenu.
		0,									// Flags field.
	}
};

/* Balloon strings for "Load General Binary File..." item */
resource 'STR#' (1110, "Load General Binary File...") {
	{
		/* [1] (used when menu item is enabled) */
		"Loads data from various kinds of binary files into Igor waves.\n\n"
		"This can load 16 or 32 bit integer and 32 or 64 bit IEEE floating point binary data, "
		"providing that you can tell it precisely how data is stored in the file.",
		
		/* [2] (used when menu item is disabled) */
		"",		/* item is never disabled */
		
		/* [3] (used when menu item is checked) */
		"",		/* item is never checked */
		
		/* [4] (used when menu item is marked) */
		"",		/* item is never marked */
	}
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"This is not a general binary file",
		/* [2] */
		"There is no data in file",
		/* [3] */
		"Expected name of general binary file",
		/* [4] */
		"Expected base name for new waves",
		/* [5] */
		"Expected file type",
		/* [6] */
		"Too many file types (4 allowed)",
		/* [7] */
		"Data length in bits must be 8, 16, 32 or 64",
		/* [8] */
		"Number of arrays must be >= 1",
		/* [9] */
		"File contains too few bytes for specified number of bytes/point, points and arrays",
		/* [10] */
		"Bad data type value",
		/* [11] */
		"GBLoadWave requires Igor Pro version 6.20 or later",
		/* [12] */
		"Valid floating point formats are 1 (IEEE) and 2 (VAX)",
		/* [13] */
		"The array is too big for an Igor wave.",
	}
};

resource 'XOPI' (1100) {
	XOP_VERSION,							// XOP protocol version.
	DEV_SYS_CODE,							// Development system information.
	0,										// Obsolete - set to zero.
	0,										// Obsolete - set to zero.
	XOP_TOOLKIT_VERSION,					// XOP Toolkit version.
};

resource 'XOPC' (1100) {
	{
		"GBLoadWave",						/* name of operation */
		XOPOp+UtilOP+compilableOp,			/* operation's category */
	}
};

resource 'ALRT' (1258, purgeable) {
	{88, 62, 250, 390},
	1258,
	{	/* array: 4 elements */
		/* [1] */
		OK, visible, silent,
		/* [2] */
		OK, visible, silent,
		/* [3] */
		OK, visible, silent,
		/* [4] */
		OK, visible, silent
	},
	noAutoCenter
};

resource 'DITL' (1258) {
	{
		/* [1] */
		{124, 10, 146, 56},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{5, 97, 21, 243},
		StaticText {
			disabled,
			"GBLoadWave Error"
		},
		/* [3] */
		{50, 90, 115, 340},
		StaticText {
			disabled,
			"^0"
		},
	}
};

resource 'dlgx' (1260, "Load General Binary") {
	versionZero {
		kDialogFlagsUseThemeBackground | kDialogFlagsUseControlHierarchy | kDialogFlagsUseThemeControls	| kDialogFlagsHandleMovableModal
	}
};

resource 'CNTL' (1100, "Input Type Popup Menu", purgeable) {
	{30, 90, 50, 285},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1101, "Float Format Popup Menu", purgeable) {
	{29, 356, 49, 454},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1102, "Path Popup Menu", purgeable) {
	{160, 56, 180, 206},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1103, "Byte Order Popup Menu", purgeable) {
	{63, 376, 83, 524},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1104, "Output Type Popup Menu", purgeable) {
	{230, 93, 250, 285},
	0,			// Title constant.
	visible,
	0,			// Width of title in pixels.
	-12345,		// MENU resource ID; MUST BE -12345!
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,		// CDEF ID
	0,			// Refcon
	""			// Title
};

resource 'CNTL' (1105, "Command Box") {
	{312, 12, 332, 576},
	0,
	visible,
	0,
	0,
	kControlGroupBoxTextTitleProc,
	0,
	""
};

resource 'CNTL' (1106, "File Underline") {
	{177, 302, 182, 561},
	0,
	visible,
	0,
	0,
	kControlSeparatorLineProc,
	0,
	""
};

resource 'CNTL' (1107, "Input Group Box") {
	{8, 12, 195, 576},
	0,
	visible,
	0,
	0,
	kControlGroupBoxTextTitleProc,
	0,
	"Input File"
};

resource 'CNTL' (1108, "Output Group Box") {
	{201, 12, 303, 576},
	0,
	visible,
	0,
	0,
	kControlGroupBoxTextTitleProc,
	0,
	"Output Waves"
};

resource 'DLOG' (1260) {
	{50, 30, 430, 620},
	movableDBoxProc,
	invisible,
	noGoAway,
	0x0,
	1260,
	"Load General Binary",
	noAutoCenter
};

resource 'DITL' (1260) {						/* Main dialog */
	{	/* array DITLarray: 34 elements */
		/* [1] */
		{347, 12, 367, 82},
		Button {
			enabled,
			"Do It"
		},
		/* [2] */
		{347, 506, 367, 576},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{347, 97, 367, 211},
		Button {
			enabled,
			"To Cmd Line"
		},
		/* [4] */
		{347, 227, 367, 307},
		Button {
			enabled,
			"To Clip"
		},
		/* [5] */
		{312, 12, 332, 576},
		Control {
			enabled,
			1105
		},
		/* [6] */
		{347, 420, 367, 490},
		Button {
			enabled,
			"Help"
		},

		/* [7] */
		{8, 12, 195, 576},
		Control {					// Input Group Box
			enabled,
			1107
		},
		/* [8] */
		{32, 18, 50, 90},
		StaticText {
			enabled,
			"Data Type:"
		},
		/* [9] */					// Input type popup menu.
		{30, 90, 50, 285},
		Control {
			enabled,
			1100
		},
		/* [10] */
		{31, 302, 48, 357},
		StaticText {
			enabled,
			"Format:"
		},
		/* [11] */					// Float format popup menu.
		{29, 356, 49, 454},
		Control {
			enabled,
			1101
		},
		/* [12] */
		{163, 18, 181, 55},
		StaticText {
			enabled,
			"Path:"
		},
		/* [13] */					// Path popup menu.
		{160, 56, 180, 206},
		Control {
			enabled,
			1102
		},
		/* [14] */
		{159, 221, 179, 291},
		Button {
			enabled,
			"File..."
		},
		/* [15] */
		{162, 302, 177, 561},
		StaticText {
			disabled,
			""
		},
		/* [16] */
		{177, 302, 182, 561},
		Control {
			disabled,
			1106
		},
		/* [17] */
		{65, 18, 83, 206},
		StaticText {
			enabled,
			"Bytes to skip at start of file"
		},
		/* [18] */
		{65, 210, 80, 265},
		EditText {
			enabled,
			"0"
		},
		/* [19] */
		{94, 45, 112, 206},
		StaticText {
			enabled,
			"Number of arrays in file"
		},
		/* [20] */
		{94, 210, 109, 265},
		EditText {
			enabled,
			"auto"
		},
		/* [21] */
		{122, 34, 140, 206},
		StaticText {
			enabled,
			"Number of points in array"
		},
		/* [22] */
		{122, 210, 137, 265},
		EditText {
			enabled,
			"auto"
		},
		/* [23] */
		{64, 299, 82, 376},
		StaticText {
			enabled,
			"Byte order:"
		},
		/* [24] */					// Byte order popup menu.
		{63, 376, 83, 524},
		Control {
			enabled,
			1103
		},
		/* [25] */
		{120, 300, 140, 511},
		CheckBox {
			enabled,
			"Points in file are interleaved"
		},

		/* [26] */
		{201, 12, 303, 576},
		Control {
			enabled,
			1108
		},
		/* [27] */
		{261, 31, 281, 220},
		CheckBox {
			enabled,
			"Overwrite existing waves"
		},
		/* [28] */
		{233, 303, 247, 378},
		StaticText {
			enabled,
			"Base name"
		},
		/* [29] */
		{233, 380, 248, 460},
		EditText {
			enabled,
			"wave"
		},
		/* [30] */
		{261, 303, 281, 382},
		Checkbox {
			enabled,
			"Scaling..."
		},
		/* [31] */
		{232, 21, 249, 93},
		StaticText {
			enabled,
			"Data Type:"
		},
		/* [32] */					// Output type popup menu.
		{230, 93, 250, 285},
		Control {
			enabled,
			1104
		},
	}
};

resource 'dlgx' (1261, "Output Data Scaling") {
	versionZero {
		kDialogFlagsUseThemeBackground | kDialogFlagsUseControlHierarchy | kDialogFlagsUseThemeControls	| kDialogFlagsHandleMovableModal
	}
};

resource 'DLOG' (1261) {
	{100, 70, 255, 404},
	movableDBoxProc,
	invisible,
	noGoAway,
	0x0,
	1261,
	"Output Data Scaling",
	noAutoCenter
};

resource 'DITL' (1261) {
	{
		/* [1] */
		{128, 14, 148, 74},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{128, 252, 148, 322},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{10, 12, 30, 325},
		StaticText {
			enabled,
			"Output data = (input data + offset) * multiplier"
		},
		/* [4] */
		{44, 104, 62, 149},
		StaticText {
			enabled,
			"offset"
		},
		/* [5] */
		{43, 159, 61, 240},
		EditText {
			enabled,
			""
		},
		/* [6] */
		{78, 82, 96, 149},
		StaticText {
			enabled,
			"multiplier"
		},
		/* [7] */
		{78, 159, 96, 240},
		EditText {
			enabled,
			""
		}
	}
};

