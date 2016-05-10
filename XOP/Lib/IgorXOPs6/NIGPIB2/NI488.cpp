/*	NI488.c

	Implements the NI4882 operation.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "NIGPIB.h"

/*	CheckSendWave(wavH, count)

	Used to make sure a wave has enough data before sending data from that wave.
	
	count is the minimum number of bytes that need to be in the wave.
	
	Returns 0 or an error code.
*/
static int
CheckSendWave(waveHndl wavH, BCInt count)
{
	int type;
	BCInt numBytesInWave;
	int numBytesPerValue, dataFormat, isComplex;
	int err;

	if (wavH == NULL)
		return NOWAV;

	type = WaveType(wavH);
	if (type == TEXT_WAVE_TYPE)
		return NO_TEXT_OP;
	
	if (err = NumTypeToNumBytesAndFormat(type, &numBytesPerValue, &dataFormat, &isComplex))
		return err;
	
	numBytesInWave = WavePoints(wavH) * numBytesPerValue;
	if (isComplex)
		numBytesInWave *= 2;
	if (numBytesInWave < count)
		return WAVE_TOO_SHORT;

	return 0;
}

/*	GetAddressListFromWave(wavH, addressList, numItemsInList)
	
	Checks wavH to make sure it is not null and is a scalar, numeric type.

	Transfers addresses from the wave into the list.
	
	Transfers addresses until:
		There are no more points in the wave.
		It hits a NOADDR value (0xFFFF).
		It runs out of room in the list.
	
	On output, addressList will contain the addresses in the wave plus NOADDR. NOADDR is
	added to the end of the addressList if it is missing from the wave. If there is not
	enough room in addressList to store all of the addresses in the wave plus NOADDR,
	it returns ADDRESS_LIST_TOO_LONG.
	
	If it succeeds, it returns 0. In this case, the list is guaranteed to contain
	a NOADDR value which marks the end of the list. Also if successful, it stores
	the number of items in the list, not including the NOADDR item, in *numItemsInListPtr.
*/
static int
GetAddressListFromWave(waveHndl wavH, Addr4882_t addressList[MAX_ADDRESS_LIST_ITEMS], int* numItemsInListPtr)
{
	int type;
	CountInt numPoints;
	IndexInt dataOffset;
	int i, numItemsInList;
	double d[2];
	int err;
	
	*numItemsInListPtr = 0;
	
	if (wavH == NULL)
		return EXPECTED_ADDRESS_LIST_WAVE;
	
	type = WaveType(wavH);
	if (type == TEXT_WAVE_TYPE)
		return EXPECTED_ADDRESS_LIST_WAVE;
	if (type & NT_CMPLX)
		return EXPECTED_ADDRESS_LIST_WAVE;
	
	numPoints = WavePoints(wavH);
	if (err = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset))
		return err;
	
	numItemsInList = 0;
	for(i = 0; i < numPoints; i++) {
		if (i >= MAX_ADDRESS_LIST_ITEMS)
			return ADDRESS_LIST_TOO_LONG;
		if (err = FetchNumericValue(type, (char*)*wavH + dataOffset, i, &d[0]))
			return err;
		addressList[i] = (Addr4882_t)d[0];
		if (addressList[i] == NOADDR)
			break;	
		numItemsInList += 1;
	}
	
	if (numItemsInList >= MAX_ADDRESS_LIST_ITEMS)
		return ADDRESS_LIST_TOO_LONG;
	
	addressList[numItemsInList] = NOADDR;
	*numItemsInListPtr = numItemsInList;
	return 0;
}

/*	StoreShortResultListInWave(resultList[MAX_ADDRESS_LIST_ITEMS], numItemsInList, wavH)
	
	Checks wavH to make sure it is not null and is a scalar, numeric type.
	
	Transfers values from resultList into the wave. If necessary, increases the size
	of the wave. It never shrinks the wave.
	
	Returns 0 if OK or an error code.
*/
static int
StoreShortResultListInWave(short resultList[], int numItemsInList, waveHndl wavH)
{
	int type;
	CountInt numPoints;
	IndexInt dataOffset;
	int i;
	double d[2];
	int err;
	
	if (wavH == NULL)
		return EXPECTED_RESULT_LIST_WAVE;
	
	type = WaveType(wavH);
	if (type == TEXT_WAVE_TYPE)
		return EXPECTED_RESULT_LIST_WAVE;
	if (type & NT_CMPLX)
		return EXPECTED_RESULT_LIST_WAVE;

	numPoints = WavePoints(wavH);
	if (numItemsInList > numPoints) {
		if (err = ChangeWave(wavH, numItemsInList, type))
			return err;	
		numPoints = numItemsInList;
	}	
	
	if (err = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset))
		return err;
	
	for(i = 0; i < numItemsInList; i++) {
		d[0] = resultList[i];
		if (err = StoreNumericValue(type, (char*)*wavH + dataOffset, i, &d[0]))
			break;
	}

	if (i > 0)
		WaveHandleModified(wavH);		// Mark wave modified.
	
	return err;
}

static int
TransferPackedBytesToWave(CountInt count, const char* buffer, waveHndl wavH)
{
	int type;
	CountInt numPointsNeeded;
	void* data;
	int err;
	
	type = WaveType(wavH);

	numPointsNeeded = count;
	if (type & NT_I16)
		numPointsNeeded = (count + 1) / 2;
	if (type & NT_I32)
		numPointsNeeded = (count + 3) / 4;
	if (type & NT_FP32)
		numPointsNeeded = (count + 3) / 4;
	if (type & NT_FP64)
		numPointsNeeded = (count + 7) / 8;
	if (type & NT_CMPLX)
		numPointsNeeded /= 2;

	if (WavePoints(wavH) < numPointsNeeded) {
		if (err = ChangeWave(wavH, numPointsNeeded, type))
			return err;
	}
	data = WaveData(wavH);				// DEREFERENCE
	memcpy(data, buffer, count);
	WaveHandleModified(wavH);			// Mark wave modified.
	
	return 0;
}

static int
DoRcvRespMsg(int boardID, waveHndl wavH, NICountInt count, int termination)
{
	char* buffer;
	int type;
	int err = 0;
	
	if (wavH == NULL)
		return NOWAV;
	
	type = WaveType(wavH);
	if (type == TEXT_WAVE_TYPE)
		return NO_TEXT_OP;
	
	buffer = (char*)NewPtr(count);
	if (buffer == NULL)
		return NOMEM;

	RcvRespMsg(boardID, buffer, count, termination);    

	if ((ibsta & ERR) == 0) {
		count = ibcntl;					// Number of bytes actually received.
		err = TransferPackedBytesToWave(count, buffer, wavH);
	}
	
	DisposePtr(buffer);

	return err;
}

static int
DoReceive(int boardID, int address, waveHndl wavH, NICountInt count, int termination)
{
	char* buffer;
	int type;
	int err = 0;
	
	if (wavH == NULL)
		return NOWAV;
	
	type = WaveType(wavH);
	if (type == TEXT_WAVE_TYPE)
		return NO_TEXT_OP;
	
	buffer = (char*)NewPtr(count);
	if (buffer == NULL)
		return NOMEM;

	Receive(boardID, address, buffer, count, termination);    

	if ((ibsta & ERR) == 0) {
		count = ibcntl;					// Number of bytes actually received.
		err = TransferPackedBytesToWave(count, buffer, wavH);
	}
	
	DisposePtr(buffer);

	return err;
}

// Operation template: NI488/Q ibask={number:ud,number:option}, ibbna={number:ud, string:bname}, ibcac={number:ud, number:v}, ibclr={number:ud}, ibcmd={number:ud, string:cmd, number:cnt}, ibconfig={number:ud, number:option, number:value}, ibdev={number:boardIndex, number:pad, number:sad, number:tmo, number:eot, number:eos}, ibdma={number:ud, number:v}, ibeos={number:ud, number:v}, ibeot={number:ud, number:v}, ibfind={string:udname}, ibgts={number:ud, number:v}, ibist={number:ud, number:v}, iblines={number:ud}, ibln={number:ud, number:pad, number:sad}, ibloc={number:ud}, iblock={number:ud}, ibonl={number:ud, number:v}, ibpad={number:ud, number:v}, ibpct={number:ud}, ibppc={number:ud, number:v}, ibrd={number:ud, number:cnt}, ibrdf={number:ud, string:fileName}, ibrpp={number:ud}, ibrsc={number:ud, number:v}, ibrsp={number:ud}, ibrsv={number:ud, number:v}, ibsad={number:ud, number:v}, ibsic={number:ud}, ibsre={number:ud, number:v}, ibtmo={number:ud, number:v}, ibtrg={number:ud}, ibunlock={number:ud}, ibwait={number:ud, number:mask}, ibwrt={number:ud, string:wrtbuffer, number:cnt}, ibwrtf={number:ud, string:fileName}, AllSpoll={number:boardID, wave:addressListWave, wave:resultListWave}, DevClear={number:boardID, number:address}, DevClearList={number:boardID, wave:addressListWave}, EnableLocal={number:boardID, wave:addressListWave}, EnableRemote={number:boardID, wave:addressListWave}, FindLstn={number:boardID, wave:addressListWave, wave:resultListWave, number:limit}, FindRQS={number:boardID, wave:addressListWave}, PassControl={number:boardID, number:address}, PPoll={number:boardID}, PPollConfig={number:boardID, number:address, number:dataLine, number:lineSense}, PPollUnconfig={number:boardID, wave:addressListWave}, RcvRespMsg={number:boardID, wave:dataWave, number:count, number:termination}, ReadStatusByte={number:boardID, number:address}, Receive={number:boardID, number:address, wave:dataWave, number:count, number:termination}, ReceiveSetup={number:boardID, number:address}, ResetSys={number:boardID, wave:addressListWave}, Send={number:boardID, number:address, wave:dataWave, number:count, number:eotMode}, SendCmds={number:boardID, wave:commandsWave, number:count}, SendDataBytes={number:boardID, wave:dataWave, number:count, number:eotMode}, SendIFC={number:boardID}, SendList={number:boardID, wave:addressListWave, wave:dataWave, number:count, number:eotMode}, SendLLO={number:boardID}, SendSetup={number:boardID, wave:addressListWave}, SetRWLS={number:boardID, wave:addressListWave}, TestSRQ={number:boardID}, TestSys={number:boardID, wave:addressListWave, wave:resultListWave}, Trigger={number:boardID, number:address}, TriggerList={number:boardID, wave:addressListWave}, WaitSRQ={number:boardID}

// Runtime param structure for NI488 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct NI488RuntimeParams {
	// Flag parameters.

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Main parameters.

	// Parameters for ibask keyword group.
	int ibaskEncountered;
	double ibask_ud;
	double ibask_option;
	int ibaskParamsSet[2];

	// Parameters for ibbna keyword group.
	int ibbnaEncountered;
	double ibbna_ud;
	Handle ibbna_bname;
	int ibbnaParamsSet[2];

	// Parameters for ibcac keyword group.
	int ibcacEncountered;
	double ibcac_ud;
	double ibcac_v;
	int ibcacParamsSet[2];

	// Parameters for ibclr keyword group.
	int ibclrEncountered;
	double ibclr_ud;
	int ibclrParamsSet[1];

	// Parameters for ibcmd keyword group.
	int ibcmdEncountered;
	double ibcmd_ud;
	Handle ibcmd_cmd;
	double ibcmd_cnt;
	int ibcmdParamsSet[3];

	// Parameters for ibconfig keyword group.
	int ibconfigEncountered;
	double ibconfig_ud;
	double ibconfig_option;
	double ibconfig_value;
	int ibconfigParamsSet[3];

	// Parameters for ibdev keyword group.
	int ibdevEncountered;
	double ibdev_boardIndex;
	double ibdev_pad;
	double ibdev_sad;
	double ibdev_tmo;
	double ibdev_eot;
	double ibdev_eos;
	int ibdevParamsSet[6];

	// Parameters for ibdma keyword group.
	int ibdmaEncountered;
	double ibdma_ud;
	double ibdma_v;
	int ibdmaParamsSet[2];

	// Parameters for ibeos keyword group.
	int ibeosEncountered;
	double ibeos_ud;
	double ibeos_v;
	int ibeosParamsSet[2];

	// Parameters for ibeot keyword group.
	int ibeotEncountered;
	double ibeot_ud;
	double ibeot_v;
	int ibeotParamsSet[2];

	// Parameters for ibfind keyword group.
	int ibfindEncountered;
	Handle ibfind_udname;
	int ibfindParamsSet[1];

	// Parameters for ibgts keyword group.
	int ibgtsEncountered;
	double ibgts_ud;
	double ibgts_v;
	int ibgtsParamsSet[2];

	// Parameters for ibist keyword group.
	int ibistEncountered;
	double ibist_ud;
	double ibist_v;
	int ibistParamsSet[2];

	// Parameters for iblines keyword group.
	int iblinesEncountered;
	double iblines_ud;
	int iblinesParamsSet[1];

	// Parameters for ibln keyword group.
	int iblnEncountered;
	double ibln_ud;
	double ibln_pad;
	double ibln_sad;
	int iblnParamsSet[3];

	// Parameters for ibloc keyword group.
	int iblocEncountered;
	double ibloc_ud;
	int iblocParamsSet[1];

	// Parameters for iblock keyword group.
	int iblockEncountered;
	double iblock_ud;
	int iblockParamsSet[1];

	// Parameters for ibonl keyword group.
	int ibonlEncountered;
	double ibonl_ud;
	double ibonl_v;
	int ibonlParamsSet[2];

	// Parameters for ibpad keyword group.
	int ibpadEncountered;
	double ibpad_ud;
	double ibpad_v;
	int ibpadParamsSet[2];

	// Parameters for ibpct keyword group.
	int ibpctEncountered;
	double ibpct_ud;
	int ibpctParamsSet[1];

	// Parameters for ibppc keyword group.
	int ibppcEncountered;
	double ibppc_ud;
	double ibppc_v;
	int ibppcParamsSet[2];

	// Parameters for ibrd keyword group.
	int ibrdEncountered;
	double ibrd_ud;
	double ibrd_cnt;
	int ibrdParamsSet[2];

	// Parameters for ibrdf keyword group.
	int ibrdfEncountered;
	double ibrdf_ud;
	Handle ibrdf_fileName;
	int ibrdfParamsSet[2];

	// Parameters for ibrpp keyword group.
	int ibrppEncountered;
	double ibrpp_ud;
	int ibrppParamsSet[1];

	// Parameters for ibrsc keyword group.
	int ibrscEncountered;
	double ibrsc_ud;
	double ibrsc_v;
	int ibrscParamsSet[2];

	// Parameters for ibrsp keyword group.
	int ibrspEncountered;
	double ibrsp_ud;
	int ibrspParamsSet[1];

	// Parameters for ibrsv keyword group.
	int ibrsvEncountered;
	double ibrsv_ud;
	double ibrsv_v;
	int ibrsvParamsSet[2];

	// Parameters for ibsad keyword group.
	int ibsadEncountered;
	double ibsad_ud;
	double ibsad_v;
	int ibsadParamsSet[2];

	// Parameters for ibsic keyword group.
	int ibsicEncountered;
	double ibsic_ud;
	int ibsicParamsSet[1];

	// Parameters for ibsre keyword group.
	int ibsreEncountered;
	double ibsre_ud;
	double ibsre_v;
	int ibsreParamsSet[2];

	// Parameters for ibtmo keyword group.
	int ibtmoEncountered;
	double ibtmo_ud;
	double ibtmo_v;
	int ibtmoParamsSet[2];

	// Parameters for ibtrg keyword group.
	int ibtrgEncountered;
	double ibtrg_ud;
	int ibtrgParamsSet[1];

	// Parameters for ibunlock keyword group.
	int ibunlockEncountered;
	double ibunlock_ud;
	int ibunlockParamsSet[1];

	// Parameters for ibwait keyword group.
	int ibwaitEncountered;
	double ibwait_ud;
	double ibwait_mask;
	int ibwaitParamsSet[2];

	// Parameters for ibwrt keyword group.
	int ibwrtEncountered;
	double ibwrt_ud;
	Handle ibwrt_wrtbuffer;
	double ibwrt_cnt;
	int ibwrtParamsSet[3];

	// Parameters for ibwrtf keyword group.
	int ibwrtfEncountered;
	double ibwrtf_ud;
	Handle ibwrtf_fileName;
	int ibwrtfParamsSet[2];

	// Parameters for AllSpoll keyword group.
	int AllSpollEncountered;
	double AllSpoll_boardID;
	waveHndl AllSpoll_addressListWave;
	waveHndl AllSpoll_resultListWave;
	int AllSpollParamsSet[3];

	// Parameters for DevClear keyword group.
	int DevClearEncountered;
	double DevClear_boardID;
	double DevClear_address;
	int DevClearParamsSet[2];

	// Parameters for DevClearList keyword group.
	int DevClearListEncountered;
	double DevClearList_boardID;
	waveHndl DevClearList_addressListWave;
	int DevClearListParamsSet[2];

	// Parameters for EnableLocal keyword group.
	int EnableLocalEncountered;
	double EnableLocal_boardID;
	waveHndl EnableLocal_addressListWave;
	int EnableLocalParamsSet[2];

	// Parameters for EnableRemote keyword group.
	int EnableRemoteEncountered;
	double EnableRemote_boardID;
	waveHndl EnableRemote_addressListWave;
	int EnableRemoteParamsSet[2];

	// Parameters for FindLstn keyword group.
	int FindLstnEncountered;
	double FindLstn_boardID;
	waveHndl FindLstn_addressListWave;
	waveHndl FindLstn_resultListWave;
	double FindLstn_limit;
	int FindLstnParamsSet[4];

	// Parameters for FindRQS keyword group.
	int FindRQSEncountered;
	double FindRQS_boardID;
	waveHndl FindRQS_addressListWave;
	int FindRQSParamsSet[2];

	// Parameters for PassControl keyword group.
	int PassControlEncountered;
	double PassControl_boardID;
	double PassControl_address;
	int PassControlParamsSet[2];

	// Parameters for PPoll keyword group.
	int PPollEncountered;
	double PPoll_boardID;
	int PPollParamsSet[1];

	// Parameters for PPollConfig keyword group.
	int PPollConfigEncountered;
	double PPollConfig_boardID;
	double PPollConfig_address;
	double PPollConfig_dataLine;
	double PPollConfig_lineSense;
	int PPollConfigParamsSet[4];

	// Parameters for PPollUnconfig keyword group.
	int PPollUnconfigEncountered;
	double PPollUnconfig_boardID;
	waveHndl PPollUnconfig_addressListWave;
	int PPollUnconfigParamsSet[2];

	// Parameters for RcvRespMsg keyword group.
	int RcvRespMsgEncountered;
	double RcvRespMsg_boardID;
	waveHndl RcvRespMsg_dataWave;
	double RcvRespMsg_count;
	double RcvRespMsg_termination;
	int RcvRespMsgParamsSet[4];

	// Parameters for ReadStatusByte keyword group.
	int ReadStatusByteEncountered;
	double ReadStatusByte_boardID;
	double ReadStatusByte_address;
	int ReadStatusByteParamsSet[2];

	// Parameters for Receive keyword group.
	int ReceiveEncountered;
	double Receive_boardID;
	double Receive_address;
	waveHndl Receive_dataWave;
	double Receive_count;
	double Receive_termination;
	int ReceiveParamsSet[5];

	// Parameters for ReceiveSetup keyword group.
	int ReceiveSetupEncountered;
	double ReceiveSetup_boardID;
	double ReceiveSetup_address;
	int ReceiveSetupParamsSet[2];

	// Parameters for ResetSys keyword group.
	int ResetSysEncountered;
	double ResetSys_boardID;
	waveHndl ResetSys_addressListWave;
	int ResetSysParamsSet[2];

	// Parameters for Send keyword group.
	int SendEncountered;
	double Send_boardID;
	double Send_address;
	waveHndl Send_dataWave;
	double Send_count;
	double Send_eotMode;
	int SendParamsSet[5];

	// Parameters for SendCmds keyword group.
	int SendCmdsEncountered;
	double SendCmds_boardID;
	waveHndl SendCmds_commandsWave;
	double SendCmds_count;
	int SendCmdsParamsSet[3];

	// Parameters for SendDataBytes keyword group.
	int SendDataBytesEncountered;
	double SendDataBytes_boardID;
	waveHndl SendDataBytes_dataWave;
	double SendDataBytes_count;
	double SendDataBytes_eotMode;
	int SendDataBytesParamsSet[4];

	// Parameters for SendIFC keyword group.
	int SendIFCEncountered;
	double SendIFC_boardID;
	int SendIFCParamsSet[1];

	// Parameters for SendList keyword group.
	int SendListEncountered;
	double SendList_boardID;
	waveHndl SendList_addressListWave;
	waveHndl SendList_dataWave;
	double SendList_count;
	double SendList_eotMode;
	int SendListParamsSet[5];

	// Parameters for SendLLO keyword group.
	int SendLLOEncountered;
	double SendLLO_boardID;
	int SendLLOParamsSet[1];

	// Parameters for SendSetup keyword group.
	int SendSetupEncountered;
	double SendSetup_boardID;
	waveHndl SendSetup_addressListWave;
	int SendSetupParamsSet[2];

	// Parameters for SetRWLS keyword group.
	int SetRWLSEncountered;
	double SetRWLS_boardID;
	waveHndl SetRWLS_addressListWave;
	int SetRWLSParamsSet[2];

	// Parameters for TestSRQ keyword group.
	int TestSRQEncountered;
	double TestSRQ_boardID;
	int TestSRQParamsSet[1];

	// Parameters for TestSys keyword group.
	int TestSysEncountered;
	double TestSys_boardID;
	waveHndl TestSys_addressListWave;
	waveHndl TestSys_resultListWave;
	int TestSysParamsSet[3];

	// Parameters for Trigger keyword group.
	int TriggerEncountered;
	double Trigger_boardID;
	double Trigger_address;
	int TriggerParamsSet[2];

	// Parameters for TriggerList keyword group.
	int TriggerListEncountered;
	double TriggerList_boardID;
	waveHndl TriggerList_addressListWave;
	int TriggerListParamsSet[2];

	// Parameters for WaitSRQ keyword group.
	int WaitSRQEncountered;
	double WaitSRQ_boardID;
	int WaitSRQParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct NI488RuntimeParams NI488RuntimeParams;
typedef struct NI488RuntimeParams* NI488RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteNI4882(NI488RuntimeParamsPtr p)
{
	Addr4882_t addressList[MAX_ADDRESS_LIST_ITEMS];		// 488.2 list of addresses.
	short shortResultList[MAX_ADDRESS_LIST_ITEMS];		// 488.2 list of results.
	int numItemsInList;
	char buffer[256];
	short sval;
	int ival;
	int quiet;
	int err;
	
	err = 0;
	
	quiet = 0;
	if (p->QFlagEncountered)
		quiet = 1;

	if (p->ibaskEncountered) {
		ibask((int)p->ibask_ud, (int)p->ibask_option, &ival);
		SetV_Flag(ival);
	}

	if (p->ibbnaEncountered) {
		if (err = GetCStringFromHandle(p->ibbna_bname, buffer, sizeof(buffer)-1))
			goto done;
		ibbna((int)p->ibbna_ud, buffer);
	}

	if (p->ibcacEncountered)
		ibcac((int)p->ibcac_ud, (int)p->ibcac_v);

	if (p->ibclrEncountered)
		ibclr((int)p->ibclr_ud);

	if (p->ibcmdEncountered) {
		if (p->ibcmd_cmd == NULL) {
			err = USING_NULL_STRVAR;
			goto done;
		}
		else {
			if (p->ibcmd_cnt <= 0)
				p->ibcmd_cnt = (double)GetHandleSize(p->ibcmd_cmd);
			ibcmd((int)p->ibcmd_ud, *p->ibcmd_cmd, (NICountInt)p->ibcmd_cnt);
		}
	}

	if (p->ibconfigEncountered)
		ibconfig((int)p->ibconfig_ud, (int)p->ibconfig_option, (int)p->ibconfig_value);

	if (p->ibdevEncountered) {
		ival = ibdev((int)p->ibdev_boardIndex,(int)p->ibdev_pad,(int)p->ibdev_sad,(int)p->ibdev_tmo,(int)p->ibdev_eot,(int)p->ibdev_eos);
		SetV_Flag(ival);
	}

	if (p->ibdmaEncountered)
		ibdma((int)p->ibdma_ud, (int)p->ibdma_v);

	if (p->ibeosEncountered)
		ibeos((int)p->ibeos_ud, (int)p->ibeos_v);

	if (p->ibeotEncountered)
		ibeot((int)p->ibeot_ud, (int)p->ibeot_v);

	if (p->ibfindEncountered) {
		if (err = GetCStringFromHandle(p->ibfind_udname, buffer, sizeof(buffer)-1))
			goto done;
		ival = ibfind(buffer);
		SetV_Flag(ival);
	}

	if (p->ibgtsEncountered)
		ibgts((int)p->ibgts_ud, (int)p->ibgts_v);

	if (p->ibistEncountered)
		ibist((int)p->ibist_ud, (int)p->ibist_v);

	if (p->iblinesEncountered) {
		#ifdef MACIGOR
			short gpib_lines;
		#else
			SHORT gpib_lines;
		#endif
		iblines((int)p->iblines_ud, &gpib_lines);
		SetV_Flag(gpib_lines);
	}

	if (p->iblnEncountered) {
		short listen;
		ibln((int)p->ibln_ud, (int)p->ibln_pad, (int)p->ibln_sad, &listen);
		SetV_Flag(listen);
	}

	if (p->iblocEncountered)
		ibloc((int)p->ibloc_ud);

	if (p->iblockEncountered)
		iblock((int)p->iblock_ud);

	if (p->ibonlEncountered)
		ibonl((int)p->ibonl_ud, (int)p->ibonl_v);

	if (p->ibpadEncountered)
		ibpad((int)p->ibpad_ud, (int)p->ibpad_v);

	if (p->ibpctEncountered)
		ibpct((int)p->ibpct_ud);

	if (p->ibppcEncountered)
		ibppc((int)p->ibppc_ud, (int)p->ibppc_v);

	if (p->ibrdEncountered) {
		BCInt tmp;
		Ptr buffer;
		BCInt bufSize;
		
		bufSize = (BCInt)p->ibrd_cnt + 1;
		buffer = (char*)NewPtr(bufSize);
		if (buffer == NULL) {
			err = NOMEM;
			goto done;
		}
		else {
			ibrd((int)p->ibrd_ud, buffer, (NICountInt)p->ibrd_cnt);

			tmp = ibcntl;									// Presumably the number of bytes read.

			// This is not necessary if ibcntl is correct but let's be safe.
			if (tmp < 0)
				tmp = 0;
			if (tmp > (NICountInt)p->ibrd_cnt)
				tmp = (NICountInt)p->ibrd_cnt;

			buffer[tmp] = 0;
			SetS_Value(buffer);
			
			DisposePtr(buffer);	// HR, 080220, 1.02.
		}
	}

	if (p->ibrdfEncountered) {
		Handle h;
		
		h = p->ibrdf_fileName;					// Handle contains file name.
		if (h == NULL) {
			err = USING_NULL_STRVAR;
			goto done;
		}
		else {
			if (err = NullTerminateHandle(h))
				goto done;
			ibrdf((int)p->ibrdf_ud, *h);
		}
	}

	if (p->ibrppEncountered) {
		char ppr;								// Note that this is signed. This is how NI defines it.
		ibrpp((int)p->ibrpp_ud,&ppr);			// Therefore, if the high bit is set, it will come out
		SetV_Flag(ppr);							// as a negative number through sign extension.
	}

	if (p->ibrscEncountered)
		ibrsc((int)p->ibrsc_ud, (int)p->ibrsc_v);

	if (p->ibrspEncountered) {
		char spr;								// Note that this is signed. This is how NI defines it.
		ibrsp((int)p->ibrsp_ud,&spr);				// Therefore, if the high bit is set, it will come out
		SetV_Flag(spr);							// as a negative number through sign extension.
	}

	if (p->ibrsvEncountered)
		ibrsv((int)p->ibrsv_ud, (int)p->ibrsv_v);

	if (p->ibsadEncountered)
		ibsad((int)p->ibsad_ud, (int)p->ibsad_v);

	if (p->ibsicEncountered)
		ibsic((int)p->ibsic_ud);

	if (p->ibsreEncountered)
		ibsre((int)p->ibsre_ud, (int)p->ibsre_v);

	if (p->ibtmoEncountered)
		ibtmo((int)p->ibtmo_ud, (int)p->ibtmo_v);

	if (p->ibtrgEncountered)
		ibtrg((int)p->ibtrg_ud);

	if (p->ibunlockEncountered)
		ibunlock((int)p->ibunlock_ud);

	if (p->ibwaitEncountered)
		ibwait((int)p->ibwait_ud, (int)p->ibwait_mask);

	if (p->ibwrtEncountered) {
		Handle h;

		h = p->ibwrt_wrtbuffer;
		if (h == NULL) {
			err = USING_NULL_STRVAR;
			goto done;
		}
		else {
			if (err = NullTerminateHandle(h))
				goto done;
			ibwrt((int)p->ibwrt_ud, *h, (NICountInt)p->ibwrt_cnt);
		}
	}

	if (p->ibwrtfEncountered) {
		Handle h;
		
		h = p->ibwrtf_fileName;					// Handle contains file name.
		if (h == NULL) {
			err = USING_NULL_STRVAR;
			goto done;
		}
		else {
			if (err = NullTerminateHandle(h))
				goto done;
			ibwrtf((int)p->ibwrtf_ud, *h);
		}
	}

	if (p->AllSpollEncountered) {
		if (err = GetAddressListFromWave(p->AllSpoll_addressListWave, addressList, &numItemsInList))
			goto done;
 		AllSpoll((int)p->AllSpoll_boardID, addressList, shortResultList);
		if ((ibsta & ERR) == 0) {					// No error?
			numItemsInList = ibcntl;				// AllSpoll stores number of responses in ibcntl, which winds up in V_ibcnt.
			if (err = StoreShortResultListInWave(shortResultList, numItemsInList, p->AllSpoll_resultListWave))
				goto done;
		}
	}

	if (p->DevClearEncountered)
		DevClear((int)p->DevClear_boardID, (Addr4882_t)p->DevClear_address);

	if (p->DevClearListEncountered) {
		if (err = GetAddressListFromWave(p->DevClearList_addressListWave, addressList, &numItemsInList))
			goto done;
		DevClearList((int)p->DevClearList_boardID, addressList);
	}

	if (p->EnableLocalEncountered) {
		if (err = GetAddressListFromWave(p->EnableLocal_addressListWave, addressList, &numItemsInList))
			goto done;
		 EnableLocal((int)p->EnableLocal_boardID, addressList);
	}

	if (p->EnableRemoteEncountered) {
		if (err = GetAddressListFromWave(p->EnableRemote_addressListWave, addressList, &numItemsInList))
			goto done;
		 EnableRemote((int)p->EnableRemote_boardID, addressList);
	}

	if (p->FindLstnEncountered) {
		if (err = GetAddressListFromWave(p->FindLstn_addressListWave, addressList, &numItemsInList))
			goto done;

		if (p->FindLstn_limit<1 || p->FindLstn_limit>MAX_ADDRESS_LIST_ITEMS) {
			err = BAD_LIMIT_FOR_FIND_LSTN;
			goto done;
		}
		else {
			FindLstn((int)p->FindLstn_boardID, addressList, shortResultList, (int)p->FindLstn_limit);
			if ((ibsta & ERR) == 0) {					// No error?
				numItemsInList = ibcntl;				// FindLstn stores number of devices found in ibcntl, which winds up in V_ibcnt.
				if (err = StoreShortResultListInWave(shortResultList, numItemsInList, p->FindLstn_resultListWave))
					goto done;
			}
		}
	}

	if (p->FindRQSEncountered) {
		if (err = GetAddressListFromWave(p->FindRQS_addressListWave, addressList, &numItemsInList))
			goto done;
		FindRQS((int)p->FindRQS_boardID, addressList, &sval);
		SetV_Flag(sval);
	}

	if (p->PassControlEncountered)
		PassControl((int)p->PassControl_boardID, (Addr4882_t)p->PassControl_address);

	if (p->PPollEncountered) {
		PPoll((int)p->PPoll_boardID, &sval);
		SetV_Flag(sval);
	}

	if (p->PPollConfigEncountered)
		PPollConfig((int)p->PPollConfig_boardID, (Addr4882_t)p->PPollConfig_address, (int)p->PPollConfig_dataLine, (int)p->PPollConfig_lineSense);

	if (p->PPollUnconfigEncountered) {
		if (err = GetAddressListFromWave(p->PPollUnconfig_addressListWave, addressList, &numItemsInList))
			goto done;
		PPollUnconfig((int)p->PPollUnconfig_boardID, addressList);
	}

	if (p->RcvRespMsgEncountered) {
		if (err = DoRcvRespMsg((int)p->RcvRespMsg_boardID, p->RcvRespMsg_dataWave, (NICountInt)p->RcvRespMsg_count, (int)p->RcvRespMsg_termination))
			goto done;
	}

	if (p->ReadStatusByteEncountered) {
		ReadStatusByte((int)p->ReadStatusByte_boardID, (Addr4882_t)p->ReadStatusByte_address, &sval);
		SetV_Flag(sval);
	}

	if (p->ReceiveEncountered) {
		if (err = DoReceive((int)p->Receive_boardID, (int)p->Receive_address, p->Receive_dataWave, (NICountInt)p->Receive_count, (int)p->Receive_termination))
			goto done;
	}

	if (p->ReceiveSetupEncountered)
		ReceiveSetup((int)p->ReceiveSetup_boardID, (Addr4882_t)p->ReceiveSetup_address);

	if (p->ResetSysEncountered) {
		if (err = GetAddressListFromWave(p->ResetSys_addressListWave, addressList, &numItemsInList))
			goto done;
		 ResetSys((int)p->ResetSys_boardID, addressList);
	}

	if (p->SendEncountered) {
		waveHndl wavH;
		int boardID, address, eotMode;
		NICountInt count;
		void* data;
		
		boardID = (int)p->Send_boardID;
		address = (int)p->Send_address;
		wavH = p->Send_dataWave;
		count = (NICountInt)p->Send_count;
		eotMode = (int)p->Send_eotMode;
		
		if (err = CheckSendWave(wavH, count))		// Checks for null wave, text wave, wave too short.
			goto done;

		data = WaveData(wavH);
		Send(boardID, address, data, count, eotMode);
	}

	if (p->SendCmdsEncountered) {
		waveHndl wavH;
		int boardID;
		NICountInt count;
		void* data;
		
		boardID = (int)p->SendCmds_boardID;
		wavH = p->SendCmds_commandsWave;
		count = (NICountInt)p->SendCmds_count;
				
		if (err = CheckSendWave(wavH, count))		// Checks for null wave, text wave, wave too short.
			goto done;

		data = WaveData(wavH);
		SendCmds(boardID, data, count);
	}

	if (p->SendDataBytesEncountered) {
		waveHndl wavH;
		int boardID, eotMode;
		NICountInt count;
		void* data;
		
		boardID = (int)p->SendDataBytes_boardID;
		wavH = p->SendDataBytes_dataWave;
		count = (NICountInt)p->SendDataBytes_count;
		eotMode = (int)p->SendDataBytes_eotMode;
			
		if (err = CheckSendWave(wavH, count))		// Checks for null wave, text wave, wave too short.
			goto done;

		data = WaveData(wavH);
		SendDataBytes(boardID, data, count, eotMode);
	}

	if (p->SendIFCEncountered)
		SendIFC((int)p->SendIFC_boardID);

	if (p->SendListEncountered) {
		waveHndl wavH;
		int boardID, eotMode;
		NICountInt count;
		void* data;
		
		boardID = (int)p->SendList_boardID;
		wavH = p->SendList_dataWave;
		count = (NICountInt)p->SendList_count;
		eotMode = (int)p->SendList_eotMode;
			
		if (err = CheckSendWave(wavH, count))		// Checks for null wave, text wave, wave too short.
			goto done;
		
		if (err = GetAddressListFromWave(p->SendList_addressListWave, addressList, &numItemsInList))
			goto done;

		data = WaveData(wavH);
		SendList(boardID, addressList, data, count, eotMode);
	}

	if (p->SendLLOEncountered)
		SendLLO((int)p->SendLLO_boardID);

	if (p->SendSetupEncountered) {
		if (err = GetAddressListFromWave(p->SendSetup_addressListWave, addressList, &numItemsInList))
			goto done;
		SendSetup((int)p->SendSetup_boardID, addressList);
	}

	if (p->SetRWLSEncountered) {
		if (err = GetAddressListFromWave(p->SetRWLS_addressListWave, addressList, &numItemsInList))
			goto done;
		SetRWLS((int)p->SetRWLS_boardID, addressList);
	}

	if (p->TestSRQEncountered) {
		TestSRQ((int)p->TestSRQ_boardID, &sval);
		SetV_Flag(sval);
	}

	if (p->TestSysEncountered) {
		if (err = GetAddressListFromWave(p->TestSys_addressListWave, addressList, &numItemsInList))
			goto done;
 		TestSys((int)p->TestSys_boardID, addressList, shortResultList);	// TestSys stores number of devices that failed in ibcntl, which winds up in V_ibcnt.
		if ((ibsta & ERR) == 0)	 {				// No error?
			if (err = StoreShortResultListInWave(shortResultList, numItemsInList, p->TestSys_resultListWave))
				goto done;
		}
	}

	if (p->TriggerEncountered)
		Trigger((int)p->Trigger_boardID, (Addr4882_t)p->Trigger_address);

	if (p->TriggerListEncountered) {
		if (err = GetAddressListFromWave(p->TriggerList_addressListWave, addressList, &numItemsInList))
			goto done;
		TriggerList((int)p->TriggerList_boardID, addressList);
	}

	if (p->WaitSRQEncountered) {
		WaitSRQ((int)p->WaitSRQ_boardID, &sval);
		SetV_Flag(sval);
	}

done:

	// These variables are set after each NI488 call.
	SetOperationNumVar("V_ibsta", ibsta);

	SetOperationNumVar("V_iberr", iberr);

	SetOperationNumVar("V_ibcnt", ibcntl);
	
	if (err == 0) {
		if (quiet == 0) {					// Want to post error on NI-488 error ?
			if (ibsta & ERR)
				err = NIErrToNIGPIBErr(iberr);
			
			if (ibsta & TIMO)
				err = EABO_ERR;
		}
	}
	
	return err;
}

int
RegisterNI4882(void)
{
	char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;
	int err;

	// NOTE: If you change this template, you must change the NI488RuntimeParams structure as well.
	// The following line will not work in Visual C++ because it has a line limit of 2048 characters.
	// cmdTemplate = "NI4882/Q ibask={number:ud,number:option}, ibbna={number:ud, string:bname}, ibcac={number:ud, number:v}, ibclr={number:ud}, ibcmd={number:ud, string:cmd, number:cnt}, ibconfig={number:ud, number:option, number:value}, ibdev={number:boardIndex, number:pad, number:sad, number:tmo, number:eot, number:eos}, ibdma={number:ud, number:v}, ibeos={number:ud, number:v}, ibeot={number:ud, number:v}, ibfind={string:udname}, ibgts={number:ud, number:v}, ibist={number:ud, number:v}, iblines={number:ud}, ibln={number:ud, number:pad, number:sad}, ibloc={number:ud}, iblock={number:ud}, ibonl={number:ud, number:v}, ibpad={number:ud, number:v}, ibpct={number:ud}, ibppc={number:ud, number:v}, ibrd={number:ud, number:cnt}, ibrdf={number:ud, string:fileName}, ibrpp={number:ud}, ibrsc={number:ud, number:v}, ibrsp={number:ud}, ibrsv={number:ud, number:v}, ibsad={number:ud, number:v}, ibsic={number:ud}, ibsre={number:ud, number:v}, ibtmo={number:ud, number:v}, ibtrg={number:ud}, ibunlock={number:ud}, ibwait={number:ud, number:mask}, ibwrt={number:ud, string:wrtbuffer, number:cnt}, ibwrtf={number:ud, string:fileName}, AllSpoll={number:boardID, wave:addressListWave, wave:resultListWave}, DevClear={number:boardID, number:address}, DevClearList={number:boardID, wave:addressListWave}, EnableLocal={number:boardID, wave:addressListWave}, EnableRemote={number:boardID, wave:addressListWave}, FindLstn={number:boardID, wave:addressListWave, wave:resultListWave, number:limit}, FindRQS={number:boardID, wave:addressListWave}, PassControl={number:boardID, number:address}, PPoll={number:boardID}, PPollConfig={number:boardID, number:address, number:dataLine, number:lineSense}, PPollUnconfig={number:boardID, wave:addressListWave}, RcvRespMsg={number:boardID, wave:dataWave, number:count, number:termination}, ReadStatusByte={number:boardID, number:address}, Receive={number:boardID, number:address, wave:dataWave, number:count, number:termination}, ReceiveSetup={number:boardID, number:address}, ResetSys={number:boardID, wave:addressListWave}, Send={number:boardID, number:address, wave:dataWave, number:count, number:eotMode}, SendCmds={number:boardID, wave:commandsWave, number:count}, SendDataBytes={number:boardID, wave:dataWave, number:count, number:eotMode}, SendIFC={number:boardID}, SendList={number:boardID, wave:addressListWave, wave:dataWave, number:count, number:eotMode}, SendLLO={number:boardID}, SendSetup={number:boardID, wave:addressListWave}, SetRWLS={number:boardID, wave:addressListWave}, TestSRQ={number:boardID}, TestSys={number:boardID, wave:addressListWave, wave:resultListWave}, Trigger={number:boardID, number:address}, TriggerList={number:boardID, wave:addressListWave}, WaitSRQ={number:boardID}";

	// This is to workaround the Visual C++ limitation.
	cmdTemplate = (char*)NewPtr(4096);
	if (cmdTemplate == NULL)
		return NOMEM;
	strcpy(cmdTemplate, "NI4882/Q ibask={number:ud,number:option}, ibbna={number:ud, string:bname}, ibcac={number:ud, number:v}, ");
	strcat(cmdTemplate, "ibclr={number:ud}, ibcmd={number:ud, string:cmd, number:cnt}, ibconfig={number:ud, number:option, number:value}, ");
	strcat(cmdTemplate, "ibdev={number:boardIndex, number:pad, number:sad, number:tmo, number:eot, number:eos}, ");
	strcat(cmdTemplate, "ibdma={number:ud, number:v}, ibeos={number:ud, number:v}, ibeot={number:ud, number:v}, ibfind={string:udname}, ");
	strcat(cmdTemplate, "ibgts={number:ud, number:v}, ibist={number:ud, number:v}, iblines={number:ud}, ibln={number:ud, number:pad, number:sad}, ");
	strcat(cmdTemplate, "ibloc={number:ud}, iblock={number:ud}, ibonl={number:ud, number:v}, ibpad={number:ud, number:v}, ibpct={number:ud}, ");
	strcat(cmdTemplate, "ibppc={number:ud, number:v}, ibrd={number:ud, number:cnt}, ibrdf={number:ud, string:fileName}, ibrpp={number:ud}, ");
	strcat(cmdTemplate, "ibrsc={number:ud, number:v}, ibrsp={number:ud}, ibrsv={number:ud, number:v}, ibsad={number:ud, number:v}, ibsic={number:ud}, ");
	strcat(cmdTemplate, "ibsre={number:ud, number:v}, ibtmo={number:ud, number:v}, ibtrg={number:ud}, ibunlock={number:ud}, ");
	strcat(cmdTemplate, "ibwait={number:ud, number:mask}, ibwrt={number:ud, string:wrtbuffer, number:cnt}, ibwrtf={number:ud, string:fileName}, ");
	strcat(cmdTemplate, "AllSpoll={number:boardID, wave:addressListWave, wave:resultListWave}, DevClear={number:boardID, number:address}, ");
	strcat(cmdTemplate, "DevClearList={number:boardID, wave:addressListWave}, EnableLocal={number:boardID, wave:addressListWave}, ");
	strcat(cmdTemplate, "EnableRemote={number:boardID, wave:addressListWave}, FindLstn={number:boardID, wave:addressListWave, wave:resultListWave, number:limit}, ");
	strcat(cmdTemplate, "FindRQS={number:boardID, wave:addressListWave}, PassControl={number:boardID, number:address}, PPoll={number:boardID}, ");
	strcat(cmdTemplate, "PPollConfig={number:boardID, number:address, number:dataLine, number:lineSense}, PPollUnconfig={number:boardID, wave:addressListWave}, ");
	strcat(cmdTemplate, "RcvRespMsg={number:boardID, wave:dataWave, number:count, number:termination}, ReadStatusByte={number:boardID, number:address}, ");
	strcat(cmdTemplate, "Receive={number:boardID, number:address, wave:dataWave, number:count, number:termination}, ReceiveSetup={number:boardID, number:address}, ");
	strcat(cmdTemplate, "ResetSys={number:boardID, wave:addressListWave}, Send={number:boardID, number:address, wave:dataWave, number:count, number:eotMode}, ");
	strcat(cmdTemplate, "SendCmds={number:boardID, wave:commandsWave, number:count}, SendDataBytes={number:boardID, wave:dataWave, number:count, number:eotMode}, ");
	strcat(cmdTemplate, "SendIFC={number:boardID}, SendList={number:boardID, wave:addressListWave, wave:dataWave, number:count, number:eotMode}, ");
	strcat(cmdTemplate, "SendLLO={number:boardID}, SendSetup={number:boardID, wave:addressListWave}, SetRWLS={number:boardID, wave:addressListWave}, ");
	strcat(cmdTemplate, "TestSRQ={number:boardID}, TestSys={number:boardID, wave:addressListWave, wave:resultListWave}, Trigger={number:boardID, number:address}, ");
	strcat(cmdTemplate, "TriggerList={number:boardID, wave:addressListWave}, WaitSRQ={number:boardID}");

	runtimeNumVarList = "V_ibsta;V_iberr;V_ibcnt;V_flag";		// HR, 040909, 1.01, added missing V_flag.
	runtimeStrVarList = "S_value;";								// HR, 040909, 1.01, added missing S_value.
	err = RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(NI488RuntimeParams), (void*)ExecuteNI4882, 0);
	
	DisposePtr(cmdTemplate);
	return err;
}
