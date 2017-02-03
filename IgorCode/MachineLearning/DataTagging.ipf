// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModDataTagging
#include "::Util:IoUtil"
#include "::Util:ErrorUtil"

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
	hook_cursor_current_directory("ForceReviewGraph")
End Function