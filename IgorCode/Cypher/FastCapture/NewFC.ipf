#pragma rtGlobals=3		// Use modern global access method and strict wave access.

Function JustSetupStream(speed,timespan,wave0,wave1,[Wave0String,Wave1String])
	variable speed, timespan
	string Wave0String,Wave1String
	wave wave0,wave1
	td_ws("Cypher.crosspoint.infastb","Defl") //We will make the deflection measurement on inFastB
	//This sets default behavior which is read Input.FastA and the Zsensor
	if( ParamIsDefault(Wave0String))
		Wave0String="Input.FastB"
	
	endif
	
	if( ParamIsDefault(Wave1String))
		Wave1String="LVDT.Z" //

	endif
	 td_ws("Cypher.input.fastB.gain","0 dB")
	variable totpoints
	Variable error = 0
	if(speed==0) //Running at 500 kHz
		totpoints=timespan*5e5
		td_wv("cypher.input.fastB.filter.freq",2.5e5)
		td_wv("cypher.lvdt.z.filter.freq",5e3)
	elseif(speed==1) //Running at 2 MHz
		totpoints=timespan*2e6
		td_wv("cypher.input.fastB.filter.freq",4e5)
		td_wv("cypher.lvdt.z.filter.freq",5e3)
	else
		print "invalid capture rate"
		error+=-1
		return error
	endif
		
	if(totpoints>=20e7)//Limits us to 20 seconds of 2 MHz data at the moment until we can play with the values
		print "Total points capped"
		error+=-2
		totpoints=20e7
	endif
	
	if(speed==0) //Running at 500 kHz
		Make/O/N=(totpoints) H0, H1	// N can be arbitrary long but beware Igor memory limitations
		duplicate/o H0, wave0
		duplicate/o H1,wave1
		killwaves H0,H1
	else
		Make/O/N=(totpoints) H0, H1	// N can be arbitrary long but beware Igor memory limitations
		duplicate/o H0, wave0
		duplicate/o H1,wave1
		killwaves H0,H1

	endif
	
	
	
	
	error += td_StopStream("Cypher.Stream.0")			// stop the stream	

	error += td_ws("Cypher.Stream.0.Channel.0", Wave0String)		// only Cypher signals
	error += td_ws("Cypher.Stream.0.Channel.1", Wave1String)		// do not use "Cypher" in front of the channel names
	
	if(speed==0) //Running at 500 kHz
		error += td_ws("Cypher.Stream.0.Rate", "500 kHz")
	
	elseif(speed == 1)
		
		error += td_ws("Cypher.Stream.0.Rate", "2 MHz")
	endif
	
	error += td_ws("Cypher.Stream.0.Events", "1")

	error += td_DebugStream("Cypher.Stream.0.Channel.0", Wave0, "")
	error += td_DebugStream("Cypher.Stream.0.Channel.1", Wave1, "") 
	
	error += td_SetupStream("Cypher.Stream.0")
	
	//error+=td_ws("ARC.Events.once", "1") //Here's how you fire~~

	return error
End