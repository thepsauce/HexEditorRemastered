#include <stdio.h>
#include <stdlib.h>
#include "wincomps.h"

int main(int argc, char **argv)
{
    Resource_Init();
    InitClasses();
    Manager_Init();
    ShowWindow(Manager_GetMainWindow(), SW_NORMAL);
    UpdateWindow(Manager_GetMainWindow());
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if(msg.message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            switch((char) msg.wParam)
            {
            case 'O': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_OPEN, 0); break;
            case 'W': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_CLOSE, 0); break;
            case 'T': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_NEW, 0); break;
            case 'S': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_SAVE, 0); break;
            case 'C': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_COPY, 0); break;
            case 'V': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_PASTE, 0); break;
            case 'X': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_CUT, 0); break;
            case 'F': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_SEARCH, 0); break;
            case 'R': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_REPLACE, 0); break;
            case 'G': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_GOTO, 0); break;
            case 'Z': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_UNDO, 0); break;
            case 'Y': SendMessage(Manager_GetMainWindow(), WM_COMMAND, IDM_REDO, 0); break;
            }
        }
    }
    return 0;
}
