// ==WindhawkMod==
// @id              uiribbon-fix
// @name            Fix Thy UIRibbon Offset
// @version         0.1
// @author          BabeImOnFire
// @include         explorer.exe
// @include         wordpad.exe
// @include         mspaint.exe
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*

# Fix Thy UIRibbon Offset

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

*/
// ==/WindhawkModReadme==

#include <libloaderapi.h>

const int DEFAULT_RETURN_VALUE = 0;
int (WINAPI *GetSystemMetricsForDpi1)(int, int);
int (WINAPI *GetSystemMetrics1)(int);

int WINAPI hookGetSystemMetricsForDpi(int nIndex, int dpi) {
    return (nIndex == SM_CXFRAME) ? DEFAULT_RETURN_VALUE : GetSystemMetricsForDpi1(nIndex, dpi);
}
int WINAPI hookGetSystemMetrics(int nIndex) {
    return (nIndex == SM_CXFRAME) ? DEFAULT_RETURN_VALUE : GetSystemMetrics1(nIndex);
}

BOOL Wh_ModInit() {
    Wh_Log(L"is this thing even working???");

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!Wh_SetFunctionHook((void *)GetProcAddress(user32, "GetSystemMetricsForDpi"), (void *)hookGetSystemMetricsForDpi, (void **)&GetSystemMetricsForDpi1)) {
        Wh_Log(L"GetSystemMetricsForDpi is a failure!!");
        return FALSE;
    }
    if (!Wh_SetFunctionHook((void *)GetProcAddress(user32, "GetSystemMetrics"), (void *)hookGetSystemMetrics, (void **)&GetSystemMetrics1)) {
        Wh_Log(L"GetSystemMetrics is a failure!!");
        return FALSE;
    }
	if (!user32) {
        Wh_Log(L"User32 is a failure!!");
        return FALSE;
    }
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Unload now!!");
}