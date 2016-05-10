/*	XFUNC1.c -- illustrates Igor external functions.
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "XFUNC1.h"

/* Global Variables (none) */

#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct XFUNC1AddParams  {
	double p2;
	double p1;
	double result;
};
typedef struct XFUNC1AddParams XFUNC1AddParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
XFUNC1Add(XFUNC1AddParams* p)
{
	p->result = p->p1 + p->p2;
	
	return(0);					/* XFunc error code */
}


#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct XFUNC1DivParams  {
	double p2;
	double p1;
	double result;
};
typedef struct XFUNC1DivParams XFUNC1DivParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
XFUNC1Div(XFUNC1DivParams* p)
{
	p->result = p->p1 / p->p2;
	
	return(0);					/* XFunc error code */
}


#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct DPComplexNum {
	double real;
	double imag;
};
#pragma pack()		// Reset structure alignment to default.

#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct XFUNC1ComplexConjugateParams  {
	struct DPComplexNum p1;					// Complex parameter
	struct DPComplexNum result;				// Complex result
};
typedef struct XFUNC1ComplexConjugateParams XFUNC1ComplexConjugateParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
XFUNC1ComplexConjugate(XFUNC1ComplexConjugateParams* p)
{
	p->result.real = p->p1.real;
	p->result.imag = -p->p1.imag;

	return 0;
}


static XOPIORecResult
RegisterFunction()
{
	int funcIndex;

	funcIndex = (int)GetXOPItem(0);	/* which function invoked ? */
	switch (funcIndex) {
		case 0:						/* XFUNC1Add(p1, p2) */
			return (XOPIORecResult)XFUNC1Add;
			break;
		case 1:						/* XFUNC1Div(p1, p2) */
			return (XOPIORecResult)XFUNC1Div;
			break;
		case 2:						/* XFUNC1ComplexConjugate(p1) */
			return (XOPIORecResult)XFUNC1ComplexConjugate;
			break;
	}
	return 0;
}

/*	DoFunction()

	This will actually never be called because all of the functions use the direct method.
	It would be called if a function used the message method. See the XOP manual for
	a discussion of direct versus message XFUNCs.
*/
static int
DoFunction()
{
	int funcIndex;
	void *p;				/* pointer to structure containing function parameters and result */
	int err;

	funcIndex = (int)GetXOPItem(0);	/* which function invoked ? */
	p = (void*)GetXOPItem(1);		/* get pointer to params/result */
	switch (funcIndex) {
		case 0:						/* XFUNC1Add(p1, p2) */
			err = XFUNC1Add((XFUNC1AddParams*)p);
			break;
		case 1:						/* XFUNC1Div(p1, p2) */
			err = XFUNC1Div((XFUNC1DivParams*)p);
			break;
		case 2:						/* XFUNC1ComplexConjugate(p1) */
			err = XFUNC1ComplexConjugate((XFUNC1ComplexConjugateParams*)p);
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
XOPMain(IORecHandle ioRecHandle)			// The use of XOPMain rather than main means this XOP requires Igor Pro 6.20 or later
{	
	XOPInit(ioRecHandle);					// Do standard XOP initialization
	SetXOPEntry(XOPEntry);					// Set entry point for future calls
	
	if (igorVersion < 620) {
		SetXOPResult(OLD_IGOR);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
