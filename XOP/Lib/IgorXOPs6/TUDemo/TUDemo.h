/* TUDemo.h -- equates for TUDemo XOP */
 
/* TUDemo custom error codes */
#define CANT_OPEN_WINDOW 1 + FIRST_XOP_ERR
#define OLD_IGOR 2 + FIRST_XOP_ERR

/* TUDemo miscellaneous equates */
#define MIN_WIN_WIDTH 100
#define MIN_WIN_HEIGHT 100

/* miscellaneous strings resource */
#define MISCSTR_ID 1102

/* resource ID for alert dialog */
#define DO_ALERT_ID 1258

/* TUDemo submenu item numbers */
enum {
	TUDemo_Open=1,
	TUDemo_Status,
	TUDemo_Insert_Text,
	TUDemo_Save_Text,
	TUDemo_GetAllText_Test,
	TUDemo_FetchSelectedText_Test,
	TUDemo_SetSelection_Test,
	TUDemo_Quit,
	TUDemo_NumberOfMenuItems=TUDemo_Quit
};

/* Prototypes */
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);
