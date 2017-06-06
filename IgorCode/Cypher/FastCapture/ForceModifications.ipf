// Use modern global access method, strict compilation
#pragma rtGlobals=3	

#pragma ModuleName = ModForceModifications

function DoForceFunc(ctrlName)			//this handles all of the Force buttons
	string ctrlName

	if (GV("DoThermal"))
		DoThermalFunc("ReallyStopButton_1")
	endif
	PV("ElectricTune;LateralTune;",0)
	
	Variable RemIndex = strsearch(ctrlName,"Force",0)-1
	if (RemIndex > 0)
		ctrlName = ctrlName[0,RemIndex]		//strip out the important part of the ctrlName
	endif
	String ErrorStr = ""


	Variable TabNum
	String TabStr, DoIVBias
	TabNum = ARPanelTabNumLookUp("ForcePanel")
	TabStr = "_"+num2str(TabNum)

	strswitch (ctrlName)
	
		case "RTUpdatePrefs":
			SVAR WhatsRunning = $InitOrDefaultString(GetDF("Main")+"WhatsRunning","")
			if (WhichListItem("Force",WhatsRunning,";",0,0) >= 0)
				IR_stopOutWaveBank(1)		//stop the updates from continuing.
				Print "Can not change prefernce mid-force plot, RT update turned off temporarily\rplease change prefernces after this force plot"
				DoWindow/H
				return(0)
			endif
			String OpsList = "Never;Auto;Always;"
			String ThisOps = UiMenu2("Update the Force Graph in Real Time?",OpsList,StringFromList(GV("DoRTForceUpdate"),OpsList,";"))
			if (Strlen(ThisOps))		//something was selected.
				Variable WhichOne = WhichListItem(ThisOps,OpsList,";",0,0)
				PV("DoRTForceUpdate",WhichOne)
			endif
				
			return(0)

		case "Split":

			PV("VelocitySynch",0)
			DoWindow/K MasterPanel
			if (V_flag)
				MakePanel("Master")
			endif
			DoWindow/K ForcePanel
			if (V_flag)
				MakePanel("Force")
			endif
			ForceSetVarFunc("ForceScanRateSetVar"+TabStr,GV("ForceScanRate"),"",":Variables:ForceVariablesWave[%ForceScanRate]")
			return 0

		case "Sync":

			PV("VelocitySynch",1)
			DoWindow/K MasterPanel
			if (V_flag)
				MakePanel("Master")
			endif
			DoWindow/K ForcePanel
			if (V_flag)
				MakePanel("Force")
			endif
			ForceSetVarFunc("ForceScanRateSetVar"+TabStr,GV("ForceScanRate"),"",":Variables:ForceVariablesWave[%ForceScanRate]")
			return 0

		case "Many":					//pull until told to stop
		case "ManyTrigger":
//			if (CheckContinuousTriggeredDwell())
//				DoAlert 0, "We can't do continuous triggered curves with dwell between the force curves at this time."
//				return 1
//			endif
			if (!CanWeGoYet(CtrlName))
				return(0)
			endif
			ARManageRunning("Engage;",0)
			AR_Stop(OKList="FreqFB;DriveFB;Force;PotFB;FMap;PMap;")
			ARManageRunning("Force",1)
			if (GV("ContForce") != 2)		//Not force Map
				ARStatus(1,"Continuous Force Plots Running")
				PV("ContForce",1)			//this means that we will keep pulling
			endif
			PV("ContinuousForceCounter",0)		//reset the counter.
			Switch (GV("MicroscopeID"))
				case cMicroscopeCypher:
				case cMicroscopeInfinity:
					PV("ARCZ",GV("ARCZ") | 0x2)
					Break
					
			endswitch
			HideForceButtons(ctrlName)		//this deals with the buttons
			//stop what is going on now
			ErrorStr += IR_StopInWaveBank(-1)
			ErrorStr += IR_StopOutWaveBank(-1)
			break

		case "Ramp":
		case "Single":				//pull once
		case "SingleTrigger":
			if (!CanWeGoYet(CtrlName))
				return(0)
			elseif (GV("ARDoIVFP") && GV("TriggerChannel"))
				DoIVBias = TheDoIVDAC()
				Struct ARTipHolderParms TipParms
				ARGetTipParms(TipParms)
				if (StringMatch(DoIVBias,"TipHeaterDrive") && !Strlen(TipParms.TipTempXPTName))
					DoAlert 0,"You can not drive the TipHeaterDrive with your tip holder."
					return(0)
				elseif (StringMatch(DoIVBias,"TipBias") && GV("TuneLockin"))
					DoAlert 1,"You can not drive the Tip Bias with the "+GetMicroscopeName()+" lockin yet.\rClick Yes to set the DDS to the ARC."
					Switch (V_Flag)
						case 1:
							TunePanelPopupFunc("TuneLockinPopup_X",1,StringFromList(0,GUS("TuneLockin"),";"))
							break
							
						default:
							return(0)
							break
					endswitch
				endif
			endif
			ARManageRunning("Engage;",0)
			AR_Stop(OKList="FreqFB;DriveFB;Force;PotFB;FMap;PMap;")
			ARManageRunning("Force",1)
			ARStatus(1,"Single Force Plot Running")

			PV("ContForce",0)
			Switch (GV("MicroscopeID"))
				case cMicroscopeCypher:
				case cMicroscopeInfinity:
					PV("ARCZ",GV("ARCZ") | 0x2)
					Break
					
			endswitch
			PV("ContinuousForceCounter",0)		//reset the counter.
			PV("RTForceSkipFactor",1)		//reset so we can retime things.
			HideForceButtons(ctrlName)		//this deals with the buttons
			//stop what is going on now
			ErrorStr += IR_StopInWaveBank(-1)
			ErrorStr += IR_StopOutWaveBank(-1)
			break

		case "StopTrigger":					//stop continuous pulling gracefully
		case "Stop":					//stop continuous pulling gracefully
		
		
			if (ItemsInList(GetRTStackInfo(0),";") <= 1)		//user clicked on button
				if (GV("FMapStatus"))
					Execute/P/Q/Z "DoScanFunc(\"StopScan_4\")"
					return(0)
				endif
			endif
		
			ARManageRunning("Force",0)
			ARStatus(0,"Ready")


			PV("ContForce",0)			//set this to one
			//clear the trigger, pulling will stop after the next one
			ErrorStr += num2str(td_WriteString("Event."+cForceStartEvent,"clear"))+","
			HideForceButtons("Single")		//this deals with the buttons
			ToggleSTM(0)
			return 0					//no need to do more

		case "Reset":					//stop everything now!
		case "ResetTrigger":
			ARManageRunning("Force",0)
			ARStatus(0,"Ready")

			//stop what is going on now
			ErrorStr += IR_StopInWaveBank(-1)
			ErrorStr += IR_StopOutWaveBank(-1)
			ErrorStr += SetLowNoise(0)
			HideForceButtons("Stop")		//reset the buttons
			ErrorStr += num2str(td_WriteString("CTFC.EventEnable","Never"))+","
			ErrorStr += num2str(td_WriteString("OutWave0StatusCallback",""))+","
			ClearEvents()
			ToggleSTM(0)
			return 0					//we are through

//		case "Graph":					//make a graph
//			oldMakeForceGraph()			//this does that
//			return 0

		case "Channel":
			string graphStr = StringFromList(0,WinList("*",";","WIN:64"))	//the panel the button is on should be on top
			MakePanel("ForceChannel")									//make the force channel panel
			AttachPanel("ForceChannelPanel",graphStr)						//stick it to the panel with the just pushed button
			return 0

		case "Save":
			if (!(4 & GV("SaveForce")))
				PV("SaveForce",GV("SaveForce")+4)
			endif
			if (!GV("ContForce"))
				SaveForceFunc()
				ForceRealTimeUpdateOffline()
			endif
			return 0

		case "Go":
			GoToSpot()
			return 0

		case "Clear":
		case "Draw":
		case "Done":
			DrawSpot(ctrlName)
			return 0

		case "Review":
			MakePanel("MasterForce")
			return 0
			
		default:
		
			return 1
			
			
	endswitch

	UpdateInvolsBySum()
	string SavedDataFolder = GetDataFolder(1)
	SetDataFolder root:Packages:MFP3D:Force:		//all of the Force waves live in here
	
	UpdateAllControls("StopEngageButton",cEngageButtonTitle,"SimpleEngageButton","SimpleEngageMe",DropEnd=2)
	
	Variable limitValue = GV("MaxContinuousForce")
	if ((numtype(LimitValue) == 0) && GV("ContForce"))
		ARStatusProgBar(1,"Force Plots",LimitValue,0)
	endif
	SwapZDAC(0)		//CTFC can't run the Cypher Z DAC and out waves can't either
	if (CheckYourZ(0))		//if we are engaged, set the Start distance to the Force distance above where we currently are.
		Struct WMButtonAction ButtonStruct
		ButtonStruct.Win = "MasterPanel"
		ButtonStruct.CtrlName = "ForceStartDistButton"+"_2"
		ButtonStruct.EventMod = 2^3
		ButtonStruct.EventCode = 2
		ButtonStruct.UserData = ""
		ARButtonFunc(ButtonStruct)
	endif
	ir_StopPISLoop(NaN,LoopName="HeightLoop")
	ErrorStr += SetLowNoise(1)
	if (!GV("DontChangeXPT"))
		SVAR CurrentXPT = root:Packages:MFP3D:XPT:Current
		SVAR State = root:Packages:MFP3D:XPT:State
		if (GV("ForceCrosspointChange") || !(stringmatch(CurrentXPT,"*Force") && stringmatch(State,"Loaded")))
			//AdjustXPT("Force","Force")
			LoadXPTState("Force")
			//Wave/T AllAlias = root:Packages:MFP3D:Hardware:AllAlias
			//Wave/T XPTLoad = root:Packages:MFP3D:XPT:XPTLoad
			If (GVU("InputRange") && (GV("ImagingMode") == cACMode) && GrepString(ir_ReadALias("Deflection"),"Fast"))
				SetAutoInputRange(1,freeAirAmp=9)
			endif
		endif
	endif
	Variable Error = DoubleCheckInputRange()
	if (Error)
		ForcePlotCleanup()
		return(0)
	endif

	variable rampInterp = 0
	variable startDist, startDistVolts, zLVDTSens = GV("ZLVDTSens")
	
	startDist = GV("StartDist")
	startDistVolts = startDist/GV("ZPiezoSens")
	
	String Callback = ""
	if (StringMatch(CtrlName,"Ramp*"))
		Callback = "ZRampCallback()"
	else
		Callback = "FinishForceFunc(\""+ctrlName+"\")"
	endif
	
	String RampChannel, TriggerChannel, Event, EventRamp, EventDwell
	Variable TriggerPoint,IsGreater,RampVelocity
	Variable DoTrigger = GV("TriggerChannel")
	Variable ForceDistSign = GV("ForceDistSign")	
	Event = cForceStartEvent
	EventRamp = cForceRampEvent
	EventDwell = cForceDwellEvent
	if (!DoTrigger)
		EventRamp = "Always"
		EventDwell = "Never"
		ir_StopPISLoop(naN,LoopName="DwellLoop")
//		//also clear PISloop1 if it is set to.
//		Wave/T PISLoopWave = root:Packages:MFP3D:Main:PISLoopWave
//		if (StringMatch(PISLoopWave[1][6],"Z%output"))
//			ErrorStr += num2str(ir_stopPISLoop(1))+","
//		endif
		
	endif
		
	Variable IsAbsTrig = GV("TriggerType")
	
	Variable ForceMode = GV("ForceMode")
	
	if (ForceMode)
		startDistVolts = (startDistVolts-70)*GV("ZPiezoSens")/GV("ZLVDTSens")+GV("ZLVDTOffset")
		Struct ARFeedbackStruct FB
		ARGetFeedbackParms(FB,"outputZ")
		FB.StartEvent = EventRamp
		FB.StopEvent = EventDwell
		if (DoTrigger)
			FB.StopEvent += ";"+cFMapStartEvent
		endif
		ErrorStr += ir_WritePIDSloop(FB)
		RampChannel = "$"+FB.LoopName+".Setpoint"//"Setpoint%PISLoop2"
		if (DoTrigger)
			ErrorStr += num2str(td_WriteString("Event."+EventDwell,"Clear"))+","
			ErrorStr += num2str(td_WriteString("Event."+EventRamp,"Set"))+","
		endif
	else
		ir_StopPISLoop(naN,LoopName="outputZLoop")
		ir_StopPISLoop(naN,LoopName="HeightLoop")
		RampChannel = "Output.Z"
	endif
	CheckYourZ(1)

	
	if (ForceMode)
		RampVelocity = 5
	else
		RampVelocity = 50
	endif
	
	if (DoTrigger)
		Struct ARForceChannelStruct ForceChannelParms
		GetForceChannelParms(ForceChannelParms)
		if ((GV("ImagingMode") == cPFMMode) && IsForceChannel(ForceChannelParms,"Frequency") && GV("DualACMode"))
			PV("DFRTOn",1)
			PV("AppendThermalBit",GV("AppendThermalBit") | 2^GetThermalBit("AppendDFRTFreqBox"))
		endif

		Struct ARCTFCParms CTFCParms
		ARGetCTFCParms(CTFCParms)

		CTFCParms.RampDistance[0] = StartDistVolts-td_ReadValue(CTFCParms.RampChannel)
		CTFCParms.RampDistance[1] = 0
	
		CTFCParms.RampRate[0] = RampVelocity*Sign(CTFCParms.RampDistance[0])
		CTFCParms.RampRate[1] = 0
		CTFCParms.DwellTime[0] = 0
		CTFCParms.DwellTime[1] = 0
		CTFCParms.TriggerChannel[1] = "output.Dummy"
		CTFCParms.Callback = Callback
		CTFCParms.RampEvent = EventRamp
		CTFCParms.StartEvent = Event
		CTFCParms.DwellEvent = cForceInitDwellEvent
		if (((CTFCParms.RampDistance[0] < 0) && (ForceDistSign > 0)) || ((CTFCParms.RampDistance[0] > 0) && (ForceDistSign < 0)))
			CTFCParms.TriggerChannel[0] = "output.Dummy"
		endif
		ErrorStr += ir_WriteCTFC(CTFCParms)
		ErrorStr += num2str(TD_WriteString("Event."+event,"once"))+","
	else	
		
		ErrorStr += num2str(td_SetRamp(4,RampChannel,RampVelocity,startDistVolts,"",0,0,"",0,0,Callback))+","
	endif
	PV("ZStateChanged",0)
	
	ARReportError(ErrorStr)
	SetDataFolder(SavedDataFolder)
End Function //DoForceFunc
