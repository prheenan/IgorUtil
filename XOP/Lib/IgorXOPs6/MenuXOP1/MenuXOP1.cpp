/*	MenuXOP1.c -- for illustrating XOP menu features.

	HR, 020919: Revamped to use Operation Handler so that the MenuXOP1 operation
	can be called from a user function. This required changing the syntax of the
	operation to use keyword={values} syntax.

	Command line operations:
		MenuXOP1 quit
			Causes MenuXOP1 to be unloaded from memory.
			
		MenuXOP1 show=resourceMenuID
		MenuXOP1 hide=resourceMenuID
			Causes the specified menu to be shown or hidden.

			resourceMenuID is the resource ID for the menu in the XOP's resource fork

			Only main menu bar menus can be shown or hidden (try 100 and 101)
			
		MenuXOP1 enable={resourceMenuID,actualtemNumber}
		MenuXOP1 disable={resourceMenuID,actualtemNumber}
			Causes the specified menu item to be shown or hidden.

			resourceMenuID is the resource ID for the menu in the XOP's resource fork
			(100 through 104 on Macintosh, 101 through 105 on Windows)
			
			actualItem number is the item number in the menu starting from 1.
	
	HR, 091021
		Updated for 64-bit compatibility.

	HR, 2013-02-08
		Updated for Xcode 4 compatibility. Changed to use XOPMain instead of main.
		As a result the XOP now requires Igor Pro 6.20 or later.
*/

#include "XOPStandardHeaders.h"				// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "MenuXOP1.h"

// These equates must agree with MENU, XMN1, XSM1 and XMI1 resources.
#ifdef MACIGOR
	#define MAINMENU_ID 100					// Menu resource ID of main menu.
	#define SUBMENU1_ID 101					// Menu resource ID of first submenu.
	#define SUBMENU2_ID 102					// Menu resource ID of second submenu.
	#define SUBMENU3_ID 103					// Menu resource ID of third submenu.
	#define SUBMENU4_ID 104					// Menu resource ID of fourth submenu.
#endif
#ifdef WINIGOR
	#include "resource.h"					// Contains symbols for menu resource IDs.
	#define MAINMENU_ID IDR_MENU1			// Menu resource ID of main menu.
	#define SUBMENU1_ID IDR_MENU2			// Menu resource ID of first submenu.
	#define SUBMENU2_ID IDR_MENU3			// Menu resource ID of second submenu.
	#define SUBMENU3_ID IDR_MENU4			// Menu resource ID of third submenu.
	#define SUBMENU4_ID IDR_MENU5			// Menu resource ID of fourth submenu.
#endif

// Global Variables
MenuHandle mainMenuHandle;					// Menu handle for main MenuXOP1 menu

/*	GetMenuHandleAndItem(menuIDIn, itemNumberIn, mHandleOut, itemNumberOut)

	menuIDIn and itemNumberIn are parameters supplied to MenuXOP1 enable or MenuXOP1 disable.
	
	Returns menu handle and item number via mHandleOut, itemNumberOut.
	*mHandleOut will be NULL if the specified menu is hidden.
	
	Function result is 0 if OK or error code.
*/
static int
GetMenuHandleAndItem(int menuIDIn, int itemNumberIn, MenuHandle* mHandleOut, int* itemNumberOut)
{
	MenuHandle mHandle;
	int menuID, itemNumber;
	int isIgorMenu;

	*mHandleOut = NULL;
	*itemNumberOut = 0;
	
	menuID = ResourceToActualMenuID(menuIDIn);
	isIgorMenu = menuID == 0;
	if (isIgorMenu) {
		menuID = menuIDIn;				// Presumably an Igor menu with item added by XOP.
		if (menuID!=MISCID && menuID!=ANALYSISID)
			return BAD_MENU_ID;			// Only these menus are supported by MenuXOP1.
	}

	itemNumber = itemNumberIn;
	if (isIgorMenu) {
		if (ActualToResourceItem(menuID,itemNumber) == 0)
			return BAD_ITEM_NUM;
	}

	mHandle = GetMenuHandle(menuID);
	if (mHandle == NULL)
		return 0;						// Menu is hidden -- not in menu bar.
		
	if (itemNumber<1 || itemNumber>CountMItems(mHandle))
		return BAD_ITEM_NUM;
		
	*mHandleOut = mHandle;
	*itemNumberOut = itemNumber;
	
	return 0;
}

// MenuXOP1 enable={number,number}, disable={number,number}, hide=number, show=number, quit

// Runtime param structure for MenuXOP1 operation.
#pragma pack(2)		// All structures passed between Igor and XOP are two-byte aligned.
struct MenuXOP1RuntimeParams {
	// Flag parameters.

	// Main parameters.

	// Parameters for enable keyword group.
	int enableEncountered;
	double enableMenuID;
	double enableItemNumber;
	int enableParamsSet[2];

	// Parameters for disable keyword group.
	int disableEncountered;
	double disableMenuID;
	double disableItemNumber;
	int disableParamsSet[2];

	// Parameters for hide keyword group.
	int hideEncountered;
	double hideMenuID;
	int hideParamsSet[1];

	// Parameters for show keyword group.
	int showEncountered;
	double showMenuID;
	int showParamsSet[1];

	// Parameters for quit keyword group.
	int quitEncountered;
	// There are no fields for this group because it has no parameters.

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct MenuXOP1RuntimeParams MenuXOP1RuntimeParams;
typedef struct MenuXOP1RuntimeParams* MenuXOP1RuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

/*	ExecuteMenuXOP1(p)
	
	Executes MenuXOP1 operation.
*/
extern "C" int
ExecuteMenuXOP1(MenuXOP1RuntimeParamsPtr p)
{
	int menuID, itemNumber;
	MenuHandle mHandle;
	int err = 0;

	if (p->enableEncountered) {
		if (err = GetMenuHandleAndItem((int)p->enableMenuID, (int)p->enableItemNumber, &mHandle, &itemNumber))
			return err;

		if (mHandle != NULL)
			EnableItem(mHandle, itemNumber);
	}

	if (p->disableEncountered) {
		if (err = GetMenuHandleAndItem((int)p->disableMenuID, (int)p->disableItemNumber, &mHandle, &itemNumber))
			return err;

		if (mHandle != NULL)
			DisableItem(mHandle, itemNumber);
	}
	
	if (p->hideEncountered) {
		menuID = (int)p->hideMenuID;
		if (menuID != MAINMENU_ID)
			return MAIN_MENU_ONLY;
		
		menuID = ResourceToActualMenuID(menuID);
		mHandle = GetMenuHandle(menuID);
		if (mHandle != NULL) {							// Menu showing ?
			WMDeleteMenu(menuID);
			WMDrawMenuBar();
		}
	}
	
	if (p->showEncountered) {
		menuID = (int)p->showMenuID;
		if (menuID != MAINMENU_ID)
			return MAIN_MENU_ONLY;
		
		menuID = ResourceToActualMenuID(menuID);
		mHandle = GetMenuHandle(menuID);
		if (mHandle == NULL) {							// Menu not already showing ?
			WMInsertMenu(mainMenuHandle, 0);
			WMDrawMenuBar();
		}
	}
	
	if (p->quitEncountered) {
		// If you unload MenuXOP1 while main menu is hidden it will not be retrievable if MenuXOP1 is reloaded.
		menuID = ResourceToActualMenuID(MAINMENU_ID);
		if (GetMenuHandle(menuID) == NULL) {
			WMInsertMenu(mainMenuHandle, 0);
			WMDrawMenuBar();
		}
		SetXOPType(TRANSIENT);							// Tell Igor to unload XOP.
	}

	return err;
}

/*	RegisterMenuXOP1()
	
	Registers MenuXOP1 operation with Igor's Operation Handler.
*/
static int
RegisterMenuXOP1(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;

	cmdTemplate = "MenuXOP1 enable={number,number}, disable={number,number}, hide=number, show=number, quit";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	
	/*	We pass NULL here instead of ExecuteMenuXOP1 because we want the operation to be called via
		the EXECUTE_OPERATION message, not directly. This is because we want to be able to unload
		the XOP from memory.
	*/
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(MenuXOP1RuntimeParams), NULL, 0);
}

/*	XOPMenuItem()

	XOPMenuItem is called when user selects XOPs menu item.
*/
static int
XOPMenuItem(void)
{
	MenuHandle mHandle;
	int resourceMenuID, actualMenuID, menuID;
	int resourceItemNumber, actuaItemNumber, itemNumber;
	int isIgorMenu, isXOPMenu;
	char message[100];
	char buf[256];
	
	isIgorMenu = isXOPMenu = 0;
	
	actualMenuID = (int)GetXOPItem(0);				// Get actual menu ID.
	actuaItemNumber = (int)GetXOPItem(1);
	
	mHandle = GetMenuHandle(actualMenuID);			// This would be NIL if the menu were hidden.
	
	resourceMenuID = ActualToResourceMenuID(actualMenuID);	// convert to resource menu ID.
	if (resourceMenuID != 0) {						// Menu added by XOP ?.
		menuID = resourceMenuID;
		isXOPMenu = 1;
		if (igorVersion >= 200)
			mHandle = ResourceMenuIDToMenuHandle(resourceMenuID);	// NIL if not menu added by XOP.
	}
	else {											// This is an Igor menu.
		menuID = actualMenuID;
		isIgorMenu = 1;
	}
	
	resourceItemNumber = ActualToResourceItem(actualMenuID, actuaItemNumber);
	if (resourceItemNumber != 0)					// Item added by XOP to Igor menu ?.
		itemNumber = resourceItemNumber;
	else											// This is an XOP menu.
		itemNumber = actuaItemNumber;
	
	switch (menuID) {
		case MAINMENU_ID:				// MenuXOP1 menu in main menu bar.
			switch (itemNumber) {
				case 1:
					// This item has a submenu so this item should never be selected.
					strcpy(message, "MenuXOP1: Item1 selected");
					break;
				case 2:
					strcpy(message, "MenuXOP1: Item2 selected");
					break;
				case 3:
					// This item has a submenu so this item will never be selected.
					break;
				default:
					strcpy(message, "MenuXOP1: unknown menu item selected");
					break;
			}
			break;
			
		case SUBMENU1_ID:				// Submenu in item 1 of main menu bar.
			switch (itemNumber) {
				case 1:
					strcpy(message, "MenuXOP1: Subitem1_1 selected");
					break;
				case 2:
					strcpy(message, "MenuXOP1: Subitem1_2 selected");
					break;
				case 3:
					strcpy(message, "MenuXOP1: Subitem1_3 selected");
					break;
				default:
					strcpy(message, "MenuXOP1: unknown menu item selected");
					break;
			}
			break;
			
		case SUBMENU2_ID:				// Submenu in item 3 of main menu bar.
			switch (itemNumber) {
				case 1:
					strcpy(message, "MenuXOP1: Subitem3_1 selected");
					break;
				case 2:
					// This item has a submenu so this item will never be selected.
					break;
				case 3:
					strcpy(message, "MenuXOP1: Subitem3_3 selected");
					break;
				default:
					strcpy(message, "MenuXOP1: unknown menu item selected");
					break;
			}
			break;
			
		case SUBMENU3_ID:				// Sub-submenu in item 2 of 2nd submenu.
			switch (itemNumber) {
				case 1:
					strcpy(message, "MenuXOP1: Subitem3_2_1 selected");
					break;
				case 2:
					strcpy(message, "MenuXOP1: Subitem3_2_2 selected");
					break;
				case 3:
					strcpy(message, "MenuXOP1: Subitem3_2_3 selected");
					break;
				default:
					strcpy(message, "MenuXOP1: unknown menu item selected");
					break;
			}
			break;
			
		case SUBMENU4_ID:				// Submenu in item added to Analysis menu.
			switch (itemNumber) {
				case 1:
					strcpy(message, "MenuXOP1: Analysis Item1 selected");
					break;
				case 2:
					strcpy(message, "MenuXOP1: Analysis Item2 selected");
					break;
				case 3:
					strcpy(message, "MenuXOP1: Analysis Item3 selected");
					break;
				default:
					strcpy(message, "MenuXOP1: unknown menu item selected");
					break;
			}
			break;
			
		case MISCID:		// MISCID = 8 (see IgorXOP.h).
			switch (itemNumber) {
				case 1:
					strcpy(message, "MenuXOP1: Misc1 selected");
					break;
				
				case 2:
					strcpy(message, "MenuXOP1: Misc2 selected");
					break;
			}
			break;
			
		default:
			strcpy(message, "MenuXOP1: unknown menu item selected");
			break;
	}
	
	sprintf(buf, "%s"CR_STR, message);
	XOPNotice(buf);
	
	if (isIgorMenu) {
		sprintf(buf, "mHandle=%p, actualMenuID=%d, resourceItemNumber=%d, actuaItemNumber=%d"CR_STR,
					  mHandle, actualMenuID, resourceItemNumber, actuaItemNumber);
		XOPNotice(buf);
	}
	if (isXOPMenu) {
		sprintf(buf, "mHandle=%p, resourceMenuID=%d, actualMenuID=%d, actuaItemNumber=%d"CR_STR,
					  mHandle, resourceMenuID, actualMenuID, actuaItemNumber);
		XOPNotice(buf);
	}
	
	return(0);
}

/*	XOPMenuEnable()

	Sets XOPs menu items according to current conditions.
*/
static void
XOPMenuEnable(void)
{
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP when the message specified by the
	host is other than INIT.
*/
extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;
	
	switch (GetXOPMessage()) {
		case MENUITEM:								// XOPs menu item selected.
			XOPMenuItem();
			break;

		case MENUENABLE:							// Enable/disable XOPs menu item.
			XOPMenuEnable();
			break;
			
		case EXECUTE_OPERATION:
			{
				void* params;
				params = (void*)GetXOPItem(1);
				result = ExecuteMenuXOP1((MenuXOP1RuntimeParams*)params);
			}
			break;
	}
	
	SetXOPResult(result);
}

/*	XOPMain(ioRecHandle)

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.

	main does any necessary initialization and then sets the XOPEntry field of the
	ioRecHandle to the address to be called for future messages.
*/
HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)				// The use of XOPMain rather than main means this XOP requires Igor Pro 6.20 or later
{
	int err;
	
	XOPInit(ioRecHandle);						// Do standard XOP initialization.
	if (igorVersion < 620) {
		SetXOPResult(OLD_IGOR);
		return EXIT_FAILURE;
	}
	
	if (err = RegisterMenuXOP1()) {
		SetXOPResult(err);
		return EXIT_FAILURE;
	}
	
	SetXOPEntry(XOPEntry);						// Set entry point for future calls.
	SetXOPType(RESIDENT);						// Specify XOP to stick around and to Idle.
	
	mainMenuHandle = ResourceMenuIDToMenuHandle(MAINMENU_ID);
	
	SetXOPResult(0L);
	return EXIT_SUCCESS;
}
