#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x10, final, 0x00, 0,				/* version bytes and country integer */
	"1.3",
	"1.3, Copyright 1994-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor 6.20 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"Expected resource ID of main menu (100)",
		/* [2] */
		"Expected resource ID of MenuXOP1 menu (100 to 104, 5 for Analysis, 8 for Misc)",
		/* [3] */
		"Expected item number of MenuXOP1 menu item",
		/* [4] */
		"MenuXOP1 requires Igor version 6.20 or later",
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
		"MenuXOP1",							/* name of operation */
		XOPOp+UtilOP+compilableOp,			/* operation's category */
	}
};

/* MENU Resources for MenuXOP1 XOP */

resource 'MENU' (100) {
	100,
	textMenuProc,
	allEnabled,
	enabled,
	"MenuXOP1",
	{	/* array: 3 elements */
		/* [1] */
		"Item1", noIcon, noKey, noMark, plain,
		/* [2] */
		"Item2", noIcon, noKey, noMark, plain,
		/* [3] */
		"Item3", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (101) {
	101,
	textMenuProc,
	allEnabled,
	enabled,
	"Submenu1",
	{	/* array: 3 elements */
		/* [1] */
		"Subitem1_1", noIcon, noKey, noMark, plain,
		/* [2] */
		"Subitem1_2", noIcon, noKey, noMark, plain,
		/* [3] */
		"Subitem1_3", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (102) {
	102,
	textMenuProc,
	allEnabled,
	enabled,
	"Submenu3",
	{	/* array: 3 elements */
		/* [1] */
		"Subitem3_1", noIcon, noKey, noMark, plain,
		/* [2] */
		"Subitem3_2", noIcon, noKey, noMark, plain,
		/* [3] */
		"Subitem3_3", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (103) {
	103,
	textMenuProc,
	allEnabled,
	enabled,
	"Submenu3",
	{	/* array: 3 elements */
		/* [1] */
		"Subitem3_2_1", noIcon, noKey, noMark, plain,
		/* [2] */
		"Subitem3_2_2", noIcon, noKey, noMark, plain,
		/* [3] */
		"Subitem3_2_3", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (104) {
	104,
	textMenuProc,
	allEnabled,
	enabled,
	"MenuXOP1",
	{	/* array: 3 elements */
		/* [1] */
		"Analysis Item1", noIcon, noKey, noMark, plain,
		/* [2] */
		"Analysis Item2", noIcon, noKey, noMark, plain,
		/* [3] */
		"Analysis Item3", noIcon, noKey, noMark, plain
	}
};

/* description of menus added to Igor's main menu bar */
resource 'XMN1' (1100) {
	{	
		/* [1] */					/* just one main menu added */
		100,						/* menu resource ID is 100 */
		1,							/* show menu when Igor is launched */
	}
};

/* description of submenus added to XOP menus */
resource 'XSM1' (1100) {
	{								/* 3 submenus added */
		/* [1] */
		101,						/* add submenu with resource ID 101 */
		100,						/* to menu with resource ID 100 */
		1,							/* to item 1 of menu */
		/* [2] */
		102,						/* add submenu with resource ID 102 */
		100,						/* to menu with resource ID 100 */
		3,							/* to item 3 of menu */
		/* [3] */
		103,						/* add submenu with resource ID 103 */
		102,						/* to menu with resource ID 102 */
		2,							/* to item 2 of menu */
	}
};

#define TEST_ITEM_REQUIRES_FLAGS 0

/* description of menu items added to built-in Igor menus */
resource 'XMI1' (1100) {
	{								/* adds two items */
		/* [1] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"MenuXOP1 Misc1",			/* this is text for added menu item */
		0,							/* this item has no submenu */
		0,							/* flags field */
		/* [2] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"MenuXOP1 Misc2",			/* this is text for added menu item */
		0,							/* this item has no submenu */
		0,							/* flags field */
		/* [3] */
		5,							/* add item to menu with ID=5 (Analysis menu) */
		"MenuXOP1 Analysis",		/* this is text for added menu item */
		104,						/* resource ID for submenu for added item = 104 */
		0,							/* flags field */
		
		/*	The remaining items are for testing to make sure the
			ITEM_REQUIRES_ bit flags in the XMI1 item flags field
			work properly.
		*/
#if TEST_ITEM_REQUIRES_FLAGS
		/* [4] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_WAVES",
		0,
		ITEM_REQUIRES_WAVES,
		/* [5] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_GRAPH",
		0,
		ITEM_REQUIRES_GRAPH,
		/* [6] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_TABLE",
		0,
		ITEM_REQUIRES_TABLE,
		/* [7] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_LAYOUT",
		0,
		ITEM_REQUIRES_LAYOUT,
		/* [8] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_GRAPH_OR_TABLE",
		0,
		ITEM_REQUIRES_GRAPH_OR_TABLE,
		/* [9] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_TARGET",
		0,
		ITEM_REQUIRES_TARGET,
		/* [10] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_PANEL",
		0,
		ITEM_REQUIRES_PANEL,
		/* [11] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_NOTEBOOK",
		0,
		ITEM_REQUIRES_NOTEBOOK,
		/* [12] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_GRAPH_OR_PANEL",
		0,
		ITEM_REQUIRES_GRAPH_OR_PANEL,
		/* [13] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_DRAW_WIN",
		0,
		ITEM_REQUIRES_DRAW_WIN,
		/* [14] */
		8,							/* add item to menu with ID=8 (Misc menu) */
		"ITEM_REQUIRES_PROC_WIN",
		0,
		ITEM_REQUIRES_PROC_WIN,
#endif
	}
};

/* Balloon strings for 'MenuXOP1 Misc1' item */
resource 'STR#' (1110, "MenuXOP1 Misc1") {
	{
		/* [1] (used when menu item is enabled) */
		"This item, “MenuXOP1 Misc1’, was put into Igor’s ‘Misc’ menu by the MenuXOP1 XOP. "
		"The item is enabled.",
		
		/* [2] (used when menu item is disabled) */
		"This item, “MenuXOP1 Misc1’, was put into Igor’s ‘Misc’ menu by the MenuXOP1 XOP. "
		"The item is disabled.",
		
		/* [3] (used when menu item is checked) */
		"This item, “MenuXOP1 Misc1’, was put into Igor’s ‘Misc’ menu by the MenuXOP1 XOP. "
		"The item is checked.",
		
		/* [4] (used when menu item is marked) */
		"This item, “MenuXOP1 Misc1’, was put into Igor’s ‘Misc’ menu by the MenuXOP1 XOP. "
		"The item is marked.",
	}
};

/* Balloon strings for 'MenuXOP1 Misc2' item. */
resource 'STR#' (1111, "MenuXOP1 Misc2") {
	{
		/* [1] (used when menu item is enabled) */
		"This item, “MenuXOP1 Misc2’, was put into Igor’s ‘Misc’ menu by the MenuXOP1 XOP. "
		"The item is enabled.",
		
		/* [2] (used when menu item is disabled) */
		"This item, “MenuXOP1 Misc2’, was put into Igor’s ‘Misc’ menu by the MenuXOP1 XOP. "
		"The item is disabled.",
		
		/* [3] (used when menu item is checked) */
		"This item, “MenuXOP1 Misc2’, was put into Igor’s ‘Misc’ menu by the MenuXOP1 XOP. "
		"The item is checked.",
		
		/* [4] (used when menu item is marked) */
		"This item, “MenuXOP1 Misc2’, was put into Igor’s ‘Misc’ menu by the MenuXOP1 XOP. "
		"The item is marked.",
	}
};

/* Balloon strings for 'MenuXOP1 Analysis' item. */
/* NOTE: Since the "MenuXOP1 Analysis" item is to be a submenu, we leave */
/*		 all strings empty except for the 'disabled' state string. */
resource 'STR#' (1112, "MenuXOP1 Analysis") {
	{
		/* [1] (used when menu item is enabled) */
		"",
		
		/* [2] (used when menu item is disabled) */
		"This item was put into Igor’s ‘Analysis’ menu by the MenuXOP1 XOP. The item is disabled.",
		
		/* [3] (used when menu item is checked) */
		"",
		
		/* [4] (used when menu item is marked) */
		"",
	}
};

