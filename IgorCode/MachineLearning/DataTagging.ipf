// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModDataTagging
#include "::Util:IoUtil"
#include "::Util:PlotUtil"
#include "::Util:Numerical"
#include "::Util:ErrorUtil"
#include "::Cypher:asylum_interface"

Macro DataTagging()
	ModDataTagging#Main()
End Macro

Static Function /S base_tagging_folder()
	return "root:prh:tagging:"
End Function

Static Function /S default_filtered_folder()
	return (base_tagging_folder() + "filtered:")
End Function

Static Function setup_tagging()
	NewDataFolder /O root:prh
	NewDataFolder /O root:prh:tagging
	NewDataFolder /O root:prh:tagging:filtered
End Function

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
	// Given a data folder, and a base name associted with the force review graph,
	// gets the full offset associated with trace (ie: if trace is just the retract, also adds
	// in the approach and dwell points for the 
	//
	// Args:
	//	data_folder: where all the traces live
	// 	base_name: the start of the name; we can add '_Ext' to this and get the
	//	(for example) approach wave name (e.g. "Image0010Defl")
	//	trace: the actual trace we want to offset
	// Returns:
	// 	The actual offset needed in the original data 
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
	// we want the absolute coordinate, regardless of whatever asylum says
	// It 'slices' the data in force review based on the 'indexes' note variable 
	String saved_idx= ModAsylumInterface#get_indexes(Note($trace))
	Variable asylum_offset = ModAsylumInterface#get_index_field_element(saved_idx,0)
	offset = offset + asylum_offset	
	return offset
End Function

Static Function relative_coordinates(window_var,point_var,get_x)
	// Get a point in relative axis coordinates: how far we are 
	// from the top/left if this is y or x
	//
	// Args:
	//	window_var: the rectangle of the window
	//	point: the point within the window
	//	get_x: if we should return the x or y
	// Returns:
	//	the 0/1 relative index...
	struct rect & window_var
	struct point & point_var
	Variable get_x
	Variable coord,min_coord,max_coord
	// (0,0) is at the top left 
	If (get_x)
		coord = point_var.h
		min_coord = window_var.left
		max_coord = window_var.right
	Else
		coord = point_var.v
		min_coord = window_var.top
		max_coord = window_var.bottom
	EndIf
	return max(0,coord-min_coord)/(max_coord-min_coord)
End Function

Static Function delete_tmp_data(fig)
	String fig 
	String filtered_folder = default_filtered_folder()						
	String restore_folder = ModIoUtil#cwd()
	SetDataFolder $(filtered_folder)
	DFREF data_folder = GetDataFolderDFR()
	Variable i 
	do
		Wave /Z w = 	WaveRefIndexedDFR(data_folder,i)	
		if (!WaveExists(w))
			break
		EndIf
		// Remove the wave from the graph
		String trace_name = NameOfWave(w)
		if (ModPlotUtil#trace_on_graph(fig,trace_name))
			RemoveFromGraph /W=$(fig) $(trace_name) 
		EndIf
		i += 1
	while(1)
	KillWaves /A
	SetDataFolder $(restore_folder)
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
	// XXX check if filtered wave has been added 
	String fig = s.WinName	
	strswitch(s.eventname)
		case "modified":
			break
		case "mousemoved":
			String trace_list = ModPlotUtil#trace_list(fig)
			// Get the Number of elements in the trace list 
			String sep = ","
			Variable n = ItemsInList(trace_list,sep)
			if (n == 0)
				break
			EndIf
			// POST: something on plot 
			String filtered_folder = default_filtered_folder()					
			String suffix = "_F"	
			String first = ModIoUtil#string_element(trace_list,0,sep=sep)
			if (!WaveExists($(filtered_folder + first + suffix)))
				delete_tmp_data(fig)
			else
				break
			EndIf
			// POST: need to make all the waves
			Variable i
			For (i=0; i<n; i+= 1)
				String wave_name = ModIoUtil#string_element(trace_list,i,sep=sep)
				Wave wave_tmp = TraceNameToWaveRef(fig,wave_name)
				String to_check = filtered_folder + wave_name + suffix
				// Then make it by filtering
				Duplicate /O wave_tmp $to_check
				Variable n_points = DimSize(wave_tmp,0) * 1e-3
				ModNumerical#savitsky_smooth($to_check,n_points=n_points)
				// Add it to the graph
				// /Q: prevent other graphs from updating
				// /W: specify window
				// /C: specify color 
				AppendToGraph/L=$("L0")/Q/W=$(fig)/C=(0,0,0) $to_check
			EndFor
		case "mousewheel":
			// XXX zoom in... not quite working right...
			break		
			Variable rel_x = relative_coordinates(s.WinRect,s.mouseLoc,1)
			Variable rel_y = relative_coordinates(s.WinRect,s.mouseLoc,0)
			// Get the current x limits
			Variable x_low,x_high
			ModPlotUtil#get_xlim(x_low,x_high,m_window=s.winName)
			ModErrorUtil#assert(x_low < x_high,msg="No axis range to work with")
			Variable range_x = x_high-x_low
			Variable centered_point = x_low + (rel_x * range_x)
			Variable zoom = 3
			// if we move the wheel up, zoom in (factor < 0)
			// if we move the wheel down, zoom out (factor > 0)
			Variable factor = s.wheelDy > 0 ? 1/zoom : zoom
			Variable low = centered_point - factor * range_x
			Variable high = centered_point + factor * range_x
			// limit the zoom to the plotted data
			String x_wave = AxisInfo(ModPlotUtil#gcf(),"CWAVE")
			Wave trace_plotted = info_struct.trace_reference
			low = max(0,low)
			high = min(rightx(trace_plotted),high)	
			SetAxis /A=2/W=$(s.winName)			
			ModPlotUtil#xlim(low,high,graphName=s.winName)
			break
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

Static Function Main([error_on_no_force_review])
	// Description goes here
	//
	// Args:
	//		error_on_no_force_review: if true (default), throws an
	//		error when there is no force review
	//		
	// Returns:
	//
	//
	Variable error_on_no_force_review
	if (ParamIsDefault(error_on_no_force_review))
		error_on_no_force_review = 1
	EndIf
	String window_name = "ForceReviewGraph"
	setup_tagging()
	delete_tmp_data(window_name)	
	// Make sure the window exists, otherwise just do a top-level
	if (!ModIoUtil#WindowExists(window_name))
		print("Couldn't find force review panel, attempting top level graph instead")
		ModErrorUtil#assert(!error_on_no_force_review)
		window_name = ModPlotUtil#gcf()
	EndIf
	hook_cursor_current_directory(window_name)
End Function