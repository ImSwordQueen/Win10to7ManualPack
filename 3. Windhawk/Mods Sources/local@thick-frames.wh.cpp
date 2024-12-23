// ==WindhawkMod==
// @id              thick-frames
// @name            DWM thick frames test
// @version         0.1
// @include         dwm.exe
// @architecture    x86-64
// ==/WindhawkMod==

#include <uxtheme.h>
#include <windhawk_api.h>
#include <windhawk_utils.h>

bool(__thiscall* SetMargin)(struct _MARGINS*, int, int, int, int, struct _MARGINS const*);

bool __thiscall SetMargin_hook(struct _MARGINS* a1, int cxLeftWidth, int cxRightWidth, int cyTopHeight, int cyBottomHeight, struct _MARGINS const* a6) {
    return SetMargin(a1, cxLeftWidth, cxRightWidth, cyTopHeight, cyBottomHeight, a6);
}

bool(__thiscall* UpdateMarginsDependentOnStyle)(void*);
bool __thiscall UpdateMarginsDependentOnStyle_hook(void* pThis) {
    int ret = UpdateMarginsDependentOnStyle(pThis);
    struct tagRECT Rect;
    struct _MARGINS a3;
    DWORD* windowData;
    int dwStyle;
    DWORD dwExStyle;
    int v8;
    LONG v9;
    int v10;
    int v11;
    LONG right;
    int v13;
    int v14;
    LONG v15;
    LONG bottom;

    windowData = (DWORD*)*((unsigned long long*)pThis + 91);
    dwStyle = windowData[25];
    dwExStyle = windowData[26];

    // CalculateOutsideMargins from Windows 7
    Rect.left = 0;
    Rect.top = 0;
    Rect.right = 0;
    Rect.bottom = 0;
    AdjustWindowRectEx(&Rect, dwStyle | 0x40000, 0, dwExStyle);
    v8 = windowData[16];
    v9 = windowData[17];
    v10 = -Rect.left;
    v11 = -Rect.top;
    if (v8 > -Rect.left)
        v10 = windowData[16];
    a3.cxLeftWidth = v8 - v10;
    right = Rect.right;
    if (v9 > Rect.right)
        right = v9;
    v13 = v9 - right;
    v14 = windowData[18];
    a3.cxRightWidth = v13;
    v15 = windowData[19];
    if (v14 > v11)
        v11 = v14;
    a3.cyTopHeight = v14 - v11;
    bottom = Rect.bottom;
    if (v15 > Rect.bottom)
        bottom = v15;
    a3.cyBottomHeight = v15 - bottom;
    
    // Check NC size
    if (windowData[16] > 0 && windowData[17] > 0 && windowData[18] > 0 && windowData[19] > 0) {
        SetMargin((struct _MARGINS*)((char*)pThis + 628), a3.cxLeftWidth,
                  a3.cxRightWidth, a3.cyTopHeight, a3.cyBottomHeight, 0);
    }

    return ret;
}

BOOL Wh_ModInit() {
    Wh_Log(L"Init");

    HMODULE uDWM = LoadLibraryW(L"uDWM.dll");

    if (!uDWM) {
        Wh_Log(L"Failed to load uDWM.dll");
        return FALSE;
    }

    WindhawkUtils::SYMBOL_HOOK symbolHooks[] = {
        {
            {L"private: bool __cdecl CTopLevelWindow::UpdateMarginsDependentOnStyle(void)"},
            (void**)&UpdateMarginsDependentOnStyle,
            (void*)UpdateMarginsDependentOnStyle_hook,
        },
        {
            {L"bool __cdecl SetMargin(struct _MARGINS *,int,int,int,int,struct _MARGINS const *)"},
            (void**)&SetMargin,
            (void*)SetMargin_hook,
        },
    };

    if (!WindhawkUtils::HookSymbols(uDWM, symbolHooks, ARRAYSIZE(symbolHooks))) {
        Wh_Log(L"Error hooking");
        return FALSE;
    }
    return TRUE;
}