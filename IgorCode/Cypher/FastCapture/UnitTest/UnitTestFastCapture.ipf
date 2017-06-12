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
		input_base = "Image0010"
	endIf
	// go ahead and manually save out the struct we want..
	struct indenter_info inf_tmp
	setup(input_base,inf_tmp)
	// // check saving out the data (test this before the gui-changing functions, below
	ModFastIndenter#align_and_save_struct(inf_tmp)
	// XXX TODO: read back in the file, make sure the data wasn't corrupted 
	// (1) Is the data the same?
	// (2) Is the note the same
	// XXX DEBUGGING...
	Wave /T globals = root:packages:MFP3D:Main:Strings:GlobalStrings
	String path_to_save = globals[%SaveImage]
	// XXX TODO: read back in the data...
	// POST: saving everything works
	// // make sure the GUI-handling works
	ModFastIndenter#setup_gui_for_fast_capture()
 	// POST: GUI-handling works as desired. 
	// // Check the the setup is OK
	ModFastIndenter#setup_indenter()	
	// POST: setup is also fine; only potential problem is the actual run-code, 
	// which can be unit tested by attempting to take data. 
End Function