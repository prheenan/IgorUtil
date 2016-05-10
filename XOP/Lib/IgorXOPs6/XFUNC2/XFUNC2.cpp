/*	XFUNC2.c -- illustrates Igor external functions.
	
	3/29/94
		Compiled version 1.01 which does not require the math coprocessor.
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "XFUNC2.h"

/* Global Variables */
extern int hasFPU;					/* in XOPSupport.c */

static XOPIORecResult
RegisterFunction()
{
	int funcIndex;

	funcIndex = (int)GetXOPItem(0);		/* which function invoked ? */
	switch (funcIndex) {
		case 0:							/* y = a + b*log(c*x) (a, b and c are in coef wave) */
			return (XOPIORecResult)logfit;
			break;

		case 1:							/* plgndr(l, m, x) (see Numerical Recipes in C) */
			return (XOPIORecResult)plgndr;
			break;
	}
	return 0;
}

static int
DoFunction()
{
	int funcIndex;
	void *p;				/* pointer to structure containing function parameters and result */
	int err;

	funcIndex = (int)GetXOPItem(0);	/* which function invoked ? */
	p = (void*)GetXOPItem(1);		/* get pointer to params/result */
	switch (funcIndex) {
		case 0:						/* y = a + b*log(c*x) (a, b and c are in coef wave */
			err = logfit((LogFitParams*)p);
			break;

		case 1:						/* plgndr(l, m, x) (see Numerical Recipes in C) */
			err = plgndr((PlgndrParams*)p);
			break;
	}
	return(err);
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
		case FUNCTION:								/* our external function being invoked ? */
			result = DoFunction();
			break;
			
		case FUNCADDRS:
			result = RegisterFunction();
			break;
	}
	SetXOPResult(result);
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
	XOPInit(ioRecHandle);				// Do standard XOP initialization
	SetXOPEntry(XOPEntry);				// Set entry point for future calls

	if (igorVersion < 620) {
		SetXOPResult(IGOR_OBSOLETE);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
