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
	//		velocity_m_per_s: if present, this velocity is for the unfolding region(s), and -1 * velocity_m_per_s
	//		is for the refolding region(s). This will over-ride fraction_velocity_fit, but is useful if flickering
	//		is present early in the data.
	//
	//		kbT: energy, in joules, at the current temperature. e.g: 4.1e-21 J
	//
	//		path_to_input_file: where the pxp you want to analyze is. Should have a single wave 
	// 		like <x><d>_Sep and a single wave like <x><d>_Force which are zeroed in separation and
	// 		force and have an integer number of retract/approach pairs. <x> and <y> can be anything,
	//  		<d> should be a 4-digit identifier (e.g. "Image0001_Sep" would be OK)
	Variable number_of_pairs
	Variable number_of_bins
	Variable fraction_velocity_fit
	Variable f_one_half_N
	Variable flip_forces
	Variable velocity_m_per_s
	Variable kbT 
	Struct RuntimeMetaInfo meta
EndStructure

Static Function /S full_path_to_iwt_folder(options)
	// Function that gives the full path to the inverse weierstrass python folder
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure, initialized (see inverse_weierstrass_options function)
	// Returns:
	//		string to full path
	Struct InverseWeierstrassOptions & options
	return options.meta.path_to_research_directory + path_to_iwt_folder
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
	ModOperatingSystemUtil#append_argument(Output,"flip_forces",num2str(opt.flip_forces))
	if (opt.kbT > 0)
		ModOperatingSystemUtil#append_argument(Output,"k_T",num2str(opt.kbT))		
	endif 
	if (opt.fraction_velocity_fit > 0)
		ModOperatingSystemUtil#append_argument(Output,"fraction_velocity_fit",num2str(opt.fraction_velocity_fit))
	endif
	if (opt.velocity_m_per_s > 0) 
		ModOperatingSystemUtil#append_argument(Output,"velocity",num2str(opt.velocity_m_per_s))
	EndIf
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
	// Manually set the main path and output file
	user_options.meta.path_to_main = full_path_to_iwt_main(user_options)
	user_options.meta.path_to_output_file = output_file_name(user_options)
	// make a local copy of user_options, since we have to mess with paths (ugh)
	// and we want to adhere to principle of least astonishment
	Struct InverseWeierstrassOptions options 
	options = user_options
	ModOperatingSystemUtil#get_updated_options(options.meta)
	// Run the python code 
	String PythonCommand = ModInverseWeierstrass#python_command(options)	
	ModOperatingSystemUtil#execute_python(PythonCommand)
	// Get the data into waves
	// Make a wave to hold the output waves in the order we want
	Make /O/FREE/WAVE wave_tmp = {output.molecular_extension_meters,output.energy_landscape_joules,output.tilted_energy_landscape_joules}
	ModOperatingSystemUtil#get_output_waves(wave_tmp,options.meta,skip_lines=2)
End Function
