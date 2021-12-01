#include "wincomps.h"

static HWND MainWindow;
static HWND GotoBox;
static HWND SearchBox;
static HWND ReplaceBox;
static HWND SettingsBox;
static HWND HexView;
static HWND Toolbar;
static HWND TabControl;
static HWND StatusBar;

HWND Manager_GetMainWindow(void)
{
    return MainWindow;
}

HWND Manager_GetToolbar(void)
{
    return Toolbar;
}

HWND Manager_GetGotoBox(void)
{
    return GotoBox;
}

HWND Manager_GetSearchBox(void)
{
    return SearchBox;
}

HWND Manager_GetReplaceBox(void)
{
    return ReplaceBox;
}

HWND Manager_GetSettingsBox(void)
{
    return SettingsBox;
}

HWND Manager_GetHexView(void)
{
    return HexView;
}

HWND Manager_GetTabControl(void)
{
    return TabControl;
}

HWND Manager_GetStatusBar(void)
{
    return StatusBar;
}

HEXVIEWINFO *Manager_GetActiveHexViewInfo(void)
{
    return (HEXVIEWINFO*) GetWindowLongPtr(HexView, GWLP_USERDATA);
}

BOOL Manager_SaveActiveHexViewInfoFile(void)
{
    HEXVIEWINFO *hvi;
    FILE *fp;
    if(!(hvi = Manager_GetActiveHexViewInfo()) || !(fp = fopen(hvi->filePath, "wb")))
        return 0;
    fwrite(hvi->chunk, 1, hvi->chunkCount, fp);
    fclose(fp);
    hvi->changeRecentSpot = hvi->changeIndex;
    return 1;
}

void Manager_SetActiveHexViewInfo(HEXVIEWINFO *hvi)
{
    RECT r;
    SetWindowLongPtr(HexView, GWLP_USERDATA, (LONG_PTR) hvi);
    if(hvi)
    {
        GetClientRect(HexView, &r);
        HVI_ManagerRoutine46(hvi, &r);
        RedrawWindow(HexView, &r, NULL, RDW_INVALIDATE | RDW_ERASE);
    }
}

HEXVIEWINFO *Manager_CreateHexViewInfo(HEXVIEWINFO *hvi, char *filePath)
{
    HEXSTYLE *style = Manager_GetStyle();
    if(!hvi)
        hvi = malloc(sizeof(*hvi));
    memset(hvi, 0, sizeof(*hvi));
    hvi->filePath = filePath;
    hvi->radix = style->hexDefRadix;
    hvi->charGap = style->hexDefCharGap;
    hvi->charLength = style->hexDefCharLength;
    hvi->columnCount = style->hexDefColumnCount;
    hvi->addressCount = style->hexDefAddressCount;
    hvi->isRecentFile = filePath != NULL;
    FILE *fp = fopen(filePath, "rb");
    if(fp)
    {
        // get file length
        fseek(fp, 0, SEEK_END);
        long len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(hvi->chunk = malloc(len), 1, len, fp);
        hvi->chunkCount = hvi->chunkCapacity = len;
        fclose(fp);
    }
    else
    {
        hvi->chunkCount = hvi->chunkCapacity = hvi->columnCount;
        memset(hvi->chunk = malloc(hvi->columnCount), 0, hvi->columnCount);
    }
    hvi->measurements.dumpOffset = 20;
    HVI_UpdateBuffer(hvi);
    return hvi;
}

HEXVIEWINFO *Manager_CreateAndSetInfoExt(HEXVIEWINFO *hvi, HEXVIEWSKELETON *hvs)
{
    if(!hvi)
        hvi = malloc(sizeof(*hvi));
    memset(hvi, 0, sizeof(*hvi));
    hvi->filePath = hvs->filePath;
    hvi->isRecentFile = hvi->filePath != NULL;
    hvi->marks = hvs->marks;
    hvi->markCount = hvs->markCount;
    hvi->addressCount = hvs->addressCount;
    hvi->columnCount = hvs->columnCount;
    hvi->radix = hvs->radix;
    hvi->charLength = hvs->charLength;
    hvi->charGap = hvs->charGap;
    hvi->caretIndex = hvi->selectIndex = hvs->caretIndex;
    hvi->subCaretIndex = hvs->subCaretIndex;
    hvi->dumpPressed = hvs->dumpPressed;
    hvi->xCurrentScroll = hvs->xCurrentScroll;
    hvi->yCurrentScroll = hvs->yCurrentScroll;
    FILE *fp = fopen(hvi->filePath, "rb");
    if(fp)
    {
        // get file length
        fseek(fp, 0, SEEK_END);
        long len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(hvi->chunk = malloc(len), 1, len, fp);
        hvi->chunkCount = hvi->chunkCapacity = len;
        fclose(fp);
    }
    else
    {
        hvi->chunkCount = hvi->chunkCapacity = hvi->columnCount;
        memset(hvi->chunk = malloc(hvi->columnCount), 0, hvi->columnCount);
    }
    hvi->measurements.dumpOffset = 20;
    HVI_UpdateBuffer(hvi);
    return hvi;
}

HEXSTYLE style;

HEXSTYLE *Manager_GetStyle(void)
{
    return &style;
}

void Manager_SetStyle(const HEXSTYLE *newStyle)
{
    LOGFONT lf;
    HFONT font;
    int tabCnt;
    DeleteObject(style.hexTabBackgroundBrush);
    DeleteObject(style.globalFont);
    DeleteObject(style.hexTabBrush);
    DeleteObject(style.hexTabLinePen);
    memcpy(&style, newStyle, sizeof(*newStyle));
    style.hexTabBackgroundBrush = CreateSolidBrush(style.hexTabBackgroundColor);
    style.hexTabBrush = CreateSolidBrush(style.hexTabTopColor);
    style.hexTabLinePen = CreatePen(PS_SOLID, 2, style.hexTabXButtonColor);
    font = Resource_GetFont(style.globalFontIndex);
    GetObject(font, sizeof(lf), &lf);
    lf.lfHeight = style.hexFontSize;
    lf.lfWidth = 0;
    style.globalFont = CreateFontIndirect(&lf);
    tabCnt = SendMessage(TabControl, HEXTC_GETITEMCOUNT, 0, 0);
    while(tabCnt--)
         HVI_UpdateBuffer((HEXVIEWINFO*) SendMessage(TabControl, HEXTC_GETITEM, tabCnt, 0));
    RedrawWindow(HexView, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
    RedrawWindow(Toolbar, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
    SendMessage(TabControl, HEXTC_REMEASURE, 0, 0);
}

void Manager_AddFont(HFONT font)
{
    Resource_AddFont(font);
}

typedef struct {
    char **tabNames;
    HEXVIEWSKELETON *hexViewSkeletons;
    int tabIndex;
    int tabCount;
    RECT rects[5];
} HEXTABSKELETON;

static const char *FileHeader = "HE_21-22";
static char TextBuffer[MAX_PATH];

BOOL Manager_SaveFile(void)
{
    FILE *fp;
    HEXVIEWINFO *hvi;
    HEXTABINFO hti;
    int tabCnt, tabIndex;
    int markCnt;
    MARK *marks;
    RECT r;
    sprintf(TextBuffer, "%s\\.save", Resource_GetHexEditorFolder());
    fp = fopen(TextBuffer, "wb");
    if(fp)
    {
        tabCnt = SendMessage(TabControl, HEXTC_GETITEMCOUNT, 0, 0);
        tabIndex = SendMessage(TabControl, HEXTC_GETCURSEL, 0, 0);

        fwrite(FileHeader, 1, 8, fp);
        // write style
        fwrite(&style.globalFontIndex, sizeof(style.globalFontIndex), 1, fp);
        fwrite(&style.hexFontSize, sizeof(style.hexFontSize), 1, fp);
        fwrite(&style.hexTextColor, sizeof(style.hexTextColor), 1, fp);
        fwrite(&style.hexBackgroundColor, sizeof(style.hexBackgroundColor), 1, fp);
        fwrite(&style.hexForegroundColor, sizeof(style.hexForegroundColor), 1, fp);
        fwrite(&style.hexCaretColor, sizeof(style.hexCaretColor), 1, fp);
        fwrite(&style.hexSelectionColor, sizeof(style.hexSelectionColor), 1, fp);
        fwrite(&style.hexMarkerColor, sizeof(style.hexMarkerColor), 1, fp);
        fwrite(&style.hexDefRadix, sizeof(style.hexDefRadix), 1, fp);
        fwrite(&style.hexDefCharLength, sizeof(style.hexDefCharLength), 1, fp);
        fwrite(&style.hexDefCharGap, sizeof(style.hexDefCharGap), 1, fp);
        fwrite(&style.hexDefColumnCount, sizeof(style.hexDefColumnCount), 1, fp);
        fwrite(&style.hexDefAddressCount, sizeof(style.hexDefAddressCount), 1, fp);
        fwrite(&style.hexTabWidth, sizeof(style.hexTabWidth), 1, fp);
        fwrite(&style.hexTabHeight, sizeof(style.hexTabHeight), 1, fp);
        fwrite(&style.hexTabPadding, sizeof(style.hexTabPadding), 1, fp);
        fwrite(&style.hexTabFitMode, sizeof(style.hexTabFitMode), 1, fp);
        fwrite(&style.hexTabTextColor, sizeof(style.hexTabTextColor), 1, fp);
        fwrite(&style.hexTabTopColor, sizeof(style.hexTabTopColor), 1, fp);
        fwrite(&style.hexTabBottomColor, sizeof(style.hexTabBottomColor), 1, fp);
        fwrite(&style.hexTabBackgroundColor, sizeof(style.hexTabBackgroundColor), 1, fp);
        fwrite(&style.hexTabXButtonColor, sizeof(style.hexTabXButtonColor), 1, fp);

        fwrite(&tabCnt, sizeof(tabCnt), 1, fp);
        fwrite(&tabIndex, sizeof(tabIndex), 1, fp);
        while(tabCnt--)
        {
            SendMessage(TabControl, HEXTC_GETITEMINFO, tabCnt, (LPARAM) &hti);
            hvi = hti.hexInfo;
            if(!hvi->isRecentFile)
                continue;
            fwrite(hti.name, 1, strlen(hti.name) + 1, fp);
            fwrite(hvi->filePath, 1, strlen(hvi->filePath) + 1, fp);
            markCnt = hvi->markCount;
            marks = hvi->marks;
            fwrite(&markCnt, sizeof(markCnt), 1, fp);
            while(markCnt--)
            {
                fwrite(&marks->index, sizeof(marks->index), 1, fp);
                fwrite(&marks->color, sizeof(marks->color), 1, fp);
                marks++;
            }
            fwrite(&hvi->columnCount, sizeof(hvi->columnCount), 1, fp);
            fwrite(&hvi->radix, sizeof(hvi->radix), 1, fp);
            fwrite(&hvi->addressCount, sizeof(hvi->addressCount), 1, fp);
            fwrite(&hvi->charLength, sizeof(hvi->charLength), 1, fp);
            fwrite(&hvi->charGap, sizeof(hvi->charGap), 1, fp);
            fwrite(&hvi->caretIndex, sizeof(hvi->caretIndex), 1, fp);
            fwrite(&hvi->subCaretIndex, sizeof(hvi->subCaretIndex), 1, fp);
            fwrite(&hvi->dumpPressed, sizeof(hvi->dumpPressed), 1, fp);
            fwrite(&hvi->xCurrentScroll, sizeof(hvi->xCurrentScroll), 1, fp);
            fwrite(&hvi->yCurrentScroll, sizeof(hvi->yCurrentScroll), 1, fp);
        }
        GetWindowRect(MainWindow, &r);
        fwrite(&r, sizeof(r), 1, fp);
        GetWindowRect(SearchBox, &r);
        fwrite(&r, sizeof(r), 1, fp);
        GetWindowRect(ReplaceBox, &r);
        fwrite(&r, sizeof(r), 1, fp);
        GetWindowRect(GotoBox, &r);
        fwrite(&r, sizeof(r), 1, fp);
        GetWindowRect(SettingsBox, &r);
        fwrite(&r, sizeof(r), 1, fp);
        fclose(fp);
        return 1;
    }
    return 0;
}

BOOL Manager_LoadFile(HEXTABSKELETON *skeleton, HEXSTYLE *style)
{
    FILE *fp;
    HEXVIEWSKELETON *hvs;
    int tabCnt, tabIndex;
    int index;
    int markCnt;
    char **tabName;
    MARK *marks;
    sprintf(TextBuffer, "%s\\.save", Resource_GetHexEditorFolder());
    fp = fopen(TextBuffer, "rb");
    if(fp)
    {
        if(fread(TextBuffer, 1, 8, fp) != 8 || memcmp(TextBuffer, FileHeader, 8))
        {
            printf("error loading save: wrong header\n");
            fclose(fp);
            return 0;
        }
        // read style
        fread(&style->globalFontIndex, sizeof(style->globalFontIndex), 1, fp);
        fread(&style->hexFontSize, sizeof(style->hexFontSize), 1, fp);
        fread(&style->hexTextColor, sizeof(style->hexTextColor), 1, fp);
        fread(&style->hexBackgroundColor, sizeof(style->hexBackgroundColor), 1, fp);
        fread(&style->hexForegroundColor, sizeof(style->hexForegroundColor), 1, fp);
        fread(&style->hexCaretColor, sizeof(style->hexCaretColor), 1, fp);
        fread(&style->hexSelectionColor, sizeof(style->hexSelectionColor), 1, fp);
        fread(&style->hexMarkerColor, sizeof(style->hexMarkerColor), 1, fp);
        fread(&style->hexDefRadix, sizeof(style->hexDefRadix), 1, fp);
        fread(&style->hexDefCharLength, sizeof(style->hexDefCharLength), 1, fp);
        fread(&style->hexDefCharGap, sizeof(style->hexDefCharGap), 1, fp);
        fread(&style->hexDefColumnCount, sizeof(style->hexDefColumnCount), 1, fp);
        fread(&style->hexDefAddressCount, sizeof(style->hexDefAddressCount), 1, fp);
        fread(&style->hexTabWidth, sizeof(style->hexTabWidth), 1, fp);
        fread(&style->hexTabHeight, sizeof(style->hexTabHeight), 1, fp);
        fread(&style->hexTabPadding, sizeof(style->hexTabPadding), 1, fp);
        fread(&style->hexTabFitMode, sizeof(style->hexTabFitMode), 1, fp);
        fread(&style->hexTabTextColor, sizeof(style->hexTabTextColor), 1, fp);
        fread(&style->hexTabTopColor, sizeof(style->hexTabTopColor), 1, fp);
        fread(&style->hexTabBottomColor, sizeof(style->hexTabBottomColor), 1, fp);
        fread(&style->hexTabBackgroundColor, sizeof(style->hexTabBackgroundColor), 1, fp);
        fread(&style->hexTabXButtonColor, sizeof(style->hexTabXButtonColor), 1, fp);
        style->hexTabBackgroundBrush = CreateSolidBrush(style->hexTabBackgroundColor);
        style->hexTabBrush = CreateSolidBrush(style->hexTabTopColor);
        style->hexTabLinePen = CreatePen(PS_SOLID, 2, style->hexTabXButtonColor);

        printf("[Style font index=%d]\n", style->globalFontIndex);
        printf("[Style background color=0x%lx]\n", style->hexBackgroundColor);
        printf("[Style text color=0x%lx]\n", style->hexTextColor);
        printf("[Style caret color=0x%lx]\n", style->hexCaretColor);
        printf("[Style foreground color=0x%lx]\n", style->hexForegroundColor);
        printf("[Style marker color=0x%lx]]\n", style->hexMarkerColor);
        printf("[Style selection color=0x%lx]]\n", style->hexSelectionColor);
        printf("[Style address count=%d]\n", style->hexDefAddressCount);
        printf("[Style char gap=%d]\n", style->hexDefCharGap);
        printf("[Style char length=%d]\n", style->hexDefCharLength);
        printf("[Style column count=%d]\n", style->hexDefColumnCount);
        printf("[Style radix=%d]\n", style->hexDefRadix);
        printf("[Style tab top color=0x%lx]]\n", style->hexTabTopColor);
        printf("[Style tab bottom color=0x%lx]]\n", style->hexTabBottomColor);
        printf("[Style tab text color=0x%lx]\n", style->hexTabTextColor);
        printf("[Style tab fit mode=%d]\n", style->hexTabFitMode);
        printf("[Style tab size=(%d, %d)]\n", style->hexTabWidth, style->hexTabHeight);
        // update properties of tab control controls
        // sprintf(textBuffer, "%d", HexStyle->columnCount);
        // TODO: SetWindowText(TBC_GetControl(TBCGC_ID_COLUMNCOUNT), textBuffer);
        // TODO: SendMessage(TBC_GetControl(TBCGC_ID_COLUMNCOUNTUPDOWN), UDM_SETPOS, 0, HexStyle->columnCount);
        // TODO: int cbIndex = HexStyle->radix == 16 ? 0 : HexStyle->radix == 10 ? 1 : HexStyle->radix == 8 ? 2 : 3;
        // TODO: SendMessage(TBC_GetControl(TBCGC_ID_RADIX), CB_SETCURSEL, cbIndex, 0);

        fread(&tabCnt, sizeof(tabCnt), 1, fp);
        fread(&tabIndex, sizeof(tabIndex), 1, fp);
        printf("[Tab count=%d, index=%d]\n", tabCnt, tabIndex);
        char *pBuf;
        skeleton->tabCount = tabCnt;
        skeleton->tabIndex = tabIndex;
        tabName = skeleton->tabNames = malloc(sizeof(*skeleton->tabNames) * tabCnt);
        hvs = skeleton->hexViewSkeletons = malloc(sizeof(*skeleton->hexViewSkeletons) * tabCnt);
        while(tabCnt--)
        {
            pBuf = *tabName = malloc(MAX_PATH);
            while((*(pBuf++) = fgetc(fp)));
            pBuf = hvs->filePath = malloc(MAX_PATH);
            while((*(pBuf++) = fgetc(fp)));
            printf("\t[File title=%s, path=%s]\n", *tabName, hvs->filePath);
            fread(&markCnt, sizeof(markCnt), 1, fp);
            marks = hvs->marks = malloc(sizeof(*marks) * markCnt);
            hvs->markCount = markCnt;
            printf("\t[Mark count=%u]\n", markCnt);
            while(markCnt--)
            {
                fread(&marks->index, sizeof(marks->index), 1, fp);
                fread(&marks->color, sizeof(marks->color), 1, fp);
                printf("\t\t[Mark index=%u, color=%lx]\n", marks->index, marks->color);
                marks++;
            }
            fread(&hvs->columnCount, sizeof(hvs->columnCount), 1, fp);
            fread(&hvs->radix, sizeof(hvs->radix), 1, fp);
            fread(&hvs->addressCount, sizeof(hvs->addressCount), 1, fp);
            fread(&hvs->charLength, sizeof(hvs->charLength), 1, fp);
            fread(&hvs->charGap, sizeof(hvs->charGap), 1, fp);
            fread(&hvs->caretIndex, sizeof(hvs->caretIndex), 1, fp);
            fread(&hvs->subCaretIndex, sizeof(hvs->subCaretIndex), 1, fp);
            fread(&hvs->dumpPressed, sizeof(hvs->dumpPressed), 1, fp);
            fread(&hvs->xCurrentScroll, sizeof(hvs->xCurrentScroll), 1, fp);
            fread(&hvs->yCurrentScroll, sizeof(hvs->yCurrentScroll), 1, fp);
            printf("\t[Caret index=%u, sub caret index=%u]\n", hvs->caretIndex, hvs->subCaretIndex);
            printf("\t[Address count=%u]\n", hvs->addressCount);
            printf("\t[Radix=%u]\n", hvs->radix);
            printf("\t[Char length=%u, Char gap=%u]\n", hvs->charLength, hvs->charGap);
            printf("\t[Dump pressed=%u]\n", hvs->dumpPressed);
            printf("\t[Scroll x=%u, y=%u]\n", hvs->xCurrentScroll, hvs->yCurrentScroll);
            index++;
            hvs++;
            tabName++;
        }
        fread(skeleton->rects, sizeof(RECT), 1, fp);
        fread(skeleton->rects + 1, sizeof(RECT), 1, fp);
        fread(skeleton->rects + 2, sizeof(RECT), 1, fp);
        fread(skeleton->rects + 3, sizeof(RECT), 1, fp);
        fread(skeleton->rects + 4, sizeof(RECT), 1, fp);
        fclose(fp);
        return 1;
    }
    return 0;
}

void Manager_Init(void)
{
    HEXSTYLE style;
    HEXTBBUTTON htb;
    HEXTABSKELETON hts;

    int cnt;
    char **names;
    HEXVIEWSKELETON *hvs;
    HEXTABINFO hti;
    RECT r;

    MainWindow = CreateWindow(WC_MAIN, "Hex Editor v2.0", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                              NULL, CreateMenuBar(), GetModuleHandle(NULL), NULL);
    HexView = CreateWindow(WC_HEXVIEW, NULL, WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL, 0, 0, 0, 0, MainWindow, NULL, NULL, NULL);
    TabControl = CreateWindow(WC_HEXTABCONTROL, NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, MainWindow, NULL, NULL, NULL);
    Toolbar = CreateWindow(WC_HEXTOOLBARCONTROL, NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, MainWindow, NULL, NULL, NULL);
    HEXTBBUTTON hexTbButtons[] = {
        { "New", Resource_GetImage(ICON_ID_NEW),     IDM_NEW,   { 0 }, 0 },
        { "Close", Resource_GetImage(ICON_ID_CLOSE), IDM_CLOSE, { 0 }, 0 },
        { "Open", Resource_GetImage(ICON_ID_OPEN),   IDM_OPEN,  { 0 }, 0 },
        { "Save", Resource_GetImage(ICON_ID_SAVE),   IDM_SAVE,  { 0 }, 0 },
        { NULL, NULL, 0, { 0 }, HTB_SEPERATOR },
        { "Search", Resource_GetImage(ICON_ID_SEARCH),   IDM_SEARCH,  { 0 }, 0 },
        { "Replace", Resource_GetImage(ICON_ID_REPLACE), IDM_REPLACE, { 0 }, 0 },
        { NULL, NULL, 0, { 0 }, HTB_SEPERATOR },
        { "Copy", Resource_GetImage(ICON_ID_COPY),   IDM_COPY,  { 0 }, 0 },
        { "Paste", Resource_GetImage(ICON_ID_PASTE), IDM_PASTE, { 0 }, 0 },
        { "Cut", Resource_GetImage(ICON_ID_CUT),     IDM_CUT,   { 0 }, 0 },
        { NULL, NULL, 0, { 0 }, HTB_SEPERATOR },
        { "Undo", Resource_GetImage(ICON_ID_UNDO),  IDM_UNDO, { 0 }, 0 },
        { "Redo", Resource_GetImage(ICON_ID_REDO),  IDM_REDO,  { 0 }, 0 }
    };
    cnt = sizeof(hexTbButtons) / sizeof(hexTbButtons[0]);
    r = (RECT) {
        .left = 2,
        .top = 2,
        .right = 38,
        .bottom = 38
    };
    for(int i = 0; i < cnt; i++)
    {
        if(hexTbButtons[i].flags == HTB_SEPERATOR)
            r.right = r.left + 20;
        hexTbButtons[i].rcIcon = r;
        r.left = r.right + 2;
        r.right = r.left + 36;
    }
    SendMessage(Toolbar, HEXTB_SETITEMS, (WPARAM) cnt, (LPARAM) hexTbButtons);

    StatusBar = CreateWindow(STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, MainWindow, NULL, NULL, NULL);
    if(Manager_LoadFile(&hts, &style))
    {
        MoveWindow(MainWindow, hts.rects[0].left, hts.rects[0].top, hts.rects[0].right - hts.rects[0].left, hts.rects[0].bottom - hts.rects[0].top, FALSE);
        SearchBox = CreateWindow(WC_SEARCH, "Search", WS_DLGFRAME | WS_POPUPWINDOW,
                                 hts.rects[1].left, hts.rects[1].top, hts.rects[1].right - hts.rects[1].left, hts.rects[1].bottom - hts.rects[1].top,
                                 MainWindow, NULL, NULL, NULL);
        ReplaceBox = CreateWindow(WC_REPLACE, "Replace", WS_DLGFRAME | WS_POPUPWINDOW,
                                 hts.rects[2].left, hts.rects[2].top, hts.rects[2].right - hts.rects[2].left, hts.rects[2].bottom - hts.rects[2].top,
                                 MainWindow, NULL, NULL, NULL);
        GotoBox = CreateWindow(WC_GOTO, "Goto", WS_DLGFRAME | WS_POPUPWINDOW,
                                 hts.rects[3].left, hts.rects[3].top, hts.rects[3].right - hts.rects[3].left, hts.rects[3].bottom - hts.rects[3].top,
                                 MainWindow, NULL, NULL, NULL);
        SettingsBox = CreateWindow(WC_HEXSETTINGS, "Settings", WS_DLGFRAME | WS_POPUPWINDOW,
                                 hts.rects[4].left, hts.rects[4].top, hts.rects[4].right - hts.rects[4].left, hts.rects[4].bottom - hts.rects[4].top,
                                 MainWindow, NULL, NULL, NULL);
        cnt = hts.tabCount;
        names = hts.tabNames;
        hvs = hts.hexViewSkeletons;
        while(cnt)
        {
            hti.name = *names;
            hti.hexInfo = Manager_CreateAndSetInfoExt(NULL, hvs);
            SendMessage(TabControl, HEXTC_INSERTTAB, hts.tabCount - cnt, (LPARAM) &hti);
            free(hti.name);
            names++;
            hvs++;
            cnt--;
        }
        free(hts.tabNames);
        free(hts.hexViewSkeletons);
        SendMessage(TabControl, HEXTC_SETCURSEL, hts.tabIndex, (LPARAM) &hti);
    }
    else
    {
        style = (HEXSTYLE) {
            .globalFontIndex = 0,
            .hexFontSize = 21,
            .hexTextColor = RGB(245, 255, 59),
            .hexBackgroundColor = 0x404040,
            .hexForegroundColor = RGB(59, 245, 255),
            .hexCaretColor = RGB(69, 59, 255),
            .hexSelectionColor = RGB(69, 59, 255),
            .hexMarkerColor = 0x38ff42,
            .hexDefAddressCount = 8,
            .hexDefRadix = 16,
            .hexDefCharLength = 2,
            .hexDefCharGap = 1,
            .hexDefColumnCount = 20,
            .hexTabWidth = 125,
            .hexTabHeight = 22,
            .hexTabPadding = 3,
            .hexTabFitMode = 0,
            .hexTabTextColor = RGB(245, 255, 59),
            .hexTabTopColor = 0x909090,
            .hexTabBottomColor = 0x404040,
            .hexTabBackgroundColor = 0x868686,
            .hexTabXButtonColor = 0x338800
        };
        SearchBox = CreateWindow(WC_SEARCH, "Search", WS_DLGFRAME | WS_POPUPWINDOW, 800, 300, 300, 220, MainWindow, NULL, NULL, NULL);
        ReplaceBox = CreateWindow(WC_REPLACE, "Replace", WS_DLGFRAME | WS_POPUPWINDOW, 810, 310, 300, 250, MainWindow, NULL, NULL, NULL);
        GotoBox = CreateWindow(WC_GOTO, "Goto", WS_DLGFRAME | WS_POPUPWINDOW, 820, 320, 300, 250, MainWindow, NULL, NULL, NULL);
        SettingsBox = CreateWindow(WC_HEXSETTINGS, "Settings", WS_DLGFRAME | WS_POPUPWINDOW, 830, 330, 300, 450, MainWindow, NULL, NULL, NULL);
    }
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETFONT, (LPARAM) style.globalFontIndex);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETFONTSIZE, style.hexFontSize);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTEXTCOLOR, style.hexTextColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETBACKGROUNDCOLOR, style.hexBackgroundColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETFOREGROUNDCOLOR, style.hexForegroundColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETCARETCOLOR, style.hexCaretColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETSELECTCOLOR, style.hexSelectionColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETMARKERCOLOR, style.hexMarkerColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABBACKGROUNDCOLOR, style.hexTabBackgroundColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABGRADIENTTOPCOLOR, style.hexTabTopColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABGRADIENTBOTTOMCOLOR, style.hexTabBottomColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABXBUTTONCOLOR, style.hexTabXButtonColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABTEXTCOLOR, style.hexTabTextColor);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABWIDTH, style.hexTabWidth);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABHEIGHT, style.hexTabHeight);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABPADDING, style.hexTabPadding);
    SendMessage(SettingsBox, WM_COMMAND, IDHS_SETTABFITMODE, style.hexTabFitMode);
    Manager_SetStyle(&style);
}

void Manager_Destroy(void)
{
    Manager_SaveFile();
}
