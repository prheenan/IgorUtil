#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x00, final, 0x00, 0,				/* version bytes and country integer */
	"1.00",
	"1.00, Copyright 1993-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor Pro 6.20 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"XFUNC3 requires Igor Pro 6.20 or later.",
		/* [2] */
		"XFUNC3 XOP was called to execute an unknown function.",
		/* [3] */
		"Input string is non-existent.",
	}
};

/* no menu item */

resource 'XOPI' (1100) {
	XOP_VERSION,							// XOP protocol version.
	DEV_SYS_CODE,							// Development system information.
	0,										// Obsolete - set to zero.
	0,										// Obsolete - set to zero.
	XOP_TOOLKIT_VERSION,					// XOP Toolkit version.
};

resource 'XOPF' (1100) {
	{
		/* str1 = xstrcat0(str2, str3) */	/* This uses the direct call method */
		"xstrcat0",							/* function name */
		F_STR | F_EXTERNAL,					/* function category (string) */
		HSTRING_TYPE,						/* return value type str1 (string handle) */			
		{
			HSTRING_TYPE,					/* str2 (string handle) */
			HSTRING_TYPE,					/* str3 (string handle) */
		},

		/* str1 = xstrcat1(str2, str3) */	/* This uses the message call method */
		"xstrcat1",							/* function name */
		F_STR | F_EXTERNAL,					/* function category (string) */
		HSTRING_TYPE,						/* return value type (string handle) */			
		{
			HSTRING_TYPE,					/* str2 (string handle) */
			HSTRING_TYPE,					/* str3 (string handle) */
		},
	}
};
