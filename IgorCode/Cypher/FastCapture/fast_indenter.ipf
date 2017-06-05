// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModFastIndenter
#include ":NewFC"
#include ":::Util:IoUtil"
#include ":::Util:PlotUtil"


Static Function /WAVE master_variable_info()
	// Returns: the master panel variable wave
	return root:packages:MFP3D:Main:Variables:MasterVariablesWave
End Function

Static Function /S master_base_name()
	// Returns: the master panel base name
	SVAR str_v = root:packages:MFP3D:Main:Variables:BaseName
	// XXX check SVAR exists.
	return str_v
End Function

Static Function default_speed()
	// Returns: the default speed for the indenter capture
	return 0
End Function

Static Function default_timespan()
	// Returns:  the default time for the indenter capture
	return 5
End Function

Static Function /S default_wave_base_suffix()
	// Returns: the default fast-capture suffix for the indenter
	return "_fci_"
End Function

Static Function /S default_save_folder()
	// Returns: the default location to save the data
	return "root:prh:fast_indenter:data:" 
End Function	

Static Function setup_directory_sturcture()
	// This function ensures the indenter directory structure is set up
	// /O: OK if doesn't already exist, don't overwrite
	NewDataFolder /O root:prh
	NewDataFolder /O root:prh:fast_indenter
	NewDataFolder /O root:prh:fast_indenter:data
End Function

Static Function current_image_suffix()
	// Returns: the current image id or suffix (e.g. 1 for 0001)
	return master_variable_info()[%BaseSuffix][%Value]
End Function

Static Function /S formatted_wave_name(base,suffix,[type])
	// Formats a wave name as the cyper (e.g.  Image_0101Deflv)
	//
	// Args:
	//	base: name of the wave (e.g. 'Image')
	//	suffix: id of the wave (e.g. 1)
	//     type: optional type after the suffix (e.g. 'force')
	// Returns:
	//	Wave formatted as an asylum name would be, given the inputs
	String base,type
	Variable suffix
	String to_ret
	if (ParamIsDefault(type))
		type = ""
	EndIf
	// Formatted like <BaseName>_<justified number><type>
	// e.g. Image_0101Deflv
	sprintf to_ret,"%s_%04d%s",base,suffix,type
	return to_ret
End Function

Static Function /S default_wave_base_name()
	// Returns: the default wave, according to the (global / cypher) Suffix and base
	Variable suffix =current_image_suffix()
	String base = master_base_name()
	return  formatted_wave_name(base,suffix)
End Function 

Static Function capture_indenter([speed,timespan,wave0,wave1])
	//	Starts the fast capture routine using the indenter panel, 
	//	accounting for the parameters and notes appropriately.
	//
	//Args:
	//		see ModFastIndenter#fast_capture_setup
	// Returns
	//		result of ModFastIndenter#fast_capture_setup
	Variable speed,timespan
	Wave wave0,wave1
	// determine what the actual values of the parameters are
	speed = ParamIsDefault(speed) ? default_speed() : speed
	timespan=ParamIsDefault(timespan) ? default_timespan() : timespan
	// Determine the output wave names...
	String default_base = (default_wave_base_name() + default_wave_base_suffix())
	// Make sure the output folder exists
	String default_save = default_save_folder()
	setup_directory_sturcture()	
	// POST: data folder exists, get the output path
	String default_base_path = default_save + default_base
	String default_y_path = default_base_path + "Deflv"
	String default_x_path = default_base_path + "ZSnsr"
	if (ParamIsDefault(wave0))
		Make /O/N=0 $default_y_path
		Wave wave0 = $(default_y_path)
	endif
	if (ParamIsDefault(wave1))
		Make /O/N=0 $default_x_path
		Wave wave1 = $(default_y_path)
	endif
	// POST: all parameters set.
	Variable to_ret = ModFastCapture#fast_capture_setup(speed,timespan,wave0,wave1)
	// POST: fast capture is setup 
	// Call Fast Capture
	ModFastCapture#fast_capture_start()
	// Call the single force curve
	DoForceFunc("Single")
	// POST: data is saved into the waves we want
	// get the current figure, assuming it is the force review, etc..
	String figure = ModPlotUtil#gcf()
	// Get the *reference* to the wave we want
	Variable current_suffix = current_image_suffix()
	String base_name = master_base_name()
	String full_path = formatted_wave_name(base_name,current_suffix,type="DeflV")
	Wave low_res_wave = TraceNameToWaveRef(figure,full_path)
	// get the note of the wave we want (want them to be consistent)
	String low_res_note = note(low_res_wave)
	// Add the note to the higher-res waves
	// XXX fix deltax, etc?
	Note wave0, low_res_note
	Note wave1, low_res_note
	// save out the high resolution wave
	return to_ret
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
	capture_indenter()
End Function