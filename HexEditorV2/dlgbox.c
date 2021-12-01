#include "wincomps.h"

static char TextBuffer[MAX_PATH];

typedef struct {
    HWND hWnd;
    RECT rcTab;
    HBITMAP hBmp;
    BOOL isApplied;
} HEXSETTINGSTAB;

static HEXSETTINGSTAB Tabs[4];

HWND Settings_GetTab(int tabIndex)
{
    return (Tabs + tabIndex)->hWnd;
}

#define IDS_SEARCH 1
#define IDS_STARTOFFILE 2
#define IDS_FROMCURSOR 3
#define IDS_CASESENSITIVE 4
#define IDS_LOOPBACK 5
#define IDS_NUMBERMODE 6

static size_t GetSearchText(HWND parent, HWND edit, char *result, int max, BOOL numberMode)
{
    GetWindowText(edit, result, max);
    size_t len = strlen(result);
    if(!len)
        return 0;
    if(numberMode)
    {
        char copy[len];
        char *cc = copy;
        char *cres = result;
        char cr;
        size_t c = 2;
        size_t newLen = 0;
        char hx[2];
        while(len--)
        {
            cr = *(cres++);
            if(isspace(cr))
            {
                if(c < 2)
                {
                    *(cc++) = c == 1 ? hx[1] : (hx[1] << 4) | hx[0];
                    c = 2;
                    hx[0] = 0;
                    newLen++;
                }
            }
            else
            {
                cr = cr >= '0' && cr <= '9' ? cr - '0' :
                     cr >= 'a' && cr <= 'f' ? cr - 'a' + 10 :
                     cr >= 'A' && cr <= 'F' ? cr - 'A' + 10 : -1;
                if(cr < 0 || !c)
                {
                    MessageBox(parent, "Invalid number format", "Error", MB_OK | MB_ICONERROR);
                    return 0;
                }
                c--;
                hx[c] = cr;
            }
        }
        if(c < 2)
        {
            *(cc++) = c == 1 ? hx[1] : (hx[1] << 4) | hx[0];
            newLen++;
        }
        *cc = 0;
        memcpy(result, copy, (len = newLen) + 1);
    }
    return len;
}

static inline BOOL CompareChar(UCHAR *c1, UCHAR *c2, BOOL ignoreCase)
{
    return ignoreCase ? tolower(*c1) == tolower(*c2) : *c1 == *c2;
}

static INT64 SearchString(UCHAR *bytes, UINT byteCnt, UCHAR *search, UINT sLen, UINT startFrom, BOOL ignoreCase)
{
    bytes += startFrom;
    byteCnt -= startFrom;
    const UINT bc = byteCnt;
    const UINT sl = sLen;
    UCHAR *sr = search;
    BOOL b;
    while(byteCnt--)
    {
        if(CompareChar(bytes, search, ignoreCase))
        { sLen--; search++; }
        else
        { b = CompareChar(bytes, sr, ignoreCase);
            sLen = sl - b; search = sr + b; }
        if(!sLen)
            return bc - byteCnt - sl + startFrom;
        bytes++;
    }
    return -1;
}

LRESULT CALLBACK SearchProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND searchEdit;
    HEXVIEWINFO *hvi;
    RECT r;
    switch(msg)
    {
    case WM_CLOSE: ShowWindow(hWnd, SW_HIDE); return 0;
    case WM_CREATE:
    {
        GetClientRect(hWnd, &r);
        const int buttonHeight = 50;
        const int buttonY = r.bottom - buttonHeight - 10;
        searchEdit =
        CreateWindow("Edit", "", WS_BORDER | WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
                     10, 20, r.right - 10 - 10, 30, hWnd, NULL, NULL, NULL);
        CreateWindow("Button", "Case Sensitive", WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                     10, 60, 130, buttonHeight / 2, hWnd, (HMENU) IDS_CASESENSITIVE, NULL, NULL);
        CreateWindow("Button", "Number mode", WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                     r.right - 140, 60, 130, buttonHeight / 2, hWnd, (HMENU) IDS_NUMBERMODE, NULL, NULL);
        CreateWindow("Button", "Loop back", WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                     10, 60 + buttonHeight / 2, 130, buttonHeight / 2, hWnd, (HMENU) IDS_LOOPBACK, NULL, NULL);
        CreateWindow("Button", "Start of file", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
                     150, buttonY, 130, buttonHeight / 2, hWnd, (HMENU) IDS_STARTOFFILE, NULL, NULL);
        CreateWindow("Button", "From cursor", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
                     150, buttonY + 25, 130, buttonHeight / 2, hWnd, (HMENU) IDS_FROMCURSOR, NULL, NULL);
        CreateWindow("Button", "Search", WS_VISIBLE | WS_CHILD,
                     10, buttonY, 130, buttonHeight, hWnd, (HMENU) IDS_SEARCH, NULL, NULL);
        CheckDlgButton(hWnd, IDS_CASESENSITIVE, 1);
        CheckDlgButton(hWnd, IDS_LOOPBACK, 1);
        CheckDlgButton(hWnd, IDS_STARTOFFILE, 1);
        return 0;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDS_SEARCH:
        {
            if(!(hvi = Manager_GetActiveHexViewInfo()))
                return 0;
            size_t matchLen = GetSearchText(hWnd, searchEdit, TextBuffer, sizeof(TextBuffer), IsDlgButtonChecked(hWnd, IDS_NUMBERMODE));
            if(!matchLen)
                return 0;
            UINT start = IsDlgButtonChecked(hWnd, IDS_FROMCURSOR) ? hvi->caretIndex + 1 : 0;
            BOOL ignoreCase = !IsDlgButtonChecked(hWnd, IDS_CASESENSITIVE);
            BOOL loopBack = IsDlgButtonChecked(hWnd, IDS_LOOPBACK);
            INT64 s;
            if((s = SearchString(hvi->chunk, hvi->chunkCount, (UCHAR*) TextBuffer, matchLen, start, ignoreCase)) >= 0
                || (loopBack && (s = SearchString(hvi->chunk, hvi->chunkCount, (UCHAR*) TextBuffer, matchLen, 0, ignoreCase)) >= 0))
                SendMessage(Manager_GetHexView(), HEXV_SETCARET, s, 0);
            else
                MessageBox(hWnd, "No match found!", "Search failed", MB_OK | MB_ICONWARNING);
            return 0;
        }
        case IDS_STARTOFFILE:
            CheckDlgButton(hWnd, IDS_STARTOFFILE, 1);
            CheckDlgButton(hWnd, IDS_FROMCURSOR, 0);
            return 0;
        case IDS_FROMCURSOR:
            CheckDlgButton(hWnd, IDS_STARTOFFILE, 0);
            CheckDlgButton(hWnd, IDS_FROMCURSOR, 1);
            return 0;
        case IDS_CASESENSITIVE:
            CheckDlgButton(hWnd, IDS_CASESENSITIVE, !IsDlgButtonChecked(hWnd, IDS_CASESENSITIVE));
            return 0;
        case IDS_LOOPBACK:
            CheckDlgButton(hWnd, IDS_LOOPBACK, !IsDlgButtonChecked(hWnd, IDS_LOOPBACK));
            return 0;
        case IDS_NUMBERMODE:
            CheckDlgButton(hWnd, IDS_NUMBERMODE, !IsDlgButtonChecked(hWnd, IDS_NUMBERMODE));
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

#define IDS_REPLACE 8

LRESULT CALLBACK ReplaceProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND searchEdit, replaceEdit;
    HEXVIEWINFO *hvi;
    URCHANGE urc;
    RECT r;
    switch(msg)
    {
    case WM_CLOSE: ShowWindow(hWnd, SW_HIDE); return 0;
    case WM_CREATE:
    {
        GetClientRect(hWnd, &r);
        const int buttonHeight = 50;
        const int buttonY = r.bottom - buttonHeight - 10;
        searchEdit =
        CreateWindow("Edit", NULL, WS_BORDER | WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
                     10, 20, r.right - 10 - 10, 30, hWnd, NULL, NULL, NULL);
        CreateWindow("Button", "Case Sensitive", WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                     10, 60, 130, buttonHeight / 2, hWnd, (HMENU) IDS_CASESENSITIVE, NULL, NULL);
        CreateWindow("Button", "Loop back", WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                     10, 60 + buttonHeight / 2, 130, buttonHeight / 2, hWnd, (HMENU) IDS_LOOPBACK, NULL, NULL);
        CreateWindow("Button", "Number mode", WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                     r.right - 140, 60, 130, buttonHeight / 2, hWnd, (HMENU) IDS_NUMBERMODE, NULL, NULL);
        CreateWindow("Button", "Start of file", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
                     150, buttonY, 130, buttonHeight / 2, hWnd, (HMENU) IDS_STARTOFFILE, NULL, NULL);
        CreateWindow("Button", "From cursor", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
                     150, buttonY + 25, 130, buttonHeight / 2, hWnd, (HMENU) IDS_FROMCURSOR, NULL, NULL);
        CreateWindow("Button", "Replace", WS_VISIBLE | WS_CHILD,
                     10, buttonY, 130, buttonHeight, hWnd, (HMENU) IDS_REPLACE, NULL, NULL);
        replaceEdit =
        CreateWindow("Edit", NULL, WS_BORDER | WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
                     10, 60 + buttonHeight + 10, r.right - 10 - 10, 30, hWnd, NULL, NULL, NULL);
        CheckDlgButton(hWnd, IDS_CASESENSITIVE, 1);
        CheckDlgButton(hWnd, IDS_LOOPBACK, 1);
        CheckDlgButton(hWnd, IDS_STARTOFFILE, 1);
        return 0;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDS_REPLACE:
        {
            if(!(hvi = Manager_GetActiveHexViewInfo()))
                return 0;
            UINT matchLen = GetSearchText(hWnd, searchEdit, TextBuffer, sizeof(TextBuffer), IsDlgButtonChecked(hWnd, IDS_NUMBERMODE));
            if(!matchLen)
                return 0;
            char replace[260];
            UINT replaceLen = GetSearchText(hWnd, replaceEdit, replace, sizeof(replace), IsDlgButtonChecked(hWnd, IDS_NUMBERMODE));

            UINT start = IsDlgButtonChecked(hWnd, IDS_FROMCURSOR) ? hvi->caretIndex + 1 : 0;
            BOOL ignoreCase = !IsDlgButtonChecked(hWnd, IDS_CASESENSITIVE);
            BOOL loopBack = IsDlgButtonChecked(hWnd, IDS_LOOPBACK);
            INT64 s;
            UINT matches = 0;
            while(1)
            {
                if((s = SearchString(hvi->chunk, hvi->chunkCount, (UCHAR*) TextBuffer, matchLen, start, ignoreCase)) < 0)
                {
                    if(loopBack)
                    {
                        start = 0;
                        loopBack = FALSE;
                        continue;
                    }
                    break;
                }
                UINT index = (UINT) s;
                SendMessage(Manager_GetHexView(), HEXV_SETCARET, index, 0);
                int mb = MessageBox(hWnd, "Do you want to replace this?", "Replace", MB_YESNO | MB_ICONQUESTION);
                matches++;
                start = index + replaceLen - 1;
                if(mb == IDYES)
                {
                    urc.index = index;
                    urc.subIndex = hvi->subCaretIndex;
                    urc.lastCharCount = matchLen;
                    if(!replaceLen)
                    {
                        urc.mode = URC_DELETE;
                    }
                    else
                    {
                        urc.mode = URC_DELINS;
                        urc.chars = (UCHAR*) replace;
                        urc.charCount = replaceLen;
                    }
                    SendMessage(Manager_GetHexView(), HEXV_DOCHANGE, (WPARAM) &urc, 0);
                    SendMessage(Manager_GetHexView(), HEXV_SETCARET, start, 0);
                }
            }
            if(!matches)
                MessageBox(hWnd, "No match found!", "Search failed!", MB_OK | MB_ICONWARNING);
            return 0;
        }
        case IDS_STARTOFFILE:
            CheckDlgButton(hWnd, IDS_STARTOFFILE, 1);
            CheckDlgButton(hWnd, IDS_FROMCURSOR, 0);
            return 0;
        case IDS_FROMCURSOR:
            CheckDlgButton(hWnd, IDS_STARTOFFILE, 0);
            CheckDlgButton(hWnd, IDS_FROMCURSOR, 1);
            return 0;
        case IDS_CASESENSITIVE:
            CheckDlgButton(hWnd, IDS_CASESENSITIVE, !IsDlgButtonChecked(hWnd, IDS_CASESENSITIVE));
            return 0;
        case IDS_LOOPBACK:
            CheckDlgButton(hWnd, IDS_LOOPBACK, !IsDlgButtonChecked(hWnd, IDS_LOOPBACK));
            return 0;
        case IDS_NUMBERMODE:
            CheckDlgButton(hWnd, IDS_NUMBERMODE, !IsDlgButtonChecked(hWnd, IDS_NUMBERMODE));
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

#define IDS_GOTO 1
#define IDS_GOBACK 2

LRESULT CALLBACK GotoProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND gotoEdit;
    static UINT goBackJump = 0;
    HEXVIEWINFO *hvi;
    switch(msg)
    {
    case WM_CLOSE: ShowWindow(hWnd, SW_HIDE); return 0;
    case WM_CREATE:
    {
        RECT r;
        GetClientRect(hWnd, &r);
        const int buttonHeight = 50;
        gotoEdit =
        CreateWindow("Edit", NULL, WS_BORDER | WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_NUMBER,
                     10, 20, r.right - 10 - 10, 30, hWnd, NULL, NULL, NULL);
        CreateWindow("Button", "Goto", WS_VISIBLE | WS_CHILD,
                     10, 60, r.right - 10 - 10, buttonHeight, hWnd, (HMENU) IDS_GOTO, NULL, NULL);
        CreateWindow("Button", "Go back", WS_VISIBLE | WS_CHILD,
                     10, 60 + buttonHeight + 10, r.right - 10 - 10, buttonHeight, hWnd, (HMENU) IDS_GOBACK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDS_GOTO:
            if((hvi = Manager_GetActiveHexViewInfo()) && hvi->chunkCount)
            {
                goBackJump = hvi->caretIndex;
                GetWindowText(gotoEdit, TextBuffer, MAX_PATH);
                UINT g = atoi(TextBuffer);
                g = min(g, hvi->chunkCount - 1);
                SendMessage(Manager_GetHexView(), HEXV_SETCARET, g, 0);
            }
            return 0;
        case IDS_GOBACK:
            if((hvi = Manager_GetActiveHexViewInfo()) && hvi->chunkCount)
            {
                UINT gbj = hvi->caretIndex;
                SendMessage(Manager_GetHexView(), HEXV_SETCARET, min(goBackJump, hvi->chunkCount - 1), 0);
                goBackJump = gbj;
            }
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK HexSettingsFontProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND fontSizeEdit;
    static HWND comboBox;
    int index, cnt;
    HFONT hFont;
    LOGFONT font;
    CHOOSEFONT cf;
    switch(msg)
    {
    case WM_CREATE:
        comboBox = CreateWindow(WC_COMBOBOX, NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS, 10, 10, 175, 400, hWnd, NULL, NULL, NULL);
        CreateWindow(WC_BUTTON, "Add font", WS_VISIBLE | WS_CHILD, 195, 10, 90, 25, hWnd, (HMENU) IDHS_BADDFONT, NULL, NULL);
        cnt = Resource_GetFontCount();
        index = 0;
        while(cnt--)
        {
            hFont = Resource_GetFont(index);
            GetObject(hFont, sizeof(font), &font);
            SendMessage(comboBox, CB_ADDSTRING, (WPARAM) 0, (LPARAM) font.lfFaceName);
            index++;
        }
        // TODO: REMOVE THIS
        SendMessage(comboBox, CB_SETCURSEL, (WPARAM) 0, (LPARAM) 0);
        CreateWindow(WC_STATIC, "Font size: ", WS_VISIBLE | WS_CHILD | ES_NUMBER, 10, 40, 100, 16, hWnd, NULL, NULL, NULL);
        fontSizeEdit = CreateWindow(WC_EDIT, "16", WS_VISIBLE | WS_CHILD, 110, 40, 175, 16, hWnd, NULL, NULL, NULL);
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_COMMAND:
        switch(wParam)
        {
        case IDHS_REQUESTSTYLE:
            ((HEXSTYLE*) lParam)->globalFontIndex = SendMessage(comboBox, CB_GETCURSEL, 0, 0);
            ((HEXSTYLE*) lParam)->globalFont = Resource_GetFont(((HEXSTYLE*) lParam)->globalFontIndex);
            GetWindowText(fontSizeEdit, TextBuffer, sizeof(TextBuffer));
            ((HEXSTYLE*) lParam)->hexFontSize = atoi(TextBuffer);
            return 0;
        case IDHS_BADDFONT:
            memset(&cf, 0, sizeof(cf));
            cf.lStructSize = sizeof (cf);
            cf.lpLogFont = &font;
            cf.Flags = CF_SCREENFONTS | CF_EFFECTS;

            if(ChooseFont(&cf))
            {
                font.lfHeight = Manager_GetStyle()->hexFontSize;
                font.lfWidth = 0;
                SendMessage(comboBox, CB_ADDSTRING, (WPARAM) 0, (LPARAM) font.lfFaceName);
                hFont = CreateFontIndirect(&font);
                Manager_AddFont(hFont);
            }
            return 0;
        case IDHS_ADDFONT:
            SendMessage(comboBox, CB_ADDSTRING, 0, lParam);
            return 0;
        case IDHS_SETFONT:
            SendMessage(comboBox, CB_SETCURSEL, lParam, 0);
            return 0;
        case IDHS_SETFONTSIZE:
            sprintf(TextBuffer, "%d", (int) lParam);
            SetWindowText(fontSizeEdit, TextBuffer);
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK HexSettingsColorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND textColorPicker, backColorPicker, foreColorPicker, caretColorPicker, selectColorPicker, markerColorPicker;
    switch(msg)
    {
    case WM_CREATE:
        textColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Text Color", WS_VISIBLE | WS_CHILD, 10, 10, 275, 30, hWnd, NULL, NULL, NULL);
        backColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Background Color", WS_VISIBLE | WS_CHILD, 10, 45, 275, 30, hWnd, NULL, NULL, NULL);
        foreColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Foreground Color", WS_VISIBLE | WS_CHILD, 10, 80, 275, 30, hWnd, NULL, NULL, NULL);
        caretColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Caret Color", WS_VISIBLE | WS_CHILD, 10, 115, 275, 30, hWnd, NULL, NULL, NULL);
        selectColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Selection Color", WS_VISIBLE | WS_CHILD, 10, 150, 275, 30, hWnd, NULL, NULL, NULL);
        markerColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Marker Color", WS_VISIBLE | WS_CHILD, 10, 185, 275, 30, hWnd, NULL, NULL, NULL);
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDHS_REQUESTSTYLE:
            ((HEXSTYLE*) lParam)->hexTextColor = SendMessage(textColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexBackgroundColor = SendMessage(backColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexForegroundColor = SendMessage(foreColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexCaretColor = SendMessage(caretColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexSelectionColor = SendMessage(selectColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexMarkerColor = SendMessage(markerColorPicker, HCP_GETCOLOR, 0, 0);
            return 0;
        case IDHS_SETTEXTCOLOR: SendMessage(textColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETBACKGROUNDCOLOR: SendMessage(backColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETFOREGROUNDCOLOR: SendMessage(foreColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETCARETCOLOR: SendMessage(caretColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETSELECTCOLOR: SendMessage(selectColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETMARKERCOLOR: SendMessage(markerColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK HexSettingsTabColorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND backgroundColorPicker, gradientTopColorPicker, gradientBottomColorPicker, xButtonColorPicker, textColorPicker;
    static HWND tabWidthEdit, tabHeightEdit, tabPaddingEdit;
    static HWND fitModeComboBox;
    switch(msg)
    {
    case WM_CREATE:
        backgroundColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Background Color", WS_VISIBLE | WS_CHILD, 10, 10, 275, 30, hWnd, NULL, NULL, NULL);
        gradientTopColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Top Gradient Color", WS_VISIBLE | WS_CHILD, 10, 45, 275, 30, hWnd, NULL, NULL, NULL);
        gradientBottomColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Bottom Gradient Color", WS_VISIBLE | WS_CHILD, 10, 80, 275, 30, hWnd, NULL, NULL, NULL);
        xButtonColorPicker = CreateWindow(WC_HEXCOLORPICKER, "X Button Color", WS_VISIBLE | WS_CHILD, 10, 115, 275, 30, hWnd, NULL, NULL, NULL);
        textColorPicker = CreateWindow(WC_HEXCOLORPICKER, "Text Color", WS_VISIBLE | WS_CHILD, 10, 150, 275, 30, hWnd, NULL, NULL, NULL);

        CreateWindow(WC_STATIC, "Tab Width: ", WS_VISIBLE | WS_CHILD | ES_NUMBER, 10, 200, 275, 16, hWnd, NULL, NULL, NULL);
        tabWidthEdit = CreateWindow(WC_EDIT, "250", WS_VISIBLE | WS_CHILD, 100, 200, 175, 16, hWnd, NULL, NULL, NULL);

        CreateWindow(WC_STATIC, "Tab Height: ", WS_VISIBLE | WS_CHILD | ES_NUMBER, 10, 221, 275, 16, hWnd, NULL, NULL, NULL);
        tabHeightEdit = CreateWindow(WC_EDIT, "22", WS_VISIBLE | WS_CHILD, 100, 221, 175, 16, hWnd, NULL, NULL, NULL);

        CreateWindow(WC_STATIC, "Tab Padding: ", WS_VISIBLE | WS_CHILD | ES_NUMBER, 10, 242, 275, 16, hWnd, NULL, NULL, NULL);
        tabPaddingEdit = CreateWindow(WC_EDIT, "5", WS_VISIBLE | WS_CHILD, 100, 242, 175, 16, hWnd, NULL, NULL, NULL);

        fitModeComboBox = CreateWindow(WC_COMBOBOX, NULL, WS_VISIBLE | WS_CHILD | CBS_HASSTRINGS | CBS_DROPDOWNLIST, 10, 263, 275, 230, hWnd, NULL, NULL, NULL);
        SendMessage(fitModeComboBox, CB_ADDSTRING, 0, (LPARAM) "Fixed Width");
        SendMessage(fitModeComboBox, CB_ADDSTRING, 0, (LPARAM) "Match Text");
        SendMessage(fitModeComboBox, CB_ADDSTRING, 0, (LPARAM) "Max Tabs");
        SendMessage(fitModeComboBox, CB_ADDSTRING, 0, (LPARAM) "Max Tabs and Match Text");
        SendMessage(fitModeComboBox, CB_SETCURSEL, 3, 0);
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDHS_REQUESTSTYLE:
            ((HEXSTYLE*) lParam)->hexTabBackgroundColor = SendMessage(backgroundColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexTabTopColor = SendMessage(gradientTopColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexTabBottomColor = SendMessage(gradientBottomColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexTabXButtonColor = SendMessage(xButtonColorPicker, HCP_GETCOLOR, 0, 0);
            ((HEXSTYLE*) lParam)->hexTabTextColor = SendMessage(textColorPicker, HCP_GETCOLOR, 0, 0);
            GetWindowText(tabWidthEdit, TextBuffer, sizeof(TextBuffer));
            ((HEXSTYLE*) lParam)->hexTabWidth = atoi(TextBuffer);
            GetWindowText(tabHeightEdit, TextBuffer, sizeof(TextBuffer));
            ((HEXSTYLE*) lParam)->hexTabHeight = atoi(TextBuffer);
            GetWindowText(tabPaddingEdit, TextBuffer, sizeof(TextBuffer));
            ((HEXSTYLE*) lParam)->hexTabPadding = atoi(TextBuffer);
            ((HEXSTYLE*) lParam)->hexTabFitMode = SendMessage(fitModeComboBox, CB_GETCURSEL, 0, 0);
            return 0;
        case IDHS_SETTABBACKGROUNDCOLOR: SendMessage(backgroundColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETTABGRADIENTTOPCOLOR: SendMessage(gradientTopColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETTABGRADIENTBOTTOMCOLOR: SendMessage(gradientBottomColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETTABXBUTTONCOLOR: SendMessage(xButtonColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETTABTEXTCOLOR: SendMessage(textColorPicker, HCP_SETCOLOR, lParam, 0); return 0;
        case IDHS_SETTABWIDTH:
            sprintf(TextBuffer, "%d", (int) lParam);
            SetWindowText(tabWidthEdit, TextBuffer);
            return 0;
        case IDHS_SETTABHEIGHT:
            sprintf(TextBuffer, "%d", (int) lParam);
            SetWindowText(tabHeightEdit, TextBuffer);
            return 0;
        case IDHS_SETTABPADDING:
            sprintf(TextBuffer, "%d", (int) lParam);
            SetWindowText(tabPaddingEdit, TextBuffer);
            return 0;
        case IDHS_SETTABFITMODE:
            SendMessage(fitModeComboBox, CB_SETCURSEL, lParam, 0);
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK HexSettingsProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int tabIndex;
    int newTabIndex;
    int xMouse, yMouse;
    static int tabCount = sizeof(Tabs) / sizeof(*Tabs);
    static const int tabHeight = 64;
    HEXSETTINGSTAB *hst;
    int cnt;
    int width, height;
    HDC hDc;
    HDC imageDc;
    PAINTSTRUCT ps;
    HBITMAP oldHbm;
    HEXSTYLE style;
    switch(msg)
    {
    case WM_CREATE:
        CreateWindow(WC_BUTTON, "Apply", WS_VISIBLE | WS_CHILD, 200, 394, 80, 20, hWnd, (HMENU) IDHS_APPLY, NULL, NULL);
        Tabs[0] = (HEXSETTINGSTAB) {
            .hWnd = CreateWindow(WC_HEXSETTINGS_FONT, NULL, WS_VISIBLE | WS_CHILD, 200, 194, 80, 20, hWnd, NULL, NULL, NULL),
            .rcTab = (RECT) {
                .left = 0,
                .top = 0,
                .right = tabHeight,
                .bottom = tabHeight
            },
            .hBmp = Resource_GetImage(ICON_ID_FONT),
            .isApplied = TRUE
        };
        Tabs[1] = (HEXSETTINGSTAB) {
            .hWnd = CreateWindow(WC_HEXSETTINGS_COLOR, NULL, WS_CHILD, 200, 194, 80, 20, hWnd, NULL, NULL, NULL),
            .rcTab = (RECT) {
                .left = tabHeight,
                .top = 0,
                .right = tabHeight * 2,
                .bottom = tabHeight
            },
            .hBmp = Resource_GetImage(ICON_ID_COLOR),
            .isApplied = TRUE
        };
        Tabs[2] = (HEXSETTINGSTAB) {
            .hWnd = CreateWindow(WC_HEXSETTINGS_TAB_COLOR, NULL, WS_CHILD, 200, 194, 80, 20, hWnd, NULL, NULL, NULL),
            .rcTab = (RECT) {
                .left = tabHeight * 2,
                .top = 0,
                .right = tabHeight * 3,
                .bottom = tabHeight
            },
            .hBmp = Resource_GetImage(ICON_ID_TAB_COLOR),
            .isApplied = TRUE
        };
        return 0;
    case WM_CLOSE: ShowWindow(hWnd, SW_HIDE); return 1;
    case WM_SIZE:
        width = LOWORD(lParam);
        height = HIWORD(lParam);
        hst = Tabs;
        cnt = tabCount;
        while(cnt--)
        {
            MoveWindow(hst->hWnd, 0, tabHeight, width, height, TRUE);
            hst++;
        }
        return 0;
    case WM_PAINT:
        hDc = BeginPaint(hWnd, &ps);
        imageDc = CreateCompatibleDC(hDc);
        oldHbm = SelectObject(imageDc, NULL);
        hst = Tabs;
        cnt = tabCount;
        while(cnt)
        {
            if(tabCount - cnt == tabIndex)
                FillRect(hDc, &hst->rcTab, GetStockObject(LTGRAY_BRUSH));
            else
                FillRect(hDc, &hst->rcTab, GetStockObject(GRAY_BRUSH));
            SelectObject(imageDc, hst->hBmp);
            TransparentBlt(hDc, hst->rcTab.left, hst->rcTab.top, hst->rcTab.right - hst->rcTab.left, hst->rcTab.bottom - hst->rcTab.top,
                           imageDc, 0, 0, 16, 16, 0xFFFFFF);
            hst++;
            cnt--;
        }
        SelectObject(imageDc, oldHbm);
        DeleteDC(imageDc);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDHS_APPLY:
            style = *Manager_GetStyle();
            for(int i = 0; i < tabCount; i++)
                SendMessage((Tabs + i)->hWnd, WM_COMMAND, IDHS_REQUESTSTYLE, (LPARAM) &style);
            Manager_SetStyle(&style);
            break;
        default:
            for(int i = 0; i < tabCount; i++)
                SendMessage((Tabs + i)->hWnd, WM_COMMAND, wParam, lParam);
        }
        return 0;
    case WM_LBUTTONDOWN:
        xMouse = GET_X_LPARAM(lParam);
        yMouse = GET_Y_LPARAM(lParam);
        if(yMouse < tabHeight)
        {
            newTabIndex = xMouse / tabHeight;
            newTabIndex = min(newTabIndex, tabCount - 1);
            if(newTabIndex != tabIndex)
            {
                // hide old window
                ShowWindow((Tabs + tabIndex)->hWnd, SW_HIDE);
                // show new window
                ShowWindow((Tabs + newTabIndex)->hWnd, SW_NORMAL);
                tabIndex = newTabIndex;
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
            }
        }
        return 0;

    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
