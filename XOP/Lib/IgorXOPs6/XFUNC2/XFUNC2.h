// XFUNC2.h -- equates for XFUNC2 XOP

// XFUNC2 custom error codes
#define REQUIRES_SP_OR_DP_WAVE 1 + FIRST_XOP_ERR
#define ILLEGAL_LEGENDRE_INPUTS 2 + FIRST_XOP_ERR

#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct LogFitParams {		// This structure must be 2-byte-aligned because it receives parameters from Igor.
	double x;				// Independent variable.
	waveHndl waveHandle;	// Coefficient wave (contains a, b, c coefficients).
	double result;
};
typedef struct LogFitParams LogFitParams;
typedef struct LogFitParams *LogFitParamsPtr;
#pragma pack()		// Reset structure alignment to default.

#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct PlgndrParams {		// This structure must be 2-byte-aligned because it receives parameters from Igor.
	double x;
	double m;
	double l;
	double result;
};
typedef struct PlgndrParams PlgndrParams;
typedef struct PlgndrParams *PlgndrParamsPtr;
#pragma pack()		// Reset structure alignment to default.


// Prototypes
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);
extern "C" int logfit(struct LogFitParams* p);
extern "C" int plgndr(struct PlgndrParams* p);

