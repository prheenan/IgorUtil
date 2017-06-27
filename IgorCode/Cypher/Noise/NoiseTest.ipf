// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModNoiseTest
#include "::FastCapture:NewFC"
#include "::FastCapture:ForceModifications"
#include ":::Util:IoUtil"
#include ":::Util:PlotUtil"
#include ":::Util:Numerical"
#include "::FastCapture:asylum_interface"

Static Function Main()
	// Runs the capture_indenter function with all defaults
	Variable timespan = 20
	Variable speed = 1
	NewDataFolder /O root:prh
	NewDataFolder /O root:prh:noise
	Make /O/N=0 root:prh:noise:defl, root:prh:noise:zsnsr
	ModFastCapture#fast_capture_setup(speed,timespan,root:prh:noise:defl,root:prh:noise:zsnsr)
	ZeroPD()	
	ModFastCapture#fast_capture_start()
End Function
