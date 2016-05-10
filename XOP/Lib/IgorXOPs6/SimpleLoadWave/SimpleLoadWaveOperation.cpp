#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "SimpleLoadWave.h"

// Operation template: SimpleLoadWave /A[=name:baseName] /D /I /N[=name:baseName] /O /P=name:pathName /Q /W [string:fileParamStr]

// Runtime param structure for SimpleLoadWave operation.
#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct SimpleLoadWaveRuntimeParams {
	// Flag parameters.

	// Parameters for /A flag group.
	int AFlagEncountered;
	char ABaseName[MAX_OBJ_NAME+1];			// Optional parameter.
	int AFlagParamsSet[1];

	// Parameters for /D flag group.
	int DFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /I flag group.
	int IFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /N flag group.
	int NFlagEncountered;
	char NBaseName[MAX_OBJ_NAME+1];			// Optional parameter.
	int NFlagParamsSet[1];

	// Parameters for /O flag group.
	int OFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /P flag group.
	int PFlagEncountered;
	char pathName[MAX_OBJ_NAME+1];
	int PFlagParamsSet[1];

	// Parameters for /Q flag group.
	int QFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Parameters for /W flag group.
	int WFlagEncountered;
	// There are no fields for this group because it has no parameters.

	// Main parameters.

	// Parameters for simple main group #0.
	int fileParamStrEncountered;
	Handle fileParamStr;					// Optional parameter.
	int fileParamStrParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct SimpleLoadWaveRuntimeParams SimpleLoadWaveRuntimeParams;
typedef struct SimpleLoadWaveRuntimeParams* SimpleLoadWaveRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
ExecuteSimpleLoadWave(SimpleLoadWaveRuntimeParamsPtr p)
{
	int fileLoaderFlags;
	char baseName[MAX_OBJ_NAME+1];
	char symbolicPathName[MAX_OBJ_NAME+1];
	char fileParam[MAX_PATH_LEN+1];
	int err = 0;

	fileLoaderFlags = 0;
	strcpy(baseName, "wave");
	*symbolicPathName = 0;
	*fileParam = 0;
	
	if (p->AFlagEncountered) {
		fileLoaderFlags |= FILE_LOADER_AUTONAME;
		if (p->AFlagParamsSet[0]) {
			if (*p->ABaseName != 0)
				strcpy(baseName, p->ABaseName);
		}
	}
	
	if (p->DFlagEncountered)
		fileLoaderFlags |= FILE_LOADER_DOUBLE_PRECISION;

	if (p->IFlagEncountered)
		fileLoaderFlags |= FILE_LOADER_INTERACTIVE;
	
	if (p->NFlagEncountered) {
		fileLoaderFlags |= FILE_LOADER_AUTONAME | FILE_LOADER_OVERWRITE;
		if (p->NFlagParamsSet[0]) {
			if (*p->NBaseName != 0)
				strcpy(baseName, p->NBaseName);
		}
	}
	
	if (p->OFlagEncountered)
		fileLoaderFlags |= FILE_LOADER_OVERWRITE;
	
	if (p->PFlagEncountered) {
		strcpy(symbolicPathName, p->pathName);

		if (*symbolicPathName != 0) {						// /P=$"" is a like no /P at all.
			char symbolicPathPath[MAX_PATH_LEN+1];

			fileLoaderFlags |= FILE_LOADER_PATH;

			// This is just to check if the symbolic path name is valid.
			if (err = GetPathInfo2(symbolicPathName, symbolicPathPath))
				return err;
		}
	}
	
	if (p->QFlagEncountered)
		fileLoaderFlags |= FILE_LOADER_QUIET;

	if (p->WFlagEncountered)
		fileLoaderFlags |= SIMPLE_READ_NAMES;
	
	if (p->fileParamStrEncountered) {
		if (err = GetCStringFromHandle(p->fileParamStr, fileParam, sizeof(fileParam)-1))
			return err;
		if (err = GetNativePath(fileParam, fileParam))
			return err;
	}

	// We have the parameters, now load the data.
	err = LoadWave(p->calledFromFunction, fileLoaderFlags, baseName, symbolicPathName, fileParam);

	return err;
}

int
RegisterSimpleLoadWave(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "SimpleLoadWave /A[=name:baseName] /D /I /N[=name:baseName] /O /P=name:pathName /Q /W [string:fileParamStr]";
	runtimeNumVarList = "V_flag;";
	runtimeStrVarList = "S_path;S_fileName;S_waveNames;";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(SimpleLoadWaveRuntimeParams), (void*)ExecuteSimpleLoadWave, 0);
}
