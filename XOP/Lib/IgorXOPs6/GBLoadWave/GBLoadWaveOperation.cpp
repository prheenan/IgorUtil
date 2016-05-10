#include "XOPStandardHeaders.h"		// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "GBLoadWave.h"

// Operation template: GBLoadWave /A[=name:ABaseName] /B[=number:lowByteFirst] /D[=number:doublePrecision] /F=number:dataFormat /I[={string:macFilterStr,string:winFilterStr}] /J=number:floatFormat /L=number:dataLengthInBits /N[=name:NBaseName] /O[=number:overwrite] /P=name:pathName /Q[=number:quiet] /S=number:skipBytes /T={number:fileDataType,number:waveDataType} /U=number:pointsPerArray /V[=number:interleaved] /W=number:numberOfArraysInFile /Y={number:offset,number:multiplier} [string:fileParamStr]

// Runtime param structure for GBLoadWave operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct GBLoadWaveRuntimeParams {
	// Flag parameters.

	// Parameters for /A flag group.
	int AFlagEncountered;
	char ABaseName[MAX_OBJ_NAME+1];			// Optional parameter.
	int AFlagParamsSet[1];

	// Parameters for /B flag group.
	int BFlagEncountered;
	double lowByteFirst;					// Optional parameter.
	int BFlagParamsSet[1];

	// Parameters for /D flag group.
	int DFlagEncountered;
	double doublePrecision;					// Optional parameter.
	int DFlagParamsSet[1];

	// Parameters for /F flag group.
	int FFlagEncountered;
	double dataFormat;
	int FFlagParamsSet[1];

	// Parameters for /I flag group.
	int IFlagEncountered;
	Handle macFilterStr;					// Optional parameter.
	Handle winFilterStr;					// Optional parameter.
	int IFlagParamsSet[2];

	// Parameters for /J flag group.
	int JFlagEncountered;
	double floatFormat;
	int JFlagParamsSet[1];

	// Parameters for /L flag group.
	int LFlagEncountered;
	double dataLengthInBits;
	int LFlagParamsSet[1];

	// Parameters for /N flag group.
	int NFlagEncountered;
	char NBaseName[MAX_OBJ_NAME+1];			// Optional parameter.
	int NFlagParamsSet[1];

	// Parameters for /O flag group.
	int OFlagEncountered;
	double overwrite;						// Optional parameter.
	int OFlagParamsSet[1];

	// Parameters for /P flag group.
	int PFlagEncountered;
	char pathName[MAX_OBJ_NAME+1];
	int PFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	double quiet;							// Optional parameter.
	int QFlagParamsSet[1];

	// Parameters for /S flag group.
	int SFlagEncountered;
	double skipBytes;
	int SFlagParamsSet[1];

	// Parameters for /T flag group.
	int TFlagEncountered;
	double fileDataType;
	double waveDataType;
	int TFlagParamsSet[2];

	// Parameters for /U flag group.
	int UFlagEncountered;
	double pointsPerArray;
	int UFlagParamsSet[1];

	// Parameters for /V flag group.
	int VFlagEncountered;
	double interleaved;						// Optional parameter.
	int VFlagParamsSet[1];

	// Parameters for /W flag group.
	int WFlagEncountered;
	double numberOfArraysInFile;
	int WFlagParamsSet[1];

	// Parameters for /Y flag group.
	int YFlagEncountered;
	double offset;
	double multiplier;
	int YFlagParamsSet[2];

	// Main parameters.

	// Parameters for simple main group #0.
	int fileParamStrEncountered;
	Handle fileParamStr;					// Optional parameter.
	int fileParamStrParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct GBLoadWaveRuntimeParams GBLoadWaveRuntimeParams;
typedef struct GBLoadWaveRuntimeParams* GBLoadWaveRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

static int
InitSettings(LoadSettingsPtr lsp)
{
	MemClear(lsp, sizeof(LoadSettings));

	lsp->flags = 0;
	lsp->inputDataType = NT_FP32;
	lsp->inputDataFormat = IEEE_FLOAT;
	lsp->inputBytesPerPoint = 4;
	lsp->floatingPointFormat = 1;				// 1 = IEEE; 2 = VAX.
	lsp->lowByteFirst = 0;
	
	lsp->outputDataType = NT_FP32;
	lsp->outputDataFormat = IEEE_FLOAT;
	lsp->outputBytesPerPoint = 4;
	lsp->offset = 0.0;							// output data = (input data + offset) * multiplier.
	lsp->multiplier = 1.0;
	
	lsp->preambleBytes = 0;
	lsp->numArrays = 0;
	lsp->arrayPoints = 0;
	lsp->interleaved = 0;
	
	return 0;
}

extern "C" int
ExecuteGBLoadWave(GBLoadWaveRuntimeParamsPtr p)
{
	LoadSettings ls;
	char baseName[MAX_OBJ_NAME+1];
	char symbolicPathName[MAX_OBJ_NAME+1];
	char fileParam[MAX_PATH_LEN+1];
	int err = 0;
	
	InitSettings(&ls);							// Set default values.
	strcpy(baseName, "wave");
	*symbolicPathName = 0;
	*fileParam = 0;

	// Check parameters here.
	
	if (p->AFlagEncountered) {					// /A[=baseName]
		ls.flags |= AUTO_NAME;
		if (p->AFlagParamsSet[0]) {
			if (*p->ABaseName)
				strcpy(baseName, p->ABaseName);
		}
	}
	
	if (p->BFlagEncountered) {					// /B[=lowOrderByteFirst]
		ls.lowByteFirst = 1;
		if (p->BFlagParamsSet[0])
			ls.lowByteFirst = (int)p->lowByteFirst;
	}
	
	if (p->DFlagEncountered) {					// /D[=doublePrecision]
		ls.flags |= DOUBLE_PRECISION;
		if (p->DFlagParamsSet[0]) {
			if (p->doublePrecision == 0)
				ls.flags &= ~DOUBLE_PRECISION;
		}
		if (ls.flags & DOUBLE_PRECISION) {
			ls.outputDataType = NT_FP64;
			ls.outputBytesPerPoint = 8;
		}
		else {
			ls.outputDataType = NT_FP32;
			ls.outputBytesPerPoint = 4;
		}
		ls.outputDataFormat = IEEE_FLOAT;
	}
	
	if (p->FFlagEncountered) {					// /F=f sets number input data format.
		ls.inputDataFormat = (int)p->dataFormat;
		if (err = NumBytesAndFormatToNumType(ls.inputBytesPerPoint, ls.inputDataFormat, &ls.inputDataType))
			return err;
	}
	
	if (p->IFlagEncountered) {					// /I[=typeStr] or /I={macFilterStr, winFilterStr}
		ls.flags |= INTERACTIVE;
		
		if (p->IFlagParamsSet[0]) {
			if (p->macFilterStr == NULL) {
				return USING_NULL_STRVAR;
			}
			else {
				#ifdef MACIGOR
					if (err = GetCStringFromHandle(p->macFilterStr, ls.filterStr, sizeof(ls.filterStr)-1))
						return err;
				#endif
			}
		}
		
		if (p->IFlagParamsSet[1]) {
			if (p->winFilterStr == NULL) {
				return USING_NULL_STRVAR;
			}
			else {
				#ifdef WINIGOR
					char* t;

					if (err = GetCStringFromHandle(p->winFilterStr, ls.filterStr, sizeof(ls.filterStr)-1))
						return err;
						
					// To make this string suitable for XOPOpenFileDialog, we need to replace tabs with nulls.
					t = ls.filterStr;
					while(*t != 0) {
						if (*t == '\t')
							*t = 0;
						t += 1;						
					}
				#endif
			}
		}
	}
	
	if (p->JFlagEncountered) {					// /J=floatingPointFormat
		ls.floatingPointFormat = (int)p->floatFormat;
		if (ls.floatingPointFormat<1 || ls.floatingPointFormat>2)
			return BAD_FP_FORMAT_CODE;
	}

	if (p->LFlagEncountered) {					// /L=dataLengthInBits
		switch ((int)p->dataLengthInBits) {
			case 8:
			case 16:
			case 32:
			case 64:
				ls.inputBytesPerPoint = (int)(p->dataLengthInBits/8);
				if (err = NumBytesAndFormatToNumType(ls.inputBytesPerPoint, ls.inputDataFormat, &ls.inputDataType))
					return err;
				break;
			default:
				return BAD_DATA_LENGTH;
				break;
		}
	}

	if (p->NFlagEncountered) {					// /N[=baseName]
		ls.flags |= AUTO_NAME | OVERWRITE;
		if (p->NFlagParamsSet[0]) {
			if (*p->NBaseName)
				strcpy(baseName, p->NBaseName);
		}
	}

	if (p->OFlagEncountered) {					// /O[=overwrite]
		ls.flags |= OVERWRITE;
		if (p->OFlagParamsSet[0]) {
			if (p->overwrite == 0)
				ls.flags &= ~OVERWRITE;
		}
	}

	// Parameters for /P flag group.			// /P=pathName
	if (p->PFlagEncountered) {
		if (*p->pathName != 0) {				// Treat /P=$"" as a NOP.
			strcpy(symbolicPathName, p->pathName);
			ls.flags |= PATH;
		}
	}

	if (p->QFlagEncountered) {					// /Q[=quiet]
		ls.flags |= QUIET;
		if (p->QFlagParamsSet[0]) {
			if (p->quiet == 0)
				ls.flags &= ~QUIET;
		}
	}

	if (p->SFlagEncountered)					// /S=skipBytes
		ls.preambleBytes = (CountInt)p->skipBytes;

	if (p->TFlagEncountered) {					// /T={inputDataType, outputDataType}
		int isComplex;

		ls.inputDataType = (int)p->fileDataType;
		if (err = NumTypeToNumBytesAndFormat(ls.inputDataType, &ls.inputBytesPerPoint, &ls.inputDataFormat, &isComplex))
			return err;
		if (isComplex)
			return BAD_DATA_TYPE;				// Complex is not supported.
		
		ls.outputDataType = (int)p->waveDataType;
		if (err = NumTypeToNumBytesAndFormat(ls.outputDataType, &ls.outputBytesPerPoint, &ls.outputDataFormat, &isComplex))
			return err;
		if (isComplex)
			return BAD_DATA_TYPE;				// Complex is not supported.
	}

	if (p->UFlagEncountered)					// /U=bytesPerWave
		ls.arrayPoints = (CountInt)p->pointsPerArray;

	if (p->VFlagEncountered) {					// /V[=interleave]
		ls.interleaved = 1;
		if (p->VFlagParamsSet[0]) {
			if (p->interleaved == 0)
				ls.interleaved = 0;
		}
	}
	
	if (p->WFlagEncountered) {					// /W=numWavesToLoad
		ls.numArrays = (int)p->numberOfArraysInFile;
		if (ls.numArrays < 1)
			return BAD_NUM_WAVES;
	}
	
	if (p->YFlagEncountered) {					// /Y={offset, multiplier}
		ls.offset = p->offset;
		ls.multiplier = p->multiplier;
	}

	if (p->fileParamStrEncountered) {
		if (p->fileParamStr == NULL)
			return USING_NULL_STRVAR;
		if (err = GetCStringFromHandle(p->fileParamStr, fileParam, sizeof(fileParam)-1))
			return err;
	}

	// Do the operation's work here.
	
	err = LoadWave(&ls, baseName, symbolicPathName, fileParam, p->calledFromFunction);

	return err;
}

int
RegisterGBLoadWave(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "GBLoadWave /A[=name:ABaseName] /B[=number:lowByteFirst] /D[=number:doublePrecision] /F=number:dataFormat /I[={string:macFilterStr,string:winFilterStr}] /J=number:floatFormat /L=number:dataLengthInBits /N[=name:NBaseName] /O[=number:overwrite] /P=name:pathName /Q[=number:quiet] /S=number:skipBytes /T={number:fileDataType,number:waveDataType} /U=number:pointsPerArray /V[=number:interleaved] /W=number:numberOfArraysInFile /Y={number:offset,number:multiplier} [string:fileParamStr]";
	runtimeNumVarList = "V_flag;";
	runtimeStrVarList = "S_fileName;S_path;S_waveNames;";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(GBLoadWaveRuntimeParams), (void*)ExecuteGBLoadWave, 0);
}
