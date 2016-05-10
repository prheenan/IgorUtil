#include "XOPStandardHeaders.r"

resource 'vers' (1) {						// XOP version info.
	0x01, 0x05, release, 0x00, 0,			// Version bytes and country integer.
	"1.05",
	"1.05 Copyright 1990-2013 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						// Igor version info.
	0x06, 0x20, release, 0x00, 0,			// Version bytes and country integer.
	"6.20",
	"(for Igor 6.20 or later)"
};

resource 'STR#' (1100) {					// Custom error messages.
	{
		// These errors are NI-defined.
		"NI-488: Board or device name not recognized (EDVR).",								// 1
		"NI-488: Function requires NI-488 driver to be controller-in-charge (ECIC).",		// 2
		"NI-488: No listener on write function (ENOL).",									// 3
		"NI-488: NI-488 driver not addressed correctly (EADR).",							// 4
		"NI-488: Invalid argument to function call (EARG).",								// 5
		"NI-488: NI-488 driver not system controller as required (ESAC).",					// 6
		"NI-488: I/O operation timed out (EABO).",											// 7
		"NI-488: Non-existent NI-488 driver board (ENEB).",									// 8
		"NI-488: DMA hardware error detected (EDMA).",										// 9
		"NI-488: Unknown error (number 9).",												// 10
		"NI-488: I/O started before previous operation completed (EOIP).",					// 11
		"NI-488: No capability for operation (ECAP).",										// 12
		"NI-488: File system error (EFSO).",												// 13
		"NI-488: Unknown error (number 13).",												// 14
		"NI-488: Timeout while sending command bytes (EBUS).",								// 15
		"NI-488: Serial poll status byte lost (ESTB).",										// 16
		"NI-488: SRQ stuck in ON position (ESRQ).",											// 17
		"NI-488: Address status change during I/O command (EASC).",							// 18
		"NI-488: DCAS occurred during I/O command (EDC).",									// 19
		"NI-488: Unknown error (number 19).",												// 20
		"NI-488: Buffer full (ETAB).",														// 21
		"NI-488: Board or device is locked (ELCK).",										// 22
		"NI-488: The ibnotify callback failed to rearm (EARM).",							// 23
		"NI-488: The input handle is invalid (EHDL).",										// 24
		"NI-488: Unknown error (number 24).",												// 25
		"NI-488: Unknown error (number 25).",												// 26
		"NI-488: Wait already in progress on input ud (EWIP).",								// 27
		"NI-488: The event notification was cancelled due to reset of interface (ERST).",	// 28
		"NI-488: Unknown error (number 28).",												// 29
		"NI-488: Unknown error (number 29).",												// 30
		"NI-488: Unknown error (number 30).",												// 31

		// These errors are NIGPIB-defined.
		"NIGPIB2 requires Igor version 6.20 or later.",										// 32
		"You must specify device with 'GPIB2 device' operation.",							// 33
		"Specified device does not exist.",													// 34
		"You must specify board with 'GPIB2 board' operation.",								// 35
		"Specified board does not exist.",													// 36
		"Bad binary data type. Codes are the same as those used for the WaveType function.",	// 37
		"Can't do GPIBBinaryWaveRead or GPIBBinaryWaveWrite on a text wave. Use GPIBWaveRead or GPIBWaveWrite.",	// 38
		"Expected the name of a real, numeric wave containing a list of NI488.2 addresses.",	// 39
		"Expected the name of a real, numeric wave to receive a list of NI488.2 results.",		// 40
		"The address list is too long (exceeds 100 values).",								// 41
		"The limit value for FindLstn must be 1 to 100.",									// 42
		"The wave does not have enough data points.",										// 43
		"Unknown NI-488 driver error.",														// 44
		"The numeric format must end with a valid numeric conversion character.",			// 45
	}
};

resource 'STR#' (1101) {					// Misc strings for XOP.
	{
		/* [1] */
		"-1",								// This item is no longer supported by the Carbon XOP Toolkit.
		/* [2] */
		"No Menu Item",						// This item is no longer supported by the Carbon XOP Toolkit.
		/* [3] */
		"NIGPIB2 Help",						// Name of XOP's help file.
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
		"NI4882",							// External operation to access NI488 board.
		XOPOp | ioOp | compilableOp,

		"GPIB2",							// External operations to communicate with NIGPIB2.
		XOPOp | ioOp | compilableOp,

		"GPIBRead2",
		XOPOp | ioOp | compilableOp,

		"GPIBWrite2",
		XOPOp | ioOp | compilableOp,

		"GPIBReadWave2",
		XOPOp | ioOp | compilableOp,

		"GPIBWriteWave2",
		XOPOp | ioOp | compilableOp,

		"GPIBReadBinary2",
		XOPOp | ioOp | compilableOp,

		"GPIBWriteBinary2",
		XOPOp | ioOp | compilableOp,

		"GPIBReadBinaryWave2",
		XOPOp | ioOp | compilableOp,

		"GPIBWriteBinaryWave2",
		XOPOp | ioOp | compilableOp,
	}
};
