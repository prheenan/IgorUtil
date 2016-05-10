
/* WindowXOP1 custom error codes */
#define OLD_IGOR 1 + FIRST_XOP_ERR

/* Prototypes */
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);

void GetHourMinuteSecond(int* h, int* m, int* s);

void DoXOPContextualHelp(XOP_WINDOW_REF w);
void DrawXOPWindow(XOP_WINDOW_REF w);
void DisplayWindowXOP1Message(XOP_WINDOW_REF w, const char* message1, const char* message2);
XOP_WINDOW_REF CreateXOPWindow(void);
void DestroyXOPWindow(XOP_WINDOW_REF w);

#ifdef MACIGOR
	void XOPWindowClickMac(WindowPtr theWindow, EventRecord* ep);
#endif

#ifdef WINIGOR
	int CreateXOPWindowClass(void);
#endif
