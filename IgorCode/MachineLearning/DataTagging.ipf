// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModDataTagging
#include "::Util:IoUtil"
#include "::Util:ErrorUtil"

// Note: hook functions *must* not be static, or all is lost.
Function window_hook_prototype(s)
	Struct WMWinHookStruct &s
End Function

Static Function /S get_output_path()
	String my_file_name =ModIoUtil#current_experiment_name()
	SVAR base = prh_tagging_output_directory
	return base + my_file_name +"_events.txt"
End Function

Function  save_cursor_updates_by_globals(s)
	// Window hook prototype function which saves the 
	Struct WMWinHookStruct &s
	Variable status_code = 0
	strswitch(s.eventname)
		case "keyboard":
			// if we hit enter on the window, go ahead and save the cursor positions out.
			if (ModIoUtil#is_ascii_enter(s.keycode))
				String window_name = s.winName
				Variable a_idx,b_idx
				String trace
				ModIoUtil#cursor_info(window_name,a_idx,b_idx,trace)
				// POST: have the A and B cursors. Save them to the output file
				Make/FREE/T/N=(1,3) tmp = {trace,num2str(a_idx),num2str(b_idx)}
				ModIoUtil#save_as_comma_delimited(tmp,get_output_path(),append_flag=2)
			EndIf
	endswitch
	return status_code
End Function

Static Function assert_window_exists(window_name)
	String window_name
	String error_msg = "Couldn't find window "+ window_name
	ModErrorUtil#Assert(ModIoUtil#WindowExists(window_name),msg=error_msg)
End Function

Static Function hook_cursor_saver_to_window(window_name,file_directory)
	String window_name,file_directory
	// only way to get the file path to the save file, above, if via a global string X_X...
	// XXX try to fix?
	// Overwrite the global variables
	String /G prh_tagging_output_directory = file_directory
	KillWaves /Z prh_tagging_wave
	Make /T/O/N=(0,3) prh_tagging_wave	
	// SetDimLabel x,y,name,wave:
	// sets number y of dimension  x (0=rows,1=columns,etc) of wave to name
	// set the first column label
	SetDimLabel 1,0,trace_name,prh_tagging_wave
	SetDimLabel 1,1,event_start_idx,prh_tagging_wave
	SetDimLabel 1,2,event_end_idx,prh_tagging_wave
	// Check that the file path is real
	ModErrorUtil#Assert(ModIoUtil#FileExists(prh_tagging_output_directory))
	// Write the output file
	Variable ref
	// Open without flags: new file, overwrites
	String output_path = get_output_path()
	Open /Z ref as output_path
	ModErrorUtil#Assert( (V_Flag == 0),msg="couldn't open file")
	Close ref
	// Make sure the window exists
	assert_window_exists(window_name)
	SetWindow $(window_name) hook(prh_hook)=save_cursor_updates_by_globals
End Function

Static Function hook_cursor_saver_interactive(window_name)
	String window_name
	String file_directory
	// we pass file_name by reference; updated if we succeed
	if (ModIoUtil#GetFolderInteractive(file_directory))
		hook_cursor_saver_to_window(window_name,file_directory)
	else
		ModErrorUtil#Assert(0,msg="couldn't find the file...")
	endif
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
	hook_cursor_saver_interactive("ForceReviewGraph")
End Function