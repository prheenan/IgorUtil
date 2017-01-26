// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModOperatingSystemUtil
#include ":ErrorUtil"

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

Static Function os_command_line_execute(execute_string)
	String execute_string
	String Command
	if (!running_windows())
		// Pass to mac scripting system
		sprintf Command,"do shell script \"%s\"",execute_string
	else
		// Pass to windows command prompt
		sprintf Command,"cmd.exe \"%s\"",execute_string
	endif	
	// UNQ: remove leading and trailing double-quote (only for mac)
	ExecuteScriptText /Z Command
	if (V_flag != 0)
		ModErrorUtil#Assert(0,msg="executing " + Command + " failed with return:"+S_Value)
	endif
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

Static Function /S sanitize_mac_path_for_igor(path)
	// Makes an absolute windows-style path igor compatible
	//
	// Args:
	//		path : absolute path to sanitize
	// Returns:
	//		path, with leading "C:/" replaced by /c/ 
	//
	String path
	String igor_path = ModOperatingSystemUtil#to_igor_path(path)
	igor_path = "Macintosh HD:" + igor_path
	// replace possible double colons
	igor_path = ModOperatingSystemUtil#replace_double(":",igor_path)
	return igor_path
End Function

