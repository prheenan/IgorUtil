#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x51, release, 0x00, 0,			/* version bytes and country integer */
	"1.51",
	"1.51, Copyright 1993-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor 6.20 or later)"
};

resource 'STR#' (1101) {					// Misc strings that Igor looks for.
	{
		/* [1] */
		"-1",								// This item is no longer supported by the Carbon XOP Toolkit.
		/* [2] */
		"---"		,						// This item is no longer supported by the Carbon XOP Toolkit.
		/* [3] */
		"SimpleLoadWave Help",				// Name of XOP's help file.
	}
};

// Description of menu items added to built-in Igor menus.
resource 'XMI1' (1100) {
	{
		50,									// Add item to menu with ID=50 (Load Waves menu).
		"Load Simple Delimited File...",	// This is text for added menu item.
		0,									// This item has no submenu.
		0,									// Flags field.
	}
};

/* Balloon strings for "Load Simple Tab-delimited File..." item */
resource 'STR#' (1110, "Load Simple Tab-delimited File...") {
	{
		/* [1] (used when menu item is enabled) */
		"Loads data from simple tab-delimited text files into Igor waves.\n"
		"This is an example for XOP programmers. Don’t use it for real work.",
		
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
		"This is not a tab-delimited TEXT file.",
		/* [2] */
		"Could not find at least one row of wave data in the file.",
		/* [3] */
		"Expected name of tab-delimited TEXT file.",
		/* [4] */
		"Expected base name for new waves.",
		/* [5] */
		"SimpleLoadWave requires Igor Pro 6.20 or later.",
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
		"SimpleLoadWave",					/* name of operation */
		XOPOp+UtilOP+compilableOp,			/* operation's category */
	}
};
