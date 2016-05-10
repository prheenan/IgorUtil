#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "WindowXOP1.h"
#include "resource.h"

/*	This is the XOP window class name. It must not conflict with other window
	class names. Therefore, use your XOP's name and make sure that your XOP's
	name is sufficiently descriptive to insure uniqueness.
*/
static char* gXOPWindowClassName = "WindowXOP1";

#define IDB_BUTTON1 101		// Button ID sent with BN_CLICKED message.

void
GetHourMinuteSecond(int* h, int* m, int* s)
{
	SYSTEMTIME st;
	
	GetLocalTime(&st);
	*h = st.wHour;
	*m = st.wMinute;
	*s = st.wSecond;
}

/*	DoXOPContextualHelp()

	Update Igor Tips window (Macintosh) or status line (Windows).
	This works with Igor Pro Carbon or later. Does nothing with older versions of Igor.
*/
void
DoXOPContextualHelp(HWND hwnd)
{
	POINT cursorPos;
	HWND hwndControl;
	RECT controlRect;
	int controlID;
	char message[256];
	
	if (GetWindow(hwnd, GW_HWNDFIRST) != hwnd)
		return;					// We are not the top window.

	if (GetCursorPos(&cursorPos) == 0)
		return;
	if (ScreenToClient(hwnd, &cursorPos) == 0)
		return;
		
	hwndControl = ChildWindowFromPointEx(hwnd, cursorPos, CWP_SKIPINVISIBLE);
	if (hwndControl==hwnd || hwndControl==NULL)
		return;
	
	controlID = GetDlgCtrlID(hwndControl);
	if (controlID == 0)
		return;
	
	// We need the location of the control in terms of client coordinates of the parent window.
	if (GetWindowRect(hwndControl, &controlRect) == 0)
		return;
	if (ScreenToClient(hwnd, (LPPOINT)&controlRect.left) == 0)
		return;
	if (ScreenToClient(hwnd, (LPPOINT)&controlRect.right) == 0)
		return;
	
	*message = 0;
	
	switch(controlID) {
		case IDB_BUTTON1:
			strcpy(message, "This contextual help is displayed by the XOPSetContextualHelpMessage routine.");
			break;
	}
	
	if (*message != 0) {
		Rect macRect;
		WinRectToMacRect(&controlRect, &macRect);
		XOPSetContextualHelpMessage(hwnd, message, &macRect);
	}
}

static void
DrawXOPWindowUsingHDC(HWND hwnd, HDC hdc, int isActive)
{
	int hour, minute, second;
	char notice[80];
	RECT clientRect;
	HBRUSH hBrush;
	
	if (GetClientRect(hwnd, &clientRect) == 0)
		return;			// Should never happen.
	
	hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(hdc, &clientRect, hBrush);

	GetHourMinuteSecond(&hour, &minute, &second);
	sprintf(notice, "The time is: %02ld:%02ld:%02ld", hour, minute, second);
	TextOut(hdc, 5, 15, notice, (int)strlen(notice));
	strcpy(notice, "WindowXOP1's menu item.");
	if (isActive)
		strcpy(notice, "Active");
	else
		strcpy(notice, "Inactive");
	TextOut(hdc, 5, 30, notice, (int)strlen(notice));
}

/*	DrawXOPWindow(hwnd)

	DrawXOPWindow draws the contents of WindowXOP1's window.
*/
void
DrawXOPWindow(HWND hwnd)
{
	HDC hdc;
	int isActive;
	
	hdc = GetDC(hwnd);
	if (hdc == NULL)
		return;			// Should never happen.

	isActive = IsXOPWindowActive(hwnd);

	DrawXOPWindowUsingHDC(hwnd, hdc, isActive);
	
	ReleaseDC(hwnd, hdc);
}

/*	DisplayWindowXOP1Message(hwnd, message1, message2)

	Used to display a message when user selects menu item or clicks button.
*/
void
DisplayWindowXOP1Message(HWND hwnd, const char* message1, const char* message2)
{
	HDC hdc;
	HBRUSH hBrush;
	RECT clientRect;
	UInt32 ticks;
	
	if (GetClientRect(hwnd, &clientRect) == 0)
		return;			// Should never happen.
	
	hdc = GetDC(hwnd);
	if (hdc == NULL)
		return;			// Should never happen.
	
	hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(hdc, &clientRect, hBrush);
	TextOut(hdc, 3, 15, message1, (int)strlen(message1));
	TextOut(hdc, 3, 30, message2, (int)strlen(message2));
		
	ReleaseDC(hwnd, hdc);
	
	/*	Need to refresh the button which may have been covered by a menu.
		Without this, the bits from the menu would appear in the button during
		the delay loop below.
	*/
	{
		HWND buttonHWND;
		buttonHWND = GetWindow(hwnd, GW_CHILD);
		if (buttonHWND != NULL)			// Should never be null.
			UpdateWindow(buttonHWND);
	}

	ticks = GetTickCount();						// Let momentous message be seen.
	while (GetTickCount() < ticks+2000)			// A Windows tick is one millisecond.
		;
}

static HWND
CreateWindowXOP1Button(HWND parentHWND)
{
	HWND buttonHWND;
	
	buttonHWND = CreateWindowEx(
		0L,														// No extended styles
		"BUTTON",												// Window class name
		"Click Me",												// Window title
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,					// Window style
		60,														// Horizontal position
		60,														// Vertical position
		75,														// Width
		25,														// Height
		parentHWND,												// Parent window
		(HMENU)IDB_BUTTON1,										// Button ID sent with BN_CLICKED message.
		(HINSTANCE)GetWindowLong(parentHWND, GWLP_HINSTANCE),	// "Identifies instance of module to be associated with the window"
		(LPVOID)NULL);											// No window creation data
	
	return buttonHWND;
}

static LRESULT CALLBACK
XOPWindowProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	int passMessageToDefProc;
	
	static UINT_PTR timerRefNum = 0;
	
	if (SendWinMessageToIgor(hwnd, iMsg, wParam, lParam, 0))	// Give Igor a crack at this message.
		return 0;
	
	passMessageToDefProc = 1;		// Determines if we pass message to default window procedure.
	switch(iMsg) {
		case WM_CREATE:
			CreateWindowXOP1Button(hwnd);
			
			// This timer is created just so we can display contextual help for controls in the window.
			timerRefNum = SetTimer(hwnd, 1, 50, NULL);
		
			// Do your custom initialization here.
			
			passMessageToDefProc = 0;
			break;
		
		case WM_MOUSEMOVE:
			{
				static HCURSOR hCursor = NULL;
				if (hCursor == NULL)
					hCursor = LoadCursor(NULL, (const char*)IDC_ARROW);
				if (hCursor != NULL)
					SetCursor(hCursor);
			}
			passMessageToDefProc = 0;
			break;
			
		case WM_TIMER:
			DoXOPContextualHelp(hwnd);
			break;

		case WM_SIZE:
			// Do your custom size processing here.
			
			passMessageToDefProc = 1;		// Windows documentation says to pass this message to the def proc.
			break;

		case WM_MOVE:
			/*	Do your custom move processing here.
				Windows XOPs do not receive the DRAGGED message from Igor.
				WM_MOVE serves the same purpose for Windows XOPs.
				However, in most cases, there is nothing needs to be done.
			*/
			passMessageToDefProc = 1;		// Windows documentation says to pass this message to the def proc.
			break;

		case WM_PAINT:
			{
				HDC hdc;
				PAINTSTRUCT ps;
				int isActive;
				
				isActive = IsXOPWindowActive(hwnd);
				hdc = BeginPaint(hwnd, &ps);
				DrawXOPWindowUsingHDC(hwnd, hdc, isActive);
				EndPaint(hwnd, &ps);
			}
			passMessageToDefProc = 0;
			break;

		case WM_ACTIVATE:
		case WM_MDIACTIVATE:
			{
				HDC hdc;
				int isActive;
				
				hdc = GetDC(hwnd);
				if (hdc == NULL)
					break;								// Should never happen.
				isActive = hwnd == (HWND)lParam;		// Are we being activated?
				DrawXOPWindowUsingHDC(hwnd, hdc, isActive);
				ReleaseDC(hwnd, hdc);
				
				if (isActive)
					TellIgorWindowStatus(hwnd, WINDOW_STATUS_ACTIVATED, 0);
				else
					TellIgorWindowStatus(hwnd, WINDOW_STATUS_DEACTIVATED, 0);
			}
			
			passMessageToDefProc = 1;		// Windows documentation doesn't say so, but I found it necessary to pass this message to the def proc. Otherwise, the window was not correctly highlighted.
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case 0:						// Message from menu or from BN_CLICKED notification.
				case 1:						// Message from accelerator.
					/*	You don't need to do anything here for menu selections from the
						Igor menu bar because these commands are dispatched by Igor to your
						XOPEntry via MENUITEM messages. However, if your window has buttons
						or if you create an overlapped window with its own menu bar, then
						you need to field messages here.
					*/
					switch(LOWORD(wParam)) {
						case ID_ACTION_DISPLAYMESSAGE:
							DisplayWindowXOP1Message(hwnd, "Thank you for selecting", "WindowXOP1's menu item.");
							break;
						case IDB_BUTTON1:
							DisplayWindowXOP1Message(hwnd, "Thank you for clicking", "WindowXOP1's button.");
							break;
					}
					break;
				
				default:
					// Ignore notification codes other than BN_CLICKED.
					break;
			}
			passMessageToDefProc = 0;
			break;
		
		case WM_KEYDOWN:
		case WM_CHAR:
			// Do your custom key processing here.
			passMessageToDefProc = 0;
			break;
			
		case WM_LBUTTONDOWN:
			// Do your custom click processing here.
			passMessageToDefProc = 0;
			break;
		
		case WM_RBUTTONDOWN:
			// Do your custom click processing here.
			passMessageToDefProc = 0;
			break;

		case WM_CLOSE:							// Message received when the user clicks the close box.
			HideAndDeactivateXOPWindow(hwnd);
			TellIgorWindowStatus(hwnd, WINDOW_STATUS_DID_HIDE, 0);
			passMessageToDefProc = 0;
			break;

		case WM_DESTROY:
			if (timerRefNum != 0)
				KillTimer(hwnd, timerRefNum);
		
			TellIgorWindowStatus(hwnd, WINDOW_STATUS_ABOUT_TO_KILL, 0);

			// Do your custom destroy processing here.
			
			passMessageToDefProc = 0;
			break;
	}

	SendWinMessageToIgor(hwnd, iMsg, wParam, lParam, 1);	// Give Igor another crack at it.

	if (passMessageToDefProc == 0)
		return 0;

	// Pass unprocessed message to default window procedure.
	return DefMDIChildProc(hwnd, iMsg, wParam, lParam);
}

int
CreateXOPWindowClass(void)		// Called from main in WindowXOP1.c.
{
	HMODULE hXOPModule;
	WNDCLASSEX  wndclass;
	ATOM atom;
	int err;
	
	err = 0;
	
	hXOPModule = XOPModule();

	wndclass.cbSize			= sizeof(wndclass);
	wndclass.style			= CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc	= XOPWindowProc;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hXOPModule;
	wndclass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor		= NULL;										// we control our cursor
	wndclass.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName	= NULL;
	wndclass.lpszClassName	= gXOPWindowClassName;
	wndclass.hIconSm		= NULL;
	atom = RegisterClassEx(&wndclass);
	if (atom == 0) {				// You will get an error if you try to register a class
		err = GetLastError();		// using a class name for an existing window class.
		err = WindowsErrorToIgorError(err);
	}
	return err;
}

HWND
CreateXOPWindow(void)
{
	HWND hwnd, parentHWND;
	int style, extendedStyle;
	int x, y, width, height;
	HMODULE hXOPModule;
	HMENU hMenu;

	// Now we can create the window.
	hXOPModule = XOPModule();
	style = WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU;
	extendedStyle = WS_EX_MDICHILD;
	x = 200;
	y = 5;
	width = 200;
	height = 120;
	
	/*	When we are creating an MDI window, this makes the window the child of the Igor MDI client window.
		When we are creating and overlapped window, this makes the window owned by Igor MDI client window.
	*/
	parentHWND = IgorClientHWND();
	
	/*	For an MDI child window, the hMenu parameter is not a menu handle but rather is a
		"child window identifier". According to the Windows documentation, this is
		supposed to be "unique for all child windows with the same parent". We use
		the GetTickCount function to insure uniqueness.
	*/
	hMenu = (HMENU)GetTickCount();
	
	hwnd = CreateWindowEx(extendedStyle, gXOPWindowClassName, "WindowXOP1", style, x, y, width, height, parentHWND, hMenu, hXOPModule, (LPARAM)NULL);

	return hwnd;
}

void
DestroyXOPWindow(HWND hwnd)
{
	TellIgorWindowStatus(hwnd, WINDOW_STATUS_ABOUT_TO_KILL, 0);

	// Do not use DestroyWindow to destroy an MDI child window!
	SendMessage(IgorClientHWND(), WM_MDIDESTROY, (WPARAM)hwnd, 0);

	TellIgorWindowStatus(hwnd, WINDOW_STATUS_DID_KILL, 0);	// This message was added in Igor Pro 6.04.
}
