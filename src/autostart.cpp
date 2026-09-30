#include "autostart.hpp"

#ifdef _WIN32

#include <windows.h>
#include <string>

namespace {

constexpr const wchar_t* RUN_KEY  = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr const wchar_t* APP_NAME = L"MinCalendar";

std::wstring getExePath() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::wstring(buf, n);
}

} // namespace

namespace autostart {

bool isEnabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;

    wchar_t value[MAX_PATH] = {};
    DWORD size = sizeof(value);
    DWORD type = 0;
    LONG res = RegQueryValueExW(hKey, APP_NAME, nullptr, &type,
                                reinterpret_cast<LPBYTE>(value), &size);
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS || type != REG_SZ) return false;
    return _wcsicmp(value, getExePath().c_str()) == 0;
}

bool setEnabled(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return false;

    bool ok = false;
    if (enable) {
        std::wstring exe = getExePath();
        LONG res = RegSetValueExW(hKey, APP_NAME, 0, REG_SZ,
                                  reinterpret_cast<const BYTE*>(exe.c_str()),
                                  static_cast<DWORD>((exe.size() + 1) * sizeof(wchar_t)));
        ok = (res == ERROR_SUCCESS);
    } else {
        LONG res = RegDeleteValueW(hKey, APP_NAME);
        ok = (res == ERROR_SUCCESS || res == ERROR_FILE_NOT_FOUND);
    }

    RegCloseKey(hKey);
    return ok;
}

} // namespace autostart

#else // ---- Linux / macOS: stub ----

namespace autostart {
bool isEnabled()             { return false; }
bool setEnabled(bool)        { return false; }
}

#endif