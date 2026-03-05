#include <windows.h>
#include <shlobj.h>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <tlhelp32.h>

#pragma comment(lib, "Shell32.lib")

std::wstring GetExecutableDirectory()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");

    return path.substr(0, lastSlash + 1);
}

std::wstring GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime;
    localtime_s(&localTime, &time);

    std::wstringstream ss;
    ss << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::wstring GetFallout4PrefsPath()
{
    PWSTR documentsPath = nullptr;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &documentsPath)))
    {
        std::wstring path(documentsPath);
        CoTaskMemFree(documentsPath);

        path += L"\\My Games\\Fallout4\\Fallout4Prefs.ini";
        return path;
    }

    return L"";
}

bool FileExists(const std::wstring& path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES &&
        !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

void FlushIni(const std::wstring& iniPath)
{
    // Forces profile API to flush cached writes
    WritePrivateProfileStringW(NULL, NULL, NULL, iniPath.c_str());
}

bool SetResolutionInIni(const std::wstring& iniPath,
    int width,
    int height,
    std::wofstream& log)
{
    if (iniPath.empty())
    {
        log << GetTimestamp() << L" ERROR: INI path is empty.\n";
        return false;
    }

    BOOL wResult = WritePrivateProfileStringW(
        L"Display",
        L"iSize W",
        std::to_wstring(width).c_str(),
        iniPath.c_str());

    BOOL hResult = WritePrivateProfileStringW(
        L"Display",
        L"iSize H",
        std::to_wstring(height).c_str(),
        iniPath.c_str());

    if (!wResult || !hResult)
    {
        log << GetTimestamp()
            << L" ERROR: Failed to write resolution to INI file.\n";
        return false;
    }

    FlushIni(iniPath);   // Explicit flush
    Sleep(2500);         // 2.5 second delay

    return true;
}

bool LaunchExecutable(const std::wstring& exePath,
    const std::wstring& workingDir,
    std::wofstream& log)
{
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessW(
        exePath.c_str(),
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        workingDir.c_str(),
        &si,
        &pi))
    {
        log << GetTimestamp()
            << L" ERROR: Failed to launch " << exePath << L"\n";
        return false;
    }

    log << GetTimestamp() << L" Launched " << exePath << L"\n";

    // Wait for the loader to spawn the actual game
    Sleep(5000);

    // Find the Fallout4.exe process (the actual game, not the loader)
    DWORD falloutPid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W pe = { sizeof(pe) };

        if (Process32FirstW(snapshot, &pe))
        {
            do
            {
                if (_wcsicmp(pe.szExeFile, L"Fallout4.exe") == 0)
                {
                    falloutPid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Wait for the actual game process
    if (falloutPid != 0)
    {
        HANDLE hGameProcess = OpenProcess(SYNCHRONIZE, FALSE, falloutPid);
        if (hGameProcess)
        {
            // Set focus multiple times to combat Steam overlay stealing focus
            for (int attempt = 0; attempt < 5; attempt++)
            {
                Sleep(2000);

                HWND hwnd = NULL;
                do {
                    hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
                    DWORD windowProcessId;
                    GetWindowThreadProcessId(hwnd, &windowProcessId);

                    if (windowProcessId == falloutPid && IsWindowVisible(hwnd))
                    {
                        // Use multiple methods to ensure focus
                        SetForegroundWindow(hwnd);
                        SetActiveWindow(hwnd);
                        BringWindowToTop(hwnd);
                        SetFocus(hwnd);
                        break;
                    }
                } while (hwnd != NULL);
            }

            log << GetTimestamp() << L" Focus attempts completed, waiting for game to exit...\n";
            log.flush();

            WaitForSingleObject(hGameProcess, INFINITE);

            log << GetTimestamp() << L" Game process exited.\n";
            CloseHandle(hGameProcess);

            // Give focus back to Steam Big Picture
            Sleep(500);
            HWND steamWindow = FindWindowW(L"CUIEngineWin32", NULL);  // Steam Big Picture class
            if (!steamWindow)
            {
                steamWindow = FindWindowW(L"SDL_app", NULL);  // Alternative Steam window class
            }
            if (!steamWindow)
            {
                steamWindow = FindWindowW(NULL, L"Steam Big Picture Mode");  // By title
            }
            if (steamWindow)
            {
                SetForegroundWindow(steamWindow);
                SetActiveWindow(steamWindow);
                BringWindowToTop(steamWindow);
            }
        }
    }

    return true;
}

int wmain()
{
    // Hide the console window
    HWND consoleWindow = GetConsoleWindow();
    ShowWindow(consoleWindow, SW_HIDE);

    std::wstring exeDir = GetExecutableDirectory();
    std::wstring logPath = exeDir + L"launcher_log.txt";

    std::wofstream log(logPath, std::ios::app);

    DEVMODEW devMode = {};
    devMode.dmSize = sizeof(devMode);

    if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devMode))
    {
        log << GetTimestamp()
            << L" ERROR: Failed to retrieve display settings.\n";
    }
    else
    {
        int screenWidth = devMode.dmPelsWidth;
        int screenHeight = devMode.dmPelsHeight;

        log << GetTimestamp()
            << L" Detected resolution: "
            << screenWidth << L"x"
            << screenHeight << L"\n";

        std::wstring iniPath = GetFallout4PrefsPath();
        SetResolutionInIni(iniPath, screenWidth, screenHeight, log);
    }

    std::wstring f4sePath = exeDir + L"f4se_loader.exe";
    std::wstring falloutPath = exeDir + L"Fallout4.exe";

    if (FileExists(f4sePath))
    {
        LaunchExecutable(f4sePath, exeDir, log);
    }
    else if (FileExists(falloutPath))
    {
        LaunchExecutable(falloutPath, exeDir, log);
    }
    else
    {
        log << GetTimestamp()
            << L" ERROR: Neither f4se_loader.exe nor Fallout4.exe found.\n";
    }

    log.close();
    return 0;
}