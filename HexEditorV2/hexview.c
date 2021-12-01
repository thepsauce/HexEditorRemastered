#include "wincomps.h"

static HDC BufferDc;

static inline char HexToChar(char hex)
{ return hex >= 0 && hex <= 9 ? '0' + hex : hex + 'A' - 10; }

static void HexToString(char *buf, UCHAR ch, UINT radix)
{
    UINT len;
    switch(radix)
    {
    case 2: len = 8; break;
    case 8: len = 3; break;
    case 10: len = 3; break;
    case 16:
        *buf = HexToChar(ch >> 4);
        *(buf + 1) = HexToChar(ch & 0xF);
        return;
    default:
        return;
    }
    buf += len - 1;
    while(ch)
    {
        *buf = (ch % radix) + '0';
        ch /= radix;
        buf--;
        len--;
    }
    while(len--)
        *(buf--) = '0';
}

static void UpdateScrollBars(HEXVIEWINFO *hvi, UINT totalWidth, UINT totalHeight, UINT windowWidth, UINT windowHeight, BOOL fRedraw);
static void Measure(HEXVIEWINFO *hvi);
static void RedrawRange(HEXVIEWINFO *hvi, UINT index, UINT cnt);
static void EraseRange(HEXVIEWINFO *hvi, UINT index, UINT cnt);
static void RedrawAddress(HEXVIEWINFO *hvi, UINT startRow, UINT rowCnt);

static HDC SwapBufferDC(HWND hWnd, HEXVIEWINFO *hvi, UINT dyc)
{
    UINT newHeight;
    RECT r;
    HEXMEASUREMENTS *hxm = &hvi->measurements;
    HEXSTYLE *style = Manager_GetStyle();
    newHeight = hxm->totalHeight + dyc * hxm->charHeight;
    HDC hdc = GetDC(hWnd);
    HDC copyDc = CreateCompatibleDC(hdc);
    SetTextColor(hdc, style->hexTextColor);
    SelectObject(copyDc, style->globalFont);
    SelectObject(copyDc, hvi->buffer = CreateCompatibleBitmap(hdc, hxm->totalWidth, newHeight));
    SetTextColor(copyDc, style->hexTextColor);
    SetBkColor(copyDc, style->hexBackgroundColor);
    SetDCBrushColor(copyDc, style->hexBackgroundColor);
    r = (RECT) {
         .left = 0,
         .top = hxm->totalHeight,
         .right = hxm->totalWidth,
         .bottom = newHeight
    };
    FillRect(copyDc, &r, GetStockObject(DC_BRUSH));
    BitBlt(copyDc, 0, 0, hxm->totalWidth, hxm->totalHeight, BufferDc, 0, 0, SRCCOPY);
    DeleteDC(BufferDc);
    ReleaseDC(hWnd, hdc);
    BufferDc = copyDc;
    GetClientRect(hWnd, &r);
    hxm->totalHeight = newHeight;
    UpdateScrollBars(hvi, hxm->totalWidth, hxm->totalHeight, r.right, r.bottom, TRUE);
    RedrawAddress(hvi, hvi->chunkCount ? 1 + (hvi->chunkCount - 1) / hvi->columnCount : 0, dyc);
    return copyDc;
}

void HVI_ManagerRoutine46(HEXVIEWINFO *hvi, const RECT *r)
{
    HEXMEASUREMENTS *hxm = &hvi->measurements;
    UpdateScrollBars(hvi, hxm->totalWidth, hxm->totalHeight, r->right, r->bottom, TRUE);
    SelectObject(BufferDc, hvi->buffer);
}

// marker //
static void ToggleMark(HEXVIEWINFO *hvi, UINT index, COLORREF color)
{
    UINT cnt = hvi->markCount;
    MARK *marks = hvi->marks;
    UINT mi;
    UINT newCnt;
    UINT insertIndex = 0;
    for(UINT c = cnt; c--; insertIndex++, marks++)
    {
        mi = marks->index;
        // remove if marked
        if(mi == index)
        {
            memmove(marks, marks + 1, sizeof(*marks) * c);
            hvi->markCount--;
            goto skip;
        }
        if(mi > index)
            break;
    }
    hvi->markCount = newCnt = cnt + 1;
    MARK *newMarks = hvi->marks = realloc(hvi->marks, sizeof(*newMarks) * newCnt);
    newMarks += insertIndex;
    memmove(newMarks + 1, newMarks,  sizeof(*marks) * (cnt - insertIndex));
    newMarks->index = index;
    newMarks->color = color;
    skip:
    RedrawRange(hvi, index, 1);
    RedrawWindow(Manager_GetHexView(), NULL, NULL, RDW_INVALIDATE);
}

static void MoveMarkers(MARK *markers, UINT markCnt, UINT from, UINT by)
{
    while(markCnt)
    {
        if(markers->index >= from)
            break;
        markCnt--;
        markers++;
    }
    while(markCnt--)
    {
        markers->index += by;
        markers++;
    }
}

static UINT RemoveMarkers(MARK *markers, UINT markCnt, UINT from, UINT to, UINT by)
{
    const UINT mc = markCnt;
    UINT mFrom, mTo;
    while(markCnt)
    {
        if(markers->index >= from)
            break;
        markCnt--;
        markers++;
    }
    mFrom = mc - markCnt;
    while(markCnt)
    {
        if(markers->index >= to)
            break;
        markCnt--;
        markers++;
    }
    mTo = mc - markCnt;
    memmove(markers + mFrom, markers + mTo, sizeof(*markers) * (mc - mTo));
    while(markCnt--)
    {
        markers->index -= by;
        markers++;
    }
    return mTo - mFrom;
}

static void UpdateScrollBars(HEXVIEWINFO *hvi, UINT totalWidth, UINT totalHeight, UINT windowWidth, UINT windowHeight, BOOL fRedraw)
{
    //printf("[TOTAL: (%d, %d), WINDOW: (%d, %d)]\n", totalWidth, totalHeight, windowWidth, windowHeight);
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    hvi->xMaxScroll = totalWidth > windowWidth ? totalWidth - windowWidth : 0;
    si.nMin  = hvi->xMinScroll;
    si.nMax  = totalWidth;
    si.nPos  = hvi->xCurrentScroll = min(hvi->xCurrentScroll, hvi->xMaxScroll);
    si.nPage = windowWidth;
    SetScrollInfo(Manager_GetHexView(), SB_HORZ, &si, fRedraw);
    hvi->yMaxScroll = totalHeight > windowHeight ? totalHeight - windowHeight : 0;
    si.nMin  = hvi->yMinScroll;
    si.nMax  = totalHeight;
    si.nPos  = hvi->yCurrentScroll = min(hvi->yCurrentScroll, hvi->yMaxScroll);
    si.nPage = windowHeight;
    SetScrollInfo(Manager_GetHexView(), SB_VERT, &si, fRedraw);
}

static void MeasureRect(const HEXVIEWINFO *hvi, UINT index, RECT *rect)
{
    const HEXMEASUREMENTS *hxm = &hvi->measurements;
    const UINT columnCnt = hvi->columnCount;
    const UINT indexX = index % columnCnt;
    const UINT indexY = index / columnCnt;
    const UINT hexCharWidth = hxm->charWidth * (hvi->charLength + hvi->charGap);
    rect->left   = hxm->addressWidth + indexX * hexCharWidth;
    rect->top    = indexY * hxm->charHeight;
    rect->right  = rect->left + hexCharWidth;
    rect->bottom = rect->top + hxm->charHeight;
}

static void Measure(HEXVIEWINFO *hvi)
{
    HEXMEASUREMENTS *hxm = &hvi->measurements;
    HEXSTYLE *style = Manager_GetStyle();
    SelectObject(BufferDc, style->globalFont);

    TEXTMETRIC tm;
    GetTextMetrics(BufferDc, &tm);

    const UINT charLen = hvi->charLength;
    const UINT charGap = hvi->charGap;
    const UINT columnCnt = hvi->columnCount;
    const UINT addressWidth = (hvi->addressCount + 2) * tm.tmAveCharWidth;
    const UINT dumpOffset = hxm->dumpOffset;
    const UINT dumpStart = addressWidth + ((charLen + charGap) * columnCnt) * tm.tmAveCharWidth + dumpOffset;

    hxm->hexWidth = dumpStart - dumpOffset;
    hxm->addressWidth = addressWidth;
    hxm->dumpStart = dumpStart;
    hxm->charWidth = tm.tmAveCharWidth;
    hxm->charHeight = tm.tmHeight;
    hxm->totalWidth = dumpStart + columnCnt * tm.tmAveCharWidth;
    hxm->totalHeight = (1 + (hvi->chunkCount - 1) / columnCnt) * tm.tmHeight;
}

static void RedrawAddress(HEXVIEWINFO *hvi, UINT startRow, UINT rowCnt)
{
    if(!rowCnt)
        return;
    const HEXMEASUREMENTS *hxm = &hvi->measurements;
    const HEXSTYLE *style = Manager_GetStyle();

    SetBkColor(BufferDc, style->hexBackgroundColor);
    // draw address
    const UINT addressCnt = hvi->addressCount + 2;
    const UINT columnCnt = hvi->columnCount;

    UINT addressValue = startRow * columnCnt;
    char address[addressCnt];
    address[addressCnt - 2] = ':';
    address[addressCnt - 1] = ' ';
    UINT y = startRow * hxm->charHeight;
    // char -> hexadecimal
    while(1)
    {
        rowCnt--;
        for(UINT i = hvi->addressCount, av = addressValue; i--; )
        {
            address[i] = HexToChar(av & 0xF);
            av >>= 4;
        }
        TextOut(BufferDc, 0, y, address, addressCnt);
        if(!rowCnt)
            break;
        y += hxm->charHeight;
        addressValue += columnCnt;
    }
}

static void EraseRange(HEXVIEWINFO *hvi, UINT index, UINT cnt)
{
    if(!cnt)
        return;
    const HEXMEASUREMENTS *hxm = &hvi->measurements;

    const UINT charLen = hvi->charLength;
    const UINT charGap = hvi->charGap;
    const UINT columnCnt = hvi->columnCount;
    const UINT addressWidth = hxm->addressWidth;
    const UINT dumpStart = hxm->dumpStart;
    const UINT charWidth = hxm->charWidth;
    const UINT charHeight = hxm->charHeight;
    const UINT hexCharWidth = charWidth * (charLen + charGap);

    UINT startCol = index % columnCnt;
    UINT startRow = index / columnCnt;

    RECT r;
    r.left = addressWidth + startCol * hexCharWidth - 1;
    r.top = startRow * charHeight;
    r.bottom = r.top + charHeight;
    UINT dumpX = dumpStart + startCol * charWidth;
    LONG l;
    while(cnt--)
    {
        l = r.left;
        r.right = l + hexCharWidth + 1;
        FillRect(BufferDc, &r, GetStockObject(DC_BRUSH));
        r.left = dumpX - 1;
        r.right = dumpX + charWidth + 1;
        FillRect(BufferDc, &r, GetStockObject(DC_BRUSH));
        r.left = l;
        index++;
        if(!(index % columnCnt))
        {
            r.top += charHeight;
            r.bottom += charHeight;
            r.left = addressWidth - 1;
            dumpX = dumpStart;
        }
        else
        {
            r.left += hexCharWidth;
            dumpX += charWidth;
        }
    }
}

static void RedrawRange(HEXVIEWINFO *hvi, UINT start, UINT cnt)
{
    if(!cnt)
        return;
    const HEXMEASUREMENTS *hxm = &hvi->measurements;
    const HEXSTYLE *style = Manager_GetStyle();

    SetBkColor(BufferDc, style->hexBackgroundColor);

    const int caretIndex = hvi->caretIndex;
    const int subCaretIndex = hvi->subCaretIndex;
    const int selectIndex = hvi->selectIndex;
    int sll, slr;
    if(selectIndex < caretIndex)
    {
        sll = selectIndex;
        slr = caretIndex;
    }
    else
    {
        sll = caretIndex;
        slr = selectIndex;
    }
    const UINT radix = hvi->radix;
    const UINT charLen = hvi->charLength;
    const UINT charGap = hvi->charGap;
    const UINT columnCnt = hvi->columnCount;
    const UINT addressWidth = hxm->addressWidth;
    const UINT dumpStart = hxm->dumpStart;
    const UINT charWidth = hxm->charWidth;
    const UINT charHeight = hxm->charHeight;
    const UINT hexCharWidth = charWidth * (charLen + charGap);

    MARK *markers = hvi->marks;
    UINT markCnt = hvi->markCount;
    while(markCnt)
    {
        if(markers->index >= start)
            break;
        markCnt--;
        markers++;
    }

    UINT startCol = start % columnCnt;
    UINT startRow = start / columnCnt;

    UCHAR ch;
    UINT x, y;
    x = addressWidth + startCol * hexCharWidth;
    y = startRow * charHeight;
    char buf[charLen];
    UCHAR *chunk = hvi->chunk + start;
    UINT dumpX = dumpStart + startCol * charWidth;
    MARK *mark;
    while(cnt--)
    {
        ch = *(chunk++);
        HexToString(buf, ch, radix);
        // for dump drawing
        ch = max(ch, ' ');
        if(markCnt && start == markers->index)
        {
            mark = markers++;
            markCnt--;
        }
        else
            mark = NULL;
        if(hvi->isSelection && start >= sll && start <= slr)
        {
            SetBkColor(BufferDc, style->hexSelectionColor);
            TextOut(BufferDc, x, y, buf, charLen);
            TextOut(BufferDc, dumpX, y, (LPCSTR) &ch, 1);
            SetBkColor(BufferDc, style->hexBackgroundColor);
        }
        else if(start == sll && start == slr)
        {
            if(hvi->dumpPressed)
            {
                SetBkColor(BufferDc, style->hexForegroundColor);
                TextOut(BufferDc, x, y, buf, charLen);
            }
            else
            {
                SetBkColor(BufferDc, style->hexCaretColor);
                TextOut(BufferDc, x, y, buf, charLen);
                SetBkColor(BufferDc, style->hexForegroundColor);
                TextOut(BufferDc, x + subCaretIndex * charWidth, y, buf + subCaretIndex, 1);
            }
            SetBkColor(BufferDc, style->hexForegroundColor);
            TextOut(BufferDc, dumpX, y, (LPCSTR) &ch, 1);
            SetBkColor(BufferDc, style->hexBackgroundColor);
        }
        else if(mark)
        {
            SetBkColor(BufferDc, mark->color);
            TextOut(BufferDc, x, y, buf, charLen);
            TextOut(BufferDc, dumpX, y, (LPCSTR) &ch, 1);
            SetBkColor(BufferDc, style->hexBackgroundColor);
        }
        else
        {
            TextOut(BufferDc, x, y, buf, charLen);
            TextOut(BufferDc, dumpX, y, (LPCSTR) &ch, 1);
        }
        start++;
        if(!(start % columnCnt))
        {
            y += charHeight;
            x = addressWidth;
            dumpX = dumpStart;
        }
        else
        {
            x += hexCharWidth;
            dumpX += charWidth;
        }
    }
}

void HVI_UpdateBuffer(HEXVIEWINFO *hvi)
{
    RECT r;
    HEXMEASUREMENTS *hxm;
    HEXSTYLE *style;
    HBITMAP newHBm;

    Measure(hvi);
    hxm = &hvi->measurements;
    GetClientRect(Manager_GetHexView(), &r);
    UpdateScrollBars(hvi, hxm->totalWidth, hxm->totalHeight, r.right, r.bottom, FALSE);
    style = Manager_GetStyle();
    newHBm = CreateBitmap(hxm->totalWidth, hxm->totalHeight, 1, 32, NULL);
    // clear old bitmap
    DeleteObject(hvi->buffer);
    // set properties of file global buffer
    SelectObject(BufferDc, hvi->buffer = newHBm);
    SetTextColor(BufferDc, style->hexTextColor);
    SetBkColor(BufferDc, style->hexBackgroundColor);
    SetDCBrushColor(BufferDc, style->hexBackgroundColor);
    // try to prevent a bug, where sub caret goes outside of bounds
    hvi->subCaretIndex = min(hvi->subCaretIndex, hvi->charLength - 1);

    r = (RECT) {
         .left = 0,
         .top = 0,
         .right = hxm->totalWidth,
         .bottom = hxm->totalHeight
    };
    FillRect(BufferDc, &r, GetStockObject(DC_BRUSH));

    if(hvi->chunkCount)
    {
        RedrawAddress(hvi, 0, 1 + (hvi->chunkCount - 1) / hvi->columnCount);
        RedrawRange(hvi, 0, hvi->chunkCount);
    }
}

LRESULT CALLBACK HexViewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL shiftDown = FALSE;
    static BOOL dragging = FALSE;
    static MARK *pressedMarker;
    static CHOOSECOLOR cc;
    static COLORREF custClrs[16];
    static HMENU markerPopMenu;
    static HMENU popMenu;
    static char textBuffer[100];

    PAINTSTRUCT ps;
    HDC hdc;
    RECT *pr;
    RECT r;
    RECT rc;
    POINT mousePos;
    POINT p;

    MARK *marks;
    UINT markCnt;

    int xDelta, yDelta;
    int xNewPos, yNewPos;

    urcmode_t mode;
    UINT changeCapacity, nextChangeIndex, changeIndex, newChangeIndex;
    URCHANGE *change;

    URCHANGE urc;

    UINT i1, i2;
    UINT start, cnt;

    UINT eraseIndex, oldIndex, oldSelectIndex;

    UCHAR *chunk, *chars;
    UINT chunkCnt, lastCnt;
    LONG delta;
    UINT uDelta;
    UINT addRows;

    UINT res;
    UINT radix;
    UINT len;
    char *num, *numBase;

    SCROLLINFO si;

    HEXVIEWINFO *hvi = (HEXVIEWINFO*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    HEXMEASUREMENTS *hxm = hvi ? hxm = &hvi->measurements : NULL;
    HEXSTYLE *style = Manager_GetStyle();

    UINT fullCharLen;
    UINT index, subIndex;
    UINT xIndex;
    UCHAR uch;
    //printf("HEXVIEW MSG(%d)\n", msg);
    if(msg == WM_CREATE)
    {
        hdc = GetDC(hWnd);
        BufferDc = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);

        markerPopMenu = CreatePopupMenu();
        popMenu = CreatePopupMenu();
        AppendMenu(markerPopMenu, MF_STRING, (UINT_PTR) 0, "Color...");
        AppendMenu(markerPopMenu, MF_STRING, (UINT_PTR) 1, "Previous marker");
        AppendMenu(markerPopMenu, MF_STRING, (UINT_PTR) 2, "Next marker");
        SetMenuItemBitmaps(markerPopMenu, 0, MF_BYCOMMAND, Resource_GetImage(ICON_ID_COLOR), Resource_GetImage(ICON_ID_COLOR));
        SetMenuItemBitmaps(markerPopMenu, 1, MF_BYCOMMAND, Resource_GetImage(ICON_ID_ARROW_LEFT), Resource_GetImage(ICON_ID_ARROW_LEFT));
        SetMenuItemBitmaps(markerPopMenu, 2, MF_BYCOMMAND, Resource_GetImage(ICON_ID_ARROW_RIGHT), Resource_GetImage(ICON_ID_ARROW_RIGHT));
        AppendMenu(markerPopMenu, MF_SEPARATOR, (UINT_PTR) 0, NULL);
        HMENU addMenu = markerPopMenu;
        for(int i = 2; i--; )
        {
            AppendMenu(addMenu, MF_STRING, (UINT_PTR) 3, "&Insert\tINS");
            AppendMenu(addMenu, MF_STRING, (UINT_PTR) 4, "&Delete\tDEL");
            AppendMenu(addMenu, MF_SEPARATOR, (UINT_PTR) 0, NULL);
            AppendMenu(addMenu, MF_STRING, (UINT_PTR) IDM_UNDO, "&Undo\tCtrl + Z");
            AppendMenu(addMenu, MF_STRING, (UINT_PTR) IDM_REDO, "&Redo\tCtrl + Y");
            AppendMenu(addMenu, MF_SEPARATOR, (UINT_PTR) 0, NULL);
            AppendMenu(addMenu, MF_STRING, (UINT_PTR) IDM_COPY, "&Copy\tCtrl + C");
            AppendMenu(addMenu, MF_STRING, (UINT_PTR) IDM_PASTE, "&Paste\tCtrl + V");
            AppendMenu(addMenu, MF_STRING, (UINT_PTR) IDM_CUT, "Cu&t\tCtrl + X");
            SetMenuItemBitmaps(addMenu, 3, MF_BYCOMMAND, Resource_GetImage(ICON_ID_PLUS), Resource_GetImage(ICON_ID_PLUS));
            SetMenuItemBitmaps(addMenu, 4, MF_BYCOMMAND, Resource_GetImage(ICON_ID_RED_X), Resource_GetImage(ICON_ID_RED_X));
            SetMenuItemBitmaps(addMenu, IDM_UNDO, MF_BYCOMMAND, Resource_GetImage(ICON_ID_UNDO), Resource_GetImage(ICON_ID_UNDO));
            SetMenuItemBitmaps(addMenu, IDM_REDO, MF_BYCOMMAND, Resource_GetImage(ICON_ID_REDO), Resource_GetImage(ICON_ID_REDO));
            SetMenuItemBitmaps(addMenu, IDM_COPY, MF_BYCOMMAND, Resource_GetImage(ICON_ID_COPY), Resource_GetImage(ICON_ID_COPY));
            SetMenuItemBitmaps(addMenu, IDM_PASTE, MF_BYCOMMAND, Resource_GetImage(ICON_ID_PASTE), Resource_GetImage(ICON_ID_PASTE));
            SetMenuItemBitmaps(addMenu, IDM_CUT, MF_BYCOMMAND, Resource_GetImage(ICON_ID_CUT), Resource_GetImage(ICON_ID_CUT));

            addMenu = popMenu;
        }
        memset(&cc, 0, sizeof(cc));
        cc.hwndOwner = hWnd;
        cc.lStructSize = sizeof(cc);
        cc.lpCustColors = custClrs;
        cc.Flags = CC_RGBINIT | CC_FULLOPEN;
        return 0;
    }
    if(hvi)
    switch(msg)
    {
    case WM_SIZE:
        UpdateScrollBars(hvi, hxm->totalWidth, hxm->totalHeight, LOWORD(lParam), HIWORD(lParam), FALSE);
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        pr = &ps.rcPaint;
        SetDCBrushColor(hdc, style->hexBackgroundColor);
        r = (RECT) {
            .left = hxm->totalWidth,
            .top = 0,
            .right = pr->right,
            .bottom = pr->bottom
        };
        FillRect(hdc, &r, GetStockObject(DC_BRUSH));
        r = (RECT) {
            .left = 0,
            .top = hxm->totalHeight,
            .right = hxm->totalWidth,
            .bottom = pr->bottom
        };
        FillRect(hdc, &r, GetStockObject(DC_BRUSH));
        BitBlt(hdc, pr->left, pr->top, hxm->totalWidth - pr->left, hxm->totalHeight - pr->top,
                   BufferDc, pr->left + hvi->xCurrentScroll, pr->top + hvi->yCurrentScroll, SRCCOPY);
        EndPaint(hWnd, &ps);
        return 0;
    case HEXV_UNDO:
        if((changeIndex = hvi->changeIndex))
        {
            newChangeIndex = hvi->changeIndex - 1;
            change = hvi->changes + newChangeIndex;
            urcmode_t mode = change->mode;
            urc.index = change->index;
            urc.subIndex = change->subIndex;
            urc.chars = change->lastChars;
            urc.charCount = change->lastCharCount;
            urc.lastChars = change->chars;
            urc.lastCharCount = change->charCount;
            urc.mode = mode == URC_DELETE ? URC_INSERT : mode == URC_INSERT ? URC_DELETE : mode;
            hvi->changeIndex = newChangeIndex;
            hvi->caretIndex = change->index;
            hvi->subCaretIndex = change->subIndex;
            wParam = (WPARAM) &urc;
            lParam = 1;
            goto do_change;
        }
        return 0;
    case HEXV_REDO:
        if((changeIndex = hvi->changeIndex) != hvi->changeCount)
        {
            newChangeIndex = changeIndex + 1;
            change = hvi->changes + changeIndex;
            hvi->caretIndex = change->index;
            hvi->changeIndex = newChangeIndex;
            hvi->subCaretIndex = change->subIndex;
            wParam = (WPARAM) change;
            lParam = 1;
            goto do_change;
        }
        return 0;
    case HEXV_REPLACESELECTION:
        replace_selection:
        cnt = wParam;
        chars = (UCHAR*) lParam;
        if(hvi->isSelection)
        {
            if(hvi->selectIndex < hvi->caretIndex)
            {
                i1 = hvi->selectIndex;
                i2 = hvi->caretIndex;
            }
            else
            {
                i1 = hvi->caretIndex;
                i2 = hvi->selectIndex;
            }
            hvi->selectIndex = hvi->caretIndex = i1;
            hvi->isSelection = FALSE;
            if(cnt)
            {
                urc.mode = URC_DELINS;
                urc.index = i1;
                urc.subIndex = hvi->subCaretIndex;
                urc.chars = chars;
                urc.charCount = cnt;
                urc.lastCharCount = i2 - i1 + 1;
                wParam = (WPARAM) &urc;
                lParam = 0;
                goto do_change;
            }
            else
            {
                urc.mode = URC_DELETE;
                urc.index = i1;
                urc.subIndex = hvi->subCaretIndex;
                urc.lastCharCount = i2 - i1 + 1;
                wParam = (WPARAM) &urc;
                lParam = 0;
                goto do_change;
            }
        }
        else if(cnt)
        {
            urc.mode = URC_INSERT;
            urc.index = hvi->caretIndex;
            urc.subIndex = hvi->subCaretIndex;
            urc.chars = chars;
            urc.charCount = cnt;
            wParam = (WPARAM) &urc;
            lParam = 0;
            goto do_change;
        }
        else
            return 0;
        return 1;
    case HEXV_MAKESELECTION:
        return 0;
    case HEXV_SETCARET:
        set_caret:
        if(!hvi->chunkCount)
            return 0;
        oldIndex = hvi->caretIndex;
        oldSelectIndex = hvi->selectIndex;
        hvi->caretIndex = wParam;
        if(!lParam)
            hvi->selectIndex = wParam;
        if(oldIndex != wParam || lParam != hvi->isSelection)
        {
            if(hvi->isSelection)
            {
                if(oldIndex < wParam)
                {
                    eraseIndex = max(oldSelectIndex, wParam);
                    oldIndex = min(oldSelectIndex, oldIndex);
                    start = oldIndex;
                    cnt = eraseIndex - oldIndex;
                }
                else
                {
                    eraseIndex = min(oldSelectIndex, wParam);
                    oldIndex = max(oldSelectIndex, oldIndex);
                    start = eraseIndex;
                    cnt = oldIndex - eraseIndex;
                }
                // erase the caret/selection
                RedrawRange(hvi, start, cnt + 1);
            }
            else
                RedrawRange(hvi, oldIndex, 1);
            // update status bar
            if(lParam)
            {
                cnt = hvi->selectIndex < hvi->caretIndex ? wParam - hvi->selectIndex : hvi->selectIndex - wParam;
            }
        }
        hvi->isSelection = lParam;
        RedrawRange(hvi, wParam, 1);
        // update scroll bars
        GetClientRect(hWnd, &rc);
        MeasureRect(hvi, wParam, &r);
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        xDelta = r.right - rc.right - hvi->xCurrentScroll;
        yDelta = r.bottom - rc.bottom - hvi->yCurrentScroll;
        // scroll horizontally
        if(xDelta > 0 || (xDelta = r.left - hvi->xCurrentScroll) < 0)
        {
            ScrollWindowEx(hWnd, -xDelta, 0, NULL, NULL, NULL, NULL, 0);
            si.nPos = hvi->xCurrentScroll += xDelta;
            SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
        }
        // scroll vertically
        if(yDelta > 0 || (yDelta = r.top - hvi->yCurrentScroll) < 0)
        {
            ScrollWindowEx(hWnd, 0, -yDelta, NULL, NULL, NULL, NULL, 0);
            si.nPos = hvi->yCurrentScroll += yDelta;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        }
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
        return 1;
    case HEXV_DOCHANGE:
        do_change:
        change = (URCHANGE*) wParam;
        mode = change->mode;
        index = change->index;
        subIndex = change->subIndex;
        cnt = change->charCount;
        chars = change->chars;
        lastCnt = change->lastCharCount;
        chunk = hvi->chunk + index;
        chunkCnt = hvi->chunkCount;
        marks = hvi->marks;
        markCnt = hvi->markCount;
        if((mode == URC_SET || (mode == URC_DELINS && cnt == lastCnt)) && !memcmp(chunk, chars, cnt))
            return 0;
        // saving the change
        if(!lParam)
        {
            changeIndex = hvi->changeIndex;
            changeCapacity = hvi->changeCapacity;
            nextChangeIndex = changeIndex + 1;
            // grow array if it is too small
            if(changeCapacity < nextChangeIndex)
                hvi->changes = realloc(hvi->changes, sizeof(URCHANGE) * (changeCapacity = (changeCapacity + 1) * CHANGE_GROW_FACTOR));
            change = hvi->changes + changeIndex;
            change->mode = mode;
            change->index = index;
            change->subIndex = subIndex;
            if(lastCnt && (mode == URC_DELETE || mode == URC_DELINS))
                memcpy(change->lastChars = malloc(lastCnt), chunk, change->lastCharCount = lastCnt);
            if(cnt && mode != URC_DELETE)
                memcpy(change->chars = malloc(cnt), chars, change->charCount = cnt);
            hvi->changeCount = hvi->changeIndex = nextChangeIndex;
            hvi->changeCapacity = changeCapacity;
        }
        switch(mode)
        {
        case URC_SET:
            memcpy(chunk, chars, cnt);
            RedrawRange(hvi, index, cnt);
            break;
        case URC_INSERT:
            if(hvi->chunkCapacity < chunkCnt + cnt)
                chunk = (hvi->chunk = realloc(hvi->chunk, hvi->chunkCapacity = chunkCnt + cnt * 2)) + index;
            memmove(chunk + cnt, chunk, chunkCnt - index);
            memcpy(chunk, chars, cnt);
            // update markers
            MoveMarkers(marks, markCnt, index, cnt);
            uDelta = ((chunkCnt - 1) % hvi->columnCount) + cnt + 1;
            if((addRows = uDelta / hvi->columnCount))
                SwapBufferDC(hWnd, hvi, addRows);
            hvi->chunkCount += cnt;
            RedrawRange(hvi, index, hvi->chunkCount - index);
            break;
        case URC_DELETE:
            hvi->chunkCount -= lastCnt;
            memmove(chunk, chunk + lastCnt, hvi->chunkCount - index);
            hvi->selectIndex = hvi->caretIndex = min(hvi->caretIndex, hvi->chunkCount - 1);
            // update markers
            hvi->markCount -= RemoveMarkers(marks, markCnt, index, index + lastCnt, lastCnt);
            EraseRange(hvi, hvi->chunkCount, lastCnt);
            RedrawRange(hvi, hvi->caretIndex, hvi->chunkCount - index);
            break;
        case URC_DELINS:
            hvi->selectIndex = hvi->caretIndex;
            delta = cnt - lastCnt;
            if(delta < 0) // more is removed
            {
                memmove(chunk + cnt, chunk + lastCnt, chunkCnt - lastCnt);
                EraseRange(hvi, chunkCnt + delta, -delta);
            }
            else if(!delta) // removed and inserted amounts are equal
            {
                memcpy(chunk, chars, cnt);
                RedrawRange(hvi, index, cnt);
                break;
            }
            else // more is inserted
            {
                if(hvi->chunkCapacity < chunkCnt + delta)
                {
                    hvi->chunk = realloc(hvi->chunk, hvi->chunkCapacity = chunkCnt + delta * 2);
                    chunk = hvi->chunk + index;
                }
                uDelta = ((chunkCnt - 1) % hvi->columnCount) + cnt + 1;
                if((addRows = uDelta / hvi->columnCount))
                    SwapBufferDC(hWnd, hvi, addRows);
                memmove(chunk + delta, chunk, chunkCnt - index - delta);
            }
            memcpy(chunk, chars, cnt);
            hvi->chunkCount += delta;
            RedrawRange(hvi, index, hvi->chunkCount - index);
            break;
        }
        wParam = index;
        lParam = 0;
        goto set_caret;
    case WM_KEYDOWN:
        // left -> move sub caret to the left
        // right -> * right
        // up -> * up
        // down -> * down
        switch(wParam)
        {
        case VK_SHIFT: shiftDown = hvi->dumpPressed = TRUE; return 0;
        case VK_DELETE:
            lParam = 0;
            if(hvi->isSelection)
            {
                wParam = 0;
                goto replace_selection;
            }
            urc.mode = URC_DELETE;
            urc.index = hvi->caretIndex;
            urc.subIndex = hvi->subCaretIndex;
            urc.lastCharCount = 1;
            wParam = (WPARAM) &urc;
            goto do_change;
        case VK_INSERT:
            wParam = 4;
            lParam = (LPARAM) memset(textBuffer, 0, 4);
            goto replace_selection;
        case VK_HOME:
            wParam = (hvi->caretIndex / hvi->columnCount) * hvi->columnCount;
            lParam = shiftDown || dragging;
            goto set_caret;
        case VK_END:
            wParam = min((hvi->caretIndex / hvi->columnCount + 1) * hvi->columnCount - 1, hvi->chunkCount - 1);
            lParam = shiftDown || dragging;
            goto set_caret;
        case VK_UP:
            if(hvi->caretIndex < hvi->columnCount)
                return 0;
            wParam = hvi->caretIndex - hvi->columnCount;
            lParam = shiftDown || dragging;
            goto set_caret;
        case VK_DOWN:
            if(hvi->caretIndex >= max(hvi->chunkCount, hvi->columnCount) - hvi->columnCount)
                return 0;
            wParam = hvi->caretIndex + hvi->columnCount;
            lParam = shiftDown || dragging;
            goto set_caret;
        case VK_LEFT:
            if(hvi->caretIndex && (!hvi->subCaretIndex || hvi->dumpPressed))
            {
                if(!hvi->dumpPressed)
                    hvi->subCaretIndex = hvi->charLength - 1;
                wParam = hvi->caretIndex - 1;
                lParam = shiftDown || dragging;
                goto set_caret;
            }
            else if(hvi->subCaretIndex)
            {
                hvi->subCaretIndex--;
                wParam = hvi->caretIndex;
                lParam = shiftDown || dragging;
                goto set_caret;
            }
            break;
        case VK_RIGHT:
            if(hvi->subCaretIndex == hvi->charLength - 1 || hvi->dumpPressed)
            {
                if(hvi->caretIndex == hvi->chunkCount - 1)
                    return 0;
                if(!hvi->dumpPressed)
                    hvi->subCaretIndex = 0;
                wParam = hvi->caretIndex + 1;
                lParam = shiftDown || dragging;
                goto set_caret;
            }
            else
            {
                hvi->subCaretIndex++;
                wParam = hvi->caretIndex;
                lParam = shiftDown || dragging;
                goto set_caret;
            }
            break;
        }
        return 0;
    case WM_KEYUP:
        if(wParam == VK_SHIFT)
            shiftDown = FALSE;
        return 0;
    case WM_CHAR:
        index = hvi->caretIndex;
        subIndex = hvi->subCaretIndex;
        // edit it like a text file
        if(hvi->dumpPressed)
        {
            urc.mode = URC_SET;
            urc.index = index;
            urc.subIndex = subIndex;
            urc.chars = ((UCHAR*) &wParam) + 1;
            urc.charCount = 1;
            lParam = (LPARAM) &urc;
            goto do_change;
        }
        uch = wParam >= '0' && wParam <= '9' ? wParam - '0' :
              wParam >= 'a' && wParam <= 'f' ? wParam - 'a' + 10 :
              wParam >= 'A' && wParam <= 'F' ? wParam - 'A' + 10 : -1;
        radix = hvi->radix;
        if(uch < 0 || uch >= radix)
            return 0;
        uch = *(hvi->chunk + index);
        len = hvi->charLength;
        num = numBase = malloc(len);
        num += len - 1;
        for(int l = len; l--; )
        {
            *(num--) = subIndex == l ? uch : uch % radix;
            uch /= radix;
        }
        res = 0;
        num = numBase;
        while(len--)
        {
            res *= radix;
            res += *num;
            num++;
        }
        free(numBase);
        res = min(res, 255);
        urc.mode = URC_SET;
        urc.index = index;
        urc.subIndex = subIndex;
        urc.chars = (UCHAR*) &res;
        urc.charCount = 1;
        lParam = (LPARAM) &urc;
        goto do_change;
    case WM_LBUTTONUP: dragging = FALSE; return 0;
    case WM_MOUSEMOVE:
        if(!(wParam & MK_LBUTTON) || (wParam & MK_CONTROL))
            return 0;
    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        mousePos = (POINT) {
            .x = GET_X_LPARAM(lParam),
            .y = GET_Y_LPARAM(lParam)
        };
        fullCharLen = hvi->charLength + hvi->charGap;
        mousePos.x += hvi->xCurrentScroll;
        mousePos.y += hvi->yCurrentScroll;
        mousePos.y /= hxm->charHeight;
        // check if dump was pressed
        if(mousePos.x >= hxm->dumpStart)
        {
            mousePos.x = (mousePos.x - hxm->dumpStart) / hxm->charWidth;
            if(mousePos.x < hvi->columnCount)
            {
                index = mousePos.x + mousePos.y * hvi->columnCount;
                if(index < 0 || index >= hvi->chunkCount)
                    return 0;
                hvi->dumpPressed = TRUE;
                if(wParam & MK_CONTROL)
                    ToggleMark(hvi, index, style->hexMarkerColor);
                else
                {
                    wParam = index;
                    lParam = dragging || shiftDown;
                    dragging = TRUE;
                    goto set_caret;
                }
            }
            return 0;
        }
        mousePos.x -= hxm->addressWidth;
        xIndex = mousePos.x /= hxm->charWidth;
        mousePos.x /= fullCharLen;
        if(mousePos.x < 0 || mousePos.x >= hvi->columnCount)
            return 0;
        index = mousePos.x + mousePos.y * hvi->columnCount;
        if(index < 0 || index >= hvi->chunkCount)
            return 0;
        hvi->dumpPressed = FALSE;
        // compute sub index
        subIndex = max(0, min(xIndex % fullCharLen, hvi->charLength - 1));
        hvi->subCaretIndex = subIndex;
        if(wParam & MK_CONTROL)
        {
            ToggleMark(hvi, index, style->hexMarkerColor);
            dragging = TRUE;
            return 0;
        }
        wParam = index;
        lParam = dragging || shiftDown;
        dragging = TRUE;
        goto set_caret;
    case WM_CONTEXTMENU:
        GetCursorPos(&mousePos);
        TrackPopupMenuEx(popMenu, TPM_LEFTALIGN | TPM_TOPALIGN, mousePos.x, mousePos.y, hWnd, NULL);
        return 0;
    case WM_COMMAND:
    {
        marks = pressedMarker;
        index = (pressedMarker - hvi->marks);
        markCnt = hvi->markCount;
        // context menu
        switch(wParam)
        {
        case 0: // color
            ChooseColor(&cc);
            pressedMarker->color = cc.rgbResult;
            RedrawRange(hvi, pressedMarker->index, 1);
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
            return 0;
        case 1: // previous
            if(index)
            {
                wParam = (marks - 1)->index;
                lParam = shiftDown;
                dragging = TRUE;
                goto set_caret;
            }
            return 0;
        case 2: // next
            if(index + 1 < markCnt)
            {
                wParam = (marks + 1)->index;
                lParam = shiftDown;
                dragging = TRUE;
                goto set_caret;
            }
            return 0;
        case 3: // insert
            SendMessage(hWnd, WM_KEYDOWN, VK_INSERT, 0);
            return 0;
        case 4: // delete
            SendMessage(hWnd, WM_KEYDOWN, VK_DELETE, 0);
            return 0;
        }
        return SendMessage(GetParent(hWnd), msg, wParam, lParam);
    }
    case WM_RBUTTONDOWN:
        mousePos = (POINT) {
            .x = GET_X_LPARAM(lParam),
            .y = GET_Y_LPARAM(lParam)
        };
        p = (POINT) {
            .x = mousePos.x + hvi->xCurrentScroll,
            .y = mousePos.y + hvi->yCurrentScroll
        };
        ClientToScreen(hWnd, &mousePos);
        markCnt = hvi->markCount;
        marks = hvi->marks;
        while(markCnt--)
        {
            MeasureRect(hvi, marks->index, &r);
            if(PtInRect(&r, p))
            {
                pressedMarker = marks;
                TrackPopupMenuEx(markerPopMenu, TPM_LEFTALIGN | TPM_TOPALIGN, mousePos.x, mousePos.y, hWnd, NULL);
                return 0;
            }
            marks++;
        }
        TrackPopupMenuEx(popMenu, TPM_LEFTALIGN | TPM_TOPALIGN, mousePos.x, mousePos.y, hWnd, NULL);
        return 0;
    case WM_HSCROLL:
        switch(LOWORD(wParam))
        {
        case SB_PAGEUP:         xNewPos = hvi->xCurrentScroll - hxm->charWidth * 8; break;
        case SB_PAGEDOWN:       xNewPos = hvi->xCurrentScroll + hxm->charWidth * 8; break;
        case SB_LINEUP:         xNewPos = hvi->xCurrentScroll - hxm->charWidth; break;
        case SB_LINEDOWN:       xNewPos = hvi->xCurrentScroll + hxm->charWidth; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:  xNewPos = HIWORD(wParam); break;
        default:                xNewPos = hvi->xCurrentScroll;
        }
        xNewPos = max(0, xNewPos);
        xNewPos = min(hvi->xMaxScroll, xNewPos);
        if(xNewPos == hvi->xCurrentScroll)
            break;

        xDelta = xNewPos - hvi->xCurrentScroll;

        hvi->xCurrentScroll = xNewPos;

        ScrollWindowEx(hWnd, -xDelta, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE);
        UpdateWindow(hWnd);

        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS;
        si.nPos   = xNewPos;
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
        break;
    case WM_MOUSEWHEEL:
    case WM_VSCROLL:
        GetClientRect(hWnd, &r);

        if(msg == WM_MOUSEWHEEL)
        {
            int deltaWheel = GET_WHEEL_DELTA_WPARAM(wParam);
            yNewPos = hvi->yCurrentScroll + (deltaWheel > 0 ? -hxm->charHeight : hxm->charHeight);
        }
        else
        {
            switch(LOWORD(wParam))
            {
            case SB_PAGEUP:         yNewPos = hvi->yCurrentScroll - hxm->charHeight * 8; break;
            case SB_PAGEDOWN:       yNewPos = hvi->yCurrentScroll + hxm->charHeight * 8; break;
            case SB_LINEUP:         yNewPos = hvi->yCurrentScroll - hxm->charHeight; break;
            case SB_LINEDOWN:       yNewPos = hvi->yCurrentScroll + hxm->charHeight; break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:  yNewPos = HIWORD(wParam); break;
            default:                yNewPos = hvi->yCurrentScroll;
            }
        }

        yNewPos = max(0, yNewPos);
        yNewPos = min(hvi->yMaxScroll, yNewPos);

        if(yNewPos == hvi->yCurrentScroll)
            break;

        yDelta = yNewPos - hvi->yCurrentScroll;

        hvi->yCurrentScroll = yNewPos;

        ScrollWindowEx(hWnd, 0, -yDelta, NULL, NULL, NULL, NULL, SW_INVALIDATE);
        UpdateWindow(hWnd);

        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS;
        si.nPos   = yNewPos;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
