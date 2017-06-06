// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModFastIndenter
#include ":NewFC"
#include ":ForceModifications"
#include ":::Util:IoUtil"
#include ":::Util:PlotUtil"
#include ":asylum_interface"

static constant max_info_chars = 100

// XXX TODO: for this to work properly, Force Review must be open, and 'last' wave must be selected.

structure indenter_info
	// Struct to use to communicate with the call back
	// <x/y_wave_high_res>: the string name of the high resolution wave
	char x_wave_high_res[max_info_chars] 
	char y_wave_high_res[max_info_chars] 
	// points_per_second: the frequency of data capture
	double points_per_second
EndStructure 

Static Function default_speed()
	// Returns: the default speed for the indenter capture
	return 0
End Function

Static Function default_timespan()
	// Returns:  the default time for the indenter capture
	return 15
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

Static Function /S get_global_wave_loc()
	// Returns: the string location of the globally-saved wave
	return "root:prh:fast_indenter:info"
End Function

Static Function save_info_struct(ind_inf)
	// Saves out the information structure (overwriting anything pre-exising one)
	// Args:
	//	ind_inf: indenter_info struct reference to save
	// Returns:
	//	Nothing
	struct indenter_info & ind_inf
	String wave_name =  get_global_wave_loc()
	Make /O/N=0 $(wave_name)
	StructPut /B=3 ind_inf $(wave_name)
End Function

Static Function get_info_struct(ind_inf)
	// Reads the existing information structure 
	//
	// Args:
	//	ind_inf: indenter_info struct reference to read into
	// Returns:
	//	Nothing
	struct indenter_info & ind_inf
	StructGet /B=3 ind_inf $(get_global_wave_loc())
End Function


Static Function /S get_force_review_wave_note()
	// Returns: 
	//	the current note associated with the last-saved wave (assuming it is p
	//     plotted in the ForceReviewGraph
	Variable current_suffix = ModAsylumInterface#current_image_suffix()-1
	String base_name = ModAsylumInterface#master_base_name()
	String trace_name = ModAsylumInterface#formatted_wave_name(base_name,current_suffix,type="Defl_Ret")
	// set the current graph to the force review panel
	String review_name = ModAsylumInterface#force_review_graph_name()	
	// get the note of DeflV on the force review graph
	String low_res_note = ModPlotUtil#top_graph_wave_note(trace_name,fig=(review_name))	
	return low_res_note
End Function

Function prh_indenter_final()
	// Call the 'normal' asylum callback, then saves out the normal data
	// Args/Returns: None
	TriggerScale()
	// POST: data is saved into the waves we want
	String low_res_note = get_force_review_wave_note()	
	// pass the information by reference
	struct indenter_info indenter_info
	get_info_struct(indenter_info)
	// Make sure we are still correctly connected after the force input (it changes the cross point)
	ModAsylumInterface#assert_infastb_correct()
	// Add the note to the higher-res waves
	Wave zsnsr_wave = $(indenter_info.x_wave_high_res)
	Wave defl_wave = $(indenter_info.y_wave_high_res)
	// update the frequency (all other information is the same)
	Variable freq = indenter_info.points_per_second
	low_res_note = ModAsylumInterface#replace_note_variable(low_res_note,"NumPtsPerSec",freq)
	Note zsnsr_wave, low_res_note
	Note defl_wave, low_res_note
	// save out the high resolution wave to *disk*
	// XXX delete the high resolution wave (in memory only)?
	ModAsylumInterface#save_to_disk_volts(zsnsr_wave,defl_wave,note_to_use=low_res_note)	
End Function

Function prh_indenter_callback(ctrl_name)
	// callback which immediately calls the asylum callback, then forwards to the custom routine
	//
	// Note: callbacks __must__ not be static, otherwise we get an error
	//
	// Args:
	//	ctrl_name: see FinishForceFunc
	// Returns: 
	//	nothing
	String ctrl_name
	// Immediately call the 'normal' Asylum trigger
	prh_FinishForceFunc(ctrl_name,callback_string="prh_indenter_final()")

End Function

Static Function capture_indenter([speed,timespan,zsnsr_wave,defl_wave])
	//	Starts the fast capture routine using the indenter panel, 
	//	accounting for the parameters and notes appropriately.
	//
	//	PRE: Defl must be connected to InFastB
	//Args:
	//		see ModFastIndenter#fast_capture_setup
	// Returns
	//		result of ModFastIndenter#fast_capture_setup
	Variable speed,timespan
	Wave zsnsr_wave,defl_wave
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
	if (ParamIsDefault(defl_wave))
		Make /O/N=0 $default_y_path
		Wave defl_wave = $(default_y_path)
	endif
	if (ParamIsDefault(zsnsr_wave))
		Make /O/N=0 $default_x_path
		Wave zsnsr_wave = $(default_x_path)
	endif
	// POST: all parameters set. Save out the structure info
	struct indenter_info inf_tmp
	inf_tmp.x_wave_high_res = ModIoUtil#GetPathToWave(zsnsr_wave)
	inf_tmp.y_wave_high_res = ModIoUtil#GetPathToWave(defl_wave)
	inf_tmp.points_per_second = ModFastCapture#speed_option_to_frequency_in_Hz(speed)
	save_info_struct(inf_tmp)
	// before anything else, make sure the review exists
	String review_name = ModAsylumInterface#force_review_graph_name()
	ModPlotUtil#assert_window_exists(review_name)
	// POST: review window exists. Make sure that defl is set up for InFastB
	ModAsylumInterface#assert_infastb_correct()
	// POST: inputs are correct, set up the fast capture
	Variable to_ret = ModFastCapture#fast_capture_setup(speed,timespan,defl_wave,zsnsr_wave)
	// Set up the CTFC to call our special callback 
	// POST: fast capture and callback is setup 
	// Call Fast Capture
	ModFastCapture#fast_capture_start()
	// Call the single force curve (using modified function from ForceModifications)
	ModForceModifications#prh_DoForceFunc("Single",non_ramp_callback="prh_indenter_callback")
	return to_ret
End Function

Static Function Main()
	// Runs the capture_indenter function with all defaults
	capture_indenter()
End Function