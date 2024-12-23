// ==WindhawkMod==
// @id              disk-pie-chart
// @name            Disk Pie Chart - Antialiased edition
// @description     Makes the graph in disk properties a pie chart again
// @version         0.2.1
// @author          teknixstuff
// @github          https://github.com/teknixstuff
// @include         *
// @architecture    x86-64
// @compilerOptions -lgdi32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Disk Pie Chart - Antialiased edition
In Windows 10, they changed the graph in disk properties to be a circle.
This mod changes it back to a pie chart.
Based on aubymori's original (aliased) version.

This version includes an antialiasing option to closer match Windows 7/8/8.1.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- clientedge: false
  $name: Client edge on colors
  $description: Render an inset border around the colors on the legend, like Windows XP and prior.
- antialiasing: 8
  $name: Antialiasing level
  $description: 0 = Windows Vista and below, 8 = Windows 7/8/8.1.
*/
// ==/WindhawkModSettings==

#include <windhawk_api.h>
#include <windhawk_utils.h>
#include <wingdi.h>

#ifdef _WIN64

#define STDCALL  __cdecl
#define SSTDCALL L"__cdecl"

#else

#define STDCALL  __fastcall
#define SSTDCALL L"__stdcall"

#endif

struct {
    bool clientedge;
    int antialiasing;
} settings;

#define IDC_DRV_FIRST                   0x3840

#define IDC_DRV_USEDCOLOR               (IDC_DRV_FIRST+0x03)
#define IDC_DRV_FREECOLOR               (IDC_DRV_FIRST+0x04)
#define IDC_DRV_CACHECOLOR              697

#define IDC_DRV_PIE                     (IDC_DRV_FIRST+0x0b)

// Pie RGB codes
const COLORREF c_crPieColors[] =
{
    RGB(  0,   0, 255),      // Blue            Used
    RGB(255,   0, 255),      // Red-Blue        Free
    RGB(  0,   0, 255),      // Light Blue      Cache Used
    RGB(  0,   0, 128),      // 1/2 Blue        Shadow Used
    RGB(128,   0, 128),      // 1/2 Red-Blue    Shadow Free
    RGB(  0,   0, 128),      // 1/2 Red-Blue    Shadow Cache Used
};

// Pie Color Types
enum
{
    DP_USEDCOLOR = 0,           // Used Color
    DP_FREECOLOR,               // Free Color
    DP_CACHECOLOR,              // Cache Color
    DP_USEDSHADOW,              // Used Shadow Color
    DP_FREESHADOW,              // Free Shadow Color
    DP_CACHESHADOW,             // Cache Shadow Color
    DP_TOTAL_COLORS     // # of entries
};

/**************************************************************************
// Performs SQR calculation
**************************************************************************/
int IntSqrt(DWORD dwNum)
{
    // We will keep shifting dwNum left and look at the top two bits.

    // initialize sqrt and remainder to 0.
    DWORD dwSqrt = 0, dwRemain = 0, dwTry;
    int i;

    // We iterate 16 times, once for each pair of bits.
    for (i=0; i<16; ++i)
    {
        // Mask off the top two bits of dwNum and rotate them into the
        // bottom of the remainder
        dwRemain = (dwRemain<<2) | (dwNum>>30);

        // Now we shift the sqrt left; next we'll determine whether the
        // new bit is a 1 or a 0.
        dwSqrt <<= 1;

        // This is where we double what we already have, and try a 1 in
        // the lowest bit.
        dwTry = (dwSqrt << 1) + 1;

        if (dwRemain >= dwTry)
        {
            // The remainder was big enough, so subtract dwTry from
            // the remainder and tack a 1 onto the sqrt.
            dwRemain -= dwTry;
            dwSqrt |= 0x01;
        }

        // Shift dwNum to the left by 2 so we can work on the next few
        // bits.
        dwNum <<= 2;
    }

    return(dwSqrt);
}

/**************************************************************************
    STDMETHODIMP Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPer1000, DWORD dwPerCache1000, const COLORREF *lpColors)
**************************************************************************/
STDMETHODIMP Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPer1000, DWORD dwPerCache1000, const COLORREF *lpColors)
{
    if (lprc == NULL || lpColors == NULL)
    {
        return E_INVALIDARG;
    }

    // The majority of this code came from "drawpie.c"
    const LONG c_lShadowScale = 6;       // ratio of shadow depth to height
    const LONG c_lAspectRatio = 2;      // ratio of width : height of ellipse

    // We make sure that the aspect ratio of the pie-chart is always preserved 
    // regardless of the shape of the given rectangle
    // Stabilize the aspect ratio now...
    LONG lHeight = lprc->bottom - lprc->top;
    LONG lWidth = lprc->right - lprc->left;
    LONG lTargetHeight = (lHeight * c_lAspectRatio <= lWidth? lHeight: lWidth / c_lAspectRatio);
    LONG lTargetWidth = lTargetHeight * c_lAspectRatio;     // need to adjust because w/c * c isn't always == w

    // Shrink the rectangle on both sides to the correct size
    lprc->top += (lHeight - lTargetHeight) / 2;
    lprc->bottom = lprc->top + lTargetHeight;
    lprc->left += (lWidth - lTargetWidth) / 2;
    lprc->right = lprc->left + lTargetWidth;

    // Compute a shadow depth based on height of the image
    LONG lShadowDepth = lTargetHeight / c_lShadowScale;

    // check dwPer1000 to ensure within bounds
    if(dwPer1000 > 1000)
        dwPer1000 = 1000;

    // Now the drawing function
    int cx, cy, rx, ry, x[2], y[2];
    int uQPctX10;
    RECT rcItem;
    HRGN hEllRect, hEllipticRgn, hRectRgn;
    HBRUSH hBrush, hOldBrush;
    HPEN hPen, hOldPen;

    rcItem = *lprc;
    rcItem.left = lprc->left;
    rcItem.top = lprc->top;
    rcItem.right = lprc->right - rcItem.left;
    rcItem.bottom = lprc->bottom - rcItem.top - lShadowDepth;

    rx = rcItem.right / 2;
    cx = rcItem.left + rx - 1;
    ry = rcItem.bottom / 2;
    cy = rcItem.top + ry - 1;
    if (rx<=10 || ry<=10)
    {
        return S_FALSE;
    }

    rcItem.right = rcItem.left + 2 * rx;
    rcItem.bottom = rcItem.top + 2 * ry;

    // Translate all parts to caresian system
    int iLoop;

    for(iLoop = 0; iLoop < 2; iLoop++)
    {
        DWORD       dwPer;

        switch(iLoop)
        {
        case 0:
            dwPer = dwPer1000;
            break;
        case 1:
            dwPer = dwPerCache1000;
            break;
        }

        // Translate to first quadrant of a Cartesian system
        uQPctX10 = (dwPer % 500) - 250;
        if (uQPctX10 < 0)
        {
            uQPctX10 = -uQPctX10;
        }

        if (uQPctX10 < 120)
        {
            x[iLoop] = IntSqrt(((DWORD)rx*(DWORD)rx*(DWORD)uQPctX10*(DWORD)uQPctX10)
                /((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

            y[iLoop] = IntSqrt(((DWORD)rx*(DWORD)rx-(DWORD)x[iLoop]*(DWORD)x[iLoop])*(DWORD)ry*(DWORD)ry/((DWORD)rx*(DWORD)rx));
        }
        else
        {
            y[iLoop] = IntSqrt((DWORD)ry*(DWORD)ry*(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)
                /((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

            x[iLoop] = IntSqrt(((DWORD)ry*(DWORD)ry-(DWORD)y[iLoop]*(DWORD)y[iLoop])*(DWORD)rx*(DWORD)rx/((DWORD)ry*(DWORD)ry));
        }

        // Switch on the actual quadrant
        switch (dwPer / 250)
        {
        case 1:
            y[iLoop] = -y[iLoop];
            break;

        case 2:
            break;

        case 3:
            x[iLoop] = -x[iLoop];
            break;

        default: // case 0 and case 4
            x[iLoop] = -x[iLoop];
            y[iLoop] = -y[iLoop];
            break;
        }

        // Now adjust for the center.
        x[iLoop] += cx;
        y[iLoop] += cy;

        // Hack to get around bug in NTGDI
        x[iLoop] = x[iLoop] < 0 ? 0 : x[iLoop];
    }

    // Draw the shadows using regions (to reduce flicker).
    hEllipticRgn = CreateEllipticRgnIndirect(&rcItem);
    OffsetRgn(hEllipticRgn, 0, lShadowDepth);
    hEllRect = CreateRectRgn(rcItem.left, cy, rcItem.right, cy+lShadowDepth);
    hRectRgn = CreateRectRgn(0, 0, 0, 0);
    CombineRgn(hRectRgn, hEllipticRgn, hEllRect, RGN_OR);
    OffsetRgn(hEllipticRgn, 0, -(int)lShadowDepth);
    CombineRgn(hEllRect, hRectRgn, hEllipticRgn, RGN_DIFF);

    // Always draw the whole area in the free shadow
    hBrush = CreateSolidBrush(lpColors[DP_FREESHADOW]);
    if(hBrush)
    {
        FillRgn(hdc, hEllRect, hBrush);
        DeleteObject(hBrush);
    }

    // Draw the used cache shadow if the disk is at least half used.
    if( (dwPerCache1000 != dwPer1000) && (dwPer1000 > 500) &&
         (hBrush = CreateSolidBrush(lpColors[DP_CACHESHADOW]))!=NULL)
    {
        DeleteObject(hRectRgn);
        hRectRgn = CreateRectRgn(x[0], cy, rcItem.right, lprc->bottom);
        CombineRgn(hEllipticRgn, hEllRect, hRectRgn, RGN_AND);
        FillRgn(hdc, hEllipticRgn, hBrush);
        DeleteObject(hBrush);
    }

    // Draw the used shadow only if the disk is at least half used.
    if( (dwPer1000-(dwPer1000-dwPerCache1000) > 500) && (hBrush = CreateSolidBrush(lpColors[DP_USEDSHADOW]))!=NULL)
    {
        DeleteObject(hRectRgn);
        hRectRgn = CreateRectRgn(x[1], cy, rcItem.right, lprc->bottom);
        CombineRgn(hEllipticRgn, hEllRect, hRectRgn, RGN_AND);
        FillRgn(hdc, hEllipticRgn, hBrush);
        DeleteObject(hBrush);
    }

    DeleteObject(hRectRgn);
    DeleteObject(hEllipticRgn);
    DeleteObject(hEllRect);

    hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    hOldPen = (HPEN__*) SelectObject(hdc, hPen);

    // if per1000 is 0 or 1000, draw full elipse, otherwise, also draw a pie section.
    // we might have a situation where per1000 isn't 0 or 1000 but y == cy due to approx error,
    // so make sure to draw the ellipse the correct color, and draw a line (with Pie()) to
    // indicate not completely full or empty pie.
    hBrush = CreateSolidBrush(lpColors[DP_USEDCOLOR]);
    hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

    Ellipse(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);

    if( (dwPer1000 != 0) && (dwPer1000 != 1000) )
    {
        // Display Free Section
        hBrush = CreateSolidBrush(lpColors[DP_FREECOLOR]);
        hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

        Pie(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom, rcItem.left, cy, x[0], y[0]);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);

        if( (x[0] != x[1]) && (y[0] != y[1]) )
        {
            // Display Cache Used dispostion
            hBrush = CreateSolidBrush(lpColors[DP_CACHECOLOR]);
            hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

            Pie(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom, x[0], y[0], x[1], y[1]);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hBrush);
        }
    }

    // Outline to bottom and sides of pie
    Arc(hdc, rcItem.left, rcItem.top+lShadowDepth, rcItem.right - 1, rcItem.bottom+lShadowDepth - 1,
        rcItem.left, cy+lShadowDepth, rcItem.right, cy+lShadowDepth-1);
    MoveToEx(hdc, rcItem.left, cy, NULL);
    LineTo(hdc, rcItem.left, cy+lShadowDepth);
    MoveToEx(hdc, rcItem.right-1, cy, NULL);
    LineTo(hdc, rcItem.right-1, cy+lShadowDepth);

    // Draw vertical lines to complete pie pieces
    if(dwPer1000 > 500 && dwPer1000 < 1000)
    {
        // Used piece
        MoveToEx(hdc, x[0], y[0], NULL);
        LineTo(hdc, x[0], y[0]+lShadowDepth);
    }

    if(dwPerCache1000 > 500 && dwPerCache1000 < 1000)
    {
        // Used Cache piece
        MoveToEx(hdc, x[1], y[1], NULL);
        LineTo(hdc, x[1], y[1]+lShadowDepth);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    return S_OK;    // Everything worked fine
}

/* Color indicators */
typedef void (STDCALL *_DrvPrshtDrawItem_t)(void *, LPDRAWITEMSTRUCT);
_DrvPrshtDrawItem_t _DrvPrshtDrawItem_orig;
void STDCALL _DrvPrshtDrawItem_hook(
    void             *lpps,
    LPDRAWITEMSTRUCT  lpdi
)
{
    switch (lpdi->CtlID)
    {
        case IDC_DRV_USEDCOLOR:
        case IDC_DRV_FREECOLOR:
        case IDC_DRV_CACHECOLOR:
        {
            COLORREF crPieClr;
            int      nPieClr;

            switch (lpdi->CtlID)
            {
                case IDC_DRV_USEDCOLOR:
                    nPieClr = DP_USEDCOLOR;
                    break;
                case IDC_DRV_FREECOLOR:
                    nPieClr = DP_FREECOLOR;
                    break;
                case IDC_DRV_CACHECOLOR:
                    nPieClr = DP_CACHECOLOR;
                    break;
            }
            crPieClr = c_crPieColors[nPieClr];

            if (crPieClr)
            {
                RECT rc;
                CopyRect(&rc, &lpdi->rcItem);

                HBRUSH hbrPieClr = CreateSolidBrush(crPieClr);
                FillRect(
                    lpdi->hDC,
                    &rc,
                    hbrPieClr
                );
                DeleteObject(hbrPieClr);

                if (settings.clientedge)
                {
                    HBRUSH hbrShadow, hbrDkShadow, hbrLight, hbrHilight;
                    hbrShadow = GetSysColorBrush(COLOR_3DSHADOW);
                    hbrDkShadow = GetSysColorBrush(COLOR_3DDKSHADOW);
                    hbrLight = GetSysColorBrush(COLOR_3DLIGHT);
                    hbrHilight = GetSysColorBrush(COLOR_3DHILIGHT);

                    rc.right = rc.left + 2;
                    FillRect(
                        lpdi->hDC,
                        &rc,
                        hbrDkShadow
                    );
                    rc.right = lpdi->rcItem.right;
                    rc.bottom = rc.top + 2;
                    FillRect(
                        lpdi->hDC,
                        &rc,
                        hbrDkShadow
                    );
                    rc.bottom = lpdi->rcItem.bottom;
                    rc.left = rc.right - 2;
                    FillRect(
                        lpdi->hDC,
                        &rc,
                        hbrLight
                    );
                    rc.left = lpdi->rcItem.left;
                    rc.top = rc.bottom - 2;
                    FillRect(
                        lpdi->hDC,
                        &rc,
                        hbrLight
                    );
                    rc.top = lpdi->rcItem.top;
                    rc.right = rc.left + 1;
                    FillRect(
                        lpdi->hDC,
                        &rc,
                        hbrShadow
                    );
                    rc.right = lpdi->rcItem.right;
                    rc.bottom = rc.top + 1;
                    FillRect(
                        lpdi->hDC,
                        &rc,
                        hbrShadow
                    );
                    rc.bottom = lpdi->rcItem.bottom;
                    rc.left = rc.right - 1;
                    FillRect(
                        lpdi->hDC,
                        &rc,
                        hbrHilight
                    );
                    rc.left = lpdi->rcItem.left;
                    rc.top = rc.bottom - 1;
                    FillRect(
                        lpdi->hDC,
                        &rc,
                        hbrHilight
                    );
                }
            }
            break;
        }
        default:
            _DrvPrshtDrawItem_orig(
                lpps, lpdi
            );
    }
}

/* Pie itself */
typedef int (STDCALL *DrawPie_t)(
    /* Function got mangled on x86 and has an extra argument */
#ifndef _WIN64
    float,
#endif
    HDC,
    LPRECT,
    DWORD,
    DWORD
);
DrawPie_t DrawPie_orig;
int STDCALL DrawPie_hook(
#ifndef _WIN64
    float  asshole,
#endif
    HDC    hDC,
    LPRECT lprc,
    DWORD  dwPer1000,
    DWORD  dwPerCache1000
)
{
    Wh_Log(L"dwPer1000: %d, dwPerCache1000: %d", dwPer1000, dwPerCache1000);

    HWND hWnd = WindowFromDC(hDC);
    if (hWnd)
    {
        HWND hPar = GetParent(hWnd);
        if (hPar)
        {
            HDC hDDC = GetDC(hPar);
            if (hDDC)
            {
                RECT rc;
                GetWindowRect(hWnd, &rc);
                MapWindowPoints(
                    HWND_DESKTOP,
                    hPar,
                    (LPPOINT)&rc,
                    2
                );

                if (settings.antialiasing == 0) {
                    rc.left -= 39;
                    rc.bottom += 5;
                    rc.right += 26;
                    Draw3dPie(hDDC, &rc, dwPer1000, dwPerCache1000, c_crPieColors);
                } else {
                    RECT rc_scaled;
                    rc_scaled.left = 0;
                    rc_scaled.top = 0;
                    rc_scaled.right = (rc.right-rc.left)*settings.antialiasing;
                    rc_scaled.bottom = (rc.bottom-rc.top)*settings.antialiasing;

                    rc.left -= 39;
                    rc.bottom += 5;
                    rc.right += 26;

                    HDC hSDC = CreateCompatibleDC(hDDC);
                    HBITMAP hSBMP = CreateCompatibleBitmap(hDDC, rc_scaled.right, rc_scaled.bottom);
                    SelectObject(hSDC, hSBMP);
                    COLORREF bkColor = GetBkColor(hDDC);
                    HBRUSH bkBrush = CreateSolidBrush(bkColor);
                    SelectObject(hSDC, bkBrush);
                    Rectangle(hSDC, rc_scaled.left-1, rc_scaled.top, rc_scaled.right+1, rc_scaled.bottom);
                    DeleteObject(bkBrush);

                    Draw3dPie(hSDC, &rc_scaled, dwPer1000, dwPerCache1000, c_crPieColors);

                    SetStretchBltMode(hDDC, HALFTONE);
                    StretchBlt(hDDC, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
                               hSDC, rc_scaled.left, rc_scaled.top, rc_scaled.right-rc_scaled.left, rc_scaled.bottom-rc_scaled.top, SRCCOPY);
                }

                ReleaseDC(hPar, hDDC);
            }
        }
    }

    return 0;
}

void LoadSettings(void)
{
    settings.clientedge = Wh_GetIntSetting(L"clientedge");
    settings.antialiasing = Wh_GetIntSetting(L"antialiasing");
}

BOOL Wh_ModInit(void)
{
    LoadSettings();

    HMODULE hShell32 = LoadLibraryW(L"shell32.dll");
    if (!hShell32)
    {
        Wh_Log(L"Failed to load shell32.dll");
        return FALSE;
    }

    WindhawkUtils::SYMBOL_HOOK hooks[] = {
        {
            {
                L"void "
                SSTDCALL
                L" _DrvPrshtDrawItem(struct DRIVEPROPSHEETPAGE const *,struct tagDRAWITEMSTRUCT const *)"
            },
            (void **)&_DrvPrshtDrawItem_orig,
            (void *)_DrvPrshtDrawItem_hook,
            false
        },
        {
            {
                #ifdef _WIN64
                L"DrawPie"
                #else
                L"_DrawPie@20"
                #endif
            },
            (void **)&DrawPie_orig,
            (void *)DrawPie_hook,
            false
        }
    };

    if (!WindhawkUtils::HookSymbols(
        hShell32,
        hooks,
        ARRAYSIZE(hooks)
    ))
    {
        Wh_Log(L"Failed to hook _DrvPrshtDrawItem");
        return FALSE;
    }

    return TRUE;
}

void Wh_ModSettingsChanged(void)
{
    LoadSettings();
}