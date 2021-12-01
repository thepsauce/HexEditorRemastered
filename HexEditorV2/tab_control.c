#include "wincomps.h"

#define TAB_INITIAL_CAPACITY 10
#define TAB_GROWTH_FACTOR 2
#define TAB_ROUND_RADIUS 8
#define TAB_MIN_WIDTH 30
//#define FACT_RECT_CORNER_CIRCLE 0.293d

typedef struct {
    HDC hexWindowDc;
    HEXTABINFO *hexTabs;
    int hexTabWidth;
    int hexTabCount;
    int hexTabIndex;
    int hexTabCapacity;
    // translation of tab control in pixels
    int hexTabTranslation;
    int hexTabOverXButtonIndex;
} HEXTABCONTROLINFO;

static void DrawSingleTab(HEXTABINFO*, BOOL, BOOL, HEXTABCONTROLINFO*);
static void GetTabRect(RECT*, HEXTABINFO*, HEXTABCONTROLINFO*);
static void GetTabXRect(const RECT*, RECT*, HEXTABCONTROLINFO*);
static void ComputeTabRect(RECT*, HEXTABINFO*, HEXTABCONTROLINFO*);
static void RecomputeTabRects(HEXTABCONTROLINFO*);
static BOOL CheckTabWindowFit(HWND, HEXTABCONTROLINFO*);

static void ComputeTabRect(RECT *rDest, HEXTABINFO *hti, HEXTABCONTROLINFO *htci)
{
    const HEXSTYLE *style = Manager_GetStyle();
    SIZE s;
    if(style->hexTabFitMode & TCLSP_TFM_MATCH_TEXT)
    {
        GetTextExtentPoint32(htci->hexWindowDc, hti->name, strlen(hti->name), &s);
        s.cx += 25;
        s.cx = (style->hexTabFitMode & TCLSP_TFM_MAX_TABS) ? min(s.cx, style->hexTabWidth) : min(s.cx, style->hexTabWidth);
    }
    else
        s.cx = htci->hexTabWidth;
    rDest->left = 0;
    rDest->top = 0;
    rDest->right = s.cx;
    rDest->bottom = style->hexTabHeight;
}

static void RecomputeTabRects(HEXTABCONTROLINFO *htci)
{
    const HEXSTYLE *style = Manager_GetStyle();
    int cnt;
    HEXTABINFO *hti;
    int left;
    RECT r;
    cnt = htci->hexTabCount;
    hti = htci->hexTabs;
    left = 0;
    while(cnt--)
    {
        ComputeTabRect(&r, hti, htci);
        hti->rcTab.left = left;
        hti->rcTab.top = r.top;
        hti->rcTab.right = left + r.right;
        hti->rcTab.bottom = r.bottom;
        left += r.right + style->hexTabPadding;
        hti++;
    }
}

// returns if this function redrew the control
static BOOL CheckTabWindowFit(HWND hWnd, HEXTABCONTROLINFO *htci)
{
    HEXSTYLE *style = Manager_GetStyle();
    HEXTABINFO *hti;
    RECT r;
    int xMax;
    int newTabTranslation = 0;
    int oldTabWidth = htci->hexTabWidth;
    if(htci->hexTabCount)
    {
        GetClientRect(hWnd, &r);
        if(style->hexTabFitMode & TCLSP_TFM_MAX_TABS)
        {
            htci->hexTabWidth = r.right / htci->hexTabCount;
            htci->hexTabWidth = min(htci->hexTabWidth, style->hexTabWidth);
            htci->hexTabWidth = max(htci->hexTabWidth, TAB_MIN_WIDTH);
            RecomputeTabRects(htci);
        }
        hti = htci->hexTabs + htci->hexTabIndex;
        newTabTranslation = -hti->rcTab.left > htci->hexTabTranslation || hti->rcTab.right + htci->hexTabTranslation > r.right ? -hti->rcTab.left : htci->hexTabTranslation;
        xMax = ((htci->hexTabs + (htci->hexTabCount - 1))->rcTab.right) + newTabTranslation;
        if(xMax < r.right)
            newTabTranslation += r.right - xMax;
        if(newTabTranslation > 0)
            newTabTranslation = 0;
        if(newTabTranslation != htci->hexTabTranslation || htci->hexTabWidth != oldTabWidth)
        {
            htci->hexTabTranslation = newTabTranslation;
            RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
            return TRUE;
        }
    }
    return FALSE;
}

static void GetTabXRect(const RECT *r, RECT *rDest, HEXTABCONTROLINFO *htci)
{
    rDest->top = r->top + 4;
    rDest->right = r->right - 4;
    rDest->bottom = r->bottom - 4;
    rDest->left = rDest->right - (rDest->bottom - rDest->top);
}

static void GetTabRect(RECT *rDest, HEXTABINFO *hti, HEXTABCONTROLINFO *htci)
{
    memcpy(rDest, &hti->rcTab, sizeof(*rDest));
    rDest->left += htci->hexTabTranslation;
    rDest->right += htci->hexTabTranslation;
}

static void DrawSingleTab(HEXTABINFO *hti, BOOL gradient, BOOL focus, HEXTABCONTROLINFO *htci)
{
    HEXSTYLE *style = Manager_GetStyle();
    TRIVERTEX triVert[2];
    GRADIENT_RECT gRect;
    RECT r, rc;
    GetTabRect(&rc, hti, htci);
    r = rc;
    SelectObject(htci->hexWindowDc, GetStockObject(NULL_PEN));
    SelectObject(htci->hexWindowDc, style->hexTabBrush);
    RoundRect(htci->hexWindowDc, rc.left, rc.top, rc.right, rc.top + 14, TAB_ROUND_RADIUS, TAB_ROUND_RADIUS);
    if(gradient)
    {
        triVert[0] = (TRIVERTEX) {
            .x = rc.left,
            .y = rc.top + TAB_ROUND_RADIUS,
            .Red   = (style->hexTabTopColor & 0xFF) << 8,
            .Green =  style->hexTabTopColor & 0xFF00,
            .Blue  = (style->hexTabTopColor & 0xFF0000) >> 8,
            .Alpha = 0,
        };
        triVert[1] = (TRIVERTEX) {
            .x = rc.right,
            .y = rc.bottom,
            .Red   = (style->hexTabBottomColor & 0xFF) << 8,
            .Green =  style->hexTabBottomColor & 0xFF00,
            .Blue  = (style->hexTabBottomColor & 0xFF0000) >> 8,
            .Alpha = 0,
        };
        gRect = (GRADIENT_RECT) {
            .UpperLeft = 0,
            .LowerRight = 1
        };
        GradientFill(htci->hexWindowDc, triVert, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
        if(focus)
            DrawFocusRect(htci->hexWindowDc, &r);
    }
    else
    {
        r.top += 8;
        FillRect(htci->hexWindowDc, &r, style->hexTabBrush);
    }
    GetTabXRect(&rc, &r, htci);
    SelectObject(htci->hexWindowDc, style->hexTabLinePen);
    MoveToEx(htci->hexWindowDc, r.left + 2, r.top + 2, NULL);
    LineTo(htci->hexWindowDc, r.right - 2, r.bottom - 2);
    MoveToEx(htci->hexWindowDc, r.right - 2, r.top + 2, NULL);
    LineTo(htci->hexWindowDc, r.left + 2, r.bottom - 2);

    SetTextColor(htci->hexWindowDc, style->hexTabTextColor);
    rc.right -= 25;
    DrawText(htci->hexWindowDc, hti->name, strlen(hti->name), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
}

LRESULT CALLBACK HexTabControlProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL onXButtonDown = FALSE;
    static int pressedTabIndex = -1;
    static int xPressedOffset = 0;
    static int xlastMoveFocusRect = -1;
    static BOOL isActiveTab;
    static BOOL focusRectActive = FALSE;
    PAINTSTRUCT ps;
    HEXVIEWINFO *hvi;
    HEXTABINFO *hti, *insertHti;
    HEXTABINFO htiMem;
    HEXSTYLE *style = Manager_GetStyle();
    HEXTABCONTROLINFO *htci = (HEXTABCONTROLINFO*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    int index, cnt;
    int moveCnt;
    int moveAmount;
    int newIndex;
    int tabWidth;
    int len;
    RECT r;
    POINT p;
    switch(msg)
    {
    case WM_CREATE:
        htci = malloc(sizeof(*htci));
        memset(htci, 0, sizeof(*htci));
        htci->hexWindowDc = GetDC(hWnd);
        SetBkMode(htci->hexWindowDc, TRANSPARENT);
        htci->hexTabs = malloc(sizeof(*htci->hexTabs) * (htci->hexTabCapacity = TAB_INITIAL_CAPACITY));
        htci->hexTabOverXButtonIndex = -1;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) htci);
        return 0;
    case WM_SIZE:
        CheckTabWindowFit(hWnd, htci);
        return 0;
    case WM_ERASEBKGND:
        GetClientRect(hWnd, &r);
        FillRect((HDC) wParam, &r, style->hexTabBackgroundBrush);
        return 1;
    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        cnt = htci->hexTabCount;
        hti = htci->hexTabs;
        index = 0;
        while(cnt)
        {
            DrawSingleTab(htci->hexTabs + index, index == htci->hexTabIndex, GetFocus() == hWnd, htci);
            cnt--;
            index++;
            hti++;
        }
        EndPaint(hWnd, &ps);
        return 0;
    case HEXTC_SETCURSEL:
        set_tab:
        index = htci->hexTabIndex;
        htci->hexTabIndex = wParam;
        if(!CheckTabWindowFit(hWnd, htci))
        {
            DrawSingleTab(htci->hexTabs + index, FALSE, FALSE, htci);
            DrawSingleTab(htci->hexTabs + htci->hexTabIndex, TRUE, GetFocus() == hWnd, htci);
        }
        Manager_SetActiveHexViewInfo((htci->hexTabs + htci->hexTabIndex)->hexInfo);
        return 0;
    case HEXTC_GETCURSEL: return htci->hexTabIndex;
    case HEXTC_GETITEMCOUNT:
        return htci->hexTabCount;
    case HEXTC_INSERTTAB:
        index = (int) wParam;
        insertHti = (HEXTABINFO*) lParam;
        len = strlen(insertHti->name) + 1;
        moveCnt = htci->hexTabCount - index;
        hti = htci->hexTabs;
        // growing array if necessary
        htci->hexTabCount++;
        if(htci->hexTabCount > htci->hexTabCapacity)
        {
            htci->hexTabCapacity *= TAB_GROWTH_FACTOR;
            hti = htci->hexTabs = realloc(htci->hexTabs, sizeof(*htci->hexTabs) * htci->hexTabCapacity);
        }
        // moving memory
        hti += index;
        memmove(hti + 1, hti, sizeof(*hti) * moveCnt);
        // setting properties
        hti->hexInfo = insertHti->hexInfo;
        hti->name = malloc(len + 1);
        *(hti->name) = '*';
        memcpy(hti->name + 1, insertHti->name, len);
        hti->name += insertHti->hexInfo->isRecentFile ? 1 : 0;
        // moving rects because of insertion
        ComputeTabRect(&hti->rcTab, hti, htci);
        if(!(style->hexTabFitMode & TCLSP_TFM_MAX_TABS))
        {
            moveAmount = hti->rcTab.right - hti->rcTab.left;
            hti->rcTab.left = !index ? 0 : (hti - 1)->rcTab.right + style->hexTabPadding;
            hti->rcTab.right = hti->rcTab.left + moveAmount;
            moveAmount += style->hexTabPadding;
            while(moveCnt--)
            {
                hti++;
                hti->rcTab.left += moveAmount;
                hti->rcTab.right += moveAmount;
            }
        }
        goto set_tab;
    case HEXTC_REMOVETAB:
        index = (int) wParam;
        if(index < 0)
            return -1;
        hvi = (htci->hexTabs + index)->hexInfo;
        if(hvi)
        {
            free(hvi->filePath);
            free(hvi->chunk);
            DeleteObject(hvi->buffer);
        }
        hti = htci->hexTabs + index;
        moveAmount = hti->rcTab.right - hti->rcTab.left + style->hexTabPadding;
        htci->hexTabCount--;
        moveCnt = htci->hexTabCount - index;
        memmove(hti, hti + 1, sizeof(*hti) * moveCnt);
        newIndex = index ? index - 1 : 0;
        // erase everything if the last tab is closed
        if(!htci->hexTabCount)
        {
            RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
        }
        else
        {
            htci->hexTabIndex = newIndex;
            while(moveCnt--)
            {
                hti->rcTab.left -= moveAmount;
                hti->rcTab.right -= moveAmount;
                hti++;
                index++;
            }
            if(!CheckTabWindowFit(hWnd, htci))
                RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
        }
        wParam = newIndex;
        goto set_tab;
    case HEXTC_GETITEM:
        return (LRESULT) (htci->hexTabs + wParam)->hexInfo;
    case HEXTC_GETITEMINFO:
        hti = htci->hexTabs + wParam;
        return (LRESULT) memcpy((void*) lParam, hti, sizeof(*hti));
    case HEXTC_REMEASURE:
        htci->hexTabWidth = style->hexTabWidth;
        RecomputeTabRects(htci);
        CheckTabWindowFit(hWnd, htci);
        RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
        return 0;
    case WM_KILLFOCUS:
        if(focusRectActive)
        {
            GetTabRect(&r, htci->hexTabs + htci->hexTabIndex, htci);
            DrawFocusRect(htci->hexWindowDc, &r);
            focusRectActive = FALSE;
            return 0;
        }
    case WM_LBUTTONDBLCLK:
        if(isActiveTab && !focusRectActive)
        {
            SetFocus(hWnd);
            GetTabRect(&r, htci->hexTabs + htci->hexTabIndex, htci);
            DrawFocusRect(htci->hexWindowDc, &r);
            focusRectActive = TRUE;
        }
        return 0;
    case WM_LBUTTONDOWN:
        p = (POINT) {
            .x = GET_X_LPARAM(lParam),
            .y = GET_Y_LPARAM(lParam)
        };
        cnt = htci->hexTabCount;
        hti = htci->hexTabs;
        while(cnt)
        {
            GetTabRect(&r, hti, htci);
            if(PtInRect(&r, p))
            {
                pressedTabIndex = htci->hexTabCount - cnt;
                isActiveTab = TRUE;
                xPressedOffset = p.x - r.left;
                // if the mouse is on the x button
                if(htci->hexTabOverXButtonIndex >= 0)
                    onXButtonDown = TRUE;
                // if an inactive tab was pressed
                if(htci->hexTabIndex != pressedTabIndex)
                {
                    if(GetFocus() == hWnd)
                    {
                        // erase focus rect
                        GetTabRect(&r, htci->hexTabs + htci->hexTabIndex, htci);
                        DrawFocusRect(htci->hexWindowDc, &r);
                    }
                    wParam = pressedTabIndex;
                    goto set_tab;
                }
                return 0;
            }
            cnt--;
            hti++;
        }
        isActiveTab = FALSE;
        return 0;
    case WM_LBUTTONUP:
        // if the mouse was pressed on a tab and on the x button
        p = (POINT) {
            .x = GET_X_LPARAM(lParam),
            .y = GET_Y_LPARAM(lParam)
        };
        if(onXButtonDown && htci->hexTabOverXButtonIndex == pressedTabIndex)
        {
            onXButtonDown = FALSE;
            if(!SendMessage(GetParent(hWnd), WM_COMMAND, IDM_CLOSE, 0))
                htci->hexTabOverXButtonIndex = -1;
        }
        // handle tab movement
        else if(xlastMoveFocusRect >= 0)
        {
            // erase last focus rect
            GetTabRect(&r, htci->hexTabs + pressedTabIndex, htci);
            tabWidth = r.right - r.left;
            // erase last focus rect
            r.left = xlastMoveFocusRect - xPressedOffset;
            r.right = r.left + tabWidth;
            DrawFocusRect(htci->hexWindowDc, &r);
            r.left = p.x - xPressedOffset;
            r.right = r.left + tabWidth;
            // user note: r, tabWidth is now constant!
            cnt = htci->hexTabCount;
            hti = htci->hexTabs;
            index = 0;
            // determine tab index based on mouse position
            while(cnt--)
            {
                    // special case where the tab was moved all the way to the left
                if((!index && r.right <= (hti->rcTab.right + htci->hexTabTranslation)) ||
                    // special case where the tab was moved all the way to the right
                   (!cnt && r.left >= (hti->rcTab.left + htci->hexTabTranslation)) ||
                    // tabs intersect half
                    ((hti->rcTab.left + htci->hexTabTranslation) < r.left ? (hti->rcTab.right + htci->hexTabTranslation) - r.left : r.right - (hti->rcTab.left + htci->hexTabTranslation))
                        >= (tabWidth >> 1))
                {
                    if(index != pressedTabIndex)
                    {
                        htci->hexTabIndex = index;
                        htiMem = *(htci->hexTabs + pressedTabIndex);
                        if(pressedTabIndex > index)
                        {
                            htiMem.rcTab.left = !index ? 0 : (htci->hexTabs + (index - 1))->rcTab.right + style->hexTabPadding;
                            htiMem.rcTab.right = htiMem.rcTab.left + tabWidth;
                            hti = htci->hexTabs + index;
                            memmove(hti + 1, hti, sizeof(*htci->hexTabs) * (pressedTabIndex - index));
                            memcpy(hti, &htiMem, sizeof(htiMem));
                            moveAmount = tabWidth + style->hexTabPadding;
                            while(index++, hti++, index <= pressedTabIndex)
                            {
                                hti->rcTab.left += moveAmount;
                                hti->rcTab.right += moveAmount;
                            }
                            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
                        }
                        else // if(pressedTabIndex < index)
                        {
                            htiMem.rcTab.left = !index ? 0 : (htci->hexTabs + index)->rcTab.right - tabWidth;
                            htiMem.rcTab.right = htiMem.rcTab.left + tabWidth;
                            hti = htci->hexTabs + pressedTabIndex;
                            memmove(hti, hti + 1, sizeof(*htci->hexTabs) * (index - pressedTabIndex));
                            hti = htci->hexTabs + index;
                            memcpy(hti, &htiMem, sizeof(htiMem));
                            moveAmount = tabWidth + style->hexTabPadding;
                            while(index--, hti--, index >= pressedTabIndex)
                            {
                                hti->rcTab.left -= moveAmount;
                                hti->rcTab.right -= moveAmount;
                            }
                            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
                        }
                    }
                    break;
                }
                hti++;
                index++;
            }
        }
        xlastMoveFocusRect = -1;
        pressedTabIndex = -1;
        return 0;
    case WM_MOUSEMOVE:
        p = (POINT) {
            .x = GET_X_LPARAM(lParam),
            .y = GET_Y_LPARAM(lParam)
        };
        if(pressedTabIndex >= 0 && !onXButtonDown)
        {
            GetTabRect(&r, htci->hexTabs + pressedTabIndex, htci);
            tabWidth = r.right - r.left;
            // erase last focus rect
            if(xlastMoveFocusRect >= 0)
            {
                r.left = xlastMoveFocusRect - xPressedOffset;
                r.right = r.left + tabWidth;
                DrawFocusRect(htci->hexWindowDc, &r);
            }
            r.left = p.x - xPressedOffset;
            r.right = r.left + tabWidth;
            DrawFocusRect(htci->hexWindowDc, &r);
            xlastMoveFocusRect = p.x;
        }
        else
        {
            cnt = htci->hexTabCount;
            hti = htci->hexTabs;
            index = -1;
            while(cnt)
            {
                GetTabRect(&r, hti, htci);
                GetTabXRect(&r, &r, htci);
                if(PtInRect(&r, p))
                {
                    index = htci->hexTabCount - cnt;
                    break;
                }
                cnt--;
                hti++;
            }
            if(htci->hexTabOverXButtonIndex != index)
            {
                if(index >= 0)
                {
                    MoveToEx(htci->hexWindowDc, r.left, r.top, NULL);
                    LineTo(htci->hexWindowDc, r.right, r.bottom);
                    MoveToEx(htci->hexWindowDc, r.right, r.top, NULL);
                    LineTo(htci->hexWindowDc, r.left, r.bottom);
                }
                if(htci->hexTabOverXButtonIndex >= 0)
                {
                    DrawSingleTab(htci->hexTabs + htci->hexTabOverXButtonIndex, htci->hexTabOverXButtonIndex == htci->hexTabIndex, GetFocus() == hWnd, htci);
                }
                htci->hexTabOverXButtonIndex = index;
            }
        }
        return 0;
    case WM_KEYDOWN:
    {
        switch(wParam)
        {
        case VK_LEFT:
            if(htci->hexTabIndex)
            {
                if(GetFocus() == hWnd)
                {
                    // erase focus rect
                    GetTabRect(&r, htci->hexTabs + htci->hexTabIndex, htci);
                    DrawFocusRect(htci->hexWindowDc, &r);
                }
                wParam = htci->hexTabIndex - 1;
                goto set_tab;
            }
            return 0;
        case VK_RIGHT:
            if(htci->hexTabIndex + 1 != htci->hexTabCount)
            {
                if(GetFocus() == hWnd)
                {
                    // erase focus rect
                    GetTabRect(&r, htci->hexTabs + htci->hexTabIndex, htci);
                    DrawFocusRect(htci->hexWindowDc, &r);
                }
                wParam = htci->hexTabIndex + 1;
                goto set_tab;
            }
            return 0;
        }
        return 0;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

