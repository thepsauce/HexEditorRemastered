#include "wincomps.h"
#include <ctype.h>

#define OFD_LOAD 0
#define OFD_SAVE 1

BOOL OpenFileDialog(OPENFILENAME *ofn, UCHAR mode)
{
    char *filePath = malloc(MAX_PATH);
    *filePath = 0;
    char *fileTitle = malloc(MAX_PATH);
    *fileTitle = 0;
    memset(ofn, 0, sizeof(*ofn));
    ofn->lStructSize = sizeof(*ofn);
    ofn->lpstrFile = filePath;
    ofn->nMaxFile = MAX_PATH;
    ofn->lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
    ofn->nFilterIndex = 1;
    ofn->lpstrFileTitle = fileTitle;
    ofn->nMaxFileTitle = MAX_PATH;
    ofn->Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    return mode == OFD_LOAD ? GetOpenFileName(ofn) : GetSaveFileName(ofn);
}

UINT WINAPI EditCopy(HWND hWnd, HEXVIEWINFO *hvi)
{
    char *lpstr;
    HGLOBAL hglbCopy;
    if(!hvi->isSelection || !OpenClipboard(hWnd))
        return 0;
    if(hvi->selectIndex == hvi->caretIndex)
    {
        CloseClipboard();
        return 0;
    }
    UINT i1, i2, c;
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
    c = i2 - i1 + 1;
    hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (c + 1) * sizeof(*hglbCopy));
    if(!hglbCopy)
    {
        CloseClipboard();
        return 0;
    }

    EmptyClipboard();

    lpstr = GlobalLock(hglbCopy);
    UCHAR *chunk = hvi->chunk + i1;
    memcpy(lpstr, chunk, c);
    *(lpstr + c) = 0;
    GlobalUnlock(hglbCopy);

    SetClipboardData(CF_TEXT, hglbCopy);

    CloseClipboard();
    return c;
}

BOOL WINAPI EditPaste(HWND hWnd, HEXVIEWINFO *hvi)
{
    HGLOBAL hglb;
    char *lpstr;

    if(!OpenClipboard(hWnd))
        return FALSE;
    if(IsClipboardFormatAvailable(CF_TEXT))
    {
        hglb = GetClipboardData(CF_TEXT);
        if(hglb)
        {
            lpstr = GlobalLock(hglb);
            if(lpstr)
            {
                SendMessage(Manager_GetHexView(), HEXV_REPLACESELECTION, strlen(lpstr), (LPARAM) lpstr);
                GlobalUnlock(hglb);
            }
        }
    }
    //else if(IsClipboardFormatAvailable())
    CloseClipboard();
    return TRUE;
}

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT r;
    static const int cParts = 3;
    static int paParts[3];

    int msgBoxCode;

    int tabCnt;

    OPENFILENAME ofn;
    HEXVIEWINFO *hvi;
    HEXTABINFO hti;
    UINT copyCnt;
    PAINTSTRUCT ps;
    //printf("MAIN WINDOW MSG(%d)\n", msg);
    switch(msg)
    {
    case WM_CREATE:
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_CLOSE:
        if(MessageBox(hWnd, "Are you sure you want to exit?", "Warning", MB_ICONEXCLAMATION | MB_APPLMODAL | MB_YESNO) == IDNO)
            return 0;
        Manager_Destroy();
        SendMessage(hWnd, WM_COMMAND, (WPARAM) IDM_CLOSE_ALL, (LPARAM) 0);
        break;
    case WM_SIZE:
        GetClientRect(hWnd, &r);
        MoveWindow(Manager_GetToolbar(), 0, 0, r.right, 40, TRUE);
        r.top += 40;
        MoveWindow(Manager_GetTabControl(), 0, r.top, r.right, 22, TRUE);
        r.top += 22;
        MoveWindow(Manager_GetHexView(), r.left, r.top, r.right - r.left, r.bottom - r.top - 22, TRUE);
        paParts[0] = r.right / 2;
        paParts[1] = r.right * 3 / 4;
        paParts[2] = r.right;
        SendMessage(Manager_GetStatusBar(), SB_SETPARTS, (WPARAM) cParts, (LPARAM) paParts);
        MoveWindow(Manager_GetStatusBar(), 0, r.bottom - 22, r.right, 22, TRUE);
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_HOTKEY:
    case WM_COMMAND:
        switch(((UCHAR) wParam))
        {
        case IDM_NEW:
            hti.hexInfo = Manager_CreateHexViewInfo(NULL, NULL);
            hti.name = "new";
            SendMessage(Manager_GetTabControl(), HEXTC_INSERTTAB, SendMessage(Manager_GetTabControl(), HEXTC_GETITEMCOUNT, 0, 0), (LPARAM) &hti);
            return 0;
        case IDM_CLOSE_ALL:
            tabCnt = SendMessage(Manager_GetTabControl(), HEXTC_GETITEMCOUNT, 0, 0);
            while(tabCnt--)
                SendMessage(hWnd, WM_COMMAND, IDM_CLOSE, (LPARAM) 0);
            return 0;
        case IDM_CLOSE:
            if(!(hvi = Manager_GetActiveHexViewInfo()))
                return 0;
            if(!hvi->isRecentFile)
            {
                msgBoxCode = MessageBox(hWnd, "Do you want to save the file before closing?", "Warning", MB_ICONEXCLAMATION | MB_APPLMODAL | MB_YESNOCANCEL);
                switch(msgBoxCode)
                {
                case IDCANCEL: return 1;
                case IDYES: if(!Manager_SaveActiveHexViewInfoFile()) goto saveAs;
                }
            }
            SendMessage(Manager_GetTabControl(), HEXTC_REMOVETAB, SendMessage(Manager_GetTabControl(), HEXTC_GETCURSEL, 0, 0), 0);
            return 0;
        case IDM_OPEN:
            if(OpenFileDialog(&ofn, OFD_LOAD))
            {
                tabCnt = SendMessage(Manager_GetTabControl(), HEXTC_GETITEMCOUNT, 0, 0);
                while(tabCnt--)
                {
                    hvi = (HEXVIEWINFO*) SendMessage(Manager_GetTabControl(), HEXTC_GETITEM, tabCnt, 0);
                    if(!strcasecmp(hvi->filePath, ofn.lpstrFile))
                    {
                        free(ofn.lpstrFile);
                        free(ofn.lpstrFileTitle);
                        SendMessage(Manager_GetTabControl(), HEXTC_SETCURSEL, tabCnt, 0);
                        return 0;
                    }
                }
                hti.hexInfo = Manager_CreateHexViewInfo(NULL, ofn.lpstrFile);
                hti.name = ofn.lpstrFileTitle;
                SendMessage(Manager_GetTabControl(), HEXTC_INSERTTAB, SendMessage(Manager_GetTabControl(), HEXTC_GETITEMCOUNT, 0, 0), (LPARAM) &hti);
                free(ofn.lpstrFileTitle);
            }
            return 0;
        case IDM_SAVE_AS:
            saveAs:
            if(!(hvi = Manager_GetActiveHexViewInfo()) || !OpenFileDialog(&ofn, OFD_SAVE))
                return 0;
            hvi->filePath = ofn.lpstrFile;
            hvi->isRecentFile = TRUE;
            SendMessage(Manager_GetTabControl(), HEXTC_RENAMETAB, SendMessage(Manager_GetTabControl(), HEXTC_GETCURSEL, 0, 0), (LPARAM) ofn.lpstrFileTitle);
            free(ofn.lpstrFileTitle);
            Manager_SaveActiveHexViewInfoFile();
            return 0;
        case IDM_SAVE:
            if(!(hvi = Manager_GetActiveHexViewInfo()) || hvi->isRecentFile)
                return 0;
            if(!Manager_SaveActiveHexViewInfoFile())
                goto saveAs;
            return 0;
        case IDM_COPY:
            if((hvi = Manager_GetActiveHexViewInfo()))
                EditCopy(hWnd, hvi);
            return 0;
        case IDM_PASTE:
            if((hvi = Manager_GetActiveHexViewInfo()))
                EditPaste(hWnd, hvi);
            return 0;
        case IDM_CUT:
            if((hvi = Manager_GetActiveHexViewInfo()) && (copyCnt = EditCopy(hWnd, hvi)))
                SendMessage(Manager_GetHexView(), HEXV_REPLACESELECTION, 0, 0);
            return 0;
        case IDM_UNDO: SendMessage(Manager_GetHexView(), HEXV_UNDO, 0, 0); return 0;
        case IDM_REDO: SendMessage(Manager_GetHexView(), HEXV_REDO, 0, 0); return 0;
        case IDM_SEARCH: ShowWindow(Manager_GetSearchBox(), SW_NORMAL); return 0;
        case IDM_REPLACE: ShowWindow(Manager_GetReplaceBox(), SW_NORMAL); return 0;
        case IDM_GOTO: ShowWindow(Manager_GetGotoBox(), SW_NORMAL); return 0;
        case IDM_ENVIRONMENT: ShowWindow(Manager_GetSettingsBox(), SW_NORMAL); return 0;
        }
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

