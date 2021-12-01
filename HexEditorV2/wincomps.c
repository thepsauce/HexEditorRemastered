#include "wincomps.h"

void InitClasses(void)
{
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpszClassName = WC_MAIN;
    wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
    wc.lpfnWndProc = MainWindowProc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
    wc.lpszClassName = WC_SEARCH;
    wc.lpfnWndProc = SearchProc;
    RegisterClass(&wc);
    wc.lpszClassName = WC_REPLACE;
    wc.lpfnWndProc = ReplaceProc;
    RegisterClass(&wc);
    wc.lpszClassName = WC_GOTO;
    wc.lpfnWndProc = GotoProc;
    RegisterClass(&wc);
    wc.lpszClassName = WC_HEXVIEW;
    wc.lpfnWndProc = HexViewProc;
    wc.hbrBackground = GetStockObject(DKGRAY_BRUSH);
    RegisterClass(&wc);
    wc.style = CS_DBLCLKS;
    wc.lpszClassName = WC_HEXTABCONTROL;
    wc.lpfnWndProc = HexTabControlProc;
    RegisterClass(&wc);
    wc.style = 0;
    wc.lpszClassName = WC_HEXTOOLBARCONTROL;
    wc.lpfnWndProc = HexToolbarControlProc;
    RegisterClass(&wc);
    wc.lpszClassName = WC_HEXCOLORPICKER;
    wc.lpfnWndProc = HexColorPickerProc;
    RegisterClass(&wc);
    wc.lpszClassName = WC_HEXSETTINGS;
    wc.lpfnWndProc = HexSettingsProc;
    RegisterClass(&wc);
    wc.lpszClassName = WC_HEXSETTINGS_FONT;
    wc.lpfnWndProc = HexSettingsFontProc;
    RegisterClass(&wc);
    wc.lpszClassName = WC_HEXSETTINGS_COLOR;
    wc.lpfnWndProc = HexSettingsColorProc;
    RegisterClass(&wc);
    wc.lpszClassName = WC_HEXSETTINGS_TAB_COLOR;
    wc.lpfnWndProc = HexSettingsTabColorProc;
    RegisterClass(&wc);
}

HMENU CreateMenuBar(void)
{
    HMENU menuBar = CreateMenu();
    HMENU menu = CreateMenu();
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_NEW, "&New\tCtrl + T");
    AppendMenu(menu, MF_SEPARATOR, (UINT_PTR) 0, NULL);
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_OPEN, "&Open...\tCtrl + O");
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_CLOSE, "&Close\tCtrl + W");
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_CLOSE_ALL, "Close &All\tCtrl + Shift + W");
    AppendMenu(menu, MF_SEPARATOR, (UINT_PTR) 0, NULL);
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_SAVE, "&Save\tCtrl + S");
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_SAVE_AS, "Save &As...\tCtrl + Shift + S");
    SetMenuItemBitmaps(menu, IDM_NEW, MF_BYCOMMAND, Resource_GetImage(ICON_ID_NEW), Resource_GetImage(ICON_ID_NEW));
    SetMenuItemBitmaps(menu, IDM_OPEN, MF_BYCOMMAND, Resource_GetImage(ICON_ID_OPEN), Resource_GetImage(ICON_ID_OPEN));
    SetMenuItemBitmaps(menu, IDM_CLOSE, MF_BYCOMMAND, Resource_GetImage(ICON_ID_CLOSE), Resource_GetImage(ICON_ID_CLOSE));
    SetMenuItemBitmaps(menu, IDM_CLOSE_ALL, MF_BYCOMMAND, Resource_GetImage(ICON_ID_CLOSE_ALL), Resource_GetImage(ICON_ID_CLOSE_ALL));
    SetMenuItemBitmaps(menu, IDM_SAVE, MF_BYCOMMAND, Resource_GetImage(ICON_ID_SAVE), Resource_GetImage(ICON_ID_SAVE));
    SetMenuItemBitmaps(menu, IDM_SAVE_AS, MF_BYCOMMAND, Resource_GetImage(ICON_ID_SAVE_AS), Resource_GetImage(ICON_ID_SAVE_AS));
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR) menu, "&File");

    menu = CreateMenu();
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_UNDO, "&Undo\tCtrl + Z");
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_REDO, "&Redo\tCtrl + Y");
    AppendMenu(menu, MF_SEPARATOR, (UINT_PTR) 0, NULL);
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_COPY, "&Copy\tCtrl + C");
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_PASTE, "&Paste\tCtrl + V");
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_CUT, "Cu&t\tCtrl + X");
    SetMenuItemBitmaps(menu, IDM_UNDO, MF_BYCOMMAND, Resource_GetImage(ICON_ID_UNDO), Resource_GetImage(ICON_ID_UNDO));
    SetMenuItemBitmaps(menu, IDM_REDO, MF_BYCOMMAND, Resource_GetImage(ICON_ID_REDO), Resource_GetImage(ICON_ID_REDO));
    SetMenuItemBitmaps(menu, IDM_COPY, MF_BYCOMMAND, Resource_GetImage(ICON_ID_COPY), Resource_GetImage(ICON_ID_COPY));
    SetMenuItemBitmaps(menu, IDM_PASTE, MF_BYCOMMAND, Resource_GetImage(ICON_ID_PASTE), Resource_GetImage(ICON_ID_PASTE));
    SetMenuItemBitmaps(menu, IDM_CUT, MF_BYCOMMAND, Resource_GetImage(ICON_ID_CUT), Resource_GetImage(ICON_ID_CUT));
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR) menu, "&Edit");

    menu = CreateMenu();
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_SEARCH, "&Search...\tCtrl + S");
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_REPLACE, "&Replace...\tCtrl + R");
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_GOTO, "&Goto...\tCtrl + G");
    SetMenuItemBitmaps(menu, IDM_SEARCH, MF_BYCOMMAND, Resource_GetImage(ICON_ID_SEARCH), Resource_GetImage(ICON_ID_SEARCH));
    SetMenuItemBitmaps(menu, IDM_REPLACE, MF_BYCOMMAND, Resource_GetImage(ICON_ID_REPLACE), Resource_GetImage(ICON_ID_REPLACE));
    SetMenuItemBitmaps(menu, IDM_GOTO, MF_BYCOMMAND, Resource_GetImage(ICON_ID_GOTO), Resource_GetImage(ICON_ID_GOTO));
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR) menu, "&Search");

    menu = CreateMenu();
    AppendMenu(menu, MF_STRING, (UINT_PTR) IDM_ENVIRONMENT, "&Environment...");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR) menu, "Se&ttings");

    return menuBar;
}

LRESULT CALLBACK HexColorPickerProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static CHOOSECOLOR cc;
    static COLORREF custClrs[16];
    PAINTSTRUCT ps;
    HDC hdc;
    RECT r;
    HBRUSH hbr;
    int oldBkMode;
    COLORREF color = (COLORREF) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    char textBuffer[0xFF];
    switch(msg)
    {
    case WM_CREATE:
        custClrs[0] = RGB(245, 255, 59);
        custClrs[1] = 0x404040;
        custClrs[3] = RGB(59, 245, 255);
        custClrs[4] = RGB(69, 59, 255);
        custClrs[5] = 0x38ff42;
        memset(&cc, 0, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = hWnd;
        cc.lpCustColors = custClrs;
        cc.Flags = CC_RGBINIT | CC_FULLOPEN;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) ((CREATESTRUCT*) lParam)->lpCreateParams);
        return 0;
    case WM_ERASEBKGND:  return 1;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &r);
        // background behind info text
        FillRect(hdc, &r, GetSysColorBrush(COLOR_INFOBK));
        // info text
        GetWindowText(hWnd, textBuffer, sizeof(textBuffer));
        oldBkMode = SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, textBuffer, strlen(textBuffer), &r, DT_VCENTER | DT_CENTER | DT_NOPREFIX | DT_SINGLELINE);
        SetBkColor(hdc, oldBkMode);
        // color square
        r.left = r.right - (r.bottom - r.top);
        hbr = CreateSolidBrush(color);
        FillRect(hdc, &r, hbr);
        DeleteObject(hbr);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_LBUTTONDOWN:
        cc.rgbResult = color;
        if(ChooseColor(&cc))
        {
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) cc.rgbResult);
            SendMessage(GetParent(hWnd), WM_COMMAND, (WPARAM) GetMenu(hWnd), (LPARAM) cc.rgbResult);
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
        }
        return 0;
    case HCP_SETCOLOR: SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) wParam); RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE); return 0;
    case HCP_GETCOLOR: return GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
