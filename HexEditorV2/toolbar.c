#include "wincomps.h"

LRESULT CALLBACK HexToolbarControlProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HEXTBBUTTON *items;
    static int itemCount;
    PAINTSTRUCT ps;
    HDC hdc;
    HDC imageDc;
    HBITMAP oldHBm;
    HEXTBBUTTON *htb;
    int cnt;
    int len;
    int index;
    int middle;
    char *name;
    POINT mp;
    RECT rc;
    switch(msg)
    {
    case WM_CREATE: return 0;
    case WM_DESTROY: return 0;
    case WM_SIZE:
        //WORD newWidth  = LOWORD(lParam);
        //WORD newHeight = HIWORD(lParam);
        return 0;
    case WM_ERASEBKGND:
        hdc = (HDC) wParam;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, Manager_GetStyle()->hexTabBackgroundBrush);
        return 1;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        imageDc = CreateCompatibleDC(hdc);
        oldHBm = SelectObject(imageDc, NULL);
        SelectObject(hdc, Manager_GetStyle()->hexTabLinePen);
        cnt = itemCount;
        htb = items;
        while(cnt--)
        {
            if((htb->flags & HTB_SEPERATOR))
            {
                middle = (htb->rcIcon.left + htb->rcIcon.right) >> 1;
                MoveToEx(hdc, middle, htb->rcIcon.top, NULL);
                LineTo(hdc, middle, htb->rcIcon.bottom);
            }
            else
            {
                SelectObject(imageDc, htb->image);
                FillRect(hdc, &htb->rcIcon, Manager_GetStyle()->hexTabBrush);
                DrawEdge(hdc, &htb->rcIcon, htb->flags & HTB_PRESSED ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
                TransparentBlt(hdc, htb->rcIcon.left + 2, htb->rcIcon.top + 2, htb->rcIcon.right - htb->rcIcon.left - 4, htb->rcIcon.bottom - htb->rcIcon.top - 4,
                       imageDc, 0, 0, 16, 16, 0xFFFFFF);
            }
            htb++;
        }
        SelectObject(imageDc, oldHBm);
        DeleteDC(imageDc);
        EndPaint(hWnd, &ps);
        return 0;
    case HEXTB_INSERTITEM:
        htb = (HEXTBBUTTON*) lParam;
        index = (int) wParam;

        items = realloc(items, (itemCount + 1) * sizeof(*items));
        memmove(items + index + 1, items + index, sizeof(*items) * (itemCount + index));
        memcpy(items + index, htb, sizeof(*items));
        itemCount++;

        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
        return 0;
    case HEXTB_SETITEMS:
        // destroying old items
        htb = items;
        while(itemCount--)
        {
            free(htb->name);
            htb++;
        }
        free(items);

        cnt = (int) wParam;
        htb = (HEXTBBUTTON*) lParam;
        // creating new items
        items = malloc(cnt * sizeof(*items));
        memcpy(items, htb, sizeof(*items) * cnt);
        itemCount = cnt;
        // copying the string of each item
        htb = items;
        while(cnt--)
        {
            if(htb->name)
            {
                len = strlen(htb->name) + 1;
                name = malloc(len);
                htb->name = memcpy(name, htb->name, len);
            }
            htb++;
        }
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
        return 0;
    case WM_LBUTTONDOWN:
        mp = (POINT) {
            .x = GET_X_LPARAM(lParam),
            .y = GET_Y_LPARAM(lParam)
        };
        cnt = itemCount;
        htb = items;
        while(cnt--)
        {
            if(PtInRect(&htb->rcIcon, mp) && !(htb->flags & HTB_SEPERATOR))
            {
                htb->flags |= HTB_PRESSED;
                RedrawWindow(hWnd, &htb->rcIcon, NULL, RDW_INVALIDATE);
                break;
            }
            htb++;
        }
        return 0;
    case WM_LBUTTONUP:
        mp = (POINT) {
            .x = GET_X_LPARAM(lParam),
            .y = GET_Y_LPARAM(lParam)
        };
        cnt = itemCount;
        htb = items;
        while(cnt--)
        {
            if((htb->flags & HTB_PRESSED) && !(htb->flags & HTB_SEPERATOR))
            {
                if(PtInRect(&htb->rcIcon, mp))
                    SendMessage(GetParent(hWnd), WM_COMMAND, htb->id, 0);
                htb->flags = 0;
                RedrawWindow(hWnd, &htb->rcIcon, NULL, RDW_INVALIDATE);
            }
            htb++;
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}


