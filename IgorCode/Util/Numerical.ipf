// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#include ":ErrorUtil"
#pragma ModuleName = ModNumerical

Static Function first_index_greater(wave_x,level)
	// Find the first index where wave_x is greater than level
	//
	// Args:
	//	wave_x: the wave we are interested in 
	// 	level: the crossing level 
	// Returns:
	//	index where the wave crossed
	
	// Arguments to FindLevel:
	// /B=1: do no averaging
	// /E=1: only where levels are increasing
	// /Q: quit run 
	// /P: return value as points (instead of as x value)
	Wave wave_x
	Variable level
	FindLevel /B=1 /EDGE=1 /Q wave_x, level
	ModErrorUtil#assert(V_flag == 0,msg="wave never crossed given level")
	// POST: found a crossing.
	return V_LevelX
End Function