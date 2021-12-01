#include "resource.h"
#include <shlobj.h>

static HFONT *HFonts;
static UINT FontCount;

void Resource_AddFont(HFONT hFont)
{
    UINT newCnt = FontCount + 1;
    HFonts = realloc(HFonts, sizeof(hFont) * newCnt);
    *(HFonts + FontCount) = hFont;
    FontCount = newCnt;
}

HFONT Resource_GetFont(int index)
{
    return HFonts[index];
}

int Resource_GetFontCount(void)
{
    return FontCount;
}

static HBITMAP HImages[ICON_COUNT];

HBITMAP Resource_GetImage(int index)
{
    return HImages[index];
}

static char HexEditorFolder[MAX_PATH];

const char *Resource_GetHexEditorFolder(void)
{
    return HexEditorFolder;
}

void Resource_Init(void)
{
    SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, HexEditorFolder);
    int i = strlen(HexEditorFolder);
    HexEditorFolder[i++] = '\\';
    HexEditorFolder[i++] = '\\';
    HexEditorFolder[i++] = 'H';
    HexEditorFolder[i++] = 'e';
    HexEditorFolder[i++] = 'x';
    HexEditorFolder[i++] = 'E';
    HexEditorFolder[i++] = 'd';
    HexEditorFolder[i++] = 'i';
    HexEditorFolder[i++] = 't';
    HexEditorFolder[i++] = 'o';
    HexEditorFolder[i++] = 'r';
    HexEditorFolder[i++] = 0;

    HBITMAP hbmIcon = LoadImage(NULL, "graphics\\ToolbarSprites.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
    HDC hdc = GetDC(NULL);
    // setting up src
    HDC hdcSrc = CreateCompatibleDC(hdc);
    HBITMAP oldHbmSrc = SelectObject(hdcSrc, hbmIcon);
    // setting up dest
    HDC hdcDest = CreateCompatibleDC(hdc);
    HBITMAP hbmDest, oldHbmDest;
    oldHbmDest = SelectObject(hdcDest, NULL);

    for(int i = 0; i < ICON_COUNT; i++)
    {
        hbmDest = HImages[i] = CreateCompatibleBitmap(hdc, ICON_SIZE, ICON_SIZE);
        SelectObject(hdcDest, hbmDest);
        BitBlt(hdcDest, 0, 0, ICON_SIZE, ICON_SIZE, hdcSrc, i * ICON_SIZE, 0, SRCCOPY);
    }

    SelectObject(hdcSrc, oldHbmSrc);
    SelectObject(hdcDest, oldHbmDest);
    DeleteDC(hdcSrc);
    DeleteDC(hdcDest);
    ReleaseDC(NULL, hdc);
    DeleteObject(hbmIcon);

    // load fonts
    HFonts = malloc(sizeof(*HFonts) * 12);
    FontCount = 12;
    HFonts[0] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Consolas");
    HFonts[1] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Courier");
    HFonts[2] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Courier New");
    HFonts[3] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "DejaVu Sans Mono");
    HFonts[4] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Fixedsys");
    HFonts[5] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Letter Gothic Std");
    HFonts[6] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Liberation Mono");
    HFonts[7] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Lucida Console");
    HFonts[8] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Miriam Fixed");
    HFonts[9] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "OCR A Std");
    HFonts[10] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Prestige Elite Std");
    HFonts[11] = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                                 ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN, "Source Code Pro");

}
