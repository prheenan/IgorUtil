/*	NIGPIB.c

	See NIGPIB documentation for use of NIGPIB XOP.
	
	3/7/94
		Added IBBNA, IBCONFIG, IBLINES, IBLLO NI488 calls.
		Bumped NIGPIB version to 1.2.

	1/17/96
		Added testMode for testing at WaveMetrics.
		Made Igor Pro 3.0-savvy (data-folder-aware, support integer waves).
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "NIGPIB.h"

/* Global Variables */
extern int gActiveBoard;				/* the unit descriptor used for interface clear */
extern int gActiveDevice;				/* the unit descriptor used for read/write */


/*	NIErrToNIGPIBErr(NIErr)

	NIErr is bitwise code as defined in decl.h.
	This routine returns the corresponding NIGPIB error, defined in NIGPIB.h.
*/
int
NIErrToNIGPIBErr(int NIErr)
{
	if (NIErr == EDVR)
		return EDVR_ERR;
		
	if (NIErr == ECIC)
		return ECIC_ERR;
		
	if (NIErr == ENOL)
		return ENOL_ERR;
		
	if (NIErr == EADR)
		return EADR_ERR;
		
	if (NIErr == EARG)
		return EARG_ERR;
		
	if (NIErr == ESAC)
		return ESAC_ERR;
		
	if (NIErr == EABO)
		return EABO_ERR;
		
	if (NIErr == ENEB)
		return ENEB_ERR;
		
	if (NIErr == EDMA)
		return EDMA_ERR;
		
	if (NIErr == EOIP)
		return EOIP_ERR;
		
	if (NIErr == ECAP)
		return ECAP_ERR;
		
	if (NIErr == EFSO)
		return EFSO_ERR;
		
	if (NIErr == EBUS)
		return EBUS_ERR;
		
	if (NIErr == ESTB)
		return ESTB_ERR;
		
	if (NIErr == ESRQ)
		return ESRQ_ERR;
	
	#ifdef EASC					// Not defined in decl-32.h as of 971208.
		if (NIErr == EASC)
			return EASC_ERR;
	#endif
		
	#ifdef EDC					// Not defined in decl-32.h as of 971208.
		if (NIErr == EDC)
			return EDC_ERR;
	#endif
		
	if (NIErr == ETAB)
		return ETAB_ERR;
		
	if (NIErr == ELCK)
		return ELCK_ERR;
	
	return UNKNOWN_NI488_DRIVER_ERROR;
}

int
IBErr(int write)	// 0 for read, 1 for write
{
	if (ibsta & ERR) {
		if (ibsta & TIMO)
			return(write ? TIME_OUT_WRITE:TIME_OUT_READ);
		else
			return(iberr + FIRST_XOP_ERR + 1);
	}
	return 0;
}

int
SetV_Flag(double value)
{
	return SetOperationNumVar("V_flag", value);
}

int
SetS_Value(const char* str)
{
	return SetOperationStrVar("S_value", str);
}

/*	XOPQuit()

	Called to clean thing up when XOP is about to be disposed.
*/
static void
XOPQuit(void)
{	
	Close_NIGPIB_IO();
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP for all messages after the
	INIT message.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;
	
	switch (GetXOPMessage()) {
		case NEW:									/* new experiment being loaded */
			SetXOPType(TRANSIENT);					/* discard NIGPIB when new experiment */
			break;

		case CLEANUP:								/* XOP about to be disposed of */
			XOPQuit();
			break;
	}												/* CMD is only message we care about */
	SetXOPResult(result);
}

static int
RegisterOperations(void)
{
	int err;
	
	if (err = RegisterNI4882())
		return err;
	
	if (err = RegisterGPIB2())
		return err;
	
	if (err = RegisterGPIBRead2())
		return err;
	
	if (err = RegisterGPIBReadWave2())
		return err;
	
	if (err = RegisterGPIBReadBinary2())
		return err;
	
	if (err = RegisterGPIBReadBinaryWave2())
		return err;
	
	if (err = RegisterGPIBWrite2())
		return err;
	
	if (err = RegisterGPIBWriteWave2())
		return err;
	
	if (err = RegisterGPIBWriteBinary2())
		return err;
	
	if (err = RegisterGPIBWriteBinaryWave2())
		return err;

	return 0;
}

/*	XOPMain(ioRecHandle)

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.

	XOPMain does any necessary initialization and then sets the XOPEntry field of the
	ioRecHandle to the address to be called for future messages.
*/
HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)		// The use of XOPMain rather than main means this XOP requires Igor Pro 6.20 or later
{
	int err;

	XOPInit(ioRecHandle);				// Do standard XOP initialization
	SetXOPEntry(XOPEntry);				// Set entry point for future calls
	SetXOPType(RESIDENT);				// Specify XOP to stick around and to receive IDLE messages
	
	if (igorVersion < 620) {			// Requires Igor 6.20 or later.
		SetXOPResult(OLD_IGOR);			// OLD_IGOR is defined in NIGPIB.h and there are corresponding error strings in NIGPIB.r and NIGPIBWinCustom.rc.
		return EXIT_FAILURE;
	}
	
	if (err = RegisterOperations()) {
		SetXOPResult(err);
		return EXIT_FAILURE;
	}
	
	err = Init_NIGPIB_IO();

	SetXOPResult(err);
	return EXIT_SUCCESS;
}
