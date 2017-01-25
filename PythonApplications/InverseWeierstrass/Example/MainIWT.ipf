// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#include "::InverseWeierstrass"
#pragma ModuleName = ModMainIWT


Static Function Main()
	// // This function shows how to use the IWT code
	// Make the input arguments
	Struct InverseWeierstrassOptions opt
	opt.number_of_pairs = 16
	opt.number_of_bins = 80
	opt.fraction_velocity_fit = 0.1
	opt.f_one_half_N = 15e-12
	opt.flip_forces = 0
	opt.path_to_research_directory = "/Users/patrickheenan/src_prh/"
	opt.path_to_input_file = "/Users/patrickheenan/src_prh/Research/Perkins/Projects/PythonCommandLine/InverseWeierstrass/Example/iwt_input.pxp"
	// Make the output waves
	Struct InverseWeierstrassOutput output
	Make /O/N=0, output.molecular_extension_meters
	Make /O/N=0, output.energy_landscape_joules 
	Make /O/N=0, output.tilted_energy_landscape_joules
	// Execte the command
	ModInverseWeierstrass#inverse_weierstrass(opt,output)
	// Make a fun plot wooo
	Display output.tilted_energy_landscape_joules vs output.molecular_extension_meters
End Function
