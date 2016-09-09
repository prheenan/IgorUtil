// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModFitUtil

 // V-88
 // N=1: don't update during the fit
 // /W=2: dont display any results for the curve fitting
 // /Q: quiet, dont print
Static Constant CURVEFIT_DEF_N_SILENT = 1
Static Constant CURVEFIT_DEF_W_SILENT = 1
// Default fraction of points for smoothing in savitsky golay
// Effectively filters to f_unfiltered*(DEF_SG_POINT_FRACTION)
Static Constant DEF_SG_POINT_FRACTION = 0.01

// Min and max of savitsky golay; must *also* take into account order
Static Constant SG_MAX_POINTS = 32767
Static Constant SG_MIN_POINTS = 3 // plus the order gives the minimim
// By default, use second order SG
Static Constant DEF_SG_ORDER = 2
Static Constant POLY_DEF_DEG = 40

// Find the intersection of
// y=a0*(x-offset0)+b0
// y=a1*(x-offset1)+b1
// a0*(x-offset0) - a1*(x-offset1) = b1 - b0
// a0*x-a1*x  = b1 - b0 + a0*offset0-a1*offset1
// x = (b1 - b0 + a0*offset0-a1*offset1)/(a0-a1)
// XXX add in offset support?
Static Function LineIntersect(a0,b0,a1,b1,[offset0,offset1])
	Variable a0,b0,a1,b1,offset0,offset1
	Variable toRet =  (b1-b0)/(a0-a1)
	return toRet
End Function


// Turns a raw number into a safe number to use for the savitsky golay filtering
// must between [SG_MIN_POINTS+order,SG_MAX_POINTS] and be odd
Static Function GetSafeSmoothingFactor(rawFactor,order)
	Variable rawFactor,order
	// First of all, we must have an integer
	Variable toRet  = ceil(rawFactor)
	toRet= max(SG_MIN_POINTS+order,toRet)
	toRet = min(toRet,SG_MAX_POINTS)
	// nPoints must be odd for savitsky golay to work
	// Note that this in combination with the ceiling might change the time constant
	// this should only be noticable is the time constant is on 
	// Note also that if we are even, we are guarenteed NOT to be at the min or max (both are odd)
	if (mod(toRet,2) == 0)
		toRet +=1
	EndIf
	return toRet
End Function

// Smooths 'inData' (in place! modifies it!) using a savitsky golay filter of nPoints (must be odd, between bounds)
// to order 'order'
Static Function SavitskySmooth(ToSmooth,[nPoints,order])
	Wave ToSmooth
	Variable nPoints,order
	Variable maxN = DimSize(ToSmooth,0)
	nPoints = ParamIsDefault(nPoints) ? ceil(DEF_SG_POINT_FRACTION*maxN) : nPoints
 	order = ParamIsDefault(order) ? DEF_SG_ORDER : order
	nPoints = GetSafeSmoothingFactor(nPoints,order)
	// /S: savitsky golay polynomial order, 2 or 4
	// Smooth, V-592:
	Smooth /S=(order) (nPoints),ToSmooth
End Function

// fits line a*x+b =y, passes b and a by *references*
// if no y is found, then fits just to the indices of X
Static Function LinearFit(mY,slope,intercept,[mX])
	Wave mY,mX
	Variable &slope,&intercept
	 // line means a linear fit..
	 Make /O/N=(2) fitCoeffs
	if (!ParamIsDefault(mX))
		 CurveFit /N=(CURVEFIT_DEF_N_SILENT)/Q/W=(CURVEFIT_DEF_W_SILENT) line, kwCWave=fitCoeffs, mY /X=mX
	else
		CurveFit /N=(CURVEFIT_DEF_N_SILENT)/Q/W=(CURVEFIT_DEF_W_SILENT) line, kwCWave=fitCoeffs, mY
	EndIf
	// POST: the coefficients are populated
	intercept = fitCoeffs[0]
	slope = fitCoeffs[1]
	// Kill the wave we used
	KillWaves /Z fitCoeffs
End Function
