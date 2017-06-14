// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModDataTagging
#include "::Util:IoUtil"
#include "::Util:PlotUtil"
#include "::Util:ErrorUtil"
#include "::Cypher:asylum_interface"

Macro DataTagging()
	ModDataTagging#Main()
End Macro

// Note: hook functions *must* not be static, or all is lost.
Function window_hook_prototype(s)
	Struct WMWinHookStruct &s
End Function

Static Function /S get_output_path()
	// Using the global string variable (!) get the output path for the saved file
	// Returns:
	//		the output path for the file we want
	String my_file_name =ModIoUtil#current_experiment_name()
	SVAR base = prh_tagging_output_directory
	return base + my_file_name +"_events.txt"
End Function

Static Function offset_from_wave_base(data_folder,base_name,trace)
	String data_folder,base_name,trace
	// get the offset associated 
	String appr_str = "_Ext",dwell_str = "_Towd",ret_str="_Ret"
	String base_path = data_folder + base_name
	String ApproachName =  (base_path + appr_str)
	String DwellName =  (base_path + dwell_str)
	Variable offset  =0 
	if (!WaveExists($ApproachName) || !WaveExists($DwellName))
		offset = 0
		return offset
	endif
	Variable n_approach = DimSize( $ApproachName,0)
	Variable n_dwell = DimSize($(DwellName),0)
	// get the actual offset
	if (ModIoUtil#string_ends_with(trace,appr_str))
		// no offset; dont do anything
		offset = 0 
	elseif(ModIoUtil#string_ends_with(trace,dwell_str))
		offset = n_approach
	else
		String err_message = "Dont recognize wave: " + base_path + trace
		ModErrorUtil#Assert(ModIoUtil#string_ends_with(trace,ret_str),msg=err_message)
		// POST: we know what is happening
		offset = n_approach + n_dwell
	endif
	return offset
End Function




Function  save_cursor_updates_by_globals(s)
	// hook functgion; called when the window has an event
	//
	// Args:
	//		s: instance of Struct WMWinHookStruct &s
	// Returns:
	//		Nothing
	// Window hook prototype function which saves the 
	Struct WMWinHookStruct &s
	Variable status_code = 0
	struct cursor_info info_struct
	strswitch(s.eventname)
		case "keyboard":
			// if we hit enter on the window, go ahead and save the cursor positions out.
			if (ModIoUtil#is_ascii_enter(s.keycode))
				String window_name = s.winName
				ModIoUtil#cursor_info(window_name,info_struct)
				// Get the data folder...
				// Args:
				//	kind: what to treturn
				// 		0 	Returns only the name of the data folder containing waveName.
				//		1 	Returns full path of data folder containing waveName, without wave name.
				//
				Variable kind =1
				String data_folder = GetWavesDataFolder(info_struct.trace_reference,kind)
				// Get the base name; we want to get the absolute offset for the entire trace...
				String base_name
				// our regex is anything, following by numbers, a (possible) single underscore, then letters
				String regex = "(.+?)[_]?[a-zA-Z]+$"
				SplitString /E=(regex) info_struct.trace_name,base_name
				Variable offset = offset_from_wave_base(data_folder,base_name,info_struct.trace_name)
				// we want the absolute coordinate, regardless of whatever asylum says
				String saved_idx= ModAsylumInterface#get_indexes(Note(info_struct.trace_reference))
				Variable asylum_offset = ModAsylumInterface#get_index_field_element(saved_idx,0)
				offset = offset + asylum_offset
				Variable start_idx = info_struct.a_idx + offset
				Variable end_idx = info_struct.b_idx + offset
				// POST: have the A and B cursors. Save them to the output file
				String start_idx_print,end_idx_print
				sprintf start_idx_print,"%d",start_idx
				sprintf end_idx_print, "%d",end_idx
				Make/FREE/T/N=(1,3) tmp = {base_name,start_idx_print,end_idx_print}
				ModIoUtil#save_as_comma_delimited(tmp,get_output_path(),append_flag=2)
			EndIf
	endswitch
	return status_code
End Function

Static Function assert_window_exists(window_name)
	// asserts that the given window exists
	//
	// Args:
	//		window_name: name of the window to check
	// Returns:
	//		Nothing
	String window_name
	String error_msg = "Couldn't find window "+ window_name
	ModErrorUtil#Assert(ModIoUtil#WindowExists(window_name),msg=error_msg)
End Function

Static Function hook_cursor_saver_to_window(window_name,file_directory)
	// sets up the hook for a window to point to save_cursor_updates_by_globals
	//
	// Args:
	//		window_name: name of the window to hook to
	//		file_directory: location where we want to save the file
	// Returns:
	//
	//	Nothing
	String window_name,file_directory
	// only way to get the file path to the save file, above, if via a global string X_X...
	// XXX try to fix?
	// Overwrite the global variables
	String /G prh_tagging_output_directory = file_directory
	// Check that the file path is real
	ModErrorUtil#Assert(ModIoUtil#FileExists(prh_tagging_output_directory))
	// Write the output file
	Variable ref
	// Open without flags: new file, overwrites
	String output_path = get_output_path()
      //       only create the file if it doesnt exist
       if (    ModIoUtil#FileExists(output_path))
               // append to the existing file
               Open /Z/A ref as output_path
       else
               //  Create a new file
               //      Open without flags: new file, overwrites
               Open /Z ref as output_path
       endif
       // check that we actually created the file
      	ModErrorUtil#Assert(ModIoUtil#FileExists(output_path))
	Close ref
	// Make sure the window exists
	assert_window_exists(window_name)
	SetWindow $(window_name) hook(prh_hook)=save_cursor_updates_by_globals
End Function

Static Function hook_cursor_saver_interactive(window_name)
	// sets up the hook for a window to point to save_cursor_updates_by_globals, asking the
	//user to give the folder 
	//
	// Args:
	//		window_name: name of the window to hook to
	// Returns:
	//		Nothing
	String window_name
	String file_directory
	// we pass file_name by reference; updated if we succeed
	if (ModIoUtil#GetFolderInteractive(file_directory))
		hook_cursor_saver_to_window(window_name,file_directory)
	else
		ModErrorUtil#Assert(0,msg="couldn't find the file...")
	endif
End Function

Static Function hook_cursor_current_directory(window_name)
	// sets up the hook for a window to point to save_cursor_updates_by_globals, saving alongside
	// wherever this pxp is
	//
	// Args:
	//		window_name: name of the window to hook to
	// Returns:
	//		Nothing
	String window_name
	PathInfo home
	// according to PathInfo:
	// "V_flag	[is set to] 0 if the symbolic path does not exist, 1 if it does exist."
	ModErrorUtil#Assert( (V_Flag == 1),msg="To use current directory as save, must save .pxp")
	// POST: path exists
	String file_directory = S_Path
	hook_cursor_saver_to_window(window_name,file_directory)
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
	String window_name = "ForceReviewGraph"
	// Make sure the window exists, otherwise just do a top-level
	if (!ModIoUtil#WindowExists(window_name))
		print("Couldn't find force review panel, attempting top level graph instead")
		window_name = ModPlotUtil#gcf()
	EndIf
	hook_cursor_current_directory(window_name)
End Function