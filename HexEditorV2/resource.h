#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include <windows.h>

#define ICON_COUNT 20
#define ICON_SIZE 16
#define ICON_ID_NEW 0
#define ICON_ID_CLOSE 1
#define ICON_ID_CLOSE_ALL 1
#define ICON_ID_COPY 2
#define ICON_ID_PASTE 3
#define ICON_ID_CUT 4
#define ICON_ID_UNDO 5
#define ICON_ID_REDO 6
#define ICON_ID_OPEN 7
#define ICON_ID_SAVE 8
#define ICON_ID_SAVE_AS 8
#define ICON_ID_SEARCH 10
#define ICON_ID_REPLACE 11
#define ICON_ID_GOTO 12
#define ICON_ID_FONT 13
#define ICON_ID_COLOR 14
#define ICON_ID_ARROW_LEFT 15
#define ICON_ID_ARROW_RIGHT 16
#define ICON_ID_PLUS 17
#define ICON_ID_RED_X 18
#define ICON_ID_TAB_COLOR 19

void Resource_AddFont(HFONT);
void Resource_Init(void);
int Resource_GetFontCount(void);
HFONT Resource_GetFont(int);
HBITMAP Resource_GetImage(int);
const char *Resource_GetHexEditorFolder(void);

#endif // __RESOURCE_H__

