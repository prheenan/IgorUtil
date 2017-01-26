// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#include "::InverseWeierstrass"
#pragma ModuleName = ModMainIWT


Static Function Main()
	// // This function shows how to use the IWT code
	// Make the input arguments
	KillWaves /A/Z
	Struct InverseWeierstrassOptions opt
	opt.number_of_pairs = 16
	opt.number_of_bins = 80
	opt.fraction_velocity_fit = 0.1
	opt.f_one_half_N = 0e-12
	opt.flip_forces = 0
	String base = "C:/Users/pahe3165/src_prh/"
	opt.path_to_input_file = base + "IgorUtil/PythonApplications/InverseWeierstrass/Example/input.pxp"
	opt.path_to_research_directory = base
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
