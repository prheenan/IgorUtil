/*
	XFUNC3.h -- equates for XFUNC3 XOP
*/

/* XFUNC3 custom error codes */

#define OLD_IGOR 1 + FIRST_XOP_ERR
#define UNKNOWN_XFUNC 2 + FIRST_XOP_ERR
#define NO_INPUT_STRING 3 + FIRST_XOP_ERR

/* Prototypes */
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);
