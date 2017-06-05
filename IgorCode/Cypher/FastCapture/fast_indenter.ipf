// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModFastIndenter
#include ":NewFC"

Static Function default_speed()
	return 0
End Function

Static Function default_timespan()
	return 5
End Function

Static Function capture_indenter([speed,timespan,wave0,wave1])
	Variable speed,timespan
	Wave wave0,wave1
	speed = ParamIsDefault(speed) ? default_speed() : speed
	timespan=ParamIsDefault(timspan) ? default_timespan() : timespan
	if (ParamIsDefault(wave0))
	
	endif
	if (ParamIsDefault(wave1))
	
	endif
	ModFastIndenter#fast_capture_setup(speed,timespan,wave0,wave1)

End Function

Static Function Main()
	// Description goes here
	//
	// Args:
	//		Arg 1:
	//		
	// Returns:
	//
	//
End Function