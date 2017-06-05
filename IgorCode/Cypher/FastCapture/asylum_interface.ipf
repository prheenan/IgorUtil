// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModAsylumInterface
#include ":NewFC"
#include ":::Util:IoUtil"

Static Function /S force_review_graph_name()
	return "Force Review Graph"
End Function

Static Function /WAVE master_variable_info()
	// Returns: the master panel variable wave
	return root:packages:MFP3D:Main:Variables:MasterVariablesWave
End Function

Static Function /S master_base_name()
	// Returns: the master panel base name
	SVAR str_v = root:packages:MFP3D:Main:Variables:BaseName
	// XXX check SVAR exists.
	return str_v
End Function

Static Function current_image_suffix()
	// Returns: the current image id or suffix (e.g. 1 for 0001)
	return master_variable_info()[%BaseSuffix][%Value]
End Function

Static Function /S formatted_wave_name(base,suffix,[type])
	// Formats a wave name as the cyper (e.g.  Image_0101Deflv)
	//
	// Args:
	//	base: name of the wave (e.g. 'Image')
	//	suffix: id of the wave (e.g. 1)
	//     type: optional type after the suffix (e.g. 'force')
	// Returns:
	//	Wave formatted as an asylum name would be, given the inputs
	String base,type
	Variable suffix
	String to_ret
	if (ParamIsDefault(type))
		type = ""
	EndIf
	// Formatted like <BaseName>_<justified number><type>
	// e.g. Image_0101Deflv
	sprintf to_ret,"%s_%04d%s",base,suffix,type
	return to_ret
End Function

Static Function /S default_wave_base_name()
	// Returns: the default wave, according to the (global / cypher) Suffix and base
	Variable suffix =current_image_suffix()
	String base = master_base_name()
	return  formatted_wave_name(base,suffix)
End Function 

Static Function save_to_disk(zsnsr_wave,defl_wave,[note_to_use])
	// Saves the given ZSnsr and deflection to disk
	//
	// Args:
	//	zsnsr_wave: Assumed ends with "ZSnsr", the Z Sensor in meters
	//	defl_wave: the deflection in meters
	// 	note_to_use: the (optional) note to save with. defaults to just defl_wave
	// Returns
	//	Nothing
	Wave zsnsr_wave,defl_wave
	String note_to_use
	if (ParamIsDefault(note_to_use))
		note_to_use = Note(defl_wave)
	endif
	Variable save_to_disk = 0x2;
	String zsnsr_wave_name = ModIoUtil#GetPathToWave(zsnsr_wave)
	String raw_wave_name = ReplaceString("ZSnsr",zsnsr_wave_name,"Raw",0)
	Duplicate /O zsnsr_wave,$(raw_wave_name)
	Wave wave_raw = $(raw_wave_name)
	// For ARSaveAsForce... (modelled after DE_SaveFC, 2017-6-5)
	// Args:
	//	1: 0x10 means 'save to disk, not memory'
	//	2: "SaveForce" is the symbolis path name 
	//	3: what we are saving (in addition to ZSnsr, or 'raw', which is required)
	//	4,5 : the actual waves
	//	6-10: empty waves (not saving)
	//	11: CustomNote: the note we are using toe save eveything
	ARSaveAsForce(save_to_disk,"SaveForce","ZSnsr,Defl",wave_raw,zsnsr_wave,defl_wave,$"",$"",$"",$"",CustomNote=note_to_use)
End Function