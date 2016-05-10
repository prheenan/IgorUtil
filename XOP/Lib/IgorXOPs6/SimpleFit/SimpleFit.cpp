/*	SimpleFit.c

	A simplified project designed to act as a template for your curve fitting function.
	The fitting function is a simple polynomial. It works but is of no practical use.
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h


// Prototypes
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);


// Custom error codes
#define OLD_IGOR 1 + FIRST_XOP_ERR
#define NON_EXISTENT_WAVE 2 + FIRST_XOP_ERR
#define REQUIRES_SP_OR_DP_WAVE 3 + FIRST_XOP_ERR

#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
typedef struct FitParams {
	double x;				// Independent variable.
	waveHndl waveHandle;	// Coefficient wave.
	double result;
} FitParams, *FitParamsPtr;
#pragma pack()		// Restore default structure alignment

/*	SimpleFit creates a polynomial of the following form. The length
	depends on the length of the coefficient wave. Returns:
	w[0] + w[1]*x + w[2]*x^2 + ...

	Warning:
		The call to WaveData() below returns a pointer to the middle
		of an unlocked Macintosh handle. In the unlikely event that your
		calculations could cause memory to move, you should copy the coefficient
		values to local variables or an array before such operations.
*/
extern "C" int
SimpleFit(FitParamsPtr p)
{
	CountInt np,i;
	double *dp;				// Pointer to double precision wave data.
	float *fp;				// Pointer to single precision wave data.
	double r,x;
	
	if (p->waveHandle == NULL) {
		SetNaN64(&p->result);
		return NON_EXISTENT_WAVE;
	}
	
	np= WavePoints(p->waveHandle);
	
	x= p->x;
	switch(WaveType(p->waveHandle)){			// We can handle single and double precision coefficient waves.
		case NT_FP32:
			fp= (float*)WaveData(p->waveHandle);
			i= np-1;
			r = fp[i];
			for(i= i-1;i>=0;i--)
				r = fp[i] + r*x;
			break;
		case NT_FP64:
			dp= (double*)WaveData(p->waveHandle);
			i= np-1;
			r = dp[i];
			for(i= i-1;i>=0;i--)
				r = dp[i] + r*x;
			break;
		default:								// We can't handle this wave data type.
			SetNaN64(&p->result);
			return REQUIRES_SP_OR_DP_WAVE;
	}
	
	p->result= r;
	
	return 0;
}

static XOPIORecResult
RegisterFunction()
{
	int funcIndex;

	funcIndex = (int)GetXOPItem(0);			// Which function invoked ?
	switch (funcIndex) {
		case 0:									// y = SimpleFit(w,x) (curve fitting function).
			return (XOPIORecResult)SimpleFit;	// This function is called using the direct method.
			break;
	}
	return 0;
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP for all
	messages after the INIT message.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;

	switch (GetXOPMessage()) {
		case FUNCADDRS:
			result = RegisterFunction();	// This tells Igor the address of our function.
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
	XOPInit(ioRecHandle);							// Do standard XOP initialization.
	SetXOPEntry(XOPEntry);							// Set entry point for future calls.
	
	if (igorVersion < 620) {
		SetXOPResult(OLD_IGOR);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
