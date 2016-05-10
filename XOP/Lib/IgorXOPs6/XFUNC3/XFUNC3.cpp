/*	XFUNC3.c -- illustrates Igor external string functions.

	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "XFUNC3.h"

#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct xstrcatParams  {
	Handle str3;
	Handle str2;
	Handle result;
};
typedef struct xstrcatParams xstrcatParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
xstrcat(xstrcatParams* p)				/* str1 = xstrcat(str2, str3) */
{
	Handle str1;						/* output handle */
	int len2, len3;
	int err=0;
	
	str1 = NULL;						/* if error occurs, result is NULL */
	if (p->str2 == NULL) {				/* error ÐÐ input string does not exist */
		err = NO_INPUT_STRING;
		goto done;
	}
	if (p->str3 == NULL)	{			/* error ÐÐ input string does not exist */
		err = NO_INPUT_STRING;
		goto done;
	}
	
	len2 = (int)GetHandleSize(p->str2);	/* length of string 2 */
	len3 = (int)GetHandleSize(p->str3);	/* length of string 3 */
	str1 = NewHandle(len2 + len3);		/* get output handle */
	if (str1 == NULL) {
		err = NOMEM;
		goto done;						/* out of memory */
	}
	
	memcpy(*str1, *p->str2, len2);
	memcpy(*str1+len2, *p->str3, len3);
	
done:
	if (p->str2)
		DisposeHandle(p->str2);			/* we need to get rid of input parameters */
	if (p->str3)
		DisposeHandle(p->str3);			/* we need to get rid of input parameters */
	p->result = str1;
	
	return(err);
}

static XOPIORecResult
RegisterFunction()
{
	int funcIndex;

	funcIndex = (int)GetXOPItem(0);			/* which function invoked ? */
	switch (funcIndex) {
		case 0:								/* str1 = xstrcat0(str2, str3) */
			return (XOPIORecResult)xstrcat;	/* This uses the direct call method - preferred. */
			break;
		case 1:								/* str1 = xstrcat1(str2, str3) */
			return 0;						/* This uses the message call method - generally not needed. */
			break;
	}
	return 0;
}

static int
DoFunction()
{
	int funcIndex;
	void *p;				/* pointer to structure containing function parameters and result */
	int err;				/* error code returned by function */

	funcIndex = (int)GetXOPItem(0);	/* which function invoked ? */
	p = (void*)GetXOPItem(1);		/* get pointer to params/result */
	switch (funcIndex) {
		case 1:						/* str1 = xstrcat2(str2, str3) */
			err = xstrcat((xstrcatParams*)p);
			break;
		default:
			err = UNKNOWN_XFUNC;
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
		SetXOPResult(OLD_IGOR);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
