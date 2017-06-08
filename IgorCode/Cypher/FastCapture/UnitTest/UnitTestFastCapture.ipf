// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModUnitTestFastCapture
#include "::asylum_interface"
#include "::fast_indenter"

Static Function setup(input_base,inf_tmp)
	// Sets up the file structure for the unit testing
	//
	// Args;
	//	input_base: the asylum-style 'stem' name of the wave we want to test (e.g.
	//	Image0010)
	//
	//	inf_tmp: the pass-by-reference struct we will use
	//Returns:
	//	nothing, but sets the structure appropriately
	String input_base
	struct indenter_info & inf_tmp
	String base = ModFastIndenter#default_save_folder()
	String base_path_high_resolution = base + input_base + ModFastIndenter#default_wave_base_suffix()
	inf_tmp.x_wave_high_res = base_path_high_resolution + "ZSnsr"
	inf_tmp.y_wave_high_res = base_path_high_resolution + "DeflV"
	inf_tmp.points_per_second = 5e5
	// save out the indenter structure.
	ModFastIndenter#save_info_struct(inf_tmp)	
End Function

Static Function select_index(select_wave,index)
	// Selects a given index at a wave
	// From igor manual:
	// 	Bit 0 (0x01):	Cell is selected.
	//
	// Args:
	//	select_wave: which wave to use 
	// 	index: what index to select
	// Returns:
	//	nothing
	Wave select_wave
	Variable index
	select_wave[index][0][0] = select_wave[index] | 0x1 
End Function

Static Function Main([input_base])
	// test that the fast capture is working
	//
	// Args:
	//		input_base: the input file to use, assumed part of the current experiment.
	//		e.g. 'Image0097'
	// Returns:
	//	 Nothing
	//
	String input_base
	if (ParamIsDefault(input_base))
		input_base = "Image0108"
	endIf
	// XXX debugging 
	String force_review = ModAsylumInterface#force_review_graph_name()
	String master_force_name = "MasterForcePanel"
	String y_axis_selector = "ForceAxesList0_0"
	ControlInfo /W=$(master_force_name) $(y_axis_selector)
	String list_wave_path = S_DataFolder + S_Value
	Wave list_wave = $(list_wave_path)
	Variable defl_exists = ModDataStructures#text_in_wave("Defl",list_wave)
	ModErrorUtil#assert(defl_exists,msg="Couldn't find Defl in Force Review Options")
	// POST: defl exists, get its index
	Variable defl_idx = ModDataStructures#element_index("Defl",list_wave)
	// Call the swapping procedure 
	Variable valid_event = 1
	Variable col = 0 
	select_index($("root:ForceCurves:Parameters:AxesListBuddy0"),defl_idx)
	HotSwapForceData(y_axis_selector,defl_idx,col,valid_event)
	// XXX select defl
	// Ensure that ZSnsr is selected as the x
	String x_axis_selector = "ForceXaxisPop_0"
	PopupMenu $(x_axis_selector) mode=5,popvalue="ZSnsr", win=$(master_force_name) 
	ChangeForceXAxis(x_axis_selector,5,"ZSnsr")
	// go ahead and manually save out the struct we want..
	struct indenter_info inf_tmp
	setup(input_base,inf_tmp)
	// do everything except the data capture (ie: the aligning and such) 
	ModFastIndenter#align_and_save_fast_capture()
End Function