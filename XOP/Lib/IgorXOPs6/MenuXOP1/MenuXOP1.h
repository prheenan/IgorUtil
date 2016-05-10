/*
 *	MenuXOP1.h
 *		equates for MenuXOP1 XOP
 *
 */
 
/* MenuXOP1 custom error codes */

#define MAIN_MENU_ONLY 1 + FIRST_XOP_ERR
#define BAD_MENU_ID 2 + FIRST_XOP_ERR
#define BAD_ITEM_NUM 3 + FIRST_XOP_ERR
#define OLD_IGOR 4 + FIRST_XOP_ERR

#define FIRST_ERR MAIN_MENU_ONLY
#define LAST_ERR OLD_IGOR


/* MenuXOP1 miscellaneous equates */


/* Prototypes */
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);
