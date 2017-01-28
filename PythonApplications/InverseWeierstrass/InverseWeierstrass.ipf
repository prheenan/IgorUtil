// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModInverseWeierstrass
#include ":::IgorCode:Util:IoUtil"
#include ":::IgorCode:Util:PlotUtil"
#include ":::IgorCode:Util:OperatingSystemUtil"


Static StrConstant path_to_iwt_folder = "Research/Perkins/Projects/PythonCommandLine/InverseWeierstrass/"
Static StrConstant iwt_file = "main_iwt.py"

Structure InverseWeierstrassOutput
	Wave molecular_extension_meters
	Wave energy_landscape_joules
	Wave tilted_energy_landscape_joules
EndStructure

Structure InverseWeierstrassOptions
	// Structure describing all the options the IWT takes
	//
	//	Args: 
	//		number_of_pairs: the number of approach/retract curves (assumed all the same length)
	//		present in the file to load. If just one approach/retract (bad idea, IWT is for ensembles), this
	//		would be one.
	//
	//		number_of_bins: number of extension bins to use. Depends on data, but setting this
	//		so the bin size is 1-10AA is a good place to start (eg: 100 for an extenesion change of 10nm)
	//
	//		fraction_velocity_fit: the IWT recquires knowing the velocity the tip-sample separation would
	//		move at in the absense of a sample. For relatively symmetric approach/retracts, < 0.5 is safe
	//		
	//		f_one_half_N: the force at which half of everything is folded/unfolded. In Newtons
	//
	//		path_to_research_directory: absolute path to directory one above Research (parent of 
	//		Research/Perkins/Projects/ ...)
	//
	//		flip_forces: if true (1), then multiply all forces by -1 to get the force on the molecule.
	// 		By default, the cypher gets the force on the tip (e.g. default is negative means the tip
	//		is pulled towards the surface), and IWT needs the force on the molecule
	//
	//		path_to_input_file: where the pxp you want to analyze is. Should have a single wave 
	// 		like <x>_Sep and a single wave like <y>_Force which are zeroed in separation and
	// 		force and have an integer number of retract/approach pairs.
	Variable number_of_pairs
	Variable number_of_bins
	Variable fraction_velocity_fit
	Variable f_one_half_N
	Variable flip_forces
	String path_to_research_directory
	String path_to_input_file
EndStructure

Static Function /S full_path_to_iwt_folder(options)
	// Function that gives the full path to the inverse weierstrass python folder
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string to full path
	Struct InverseWeierstrassOptions & options
	return options.path_to_research_directory + path_to_iwt_folder
End Function

Static Function /S output_file_name(options)
	//
	// Returns:
	//		the output file to read from
	Struct InverseWeierstrassOptions & options
	String FolderPath = ModInverseWeierstrass#full_path_to_iwt_folder(options)
	return FolderPath + "landscape.csv"
End Function

Static Function /S python_command(opt)
	// Function that turns an options structure into a python command string
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string to command, OS-specific
	Struct InverseWeierstrassOptions & opt
	String PythonCommand
	String python_str  = ModOperatingSystemUtil#python_binary_string()
	String FolderPath = ModInverseWeierstrass#full_path_to_iwt_folder(opt)
	String FullPath = ModInverseWeierstrass#full_path_to_iwt_main(opt)
	// Get just the python portion of the command
	String Output
	sprintf Output,"%s %s ",python_str,FullPath
	ModOperatingSystemUtil#append_argument(Output,"number_of_pairs",num2str(opt.number_of_pairs))
	ModOperatingSystemUtil#append_argument(Output,"number_of_bins",num2str(opt.number_of_bins))
	ModOperatingSystemUtil#append_argument(Output,"f_one_half",num2str(opt.f_one_half_N))
	ModOperatingSystemUtil#append_argument(Output,"fraction_velocity_fit",num2str(opt.fraction_velocity_fit))
	ModOperatingSystemUtil#append_argument(Output,"flip_forces",num2str(opt.flip_forces))
	String output_file = output_file_name(opt)
	String input_file = opt.path_to_input_file
	// Windows is a special flower and needs its paths adjusted
	if (running_windows())
		output_file = ModOperatingSystemUtil#sanitize_path_for_windows(output_file)
		input_file = ModOperatingSystemUtil#sanitize_path_for_windows(input_file)
	endif
	ModOperatingSystemUtil#append_argument(Output,"file_input",input_file)
	ModOperatingSystemUtil#append_argument(Output,"file_output",output_file,AddSpace=0)
	return Output
End Function


Static Function execute_python(options)
	// executes a python command, given the options
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		nothing; throws an error if it finds one.
	Struct InverseWeierstrassOptions & options
	String PythonCommand = ModInverseWeierstrass#python_command(options)
	ModOperatingSystemUtil#assert_python_binary_accessible()
	// POST: we can for sure call the python binary
	ModOperatingSystemUtil#os_command_line_execute(PythonCommand)
End Function

Static Function /S full_path_to_iwt_main(options)
	// Function that gives the full path to the inverse weierstrass python folder
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string, full path to iwt python main
	Struct InverseWeierstrassOptions & options
	return full_path_to_iwt_folder(options) + iwt_file
End Function

Static Function inverse_weierstrass(user_options,output)
	// Function that calls the IWT python code
	//
	// Args:
	// 		options : instance of the InverseWeierstrassOptions struct. 
	//		output: instance of InverseWeierstrassOutput. Note that the waves 
	//		should already be allocated
	// Returns:
	//		Nothing, but sets output appropriately. 
	//
	Struct InverseWeierstrassOutput & output
	Struct InverseWeierstrassOptions & user_options
	// make a local copy of user_options, since we have to mess with paths (ugh)
	// and we want to adhere to principle of least astonishment
	Struct InverseWeierstrassOptions options 
	options = user_options
	// do some cleaning on the input and output...
	options.path_to_input_file = ModOperatingSystemUtil#replace_double("/",options.path_to_input_file)
	options.path_to_research_directory = ModOperatingSystemUtil#replace_double("/",options.path_to_research_directory)
	String input_file_igor, python_file_igor
	String path_to_iwt_main = ModInverseWeierstrass#full_path_to_iwt_main(options)
	// first thing we do is check if all the files exist
	if (ModOperatingSystemUtil#running_windows())
		options.path_to_input_file = ModOperatingSystemUtil#sanitize_windows_path_for_igor(options.path_to_input_file)
		options.path_to_research_directory = ModOperatingSystemUtil#sanitize_windows_path_for_igor(options.path_to_research_directory)
		input_file_igor = ModOperatingSystemUtil#to_igor_path(options.path_to_input_file)
		python_file_igor = ModOperatingSystemUtil#to_igor_path(path_to_iwt_main)
	else
		input_file_igor = ModOperatingSystemUtil#sanitize_mac_path_for_igor(options.path_to_input_file)
		python_file_igor = ModOperatingSystemUtil#sanitize_mac_path_for_igor(path_to_iwt_main)
	endif
	// // ensure we can actually call the input file (ie: it should exist)
	Variable FileExists = ModIoUtil#FileExists(input_file_igor)
	String ErrorString = "Bad Path, IWT received non-existing input file: " + options.path_to_input_file
	ModErrorUtil#Assert(FileExists,msg=ErrorString)
	// POST: input file exists
	// // ensure we can actually find the python file
	FileExists = ModIoUtil#FileExists(ModOperatingSystemUtil#to_igor_path(python_file_igor))
	ErrorString = "Bad Path, IWT couldnt find python script at: " + python_file_igor
	ModErrorUtil#Assert(FileExists,msg=ErrorString)
	// POST: input and python directories are a thing!
	String output_file = output_file_name(options)
	if (running_windows())
		// much easier just to use the user's input, assume it is OK at this point.
		// note that windows needs the <.py> file path to be something like C:/...
		options.path_to_research_directory = ModOperatingSystemUtil#sanitize_path_for_windows(options.path_to_research_directory)
	endif
	// Run the python code 
	execute_python(options)
	// Get the data into wavesd starting with <basename>
	String basename = "iwt_tmp"
	// Igor is evil and uses colons, defying decades of convention for paths
	String igor_path
	if (!running_windows())
		igor_path = ModOperatingSystemUtil#sanitize_mac_path_for_igor(output_file)
	else
		igor_path = ModOperatingSystemUtil#to_igor_path(output_file)
	endif
	// Ensure the file actually got made...
	FileExists = ModIoUtil#FileExists(igor_path)
	ErrorString = "IWT couldnt find output file at: " + igor_path
	ModErrorUtil#Assert(FileExists,msg=ErrorString)
	// load the wave (the first 2 lines are header)
	Variable FirstLine = 3
	// Q: quiet
	// J: delimited text
	// D: doouble precision
	// K=1: all columns are numeric 
	// /L={x,y,x,x}: skip first y-1 lines
	// /A=<z>: auto name, start with "<z>0" and work up
	LoadWave/Q/J/D/K=1/L={0,FirstLine,0,0,0}/A=$(basename) igor_path
	// Put it in the output parts
	Duplicate /O $(basename + "0"), output.molecular_extension_meters
	Duplicate /O $(basename + "1"), output.energy_landscape_joules
	Duplicate /O $(basename + "2"), output.tilted_energy_landscape_joules
	// kill all the temporary stuff
	KillWaves /Z $(basename + "0"), $(basename + "1"), $(basename + "2")
	// kill the output file
	// /Z: if the file doesn't exist, dont worry about it  (we assert we deleted below)
	DeleteFile /Z (igor_path)
	ModErrorUtil#Assert(V_flag == 0,msg="Couldn't delete output file")
End Function
