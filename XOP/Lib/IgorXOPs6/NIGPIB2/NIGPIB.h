// NIGPIB.h - equates for NIGPIB XOP

#define NO_BOARD 32767
#define NO_DEVICE 32767


/* NIGPIB custom error codes */

// NOTE: If you change these NI-defined, change NIErrToNIGPIBErr in NIGPIB.c also.
#define EDVR_ERR 1 + FIRST_XOP_ERR				// Start NI-defined errors.
#define ECIC_ERR 2 + FIRST_XOP_ERR
#define ENOL_ERR 3 + FIRST_XOP_ERR
#define EADR_ERR 4 + FIRST_XOP_ERR
#define EARG_ERR 5 + FIRST_XOP_ERR
#define ESAC_ERR 6 + FIRST_XOP_ERR
#define EABO_ERR 7 + FIRST_XOP_ERR
#define ENEB_ERR 8 + FIRST_XOP_ERR
#define EDMA_ERR 9 + FIRST_XOP_ERR
#define UNKNOWN_ERR_9 10 + FIRST_XOP_ERR		// NI error #9 (NIGPIB error #10) is undefined.	
#define EOIP_ERR 11 + FIRST_XOP_ERR
#define ECAP_ERR 12 + FIRST_XOP_ERR
#define EFSO_ERR 13 + FIRST_XOP_ERR
#define UNKNOWN_ERR_13 14 + FIRST_XOP_ERR		// NI error #13 (NIGPIB error #14) is undefined.
#define EBUS_ERR 15 + FIRST_XOP_ERR
#define ESTB_ERR 16 + FIRST_XOP_ERR
#define ESRQ_ERR 17 + FIRST_XOP_ERR
#define EASC_ERR 18 + FIRST_XOP_ERR
#define EDC_ERR 19 + FIRST_XOP_ERR
#define UNKNOWN_ERR_19 20 + FIRST_XOP_ERR
#define ETAB_ERR 21 + FIRST_XOP_ERR
#define ELCK_ERR 22 + FIRST_XOP_ERR
#define EARM_ERR 23 + FIRST_XOP_ERR
#define EHDL_ERR 24 + FIRST_XOP_ERR
#define UNKNOWN_ERR_24 25 + FIRST_XOP_ERR
#define UNKNOWN_ERR_25 26 + FIRST_XOP_ERR
#define EWIP_ERR 27 + FIRST_XOP_ERR
#define ERST_ERR 28 + FIRST_XOP_ERR
#define UNKNOWN_ERR_28 29 + FIRST_XOP_ERR
#define UNKNOWN_ERR_29 30 + FIRST_XOP_ERR
#define UNKNOWN_ERR_30 31 + FIRST_XOP_ERR		// End of future NI-defined errors.

#define OLD_IGOR 32 + FIRST_XOP_ERR				// The rest are NIGPIB errors.
#define NO_DEVICE_ERR 33 + FIRST_XOP_ERR
#define NO_SUCH_DEVICE 34 + FIRST_XOP_ERR
#define NO_BOARD_ERR 35 + FIRST_XOP_ERR
#define NO_SUCH_BOARD 36 + FIRST_XOP_ERR
#define BAD_BINARY_TYPE 37 + FIRST_XOP_ERR
#define NO_BINARY_WAVE_OPERATIONS_ON_TEXT_WAVES 38 + FIRST_XOP_ERR
#define EXPECTED_ADDRESS_LIST_WAVE 39 + FIRST_XOP_ERR
#define EXPECTED_RESULT_LIST_WAVE 40 + FIRST_XOP_ERR
#define ADDRESS_LIST_TOO_LONG 41 + FIRST_XOP_ERR
#define BAD_LIMIT_FOR_FIND_LSTN 42 + FIRST_XOP_ERR
#define WAVE_TOO_SHORT 43 + FIRST_XOP_ERR
#define UNKNOWN_NI488_DRIVER_ERROR 44 + FIRST_XOP_ERR
#define BAD_NUMERIC_FORMAT 45 + FIRST_XOP_ERR

#define ERR_ALERT 1258

#define MAX_ADDRESS_LIST_ITEMS 100


// Globals
extern int gActiveBoard;						// The unit descriptor used for interface clear.
extern int gActiveDevice;						// The unit descriptor used for read/write.


// NICountInt is used when passing a count to the NI488 driver so that we can find where we do this.
// This may need to be tweaked for the 64-bit NI488 driver for which I have not yet compiled.
typedef CountInt NICountInt;


/* Prototypes */

// In NIGPIB.c.
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);
int NIErrToNIGPIBErr(int NIErr);
int IBErr(int write);
int SetV_Flag(double value);
int SetS_Value(const char* str);

// In NIGPIBOperation.c
int Init_NIGPIB_IO(void);
void Close_NIGPIB_IO(void);
int SetTestMode(int testMode); 
int NIGPIB_ibrd(int ud, char* buf, NICountInt count);
int NIGPIB_ibwrt(int ud, char* buf, NICountInt count);
int CheckActiveDevice(void);
int RegisterGPIB2(void);

// In NIGPIBIO.c.
int NullTerminateHandle(Handle h);
int Init_NIGPIB_IO(void);
void Close_NIGPIB_IO(void);
int SetTestMode(int testMode);
int CheckActiveDevice(void);
int RegisterGPIBRead2(void);
int RegisterGPIBWrite2(void);
int RegisterGPIBReadWave2(void);
int RegisterGPIBWriteWave2(void);
int RegisterGPIBReadBinary2(void);
int RegisterGPIBWriteBinary2(void);
int RegisterGPIBReadBinaryWave2(void);
int RegisterGPIBWriteBinaryWave2(void);

// In NI488.c
int RegisterNI4882(void);

// Include the necessary NI header file
#ifdef MACIGOR
	#define NIGPIB2_OSX
	#include <NI488/ni488.h>			// OS X Mach
#endif
#ifdef WINIGOR
	#include "ni488.h"
#endif
