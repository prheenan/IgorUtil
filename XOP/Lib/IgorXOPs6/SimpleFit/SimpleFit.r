#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x10, final, 0x00, 0,				/* version bytes and country integer */
	"1.10",
	"1.10, Copyright 1996-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor Pro 6.20 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"SimpleFit requires Igor Pro 6.20 or later",
		/* [2] */
		"Wave does not exist.",
		/* [3] */
		"Coefficient wave must be single or double precision floating point.",
	}
};

resource 'STR#' (1101) {					// Misc strings for XOP.
	{
		"-1",								// This item is no longer supported by the Carbon XOP Toolkit.
		"No Menu Item",						// This item is no longer supported by the Carbon XOP Toolkit.
		"SimpleFit Help",					// Name of XOP's help file.
	}
};

resource 'XOPI' (1100) {
	XOP_VERSION,							// XOP protocol version.
	DEV_SYS_CODE,							// Development system information.
	0,										// Obsolete - set to zero.
	0,										// Obsolete - set to zero.
	XOP_TOOLKIT_VERSION,					// XOP Toolkit version.
};

resource 'XOPF' (1100) {
	{
		"SimpleFit",						/* function name */
		F_EXP | F_EXTERNAL,					/* function category == exponential */
		NT_FP64,							/* return value type */			
		{
			NT_FP64 + WAVE_TYPE,			/* double precision wave (coefficient wave) */
			NT_FP64,						/* double precision x */
		},
	}
};
