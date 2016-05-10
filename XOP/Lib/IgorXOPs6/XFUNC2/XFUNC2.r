#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x01, final, 0x00, 0,				/* version bytes and country integer */
	"1.01",
	"1.01, Copyright 1993-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x06, 0x20, release, 0x00, 0,			/* version bytes and country integer */
	"6.20",
	"(for Igor 6.20 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"Coefficient wave must be single or double precision floating point",
		/* [2] */
		"m must be ² l; m and l must be within -1 to 1",
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
		/* y = a + b*log(c*x) */
		"logfit",							/* function name */
		F_EXP | F_EXTERNAL,					/* function category == exponential */
		NT_FP64,							/* return value type */			
		{
			NT_FP64 + WAVE_TYPE,			/* double precision wave (coefficient wave) */
			NT_FP64,						/* double precision x */
		},

		/* plgndr(l, m, x) (see Numerical Recipes in C) */
		"plgndr",							/* function name */
		F_SPEC | F_EXTERNAL,				/* function category == special */
		NT_FP64,							/* return value type */			
		{
			NT_FP64,						/* parameter types */
			NT_FP64,
			NT_FP64,
		},
	}
};
