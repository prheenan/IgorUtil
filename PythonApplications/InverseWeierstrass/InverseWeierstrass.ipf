// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModInverseWeierstrass
#include ":::IgorCode:Util:IoUtil"
#include ":::IgorCode:Util:PlotUtil"

Static StrConstant path_to_iwt_folder = "Research/Perkins/Projects/PythonCommandLine/InverseWeierstrass/"
Static StrConstant iwt_file = "main_iwt.py"

Structure InverseWeierstrassOutput
	Wave molecular_extension_meters
	Wave energy_landscape_joules
	Wave tilted_energy_landscape_joules
EndStructure

Structure InverseWeierstrassOptions
	Variable number_of_pairs
	Variable number_of_bins
	Variable fraction_velocity_fit
	Variable f_one_half_N
	Variable flip_forces
	String path_to_research_directory
EndStructure

Static Function /S append_argument(Base,Name,Value,[AddSpace])
	// Function that appends "-<Name> <Value>" to Base, possible adding a space to the end
	//
	// Args:
	//		Base: what to add to
	//		Name: argument name
	//		Value: value of the argument
	//		AddSpace: optional, add a space. Defaults to retur
	// Returns:
	//		appended Base
	String & Base
	String Name,Value
	Variable AddSpace
	String Output
	AddSpace = ParamIsDefault(AddSpace) ? 1 : AddSpace
	sprintf Output,"-%s %s",Name,Value
	Base = Base + Output
	if (AddSpace)
		Base = Base + " "
	EndIf
End Function

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

Function running_windows() 
	// Flag for is running windows
	//
	// Returns:
	//		1 if windows, 0 if mac
	String platform = UpperStr(IgorInfo(2))
	Variable pos = strsearch(platform,"WINDOWS",0)
	return pos >= 0
End

Static Function /S python_binary_string()
	// Returns string for running python given this OS
	//
	// Returns:
	//		1 if windows, 0 if mac
	if (running_windows())
		return "python"
	else
		return "//anaconda/bin/python"
	endif
End Function

Static Function /S input_file_name(options)
	//
	// Returns:
	//		the input file to use
	Struct InverseWeierstrassOptions & options
	String FolderPath = ModInverseWeierstrass#full_path_to_iwt_folder(options)
	return Folderpath + "iwt_input.pxp"
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
	Struct InverseWeierstrassOptions & opt
	// XXX check if windows
	String PythonCommand
	String python_str  = python_binary_string()
	String FolderPath = ModInverseWeierstrass#full_path_to_iwt_folder(opt)
	String FullPath = ModInverseWeierstrass#full_path_to_iwt_main(opt)
	// Get just the python portion of the command
	String Output
	sprintf Output,"%s %s ",python_str,FullPath
	append_argument(Output,"number_of_pairs",num2str(opt.number_of_pairs))
	append_argument(Output,"number_of_bins",num2str(opt.number_of_bins))
	append_argument(Output,"f_one_half",num2str(opt.f_one_half_N))
	append_argument(Output,"fraction_velocity_fit",num2str(opt.fraction_velocity_fit))
	append_argument(Output,"flip_forces",num2str(opt.flip_forces))
	String output_file = output_file_name(opt)
	String input_file = input_file_name(opt)
	append_argument(Output,"file_input",input_file)
	append_argument(Output,"file_output",output_file,AddSpace=0)
	return Output
End Function

Static Function execute_python(options)
	Struct InverseWeierstrassOptions & options
	String PythonCommand = ModInverseWeierstrass#python_command(options)
	String Command
	// XXX make windows-friendly...
	sprintf Command,"do shell script \"%s\"",PythonCommand
	print(Command)
	// UNQ: remove leading and trailing double-quote (only for mac)
	ExecuteScriptText /Z Command
	if (V_flag != 0)
		print(S_value)
	endif
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


Static Function inverse_weierstrass(separation_wave,force_wave,output,options)
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
	// // XXX Ensure that we can actually call the file
	String input_name = input_file_name(options)
	String output_name = output_file_name(options)
	// Make a graph to easily save the data out. 
	String for_pxp = ModIoUtil#UniqueGraphName("prh_iwt_tmp")
	ModPlotUtil#Figure(name=for_pxp,hide=1)
	ModPlotUtil#Plot(force_wave,mX=separation_wave)
	SaveGraphCopy /P=$"/"/O/W=$(for_pxp) as output_name
	// no longer need the graph
	KillWindow $for_pxp
	// Run the python code 
	execute_python(options)
	// Get the data
	ModIoUtil#LoadFile(input_name)
End Function
