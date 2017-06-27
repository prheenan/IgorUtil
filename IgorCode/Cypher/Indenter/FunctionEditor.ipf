// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModFunctionEditor
#include ":::Util:ErrorUtil"
#include ":::Util:PlotUtil"


Static Function /S editor_base()
	// Returns: where the indenter lives
	return "root:packages:MFP3D:FunctionEditor:Indenter:"
End Function

Static Function /S variable_wave_name()
	// Returns: the path to the variable wave (need this to,
	// for example, set the velocities)
	return  editor_base() + "FEVariablesWave"
End function

Static Function /S function_editor_window()
	// Returns: the name of the indenter graph
	return  "FEGraphIndenter"
End Function 

Static Function set_indenter_variable(name,value)
	// Sets an indenter variable (by name) to the given value
	//
	// Args:
	//	name: of the variable, see UserProgramming.ipf
	//	 value: what to set it to. right now, this is in units of whatever is already there...
	String name
	Variable value
	Struct WMSetVariableAction setvar_struct
	// These are the codes needed by ARFESetVarFunc; mouse up or enter?
	setvar_struct.EventCode = 2
	setvar_struct.EventCode = 1
	String VName = variable_wave_name() + "[%" + name + "][%Value]"
	setvar_struct.VName = VName
	setvar_struct.sVal = num2str(value)
	setvar_struct.dval = value
	setvar_struct.Win = function_editor_window()
	// POST: window exists, should be OK
	Wave setvar_struct.svWave = $(variable_wave_name())
	ARFESetVarFunc(setvar_struct)
End Function

Static Function make_segment_equilibrium(location,time_delta)
	// Makes the currently-selected segment an equilibrium segment 
	// at a certain Z with a given time
	//
	// Args:
	//	location: the z value needed
	// 	time_delta: thelength of the segment
	// Returns:
	//	nothing
	Variable location,time_delta
	set_indenter_variable("SegmentLength",time_delta)
	set_indenter_variable("SegmentStartPos",location)
	set_indenter_variable("SegmentEndPos",location)
	set_indenter_variable("SegmentVelocity",0)	
End Function

Static Function /S indenter_handle()
	// Returns: the 'handle' for the indenter
	// (used by some asylum functions)
	return "Indenter"
End Function

Static Function delete_existing_indenter()
	// Deletes the existing waves on the indenter
	// (technically, it doesn't the delete the last segment, 
	// because there always has to be one left)
	Wave wave_ref = $(variable_wave_name())
	ModErrorUtil#assert_wave_exists(wave_ref)
	String handle = indenter_handle()
	// keep deleting segments while they are a thing
	// XXX make into for loop?
	do
		Variable A, NumKilled, NumOfSegs = wave_ref[%NumOfSegments][%Value]
		ARFEDeleteFunc(handle,NumOfSegs)	
	while (NumOfSegs > 1)
End Function

Static Function setup_equilbirum_wave(locations,n_delta_per_location,time_delta)
	// sets up an equilibrium wave on the current indenter.
	//
	//	PRE: must have a current indenter	
	//
	// Args:
	//		locations: wave, size N, what the step heights are
	//		n_delta_per_location: how many 'time_delta' steps should be spent at each location
	//		time_delta: fundamental step unit
	// Returns:
	//		Nothing
	//
	Wave locations,n_delta_per_location
	Variable time_delta
	Variable n_times = DimSize(n_delta_per_location,0)
	ModErrorUtil#Assert(n_times > 0,msg="equilibrium needs at least one step")
	Variable total_deltas = sum(n_delta_per_location)
	// POST: waves exist
	// Make sure the window exists
	ModPlotUtil#assert_window_exists(function_editor_window())
	// POST: first segment is all that remains. 
	// set up its location and time
	Variable first = locations[0]
	Variable time_first = time_delta*n_delta_per_location[0]
	make_segment_equilibrium(first,time_first)
	// Finish up the rest (inserting as we go)
	Variable i 
	for (i=1;i<n_times; i+= 1)
		Variable location_tmp = locations[i]
		Variable time_tmp =  time_delta*n_delta_per_location[i]
		ARFEInsertFunc(indenter_handle(),0,nan)
		make_segment_equilibrium(location_tmp,time_tmp)
	EndFor
End Function

Static Function staircase_equilibrium(start_x,delta_x,n_steps,time_dwell,[use_reverse])
	//  easy-of-use function; has <n_steps> 'plateaus', each of length time_dwell
	// starting at start_x and separated by delta_x
	//
	// Args:
	//	start_x: where to start, in x
	//	delta_x: how much to move each step 
	//	n_steps: how many steps there are
	//  	time_dwell: how long to dwell at each step (all the same)
	//	use_reverse: if true, also walks 'back' to the original point
	//
	// Returns:
	//	Nothing
	Variable start_x,delta_x,n_steps,time_dwell,use_reverse
	if (ParamIsDefault(use_reverse))
		use_reverse = 0
	EndIf
	ModErrorUtil#Assert(n_steps > 0,msg="equilibrium needs at least one step")
	Make /FREE/N=(n_steps) data_wave
	data_wave[] = start_x + p * delta_x
	If (use_reverse)
		// reversed will have one less point (so there 
		// are no duplicate points
		Make /FREE/N=(n_steps-1) reversed
		// copy the points such that we dont 
		// include the very last point
		// data_wave idx		reversed  idx
		// n_points - 2		0   (p=0)
		// n_points - 3		1   (p=1)
		//	...				...
		// 1					n-3  (p=n-3)
		// 0					n-2  (p=n-2)
		reversed[] = data_wave[n_steps-(p+2)]
		// combine the 'there and out' wave
		// /O: overwrite
		// /NP: no promotion (dont make a 2D wave)
		Concatenate /O /NP{data_wave,reversed}, data_wave
		// update the number of points
		n_steps = (DimSize(data_wave,0))
	EndIf
	// each location has exactly one time point 
	Make /FREE/N=(n_steps) n_time_deltas
	n_time_deltas[] = 1
	setup_equilbirum_wave(data_wave,n_time_deltas,time_dwell)
End Function

Static Function default_bidirectional_staircase([start_x,delta_x,n_steps,time_dwell])
	// Sets up a 'pretty good' bidirectional staircase, assuming
	// the indenter is open with units set to nm and nm/s
	//
	//	NOTE: this deletes any existing indenter set up
	//
	// Args:
	//	 see staircase_equilibrium
	// Returns:
	//	 nothing
	Variable start_x,delta_x,n_steps,time_dwell
	start_x = ParamIsDefault(start_x) ? -30 : start_x
	delta_x = ParamIsDefault(delta_x) ? -0.5 : delta_x
	n_steps = ParamIsDefault(n_steps) ? 50: n_steps
	time_dwell= ParamIsDefault(time_dwell) ? 75e-3 : time_dwell
	delete_existing_indenter()
	staircase_equilibrium(start_x,delta_x,n_steps,time_dwell,use_reverse=1)
End Function