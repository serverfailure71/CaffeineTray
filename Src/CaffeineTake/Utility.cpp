// CaffeineTake - Keep your computer awake.
// 
// Copyright (c) 2020-2021 VacuityBox
// Copyright (c) 2022      serverfailure71
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PCH.hpp"
#include "Config.hpp"
#include "Utility.hpp"

#include <array>
#include <filesystem>

#include <ObjBase.h>
#include <ObjIdl.h>
#include <Psapi.h>
#include <ShlGuid.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <shlwapi.h>
#include <VersionHelpers.h>
#include <WinNls.h>

#if defined(FEATURE_CAFFEINETAKE_LOCKSCREEN_DETECTION)
    #include <wtsapi32.h>
#endif

namespace CaffeineTake {

auto UTF8ToUTF16 (const std::string_view str) -> std::optional<std::wstring>
{
    // Get size.
    auto size = ::MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        nullptr,
        0
    );

    if (size <= 0)
    {
        return std::nullopt;
    }

    // Convert.
    auto utf16 = std::wstring(size, L'\0');
    auto ret = ::MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        utf16.data(),
        static_cast<int>(utf16.size())
    );

    if (ret == 0)
    {
        return std::nullopt;
    }

    return utf16;
}

auto UTF16ToUTF8 (const std::wstring_view str) -> std::optional<std::string>
{
    // Get size.
    auto size = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size <= 0)
    {
        return std::nullopt;
    }
    
    // Convert.
    auto utf8 = std::string(size, '\0');
    auto ret = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        utf8.data(),
        static_cast<int>(utf8.size()),
        nullptr,
        nullptr
    );

    if (ret == 0)
    {
        return std::nullopt;
    }

    return utf8;
}

auto GetAppDataPath () -> std::filesystem::path
{
    auto appDataPath = std::array<wchar_t, MAX_PATH>();
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, appDataPath.data())))
    {
        const auto dir = std::wstring(appDataPath.data());
        return std::filesystem::path(dir);
    }

    return std::filesystem::path();
}

auto IsSessionLocked () -> SessionState
{
#if defined(FEATURE_CAFFEINETAKE_LOCKSCREEN_DETECTION)
    auto wtsinfo  = std::make_unique<WTSINFOEXW*>();
    auto retBytes = DWORD{ 0 };

    auto retCode = WTSQuerySessionInformationW(
        WTS_CURRENT_SERVER_HANDLE,
        WTS_CURRENT_SESSION,
        WTSSessionInfoEx,
        reinterpret_cast<LPWSTR*>(wtsinfo.get()),
        &retBytes
    );

    if (!retCode)
    {
        return SessionState::Unlocked;
    }

    auto isLocked = false;
    if (const auto rawPtr = *wtsinfo.get())
    {
        // WTS_SESSIONSTATE_LOCK and WTS_SESSIONSTATE_UNLOCK are reversed on Win7.
        // On newer versions it's fixed.
        if (IsWindows8OrGreater())
        {
            isLocked = rawPtr->Data.WTSInfoExLevel1.SessionFlags == WTS_SESSIONSTATE_LOCK;
        }
        else
        {
            isLocked = rawPtr->Data.WTSInfoExLevel1.SessionFlags == WTS_SESSIONSTATE_UNLOCK;
        }
    }

    WTSFreeMemory(*wtsinfo);

    return isLocked ? SessionState::Locked : SessionState::Unlocked;
#else
    return SessionState::Unlocked;
#endif
}

struct ShortcutStartupRegistryData
{
    DWORD    Flags;
    FILETIME DisabledTime;

    ShortcutStartupRegistryData ()
    {
        Flags = 0;
        DisabledTime.dwLowDateTime  = 0;
        DisabledTime.dwHighDateTime = 0;
    }
};

enum ShortcutStartupRegistryFlags
{
    SSRF_DISABLED = 1,
    SSRF_DEFAULT  = 2
};

auto GetShortcutStartupRegistryData (const std::wstring& lnk) -> std::optional<ShortcutStartupRegistryData>
{
    auto subKey = std::wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\StartupFolder\\");
    auto ssrd   = ShortcutStartupRegistryData{};
    auto cbSize = DWORD{sizeof(ssrd)};
    
    auto status = ::RegGetValueW(
        HKEY_CURRENT_USER,
        subKey.c_str(),
        lnk.c_str(),
        RRF_RT_REG_BINARY,
        NULL,
        &ssrd,
        &cbSize
    );

    return status == ERROR_SUCCESS ? ssrd : std::optional<ShortcutStartupRegistryData>(std::nullopt);
}

auto SetShortcutStartupRegistryData (const std::wstring& lnk, const ShortcutStartupRegistryData& ssrd) -> bool
{
    auto subKey = std::wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\StartupFolder\\");
    auto cbSize = DWORD{sizeof(ssrd)};

    auto status = ::RegSetKeyValueW(
        HKEY_CURRENT_USER,
        subKey.c_str(),
        lnk.c_str(),
        REG_BINARY,
        reinterpret_cast<LPCWSTR>(&ssrd),
        cbSize
    );

    return status == ERROR_SUCCESS;
}

auto IsShortcutStartupEnabled (const std::wstring& lnk) -> bool
{
    auto result = false;
    if (const auto ssrd = GetShortcutStartupRegistryData(lnk))
    {
        const auto entry = ssrd.value();

        // 2 it's enabled, 
        // 3 it's disabled.
        if ((entry.Flags & SSRF_DISABLED) != 0)
        {
            result = false;
        }
        else
        {
            result = true;
        }
    }

    return result;
}

auto EnableShortcutAutoStart (const std::wstring& lnk) -> bool
{
    auto entry = ShortcutStartupRegistryData();

    if (const auto ssrd = GetShortcutStartupRegistryData(lnk))
    {
        entry = ssrd.value();
        entry.Flags |= SSRF_DEFAULT;
    }
    else
    {
        entry.Flags = SSRF_DEFAULT;
    }

    entry.Flags = entry.Flags & ~SSRF_DISABLED;
    entry.DisabledTime.dwLowDateTime  = 0;
    entry.DisabledTime.dwHighDateTime = 0;
        
    return SetShortcutStartupRegistryData(lnk, entry);
}

auto DisableShortcutAutoStart (const std::wstring& lnk) -> bool
{
    auto entry = ShortcutStartupRegistryData();

    if (const auto ssrd = GetShortcutStartupRegistryData(lnk))
    {
        entry = ssrd.value();
    }
    else
    {
        entry.Flags = SSRF_DEFAULT;
    }

    entry.Flags = entry.Flags | SSRF_DISABLED;
    GetSystemTimeAsFileTime(&entry.DisabledTime);
        
    return SetShortcutStartupRegistryData(lnk, entry);
}

auto AddShortcutToStartup (const std::wstring& lnk, const std::filesystem::path& target) -> bool
{
    auto hr = S_OK;

    // Get startup directory path.
    auto startupPath = static_cast<wchar_t*>(nullptr);
    hr = SHGetKnownFolderPath(FOLDERID_Startup, 0, NULL, reinterpret_cast<PWSTR*>(&startupPath));
    if (FAILED(hr))
    {
        return false;
    }

    auto shortcut = std::filesystem::path(startupPath) / lnk;

    // If shortcut exists, then remove.
    if (std::filesystem::exists(shortcut))
    {
        std::filesystem::remove(shortcut);
    }

    auto psl = static_cast<IShellLinkW*>(nullptr);

    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&psl));
    if (SUCCEEDED(hr))
    {
        auto ppf = static_cast<IPersistFile*>(nullptr);
 
        // Set the path to the shortcut target and add the description.'
        const auto exePathStr = target.wstring();
        const auto exeDirStr  = target.parent_path().wstring();

        psl->SetPath(exePathStr.c_str());
        psl->SetWorkingDirectory(exeDirStr.c_str());
        //psl->SetDescription(desc);
 
        // Query IShellLink for the IPersistFile interface, used for saving the
        // shortcut in persistent storage.
        hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hr))
        {
            // Save the link by calling IPersistFile::Save.
            auto pathStr = shortcut.wstring();
            hr = ppf->Save(pathStr.c_str(), TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    
    return hr == S_OK;
}

auto ScanProcesses (std::function<ScanResult (HANDLE, DWORD, const std::wstring_view)> checkFn) -> bool
{
    // Get the list of process identifiers (PID's).
    const auto PROCESS_LIST_MAX_SIZE = 2048;

    auto processList   = std::vector<DWORD>(PROCESS_LIST_MAX_SIZE);
    auto bytesReturned = DWORD{ 0 };
    if (!EnumProcesses(processList.data(), static_cast<DWORD>(processList.size()), &bytesReturned))
    {
        //Log("EnumProcesses() failed");
        return false;
    }
    auto numberOfProccesses = bytesReturned / sizeof(DWORD);

    // Loop through running processes.
    for (auto i = unsigned long{ 0 }; i < numberOfProccesses; ++i)
    {
        auto pid = processList[i];
        if (pid != 0)
        {
            auto processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!processHandle)
            {
                continue;
            }

            // Read process executable path.
            auto imageName = std::array<wchar_t, MAX_PATH>{ 0 };
            auto size      = DWORD{ MAX_PATH };
            if (QueryFullProcessImageNameW(processHandle, 0, imageName.data(), &size))
            {
                // Execute callback.
                const auto result = checkFn(processHandle, pid, imageName.data());
                switch (result)
                {
                default:
                case ScanResult::Continue:
                    break;

                case ScanResult::Success:
                    CloseHandle(processHandle);
                    return true;

                case ScanResult::Stop:
                case ScanResult::Failure:
                    CloseHandle(processHandle);
                    return false;
                }
            }
            
            CloseHandle(processHandle);
        }
    }

    return false;
}

auto ScanWindows (std::function<ScanResult (HWND, DWORD, const std::wstring_view)> checkFn, bool onlyVisible) -> bool
{
    #define ERROR_USER_CALLBACK_SUCCESS (1 << 29) // bit 29 for user errors

    struct EnumWindowsProcData
    {
        decltype(onlyVisible) onlyVisible;
        decltype(checkFn)     checkFn;
    };

    auto enumWindowsProcData = EnumWindowsProcData {onlyVisible, checkFn};
    auto enumWindowsProc = [](HWND hWnd, LPARAM lParam) -> BOOL {
        auto payload     = reinterpret_cast<EnumWindowsProcData*>(lParam);
        auto onlyVisible = payload->onlyVisible;
        auto callbackFn  = payload->checkFn;

        if (onlyVisible && (!IsWindowVisible(hWnd) && !IsIconic(hWnd)))
        {
            return TRUE;
        }

        auto length = GetWindowTextLength(hWnd);
        if (length > 0)
        {
            const auto size = static_cast<size_t>(length) + 1;

            auto title = std::wstring(size, L'\0');
            GetWindowTextW(hWnd, title.data(), size);

            auto pid = DWORD{0};
            GetWindowThreadProcessId(hWnd, &pid);

            // Execute callback.            
            const auto result = callbackFn(hWnd, pid, title.data());
            switch (result)
            {
            default:
            case ScanResult::Continue:
                break;

            case ScanResult::Success:
                SetLastError(ERROR_USER_CALLBACK_SUCCESS);
                return FALSE;

            case ScanResult::Stop:
            case ScanResult::Failure:
                break;
            }
        }

        return TRUE;
    };

    if (!EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&enumWindowsProcData)))
    {
        // Check if callback returned true.
        const auto error = GetLastError();
        if (error == ERROR_USER_CALLBACK_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

auto GetProcessPath (DWORD pid) -> std::filesystem::path
{
    auto processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (processHandle)
    {
        // Read process executable path.
        auto imageName = std::array<wchar_t, MAX_PATH>{ 0 };
        auto size      = DWORD{ MAX_PATH };
        if (QueryFullProcessImageNameW(processHandle, 0, imageName.data(), &size))
        {
            CloseHandle(processHandle);
            return std::filesystem::path(imageName.data());
        }
            
        CloseHandle(processHandle);
    }

    return std::filesystem::path();
}

auto GetDpi (HWND hWnd) -> int
{
    auto dpi = 96;

    auto hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32)
    {
        typedef UINT (__stdcall *GetDpiForWindowFn)(HWND);
        if (auto proc = GetProcAddress(hUser32, "GetDpiForWindow"))
        {
            auto fnGetDpiForWindow = reinterpret_cast<GetDpiForWindowFn>(proc);
            dpi = fnGetDpiForWindow(hWnd);
        }

        FreeLibrary(hUser32);
    }
    
    return dpi;
}

auto HexCharToInt (const char c) -> unsigned char
{
    if ('a' <= c && c <= 'f')
    {
        return c - 'a' + 10;
    }

    if ('A' <= c && c <= 'F')
    {
        return c - 'A' + 10;
    }

    if ('0' <= c && c <= '9')
    {
        return c - '0';
    }

    //return std::numeric_limits<unsigned char>::max();
    return UCHAR_MAX;
}

#pragma region "FILETIME to system_clock"

// https://github.com/HowardHinnant/date/wiki/Examples-and-Recipes#FILETIME
// by Billy O'Neal

using std::ratio;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::system_clock;

// filetime_duration has the same layout as FILETIME; 100ns intervals
using filetime_duration = duration<int64_t, ratio<1, 10'000'000>>;
// January 1, 1601 (NT epoch) - January 1, 1970 (Unix epoch):
constexpr duration<int64_t> nt_to_unix_epoch{INT64_C(-11644473600)};

SystemTimePoint FILETIME_to_system_clock(FILETIME fileTime)
{
    const filetime_duration asDuration{
        static_cast<int64_t>((static_cast<uint64_t>(fileTime.dwHighDateTime) << 32) | fileTime.dwLowDateTime)
    };
    const auto withUnixEpoch = asDuration + nt_to_unix_epoch;
    return system_clock::time_point{
        std::chrono::duration_cast<system_clock::duration>(withUnixEpoch)
    };
}

FILETIME system_clock_to_FILETIME(SystemTimePoint systemPoint)
{
    const auto asDuration = std::chrono::duration_cast<filetime_duration>(
        systemPoint.time_since_epoch()
    );
    const auto withNtEpoch = asDuration - nt_to_unix_epoch;
    const uint64_t rawCount = withNtEpoch.count();
    FILETIME result;
    result.dwLowDateTime = static_cast<DWORD>(rawCount); // discards upper bits
    result.dwHighDateTime = static_cast<DWORD>(rawCount >> 32);
    return result;
}

#pragma endregion

} // namespace CaffeineTake
