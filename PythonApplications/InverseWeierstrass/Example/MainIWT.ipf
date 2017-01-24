// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#include "::InverseWeierstrass"
#pragma ModuleName = ModMainIWT

Static Function Main()
	// // This function shows how to use the IWT code
	// Make the input arguments
	Struct InverseWeierstrassOptions opt
	opt.number_of_curves = 1
	opt.fraction_for_velocity = 0.1
	opt.force_one_half_folded_N = 0
	opt.path_to_research_directory = "~/src_prh/"
	// Make the output waves
	Struct InverseWeierstrassOutput output
	Make /O/N=0, output.molecular_extension_meters
	Make /O/N=0, output.energy_landscape_joules 
	Make /O/N=0, output.tilted_energy_landscape_joules
	String FullPath = ModInverseWeierstrass#full_path_to_iwt_main(opt)
	// Get just the python portion of the command
	String PythonCommand
	sprintf PythonCommand,"python %s",FullPath
	String Command
	sprintf Command,"do shell script \"cd %s ; %s \"",FullPath,PythonCommand
	// UNQ: remove leading and trailing double-quote (only for mac)
	ExecuteScriptText Command
End Function