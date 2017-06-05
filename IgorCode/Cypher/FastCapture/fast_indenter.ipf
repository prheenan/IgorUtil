// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModFastIndenter
#include ":NewFC"
#include ":::Util:IoUtil"
#include ":::Util:PlotUtil"

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
	String default_base = (ModAsylumInterface#default_wave_base_name() + default_wave_base_suffix())
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
	// Get the *reference* to the wave we want
	Variable current_suffix = ModAsylumInterface#current_image_suffix()
	String base_name = ModAsylumInterface#master_base_name()
	String trace_name = ModAsylumInterface#formatted_wave_name(base_name,current_suffix,type="Defl")
	// set the current graph to the force review panel
	String review_name = ModAsylumInterface#force_review_graph_name()
	ModPlotUtil#scf(review_name)
	// get the note of DeflV on the force review graph
	String low_res_note = ModPlotUtil#top_graph_wave_note(trace_name,fig=(review_name))
	// Add the note to the higher-res waves
	// XXX fix deltax, etc?
	Note wave0, low_res_note
	Note wave1, low_res_note
	// save out the high resolution wave to *disk*
	Variable save_to_disk = 3;
	ARSaveAsForce(save_to_disk,"SaveForce","Defl;ZSnsr",wave0,wave1,$"",$"",$"",$"",$"",CustomNote=low_res_note)
	// XXX delete the high resolution wave (in memory only)?
	return to_ret
End Function

Static Function Main()
	// Runs the capture_indenter function with all defaults
	capture_indenter()
End Function