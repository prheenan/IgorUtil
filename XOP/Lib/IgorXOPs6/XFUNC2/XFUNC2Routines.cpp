/*	XFUNC2Routines.c -- number crunching routines for XFUNC2 external function.

*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "XFUNC2.h"

/* Global Variables (none) */

extern "C" int
logfit(							/* y = a + b*log(c*x) */
	struct LogFitParams* p)		/* struct is defined in XFUNC2.h */
{
	double *dPtr;				/* pointer to double precision wave data */
	float *fPtr;				/* pointer to single precision wave data */
	double a, b, c;

	// Check that wave handle is valid
	if (p->waveHandle == NIL) {
		SetNaN64(&p->result);			// Return NaN if wave is not valid
		return NULL_WAVE_OP;			// Return error to Igor
	}
	
	/* check coefficient wave's numeric type */
	switch (WaveType(p->waveHandle)) {
		case NT_FP32:
			fPtr = (float*)WaveData(p->waveHandle);
			a = fPtr[0];
			b = fPtr[1];
			c = fPtr[2];
			break;
		case NT_FP64:
			dPtr = (double*)WaveData(p->waveHandle);
			a = dPtr[0];
			b = dPtr[1];
			c = dPtr[2];
			break;
		default:								/* we can't handle this wave data type */
			SetNaN64(&p->result);				/* return NaN if wave is not single or double precision float */
			return(REQUIRES_SP_OR_DP_WAVE);
	}
	
	p->result = a + b*log10(c*p->x);
	
	return(0);
}

extern "C" int
plgndr(struct PlgndrParams* p)		/* struct is defined in XFUNC2.h */	
{					/* From "Numerical Recipes in C" */
	int l, m;
	double x;
	double fact, pll, pmm, pmmp1, somx2;
	int i, ll;

	l = (int)p->l;
	m = (int)p->m;
	x = p->x;
	
	if (m < 0 || m > l || fabs(x) > 1.0) {
		SetNaN64(&p->result);
		return(ILLEGAL_LEGENDRE_INPUTS);
	}

	pmm = 1.0;
	if (m > 0) {
		somx2 = sqrt((1.0-x) * (1.0+x));
		fact = 1.0;
		for (i=1; i <= m; i++) {
			pmm *= -fact*somx2;
			fact += 2.0;
		}
	}
	if (l == m) {
		p->result = pmm;
		return(0);
	}
	else {
		pmmp1 = x * (2*m+1) * pmm;
		if (l == (m+1)) {
			p->result = pmmp1;
			return(0);
		}
		else {
			for (ll = (m+2); ll <=l; ll++) {
				pll = (x * (2*ll-1) * pmmp1 - (ll + m - 1) * pmm)/(ll-m);
				pmm = pmmp1;
				pmmp1 = pll;
			}
			p->result = pll;
		return(0);
		}
	}
	return(0);
}
