#ifndef __HESAVE_H__
#define __HESAVE_H__

typedef struct {
    char *filePath;
    UINT index;
    UINT subIndex;
    BOOL dumpPressed;
    MARK *marks;
    UINT markCount;
} HEXVIEWSKELETON;

typedef struct {
    UINT tabCount;
    UINT tabIndex;
    const char **tabNames;
    HEXVIEWSKELETON *hexViewSkeletons;
    RECT windowPos;
    // dialog boxes
    // search box
    const char *searchTextField;
    BOOL searchCaseSensitive;
    BOOL searchLoopBack;
    BOOL searchNumberMode;
    BOOL searchStartOfFile;
    RECT searchPos;
    // replace box
    const char *replaceTextField;
    const char *replaceTextField2;
    BOOL replaceCaseSensitive;
    BOOL replaceLoopBack;
    BOOL replaceNumberMode;
    BOOL replaceStartOfFile;
    RECT replacePos;
    // goto box
    const char *gotoTextField;
    RECT gotoPos;
    HEXSTYLE *style;
} HEXSTATE;

void HEX_SaveState(HWND, HWND, HWND, HWND);
void HEX_LoadState(HEXSTATE*);

#endif // __HESAVE_H__

