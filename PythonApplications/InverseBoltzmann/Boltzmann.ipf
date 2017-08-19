// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModBoltzmann
#include ":::IgorCode:Util:IoUtil"
#include ":::IgorCode:Util:PlotUtil"
#include ":::IgorCode:Util:OperatingSystemUtil"


Static StrConstant path_to_boltzmann_folder = "Research/Perkins/Projects/PythonCommandLine/InverseBoltzmann/"
Static StrConstant boltzmann_file = "main_inverse_boltzmann.py"

Structure BoltzmannOutput
	Wave event_starts
EndStructure

Structure BoltzmannOptions
	// Structure describing all the options the boltzmann takes
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
	Variable trigger_time
	Variable dwell_time
	Variable spring_constant
	Struct RuntimeMetaInfo meta
EndStructure

Static Function /S full_path_to_boltzmann_folder(options)
	// Function that gives the full path to the inverse weierstrass python folder
	//
	// Args:
	//		options: the BoltzmannOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string to full path
	Struct BoltzmannOptions & options
	return options.meta.path_to_research_directory + path_to_boltzmann_folder
End Function

Static Function /S output_file_name(options)
	//
	// Returns:
	//		the output file to read from
	Struct BoltzmannOptions & options
	String FolderPath = full_path_to_boltzmann_folder(options)
	return FolderPath + "events.csv"
End Function

Static Function /S python_command(opt)
	// Function that turns an options structure into a python command string
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string to command, OS-specific
	Struct BoltzmannOptions & opt
	String PythonCommand
	String python_str  = ModOperatingSystemUtil#python_binary_string()
	String FolderPath = full_path_to_boltzmann_folder(opt)
	String FullPath = full_path_to_boltzmann_main(opt)
	// Get just the python portion of the command
	String Output
	sprintf Output,"%s %s ",python_str,FullPath
	ModOperatingSystemUtil#append_argument(Output,"number_of_bins",num2str(opt.threshold))
	ModOperatingSystemUtil#append_argument(Output,"interpolation_factor",num2str(opt.tau))
	ModOperatingSystemUtil#append_argument(Output,"smart_interpolation",num2str(opt.spring_constant))
	ModOperatingSystemUtil#append_argument(Output,"gaussian_stdev",num2str(opt.dwell_time))
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

Static Function /S full_path_to_boltzmann_main(options)
	// Function that gives the full path to the inverse weierstrass python folder
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string, full path to feather python main
	Struct BoltzmannOptions & options
	return full_path_to_boltzmann_folder(options) + boltzmann_file
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
	Struct BoltzmannOutput & output
	Struct BoltzmannOptions & user_options
	// Manually set the main path and output file
	user_options.meta.path_to_main = full_path_to_boltzmann_main(user_options)
	user_options.meta.path_to_output_file = output_file_name(user_options)
	// make a local copy of user_options, since we have to mess with paths (ugh)
	// and we want to adhere to principle of least astonishment
	Struct BoltzmannOptions options 
	options = user_options
	ModOperatingSystemUtil#get_updated_options(options.meta)
	// Run the python code 
	String PythonCommand = python_command(options)	
	ModOperatingSystemUtil#execute_python(PythonCommand)
	Make /O/FREE/Wave wave_tmp = {output.event_starts}
	ModOperatingSystemUtil#get_output_waves(wave_tmp,options.meta,skip_lines=2)
End Function
