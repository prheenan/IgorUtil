#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x3, final, 0x00, 0,				/* version bytes and country integer */
	"1.3",
	"1.3, Copyright 1993-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor 6.20 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"TUDemo unable to open window",		/* can't open window on initialization */
		/* [2] */
		"This version of TUDemo requires Igor version 6.20 or later",
	}
};

/* Balloon strings for "TUDemo" item */
resource 'STR#' (1110, "TUDemo") {
	{
		/* [1] (used when menu item is enabled) */
		"Brought to you by the TUDemo external operation.\n\n"
		"TUDemo is intended to show XOP programmers how to use Igor's text utility "
		"XOPSupport routines to make a simple text window.",
		
		/* [2] (used when menu item is disabled) */
		"",
		
		/* [3] (used when menu item is checked) */
		"",
		
		/* [4] (used when menu item is marked) */
		"",
	}
};

resource 'STR#' (1102) {					/* miscellaneous strings */
	{
		/* [1] */
		"No Misc Strings Yet",
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
		"TUDemo",							/* name of operation */
		XOPOp+UtilOP+compilableOp,			/* operation's category */
	}
};

// Description of menu items added to built-in Igor menus.
resource 'XMI1' (1100) {
	{
		8,							// Add item to menu with ID=8 (Misc menu).
		"TUDemo",					// This is text for added menu item.
		100,						// This item has a submenu with resourceID==100.
		0,							// Flags field.
	}
};

resource 'MENU' (100) {						/* submenu for TUDemo XOP */
	100,									/* menu ID is same as resource ID */
	textMenuProc,
	0x0001,							// Initial enable and disable state of menu items.
	enabled,
	"TUDemo",
	{
		"Open TUDemo Window", noIcon, noKey, noMark, plain,
		"Document Info", noIcon, noKey, noMark, plain,
		"Insert Text", noIcon, noKey, noMark, plain,
		"Save Text", noIcon, noKey, noMark, plain,
		"Get All Text Test", noIcon, noKey, noMark, plain,
		"Fetch Selected Text Test", noIcon, noKey, noMark, plain,
		"Set Selection Test", noIcon, noKey, noMark, plain,
		"Quit TUDemo", noIcon, noKey, noMark, plain,
	}
};
