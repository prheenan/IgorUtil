/*
 *	GBLoadWave.h
 *		equates for GBLoadWave XOP
 *
 */

/* GBLoadWave custom error codes */

#define IMPROPER_FILE_TYPE 1 + FIRST_XOP_ERR		/* not the type of file this XOP loads */
#define NO_DATA_FOUND 2 + FIRST_XOP_ERR				/* file being loaded contains no data */
#define EXPECTED_GB_FILE 3 + FIRST_XOP_ERR			/* expected name of loadable file */
#define EXPECTED_BASENAME 4 + FIRST_XOP_ERR			/* expected base name for new waves */
#define EXPECTED_FILETYPE 5 + FIRST_XOP_ERR			/* expected file type */
#define TOO_MANY_FILETYPES 6 + FIRST_XOP_ERR		/* too many file types */
#define BAD_DATA_LENGTH 7 + FIRST_XOP_ERR			/* data length in bits must be 8, 16, 32 or 64 */
#define BAD_NUM_WAVES 8 + FIRST_XOP_ERR				/* number of waves must be >= 1 */
#define NOT_ENOUGH_BYTES 9 + FIRST_XOP_ERR			/* file contains too few bytes for specified ... */
#define BAD_DATA_TYPE 10 + FIRST_XOP_ERR			/* bad data type value */
#define OLD_IGOR 11 + FIRST_XOP_ERR					/* Requires Igor Pro 6.20 or later */
#define BAD_FP_FORMAT_CODE 12 + FIRST_XOP_ERR		/* Valid floating point formats are 1 (IEEE) and 2 (VAX). */
#define ARRAY_TOO_BIG_FOR_IGOR 13 + FIRST_XOP_ERR	/* The array is too big for an Igor wave. */

#define LAST_GBLOADWAVE_ERR ARRAY_TOO_BIG_FOR_IGOR

#define ERR_ALERT 1258

/* #defines for GBLoadWave flags */

#define OVERWRITE 1						/* /O means overwrite */
#define DOUBLE_PRECISION 2				/* /D means double precision */
#define INTERACTIVE 4					/* /I means interactive -- use open dialog */
#define AUTO_NAME 8						/* /A or /N means autoname wave */
#define PATH 16							/* /P means use symbolic path */
#define QUIET 32						/* /Q means quiet -- no messages in history */
#define FROM_MENU 64					/* LoadWave summoned from menu item */
#define COMPLEX 128						/* data is complex */

/* structure used in reading file */
struct ColumnInfo {
	char waveName[MAX_OBJ_NAME+1];		/* Name of wave for this column. */
	waveHndl waveHandle;				/* Handle to this wave. */
	int wavePreExisted;					/* True if wave existed before this command ran. */
	CountInt points;					/* Total number of points in wave. */
	void *dataPtr;						/* To save pointer to start of wave data. */
};
typedef struct ColumnInfo ColumnInfo;
typedef struct ColumnInfo *ColumnInfoPtr;
typedef struct ColumnInfo **ColumnInfoHandle;

// This structure is used in memory only - not saved to disk.
struct LoadSettings {
	int flags;						// Flag bits are defined in GBLoadWave.h.
	int lowByteFirst;				// 0 = high byte first (Motorola); 1 = low byte first (Intel). Set by /B flag.
	int inputDataType;				// Igor number type (e.g., NT_FP64, NT_I32 - see IgorXOPs.h). Set by /T flag or /L flag or /F flag.
	int inputBytesPerPoint;			// 1, 2, 4 or 8. Set by /T flag or /L flag.
	int inputDataFormat;			// SIGNED_INT, UNSIGNED_INT, IEEE_FLOAT. Set by /T flag or /F flag.
	int outputDataType;				// Igor number type (e.g., NT_FP64, NT_I32 - see IgorXOPs.h). Set by /T flag or /D flag.
	int outputBytesPerPoint;		// Set by /T flag or /D flag.
	int outputDataFormat;			// SIGNED_INT, UNSIGNED_INT, IEEE_FLOAT. Set by /T flag or /D flag.
	CountInt preambleBytes;			// Bytes to skip at start of file. Set by /S flag.
	int numArrays;					// Number of waves in file 0 for auto. Set by /W flag.
	CountInt arrayPoints;			// Number of points in each array in file or 0 for auto. Set by /U flag.
	char filterStr[256];			// Set by /I= flag.
	int interleaved;				// Truth that arrays in file are interleaved. Set by /V flag.
	int floatingPointFormat;		// 1 = IEEE; 2 = VAX. Set by /J flag.
	double offset;					// Output data = (input data + offset) * multiplier.
	double multiplier;				// Set by /Y flag.
};
typedef struct LoadSettings LoadSettings;
typedef struct LoadSettings *LoadSettingsPtr;

/* Since the GBLoadInfo structure is saved to disk, we make sure that it is 2-byte-aligned. */
#pragma pack(2)

/* structure used to save GBLoadWave settings */
struct GBLoadInfo {
	short version;						/* version number for structure */
	short inputDataTypeItemNumber;		/* popup menu item number. */
	short outputDataTypeItemNumber;
	short floatFormatItemNumber;
	char filterStr[256];				// HR, 980401: Replaced Mac-specific fields with filterStr field.
	double preambleBytes;				// HR, 080820, 1.62: Changed from long to double.
	long numArrays;
	double arrayPoints;					// HR, 080820, 1.62: Changed from long to double.
	short bytesSwapped;
	short interleaved;
	short scalingEnabled;				/* Stores state of scaling checkbox in dialog. */
	short overwrite;
	char baseName[32];
	
	/* fields below added in version #2 of structure */
	char symbolicPathName[32];					
	double offset;						/* output data = (input data + offset) * multiplier */
	double multiplier;
};
typedef struct GBLoadInfo GBLoadInfo;
typedef struct GBLoadInfo *GBLoadInfoPtr;
typedef struct GBLoadInfo **GBLoadInfoHandle;

#define GBLoadInfo_VERSION 5			// HR, 080820: Incremented to 5 because of changing preambleBytes and arrayPoints fields to double.

#pragma pack()		// Reset structure alignment to default.


/* miscellaneous #defines */

#define MAX_IGOR_UNITS 4				/* max chars allowed in Igor unit string */
#define MAX_USER_LABEL 3
#define MAX_WAVEFORM_TITLE 18

/* Prototypes */

// In GBLoadWave.c
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);
int LoadWave(LoadSettings* lsp, const char* baseName, const char* symbolicPathName, const char* fileParam, int runningInUserFunction);

// In GBLoadWaveOperation.c
int RegisterGBLoadWave(void);

// In GBLoadWaveDialog.c
int GBLoadWaveDialog(void);
int GetLoadFile(const char* initialDir, const char* fileFilterStr, char *fullFilePath);

// In GBScalingDialog.c
int GBScalingDialog(double* offsetPtr, double* multiplierPtr);
