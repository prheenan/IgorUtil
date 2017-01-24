// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModInverseWeierstrass

Static StrConstant path_to_iwt_folder = "Research/Perkins/Projects/PythonCommandLine/InverseWeierstrass/"
Static StrConstant iwt_file = "main_iwt.py"

Structure InverseWeierstrassOutput
	Wave molecular_extension_meters
	Wave energy_landscape_joules
	Wave tilted_energy_landscape_joules
EndStructure

Structure InverseWeierstrassOptions
	Variable number_of_curves
	Variable fraction_for_velocity
	Variable force_one_half_folded_N
	String path_to_research_directory
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

Static Function inverse_weierstrass_options(options,[number_of_curves,fraction_for_velocity,force_one_half_folded_N,path_to_research_directory])
	// Function that makes an IWT options structure
	//
	// Args:
	//		options: the InverseWeierstrassOptions structure to make
	//		number_of_curves: how many retract/approach pairs (a single 'sawtoooth' is one).
	// 		fraction_for_velocity: amout ([0,1]) of a single retract/approach to fit to determine
	//		the separation velocity (note that this is often different than the Zsnsr velocity)
	//
	//		force_one_half_folded_N: the force (in Newtons) at which half the molecules are folded/unfolded
	//		path_to_research_directory: the absolute path to the parent folder of the Research folder 
	//		(should have Perkins/Projects/... under) Note: do *not* include Research
	// Returns:
	//		Nothing, but sets options appropriately. 
	//
	Struct InverseWeierstrassOptions & options
	Variable number_of_curves,fraction_for_velocity,force_one_half_folded_N
	String path_to_research_directory
	// Set up the variables we need
	force_one_half_folded_N = ParamIsDefault(force_one_half_folded_N) ? 0 : force_one_half_folded_N
	number_of_curves = ParamIsDefault(number_of_curves) ? 1 : number_of_curves
	fraction_for_velocity = ParamIsDefault(fraction_for_velocity) ? 0.1 : fraction_for_velocity
	if (ParamIsDefault(path_to_research_directory))
		path_to_research_directory = "~/src_prh/"
	EndIf
	options.number_of_curves = number_of_curves
	options.fraction_for_velocity = fraction_for_velocity
	options.force_one_half_folded_N = force_one_half_folded_N
End Function

Static Function inverse_weierstrass(separation_wave,force_wave,output,[options])
	// Function that calls the IWT python code
	//
	// Args:
	//		<separation/force>_wave: the separation and forces to be transformed.
	// 		options : instance of the InverseWeierstrassOptions struct. See inverse_weierstrass_options function
	//		output: instance of InverseWeierstrassOutput. Note that the waves should already be allocated
	// Returns:
	//		Nothing, but sets options appropriately. 
	//
	Wave separation_wave
	Wave force_wave
	Struct InverseWeierstrassOutput & output
	Struct InverseWeierstrassOptions & options
	// // determine our options; if none provided, make them.
	Struct InverseWeierstrassOptions to_use
	if (ParamIsDefault(options))
		Struct InverseWeierstrassOptions tmp
		inverse_weierstrass_options(tmp)
		to_use = tmp
	else
		to_use = options
	EndIf
	// POST: have options
	// // Ensure that we can actually call the file
End Function
