// ==WindhawkMod==
// @id              waitbmp-loader
// @name            Please Wait Bitmap Loader
// @description     Please Wait screen when changing themes.
// @version         0.1
// @author          Rounak, Erizur
// @include         explorer.exe
// @include         rundll32.exe
// @include         SystemSettings.exe
// @architecture    x86-64
// @compilerOptions -lcomdlg32 -lgdi32 -lGdiplus
// ==/WindhawkMod==

#include <windef.h>
#include <wingdi.h>
#ifdef _WIN64
#define STDCALL  __cdecl
#define SSTDCALL L"__cdecl"
#else
#define STDCALL  __stdcall
#define SSTDCALL L"__stdcall"
#endif
typedef unsigned __int64 QWORD;

#include <gdiplus.h>
#include <windhawk_utils.h>
#include <windows.h>
#include <thread>
#include <chrono>
using namespace Gdiplus;
typedef unsigned __int64 QWORD;

// change how fast it becomes gray
float intensity = 0.05f;
typedef void(STDCALL *CDimmedWindow_OnPaint_t)(class CDimmedWindow*, HDC);
CDimmedWindow_OnPaint_t CDimmedWindow_OnPaint;

HMODULE hThemeui;

void STDCALL CDimmedWindow_OnPaintHook(class CDimmedWindow* This, HDC hdc)
{
    HINSTANCE hInst = LoadLibrary(L"themeui.dll");
    WCHAR Buffer[32];
    LOGFONTW lf;
    WCHAR chText[32];
    if (LoadStringW(hInst, 2675, Buffer, 32))
    {
        if ( LoadStringW(hInst, 2674, lf.lfFaceName, 32) )
        {
            HFONT hf = CreateFont(_wtoi(Buffer)-6, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, lf.lfFaceName);
            SelectObject(hdc, hf);
            
            SetTextColor(hdc, RGB(0,0,0));
            SetBkMode(hdc, TRANSPARENT);
            if ( LoadStringW(hInst, 2673, chText, 260) )
            {
                UINT format = DT_CENTER | DT_VCENTER | DT_SINGLELINE;;
                POINT monitorPoint = { 0, 0 };
                auto v9 = MonitorFromPoint(monitorPoint, 1);
                struct tagMONITORINFO mi;
                mi.cbSize = 40;
                GetMonitorInfoW(v9, &mi);
                RECT rc = mi.rcMonitor;
                auto v10 = -GetSystemMetrics(77);
                auto SystemMetrics = GetSystemMetrics(76);
                OffsetRect(&rc, -SystemMetrics, v10);
                
                // blue border
                RECT textRect = rc;
                int width = 254;
                int height = 93;
                textRect.left = (rc.right - rc.left - width) / 2;
                textRect.right = textRect.left + width;
                textRect.top = (rc.bottom - rc.top - height) / 2;
                textRect.bottom = textRect.top + height;
                /*HBRUSH hBrush = CreateSolidBrush(RGB(197, 218, 231));
                FillRect(hdc, &textRect, hBrush);
                DeleteObject(hBrush);

                // black border
                textRect.left += 6;
                textRect.right -= 6;
                textRect.top += 6;
                textRect.bottom -= 6;
                hBrush = CreateSolidBrush(RGB(118,131,139));
                FillRect(hdc, &textRect, hBrush);
                DeleteObject(hBrush);

                // white rectangle
                textRect.left += 1;
                textRect.right -= 1;
                textRect.top += 1;
                textRect.bottom -= 1;
                hBrush = CreateSolidBrush(RGB(255,255,255));
                FillRect(hdc, &textRect, hBrush);
                DeleteObject(hBrush);*/

                //bitmap
                BITMAP bm;
                HBITMAP hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(601));
                GetObject(hBitmap, sizeof(bm), &bm);

                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP hbOld = (HBITMAP)SelectObject(memDC, hBitmap);

                BitBlt(hdc, textRect.left, textRect.top, bm.bmWidth, bm.bmHeight, memDC, 0, 0, SRCCOPY);

                SelectObject(memDC, hbOld);
                DeleteDC(memDC);

                // text
                DrawTextW(hdc, chText, -1, &rc, format);
            }
            DeleteObject(hf);
        }
    }
    FreeLibrary(hInst);
    HDC v4;
    if ( *((QWORD *)This + 6) && (v4 = (HDC)*((QWORD *)This + 5)) != 0)
    {
        RECT rc;
        HWND hwnd = WindowFromDC(hdc);
        GetClientRect(hwnd, &rc);

        int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = GetSystemMetrics(SM_YVIRTUALSCREEN);

        HDC hScreenDC = GetDC(NULL);
        HDC hMemDC = CreateCompatibleDC(hScreenDC);
        auto g_hbDesktop = CreateCompatibleBitmap(hScreenDC, cx, cy);
        SelectObject(hMemDC, g_hbDesktop);
        BitBlt(hMemDC, 0, 0, cx, cy, hScreenDC, x, y, SRCCOPY);
        GdiplusStartupInput gsi;
        ULONG_PTR ulToken;
        GdiplusStartup(&ulToken, &gsi, NULL);
        Bitmap *bm = new Bitmap(g_hbDesktop, NULL);

        //gray scale conversion:
        Gdiplus::ColorMatrix matrix = {
            0.3f * intensity + 1 - intensity, 0.3f * intensity, 0.3f * intensity, 0, 0,
            0.59f * intensity, 0.59f * intensity + 1 - intensity, 0.59f * intensity, 0, 0,
            0.11f * intensity, 0.11f * intensity, 0.11f * intensity + 1 - intensity, 0, 0,
            0, 0, 0, 1, 0,
            0, 0, 0, 0, 1
        };

        Gdiplus::ImageAttributes attr;
        attr.SetColorMatrix(&matrix,
            Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

        HDC hScrDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, bm->GetWidth(), bm->GetHeight());
        SelectObject(hScrDC, hBitmap);

        Gdiplus::Graphics *gr = new Gdiplus::Graphics(hScrDC);
        Gdiplus::REAL w = (Gdiplus::REAL)bm->GetWidth();
        Gdiplus::REAL h = (Gdiplus::REAL)bm->GetHeight();
        Gdiplus::RectF rect(0, 0, w, h);
        gr->DrawImage(bm, rect, 0, 0, w, h, Gdiplus::UnitPixel, &attr);

        BitBlt(hdc, 0, 0, cx, cy, hScrDC, x, y,SRCCOPY);

        DeleteDC(hScrDC);
        DeleteObject(hBitmap);
        delete gr;
        delete bm;
        DeleteObject(g_hbDesktop);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hScreenDC);
    }
}

WNDPROC DlgProc_orig;
LRESULT CALLBACK DlgProc_hook(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    class CDimmedWindow* WindowLongPtrW = (class CDimmedWindow*)GetWindowLongPtrW(hWnd, -21);
    LRESULT lr = DlgProc_orig(hWnd, uMsg, wParam, lParam);
    if (WindowLongPtrW)
    {
        // idk how this works
        CDimmedWindow_OnPaintHook(WindowLongPtrW, GetDC(*((HWND *)WindowLongPtrW + 3)));
    }
    return lr;
}


BOOL Wh_ModInit() {
    Wh_Log(L"Init");
    // private: int __cdecl CDimmedWindow::OnCreate(struct tagCREATESTRUCTW const *)
    WindhawkUtils::SYMBOL_HOOK hooks[] = {
    {
        {L"private: void __cdecl CDimmedWindow::OnPaint(struct HDC__ *)"},
        &CDimmedWindow_OnPaint,
        CDimmedWindow_OnPaintHook,
        false
    },
    {
        {L"private: static __int64 __cdecl CDimmedWindow::WndProc(struct HWND__ *,unsigned int,unsigned __int64,__int64)"},
        &DlgProc_orig,
        DlgProc_hook,
        false
    }};
    hThemeui = LoadLibraryW(L"themeui.dll");
    if (!WindhawkUtils::HookSymbols(hThemeui, hooks, ARRAYSIZE(hooks))) 
    {
        Wh_Log(L"Failed to hook DUI");
        return FALSE;
    }

    return TRUE;
}

// The mod is being unloaded, free all allocated resources.
void Wh_ModUninit() {
    Wh_Log(L"Uninit");
}
