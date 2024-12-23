// ==WindhawkMod==
// @id              personalization-themes
// @name            Personalization Custom Themes
// @description     Custom Default Themes
// @version         0.1
// @author          Rounak, Erizur
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lcomdlg32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
*/
// ==/WindhawkModReadme==

#include <windhawk_utils.h>
#include <stringapiset.h>
#include <strsafe.h>
#include <regex>
#include <string>
#include <string_view>
#include <ios>
#include <sstream>
#include <iomanip>

using namespace std::string_literals;

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

using CompareStringOrdinal_t = decltype(&CompareStringOrdinal);
CompareStringOrdinal_t CompareStringOrdinal_orig;

int WINAPI CompareStringOrdinal_hook(LPCWCH lpString1, int cchCount1, LPCWCH lpString2, int cchCount2, WINBOOL bIgnoreCase)
{
    int res = CompareStringOrdinal_orig(lpString1, cchCount1, lpString2, cchCount2, bIgnoreCase);
    if (lpString1 && lpString2)
    {
        if (wcsstr(lpString1, L".theme") != nullptr
            && wcsstr(lpString2, L".theme")!= nullptr)
        {
            Wh_Log(L"%s, %s, %d", lpString1, lpString2,res);
                if ((wcsstr(lpString1, L"aero.theme") != nullptr
                    || wcsstr(lpString1, L"aerodark.theme") != nullptr
                    || wcsstr(lpString1, L"architecture.theme") != nullptr
                    || wcsstr(lpString1, L"characters.theme") != nullptr
                    || wcsstr(lpString1, L"landscapes.theme") != nullptr
                    || wcsstr(lpString1, L"nature.theme") != nullptr
                    || wcsstr(lpString1, L"scenes.theme") != nullptr
                    || wcsstr(lpString1, L"IT.theme") != nullptr
                    || wcsstr(lpString1, L"US.theme") != nullptr)
                    &&  wcsstr(lpString2, L"spotlight.theme") != nullptr           // needs to be the last theme
                )   
                    return 2;
            return res;
        }
    }
    return res;
}
std::string hexStr(const uint8_t* data, size_t len) {
    std::stringstream ss;
    ss << std::hex;

    for (size_t i{}; i < len; ++i) {
        ss << std::setw(2) << std::setfill('0') << (int)data[i];
        ss << ' ';
    }

    return ss.str();
}

// The mod is being initialized, load settings, hook functions, and do other
// initialization stuff if required.
BOOL Wh_ModInit() {
    Wh_Log(L"Init");
    HMODULE hShell32M =  GetModuleHandle(L"api-ms-win-core-string-l1-1-0.dll");
    void* origFunc = (void*)GetProcAddress(hShell32M, "CompareStringOrdinal");
    Wh_SetFunctionHook(origFunc, (void*)CompareStringOrdinal_hook, (void**)&CompareStringOrdinal_orig);

    return TRUE;
}

// The mod is being unloaded, free all allocated resources.
void Wh_ModUninit() {
    Wh_Log(L"Uninit");
}
