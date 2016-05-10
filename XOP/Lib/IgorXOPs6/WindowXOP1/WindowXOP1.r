#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"1.20",
	"1.2, Copyright 1993-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor Pro 6.20 or later)"
};

/* Custom error messages */
resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"WindowXOP1 requires Igor Pro 6.20 or later.",
	}
};

// Description of menu items added to built-in Igor menus.
resource 'XMI1' (1100) {
	{
		8,							// Add item to menu with ID=8 (Misc menu).
		"WindowXOP1",				// This is text for added menu item.
		0,							// This item has no submenu.
		0,							// Flags field.
	}
};

/* Balloon help for the "WindowXOP1" item */
resource 'STR#' (1110, "WindowXOP1") {
	{
		/* [1] (used when menu item is enabled) */
		"WindowXOP1 is a sample XOP that illustrates adding a simple window to Igor.",
		
		/* [2] (used when menu item is disabled) */
		"",			/* the item is never disabled */
		
		/* [3] (used when menu item is checked) */
		"",			/* the item is never checked */
		
		/* [4] (used when menu item is marked) */
		"",			/* the item is never marked */
	}
};

resource 'WIND' (1100) {					/* WindowXOP1 window */
	{50, 300, 150, 495},
	noGrowDocProc,
	visible,
	goAway,
	0x0,
	"WindowXOP1",
	noAutoCenter							// Added 980303 because Apple changed the Rez templates.
};

resource 'XOPI' (1100) {
	XOP_VERSION,							// XOP protocol version.
	DEV_SYS_CODE,							// Development system information.
	0,										// Obsolete - set to zero.
	0,										// Obsolete - set to zero.
	XOP_TOOLKIT_VERSION,					// XOP Toolkit version.
};

/*	XOP has no command line operations */

