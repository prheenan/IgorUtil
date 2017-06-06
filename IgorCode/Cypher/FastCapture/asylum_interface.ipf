// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModAsylumInterface
#include ":NewFC"
#include ":::Util:IoUtil"

Static Function /S force_review_graph_name()
	return "ForceReviewGraph"
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

// XXX should make version of these functions...
//	NoteStr = ReplaceNumberbyKey("VerDate",NoteStr,VersionNumber(),":","\r")
//	NoteStr = ReplaceStringByKey("Version",NoteStr,VersionString(),":","\r")

Static Function save_to_disk_volts(zsnsr_volts_wave,defl_volts_wave,[note_to_use])
	// Saves the given waves (all in volts) to disk (in meters)
	// Args:
	// 	See: save_to_disk
	// Returns: 
	//	nothing
	Wave zsnsr_volts_wave,defl_volts_wave
	String note_to_use
	if (ParamIsDefault(note_to_use))
		note_to_use = Note(defl_volts_wave)
	endif
	// Convert the volts to meters for ZSnsr
	Duplicate /O zsnsr_volts_wave,z_meters
	Fastop z_meters=(GV("ZLVDTSens"))*zsnsr_volts_wave
	SetScale d -10, 10, "m", z_meters
  	// Convert the volts to meters for DeflV
	Duplicate /O defl_volts_wave,defl_meters
	fastop defl_meters=(GV("Invols"))*defl_volts_wave
	SetScale d -10, 10, "m", defl_meters
	// save the zsnsr and defl to the disk
	save_to_disk(z_meters,defl_meters,note_to_use)
	KillWaves /Z defl_meters,z_meters
End Function

Static Function save_to_disk(zsnsr_wave,defl_wave,note_to_use)
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
	Variable save_to_disk = 0x2;
	Duplicate /O zsnsr_wave,raw_wave
	// For ARSaveAsForce... (modelled after DE_SaveFC, 2017-6-5)
	// Args:
	//	1: 0x10 means 'save to disk, not memory'
	//	2: "SaveForce" is the symbolis path name 
	//	3: what we are saving (in addition to ZSnsr, or 'raw', which is required)
	//	4,5 : the actual waves
	//	6-10: empty waves (not saving)
	//	11: CustomNote: the note we are using toe save eveything
	ARSaveAsForce(save_to_disk,"SaveForce","ZSnsr,Defl",raw_wave,zsnsr_wave,defl_wave,$"",$"",$"",$"",CustomNote=note_to_use)
	// Clean up the wave we just made
	KillWaves /Z raw_wave
End Function


Static Function assert_infastb_correct([input_needed,msg_on_fail])
	// asserts that infastb on the cross point panel matches the needed input
	//
	// Args:
	//	input_needed: string (ADC name) we wand connected to infastb. Default: "Defl"
	//	msg_on_fail: what to do if we fail
	// Returns:
	//	Nothing, throws an error if things go wrong
	String input_needed,msg_on_fail
	String in_fast_a = ModAsylumInterface#get_InFastA() 
	// Determine the faillure method based on what we want to connect	
	if (ParamIsDefault(input_needed))
		input_needed = "Defl"
	endif
	if (ParamIsDefault(msg_on_fail))
		msg_on_fail = "InFastB must be connected to " + input_needed + ", not " + in_fast_a
	EndIf		
	Variable correct_input = ModIoUtil#strings_equal(in_fast_a,input_needed)
	ModErrorUtil#assert(correct_input,msg=msg_on_fail)	
End Function

Static Function /S get_InFastA()
	// Returns: 
	//	the name of the ADC connected to InFastA
	ControlInfo /W=$("CrosspointPanel") CypherInFastBPopup
	// S_Value is 'set to text of the current item'
	return S_Value
End Function