#ifndef __HEXFD_H__
#define __HEXFD_H__

#include "wincl.h"
#include "resource.h"
#include <stdio.h>

#define WC_MAIN "MainWindow"
#define WC_SEARCH "SearchBox"
#define WC_REPLACE "ReplaceBox"
#define WC_GOTO "GotoBox"
#define WC_HEXVIEW "HexView"
#define WC_HEXTABCONTROL "HexTabControl"
#define WC_HEXTOOLBARCONTROL "HexToolbarControl"
#define WC_HEXCOLORPICKER "HexColorPicker"
#define WC_HEXSETTINGS "HexSettings"
#define WC_HEXSETTINGS_FONT "HexSettingsFont"
#define WC_HEXSETTINGS_COLOR "HexSettingsColor"
#define WC_HEXSETTINGS_TAB_COLOR "HexSettingsTabColor"

enum {
    HCP_SETCOLOR = 0xF000,
    HCP_GETCOLOR,

    WM_APPLYSETTINGS,
    WM_UPDATESETTINGS,

    HEXV_UNDO,
    HEXV_REDO,
    HEXV_REPLACESELECTION,
    HEXV_MAKESELECTION,
    HEXV_SETCARET,
    HEXV_DOCHANGE,
    HEXV_SETINFO,

    HEXTC_GETCURSEL,
    HEXTC_SETCURSEL,
    HEXTC_REMOVETAB,
    HEXTC_INSERTTAB,
    HEXTC_GETITEMCOUNT,
    HEXTC_GETITEM,
    HEXTC_GETITEMINFO,
    HEXTC_RENAMETAB,
    HEXTC_UPDATERECENCY,
    HEXTC_REMEASURE,

    HEXTB_INSERTITEM,
    HEXTB_SETITEMS
};

LRESULT CALLBACK MainWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SearchProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ReplaceProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GotoProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HexViewProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HexTabControlProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HexToolbarControlProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HexColorPickerProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HexSettingsProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HexSettingsFontProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HexSettingsColorProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK HexSettingsTabColorProc(HWND, UINT, WPARAM, LPARAM);

typedef struct MARKTag {
    UINT index;
    COLORREF color;
} MARK;

typedef enum {
    URC_SET,
    URC_INSERT,
    URC_DELETE,
    URC_DELINS
} urcmode_t;

typedef struct {
    urcmode_t mode;
    UINT index;
    UINT subIndex;
    UCHAR *lastChars;
    UINT lastCharCount;
    UCHAR *chars;
    UINT charCount;
} URCHANGE;
// UNDO REDO CHANGE

typedef struct {
    WORD addressWidth;
    LONG hexWidth;
    WORD dumpOffset;
    LONG dumpStart;
    WORD charWidth;
    WORD charHeight;
    LONG totalWidth;
    LONG totalHeight;
} HEXMEASUREMENTS;

#define CHANGE_GROW_FACTOR 2

typedef struct {
    char *filePath;
    MARK *marks;
    UINT markCount;
    UINT columnCount;
    UINT radix;
    UINT addressCount;
    UINT charGap;
    UINT charLength;
    UINT caretIndex;
    UINT subCaretIndex;
    BOOL dumpPressed;
    UINT xCurrentScroll;
    UINT yCurrentScroll;
} HEXVIEWSKELETON;

typedef struct {
    TCITEMHEADER tcih;
    char *filePath;
    BOOL isRecentFile; // tells us if the file in file path is in sync with the hex file
    UCHAR *chunk;
    UINT chunkCount;
    UINT columnCount;
    UINT radix;
    UINT addressCount;
    UINT charGap;
    UINT charLength;
    UINT chunkCapacity;
    BOOL dumpPressed;
    UINT caretIndex;
    UINT selectIndex;
    UINT subCaretIndex;
    BOOL isSelection;
    MARK *marks;
    UINT markCount;
    HEXMEASUREMENTS measurements;
    HBITMAP buffer;
    UINT bufferWidth, bufferHeight;
    UINT xMinScroll;       // minimum horizontal scroll value
    UINT xCurrentScroll;   // current horizontal scroll value
    UINT xMaxScroll;       // maximum horizontal scroll value
    UINT yMinScroll;       // minimum vertical scroll value
    UINT yCurrentScroll;   // current vertical scroll value
    UINT yMaxScroll;       // maximum vertical scroll value
    URCHANGE *changes;
    UINT changeIndex;
    UINT changeCount;
    UINT changeRecentSpot; // spot where the file seen on screen is the same as the saved file
    UINT changeCapacity;
} HEXVIEWINFO;

typedef struct {
    HFONT globalFont;
    UINT globalFontIndex;
    UINT hexFontSize;
    COLORREF hexTextColor;
    COLORREF hexBackgroundColor;
    COLORREF hexForegroundColor;
    COLORREF hexCaretColor;
    COLORREF hexSelectionColor;
    COLORREF hexMarkerColor;
    UINT hexDefRadix;
    UINT hexDefCharLength;
    UINT hexDefCharGap;
    UINT hexDefColumnCount;
    UINT hexDefAddressCount;
    UINT hexTabWidth;
    UINT hexTabHeight;
    UINT hexTabPadding;
    UINT hexTabFitMode;
    COLORREF hexTabTextColor;
    COLORREF hexTabTopColor;
    COLORREF hexTabBottomColor;
    COLORREF hexTabBackgroundColor;
    COLORREF hexTabXButtonColor;
    HBRUSH hexTabBackgroundBrush;
    HBRUSH hexTabBrush;
    HPEN hexTabLinePen;
} HEXSTYLE;

HEXSTYLE *Manager_GetStyle(void);
void Manager_SetStyle(const HEXSTYLE*);

void Manager_Init(void);
void Manager_Destroy(void);
HWND Manager_GetMainWindow(void);
HWND Manager_GetToolbar(void);
HWND Manager_GetGotoBox(void);
HWND Manager_GetSearchBox(void);
HWND Manager_GetReplaceBox(void);
HWND Manager_GetSettingsBox(void);
HWND Manager_GetHexView(void);
HWND Manager_GetTabControl(void);
HWND Manager_GetStatusBar(void);
void Manager_AddFont(HFONT);
BOOL Manager_SaveActiveHexViewInfoFile(void);
void HVI_ManagerRoutine46(HEXVIEWINFO*, const RECT*);
void Manager_SetActiveHexViewInfo(HEXVIEWINFO*);
HEXVIEWINFO *Manager_CreateHexViewInfo(HEXVIEWINFO*, char*);
HEXVIEWINFO *Manager_CreateAndSetInfoExt(HEXVIEWINFO*, HEXVIEWSKELETON*);
HEXVIEWINFO *Manager_GetActiveHexViewInfo(void);

void InitClasses(void);

// hex settings box
enum {
    IDHS_APPLY = 1,
    IDHS_REQUESTSTYLE,
    IDHS_BADDFONT,
    IDHS_ADDFONT,
    IDHS_SETFONT,
    IDHS_SETFONTSIZE,
    IDHS_SETTEXTCOLOR,
    IDHS_SETBACKGROUNDCOLOR,
    IDHS_SETFOREGROUNDCOLOR,
    IDHS_SETCARETCOLOR,
    IDHS_SETSELECTCOLOR,
    IDHS_SETMARKERCOLOR,
    IDHS_SETTABBACKGROUNDCOLOR,
    IDHS_SETTABGRADIENTTOPCOLOR,
    IDHS_SETTABGRADIENTBOTTOMCOLOR,
    IDHS_SETTABXBUTTONCOLOR,
    IDHS_SETTABTEXTCOLOR,
    IDHS_SETTABWIDTH,
    IDHS_SETTABHEIGHT,
    IDHS_SETTABPADDING,
    IDHS_SETTABFITMODE
};

// tab control
typedef struct {
    char *name;
    HEXVIEWINFO *hexInfo;
    RECT rcTab;
} HEXTABINFO;

#define TCLSP_TFM_FIXED 0
#define TCLSP_TFM_MATCH_TEXT 1
#define TCLSP_TFM_MAX_TABS 2

// toolbar control
#define HTB_PRESSED 1
#define HTB_SEPERATOR 2

typedef struct {
    char *name;
    HBITMAP image;
    int id;
    RECT rcIcon;
    int flags;
} HEXTBBUTTON;

#define IDM_NEW         0x01
#define IDM_OPEN        0x02
#define IDM_CLOSE       0x03
#define IDM_CLOSE_ALL   0x04
#define IDM_SAVE        0x05
#define IDM_SAVE_AS     0x06
#define IDM_COPY        0x10
#define IDM_PASTE       0x11
#define IDM_CUT         0x12
#define IDM_UNDO        0x13
#define IDM_REDO        0x14
#define IDM_SETRADIX    0x20
#define IDM_SETCOLUMNS  0x21
#define IDM_SEARCH      0x31
#define IDM_REPLACE     0x32
#define IDM_GOTO        0x33
#define IDM_TYPEINSERT  0x40
#define IDM_SPINSERT    0x41
#define IDM_ENVIRONMENT 0x42

HWND CreateToolbar(HWND);
HMENU CreateMenuBar(void);
HWND CreateStatusBar(HWND);

#endif // __HEXFD_H__

