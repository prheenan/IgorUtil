// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModFeather
#include ":::IgorCode:Util:IoUtil"
#include ":::IgorCode:Util:PlotUtil"
#include ":::IgorCode:Util:OperatingSystemUtil"


Static StrConstant path_to_feather_folder = "Research/Perkins/Projects/PythonCommandLine/FEATHER/"
Static StrConstant feather_file = "main_feather.py"

Structure FeatherOutput
	Wave event_starts
EndStructure

Structure FeatherOptions
	// Structure describing all the options the feather takes
	//
	//	Args: 
	//		threshold: the probability (between 0 and 1, log spacing is a good idea)
	//		tau: the number of points to use for spline fitting XXX currently not supported
	//
	//		path_to_research_directory: absolute path to directory one above Research (parent of 
	//		Research/Perkins/Projects/ ...)
	//
	//		path_to_input_file: where the pxp you want to analyze is. Should have a single wave 
	// 		like <x><d>_Sep and a single wave like <x><d>_Force. <x> can be anything,
	//  		<d> should be a 4-digit identifier (e.g. "Image0001_Sep" would be OK)
	Variable threshold
	Variable tau
	Struct RuntimeMetaInfo meta
EndStructure

Static Function /S full_path_to_feather_folder(options)
	// Function that gives the full path to the inverse weierstrass python folder
	//
	// Args:
	//		options: the FeatherOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string to full path
	Struct FeatherOptions & options
	return options.meta.path_to_research_directory + path_to_feather_folder
End Function

Static Function /S output_file_name(options)
	//
	// Returns:
	//		the output file to read from
	Struct FeatherOptions & options
	String FolderPath = ModFeather#full_path_to_feather_folder(options)
	return FolderPath + "events.csv"
End Function

Static Function /S python_command(opt)
	// Function that turns an options structure into a python command string
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string to command, OS-specific
	Struct FeatherOptions & opt
	String PythonCommand
	String python_str  = ModOperatingSystemUtil#python_binary_string()
	String FolderPath = ModFeather#full_path_to_feather_folder(opt)
	String FullPath = ModFeather#full_path_to_feather_main(opt)
	// Get just the python portion of the command
	String Output
	sprintf Output,"%s %s ",python_str,FullPath
	ModOperatingSystemUtil#append_argument(Output,"threshold",num2str(opt.threshold))
	String output_file = output_file_name(opt)
	String input_file = opt.meta.path_to_input_file
	// Windows is a special flower and needs its paths adjusted
	if (running_windows())
		output_file = ModOperatingSystemUtil#sanitize_path_for_windows(output_file)
		input_file = ModOperatingSystemUtil#sanitize_path_for_windows(input_file)
	endif
	ModOperatingSystemUtil#append_argument(Output,"file_input",input_file)
	ModOperatingSystemUtil#append_argument(Output,"file_output",output_file,AddSpace=0)
	return Output
End Function

Static Function /S full_path_to_feather_main(options)
	// Function that gives the full path to the inverse weierstrass python folder
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string, full path to feather python main
	Struct FeatherOptions & options
	return full_path_to_feather_folder(options) + feather_file
End Function

Static Function feather(user_options,output)
	// Function that calls the IWT python code
	//
	// Args:
	// 		options : instance of the InverseWeierstrassOptions struct. 
	//		output: instance of InverseWeierstrassOutput. Note that the waves 
	//		should already be allocated
	// Returns:
	//		Nothing, but sets output appropriately. 
	//
	Struct FeatherOutput & output
	Struct FeatherOptions & user_options
	// make a local copy of user_options, since we have to mess with paths (ugh)
	// and we want to adhere to principle of least astonishment
	Struct FeatherOptions options 
	options = user_options
	// do some cleaning on the input and output...
	options.meta.path_to_input_file = ModOperatingSystemUtil#replace_double("/",options.meta.path_to_input_file)
	options.meta.path_to_research_directory = ModOperatingSystemUtil#replace_double("/",options.meta.path_to_research_directory)
	String input_file_igor, python_file_igor
	String path_to_feather_main = ModFeather#full_path_to_feather_main(options)
	// first thing we do is check if all the files exist
	if (ModOperatingSystemUtil#running_windows())
		options.meta.path_to_input_file = ModOperatingSystemUtil#sanitize_windows_path_for_igor(options.meta.path_to_input_file)
		options.meta.path_to_research_directory = ModOperatingSystemUtil#sanitize_windows_path_for_igor(options.meta.path_to_research_directory)
		input_file_igor = ModOperatingSystemUtil#to_igor_path(options.meta.path_to_input_file)
		python_file_igor = ModOperatingSystemUtil#to_igor_path(path_to_feather_main)
	else
		input_file_igor = ModOperatingSystemUtil#sanitize_mac_path_for_igor(options.meta.path_to_input_file)
		python_file_igor = ModOperatingSystemUtil#sanitize_mac_path_for_igor(path_to_feather_main)
	endif
	// // ensure we can actually call the input file (ie: it should exist)
	Variable FileExists = ModIoUtil#FileExists(input_file_igor)
	String ErrorString = "Bad Path, received non-existing input file: " + options.meta.path_to_input_file
	ModErrorUtil#Assert(FileExists,msg=ErrorString)
	// POST: input file exists
	// // ensure we can actually find the python file
	FileExists = ModIoUtil#FileExists(ModOperatingSystemUtil#to_igor_path(python_file_igor))
	ErrorString = "Bad Path, couldnt find python script at: " + python_file_igor
	ModErrorUtil#Assert(FileExists,msg=ErrorString)
	// POST: input and python directories are a thing!
	String output_file = output_file_name(options)
	if (running_windows())
		// much easier just to use the user's input, assume it is OK at this point.
		// note that windows needs the <.py> file path to be something like C:/...
		options.meta.path_to_research_directory = ModOperatingSystemUtil#sanitize_path_for_windows(options.meta.path_to_research_directory)
	endif
	// Run the python code 
	String PythonCommand = ModFeather#python_command(options)
	ModOperatingSystemUtil#execute_python(PythonCommand)
	// Get the data into wavesd starting with <basename>
	String basename = "tmp"
	String igor_path = ModOperatingSystemUtil#sanitize_path(output_file)
	// Ensure the file actually got made...
	FileExists = ModIoUtil#FileExists(igor_path)
	ErrorString = "Couldnt find output file at: " + igor_path
	ModErrorUtil#Assert(FileExists,msg=ErrorString)
	// load the wave (the first 2 lines are header)
	Variable first_line = 2
	ModOperatingSystemUtil#read_csv_to_path(basename,igor_path,first_line=first_line)
	// Put it in the output parts
	Duplicate /O $(basename + "0"), output.event_starts
	// kill all the temporary stuff
	KillWaves /Z $(basename + "0")
	// kill the output file
	// /Z: if the file doesn't exist, dont worry about it  (we assert we deleted below)
	DeleteFile /Z (igor_path)
	ModErrorUtil#Assert(V_flag == 0,msg="Couldn't delete output file")
End Function
