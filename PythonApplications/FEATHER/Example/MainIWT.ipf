// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#include "::Feather"
#pragma ModuleName = ModMainFEATHER

Static StrConstant DEF_INPUT_REL_TO_BASE =  "IgorUtil/PythonApplications/FEATHER/Example/feather_example.pxp"

Static Function Main_Windows()
	// Runs a simple IWT on patrick's windows setup
	ModMainFEATHER#Main("C:/Users/pahe3165/src_prh/")
End Function 

Static Function Main_Mac()
	// Runs a simple IWT on patrick's mac setup 
	ModMainFEATHER#Main("/Users/patrickheenan/src_prh/")
End Function

Static Function Main(base,[input_file])
	// // This function shows how to use the IWT code
	// Args:
	//		base: the folder where the Research Git repository lives 
	//		input_file: the pxp to load. If not present, defaults to 
	//		<base>DEF_INPUT_REL_TO_BASE
	String base,input_file
	if (ParamIsDefault(input_file))
		input_file  = base +DEF_INPUT_REL_TO_BASE
	EndIf
	KillWaves /A/Z
	// IWT options
	Struct FeatherOptions opt
	opt.tau = 0
	opt.threshold = 1e-2
	opt.path_to_input_file = input_file
	opt.path_to_research_directory = base
	// Make the output waves
	Struct FeatherOutput output
	Make /O/N=0, output.event_starts
	// Execte the command
	ModFeather#feather(opt,output)
	// Make a fun plot wooo
	Edit output.event_starts as "Predicted Event Indices in Wave"
End Function
