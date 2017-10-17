#pragma rtGlobals=3	

#pragma ModuleName = ModDataManagement 
#include ":::Util:IoUtil"
#include ":::Util:ErrorUtil"
#include "::asylum_interface"

Static Function /S id_pattern()
	// Returns: the pattern to use to look for the ids 
	String pattern = "\D?(\d+)\D?"	
	return pattern
End Function 

Static Function /S assert_and_return_id(str_input)
	// Asserts that str_input has an id, and returns it as a string
	String str_input
	String tmp 
	Variable match = ModIoUtil#set_and_return_if_match(str_input,id_pattern(),tmp)
	ModErrorUtil#assert(match,msg="Couldn't match " + str_input)	
	return tmp 
End Function 

Static Function /Wave get_force_review_ids()
	// Returns: the list of ids (i.e. contiguous, digits like "0040" in
	// "Image0040") on the current force review graph
	Wave /T old_ids = ModAsylumInterface#force_review_list()
	Variable i
	Variable n_old = DimSize(old_ids,0)
	for (i=0; i < n_old; i += 1)
		old_ids[i] = assert_and_return_id(old_ids[i])
	endfor 	
 	// only return ids of the expected length 
	Duplicate /FREE/T old_ids,to_ret
	Variable n_exp = 4
	for (i=0; i < n_old; i+=1)
		if (strlen(to_ret[i]) != n_exp)
			to_ret[i] = ""
		endif
	endfor 
	// POST: all the ids are more or less OK...
	return to_ret 
End Function	

Static Function delete_extra_waves()
	// Function which deletes extra high resolution waves not
	// in the Force Review graph. useful for data curation 
	String path = "root:prh:fast_indenter:data"
	Variable n = ModIoUtil#CountWaves(path)
	Wave /T old_ids = get_force_review_ids()
	Variable i
	for (i=0; i < n; i+=1)
		// Get the wave number
		String wave_name = ModIoUtil#GetWaveAtIndex(path,i,fullPath=1)
		if (!WaveExists($wave_name))
			// XXX throw error?
			continue
		endif
		String id = assert_and_return_id(wave_name)
		// POST: wave matches
		Variable id_num = str2num(id)
		Variable id_prev = id_num-1
		String id_prev_str
		sprintf id_prev_str,"%04d",id_prev
		Variable low_res_exists = ModIoUtil#string_in_wave(id_prev_str,old_ids)
		if (!low_res_exists)
			KillWaves /Z $(wave_name)
		endif
	endfor
End Function  