// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModBoltzmann
#include ":::IgorCode:Util:IoUtil"
#include ":::IgorCode:Util:PlotUtil"
#include ":::IgorCode:Util:OperatingSystemUtil"


Static StrConstant path_to_boltzmann_folder = "Research/Perkins/Projects/PythonCommandLine/InverseBoltzmann/"
Static StrConstant boltzmann_file = "main_inverse_boltzmann.py"

Structure BoltzmannOutput
	// The output from this application
	// extension_bins: the x values for the probability distribution
	// distribution: the (convolved, or 'measured') distribution, or probability at each of extension_bins
	// distribution_deconvolved: the *deconvoled* distribution 
	Wave extension_bins
	Wave distribution
	Wave distribution_deconvolved
EndStructure

Structure BoltzmannOptions
	// Structure describing all the options this application takes 
	//
	//	Args: 
	//		number_of_bins: how many bins for P(q), the probability at a given extension
	//		interpolation_factor: how much to interpolate by. This should be >=1. Having >1 means 
	//		P(q) is interpolated, reducing problems in the deconvolution steps (since sharp jumps in P(q)
	//		will cause problems when convolving). If you aren't sure, set this to 1 and smart_interpolation to 1
	//
	//		smart_interpolation: if 1, then the code determines a good interpolation factor, assuming the gaussian_stdev
	//		is set properly
	//
	//		gaussian_stdev: the stdev of the (assumed gaussian) point spread function 
	// 
	//		output_interpolated: if 1, outputs the interpolated values (ie: of size number_of_bins * interpolation_factor)
	//		If 0, decimates the data so it returns an array of exactly size number_of_bins.
	Variable number_of_bins
	Variable interpolation_factor
      Variable smart_interpolation
      Variable gaussian_stdev 
      Variable output_interpolated
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
	ModOperatingSystemUtil#append_argument(Output,"number_of_bins",num2str(opt.number_of_bins))
	ModOperatingSystemUtil#append_argument(Output,"interpolation_factor",num2str(opt.interpolation_factor))
	ModOperatingSystemUtil#append_argument(Output,"smart_interpolation",num2str(opt.smart_interpolation))
	ModOperatingSystemUtil#append_argument(Output,"gaussian_stdev",num2str(opt.gaussian_stdev))
	ModOperatingSystemUtil#append_argument(Output,"output_interpolated",num2str(opt.output_interpolated))
	ModOperatingSystemUtil#append_argument(Output,"smart_interpolation",num2str(opt.smart_interpolation))
	ModOperatingSystemUtil#append_argument(Output,"n_iters",num2str(opt.n_iters))
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

Static Function inverse_boltzmann(user_options,output)
	// Function that calls the IWT python code
	//
	// Args:
	// 		options : instance of the BoltzmannOptions struct. 
	//		output: instance of BoltzmannOutput. Note that the waves 
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
	Make /O/FREE/Wave wave_tmp = {output.extension_bins,output.distribution,output.distribution_deconvolved}
	ModOperatingSystemUtil#get_output_waves(wave_tmp,options.meta,skip_lines=2)
End Function
