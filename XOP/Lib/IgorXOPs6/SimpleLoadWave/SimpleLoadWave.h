// SimpleLoadWave.h -- quates for SimpleLoadWave XOP

// Custom error codes.
enum {
	IMPROPER_FILE_TYPE= 1 + FIRST_XOP_ERR,	/* "Not the type of file this XOP loads." */
	NO_DATA_FOUND,							/* "Could not find at least one row of wave data in the file." */
	EXPECTED_TD_FILE,						/* "Expected name of loadable file." */
	EXPECTED_BASENAME,						/* "Expected base name for new waves." */
	OLD_IGOR								/* "SimpleLoadWave requires Igor Pro 5.00 or later." */
};
#define XOP_FIRST_ERR IMPROPER_FILE_TYPE
#define XOP_LAST_ERR EXPECTED_BASENAME


// Structure used in loading file.
typedef struct ColumnInfo {
	char waveName[MAX_OBJ_NAME+1];			/* name of wave for this column */
	int waveAlreadyExisted;					/* truth that wave existed before XOP executed */
	waveHndl waveHandle;					/* handle to this wave */
	void* waveData;							/* pointer to wave data once it has been locked */
	/* put other column information here */
}ColumnInfo, *ColumnInfoPtr;


// Misc Equates
#define SIMPLE_READ_NAMES (FILE_LOADER_LAST_FLAG<<1)	/* flag bit for /W=w command line option */


// Prototypes

// In SimpleLoadWave.c
int LoadWave(int calledFromFunction, int flags, const char* baseName, const char* symbolicPathName, const char* fileParam);
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);

// In SimpleLoadWaveOperation.c
int RegisterSimpleLoadWave(void);
