// ==WindhawkMod==
// @id              aero-peek-fix
// @name            Aero Peek Fix for Windhawk
// @description     Fixes Aero Peek Windows on Windows 10 1903+
// @author          Brawllux(Original Mod)
// @include         explorer.exe
// @compilerOptions -lgdi32
// ==/WindhawkMod==

#include <windows.h>
#include <thread>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void MessageLoop() {
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

BOOL Wh_ModInit() {
    Wh_Log(L"Init");

    HINSTANCE hInstance = GetModuleHandle(NULL);
    HWND hwndConsole = GetConsoleWindow();
    ShowWindow(hwndConsole, SW_HIDE);

    const wchar_t CLASS_NAME[] = L"TransparentWindowClass";

    WNDCLASS wc = { };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"Transparent Window",
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return FALSE;
    }

    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);

    std::thread messageLoopThread(MessageLoop);
    messageLoopThread.detach();

    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Uninit");
    UnregisterClass(L"TransparentWindowClass", GetModuleHandle(NULL));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
        }
        return 0;

        case WM_ERASEBKGND:
            return 1;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
