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
	String input_file = opt.path_to_input_file
	// Windows is a special flower and needs its paths adjusted
	if (running_windows())
		output_file = sanitize_path_for_windows(output_file)
		input_file = sanitize_path_for_windows(input_file)
	endif
	append_argument(Output,"file_input",input_file)
	append_argument(Output,"file_output",output_file,AddSpace=0)
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
	String Command
	if (!running_windows())
		// Pass to mac scripting system
		sprintf Command,"do shell script \"%s\"",PythonCommand
	else
		// Pass to windows command prompt
		sprintf Command,"cmd.exe \"%s && PAUSE \"",PythonCommand
	endif	
	// UNQ: remove leading and trailing double-quote (only for mac)
	ExecuteScriptText /Z Command
	if (V_flag != 0)
		ModErrorUtil#Assert(0,msg="executing " + Command + " failed with return:"+S_Value)
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

Static Function /S replace_double(needle,haystack)
	// replaces double-instances of a needle in haystaack with a single instance
	//
	// Args:
	//		needle : what we are looking for
	//		haystack: what to search for
	// Returns:
	//		unix_style, compatible with (e.g.) GetFileFolderInfo
	//
	String needle,haystack
	return ReplaceString(needle + needle,haystack,needle)
End Function

Static Function /S to_igor_path(unix_style)
	// convers a unix-style path to an igor-style path
	//
	// Args:
	//		unix_style : absolute path to sanitize
	// Returns:
	//		unix_style, compatible with (e.g.) GetFileFolderInfo
	//
	String unix_style
	String with_colons = ReplaceString("/",unix_style,":")
	// Igor doesnt want a leading colon for an absolute path
	if (strlen(with_colons) > 1 && (cmpstr(with_colons[0],":")== 0))
		with_colons = with_colons[1,strlen(with_colons)]
	endif
	return with_colons
End Function

Static Function /S sanitize_path_for_windows(path)
	// Makes an absolute path windows-compatible.
	//
	// Args:
	//		path : absolute path to sanitize
	// Returns:
	//		path, with leading /c/ or c/ replaced by "C:/"
	//
	String path
	Variable n = strlen(path) 
	if (GrepString(path[0],"^/"))
		path = path[1,n]
	endif
	// POST: no leading /
	return replace_start("c/",path,"C:/")
End Function

Static Function /S replace_start(needle,haystack,replace_with)
	// Replaces a match of a pattern at the start of a string
	//
	// Args:
	//		needle : pattern we are looking for at the start
	//		haystack : the string we are searching in
	//		replace_with: what to replace needle with, if we find it
	// Returns:
	//		<haystack>, with <needle> replaced by <replace_with>, if we find it. 
	String needle,haystack,replace_with
	Variable n_needle = strlen(needle)
	Variable n_haystack = strlen(haystack)
	if ( (GrepString(haystack,"^" + needle)))
		haystack = replace_with + haystack[n_needle,n_haystack]
	endif 
	return haystack
End Function

Static Function /S sanitize_windows_path_for_igor(path)
	// Makes an absolute windows-style path igor compatible
	//
	// Args:
	//		path : absolute path to sanitize
	// Returns:
	//		path, with leading "C:/" replaced by /c/ 
	//
	String path
	return replace_start("C:/",path,"/c/")
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
	options.path_to_input_file = replace_double("/",options.path_to_input_file)
	options.path_to_research_directory = replace_double("/",options.path_to_research_directory)
	if (running_windows())
		options.path_to_input_file = sanitize_windows_path_for_igor(options.path_to_input_file)
		options.path_to_research_directory = sanitize_windows_path_for_igor(options.path_to_research_directory)
	endif
	// // ensure we can actually call the input file (ie: it should exist)
	Variable FileExists = ModIoUtil#FileExists(to_igor_path(options.path_to_input_file))
	String ErrorString = "IWT received non-existing input file: " + options.path_to_input_file
	ModErrorUtil#Assert(FileExists,msg=ErrorString)
	// POST: input file exists
	// // ensure we can actually find the python file
	String python_file = ModInverseWeierstrass#full_path_to_iwt_main(options)
	FileExists = ModIoUtil#FileExists(to_igor_path(python_file))
	ErrorString = "IWT couldnt find python script at: " + python_file
	ModErrorUtil#Assert(FileExists,msg=ErrorString)
	// POST: input and python directories are a thing!
	String output_file = output_file_name(options)
	// Run the python code 
	execute_python(options)
	// Get the data into wavesd starting with <basename>
	String basename = "iwt_tmp"
	// Igor is evil and uses colons, defying decades of convention for paths
	String igor_path = to_igor_path(output_file)
	// Also, requires adding to the absolute path on mac... yay...
	if (!running_windows())
		igor_path = "Macintosh HD:" + igor_path		
	endif
	// replace possible doubles colons
	igor_path = replace_double(":",igor_path)
	// load the wave (the first 2 lines are header)
	Variable FirstLine = 3
	// Q: quiet
	// J: delimited text
	// D: doouble precision
	// K=1: all columns are numeric 
	// /L={x,y,x,x}: skip first y-1 lines
	// /A=<z>: auto name, start with z and work up from zero
	LoadWave/Q/J/D/K=1/L={0,FirstLine,0,0,0}/A=$(basename) igor_path
	// Put it in the output parts
	Duplicate /O $(basename + "0"), output.molecular_extension_meters
	Duplicate /O $(basename + "1"), output.energy_landscape_joules
	Duplicate /O $(basename + "2"), output.tilted_energy_landscape_joules
	// kill all the temporary stuff
	KillWaves /Z $(basename + "0"), $(basename + "1"), $(basename + "2")
End Function
