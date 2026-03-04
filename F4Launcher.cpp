#include <windows.h>
#include <shlobj.h>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "Shell32.lib")

void PrintVaultBoy()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    printf("________________@@@@@@@@@@@@__________________\n");
    printf("______________@@____________@@__@@@@__________\n");
    printf("____________@@________________@@____@@________\n");
    printf("____________@@____________________@@__________\n");
    printf("____________@@____........________@@__________\n");
    printf("____________@@__................@@____________\n");
    printf("__________@@..........@@....@@..@@____________\n");
    printf("__________@@..........@@....@@..@@____________\n");
    printf("____________@@....................@@______@@____\n");
    printf("__________@@@@....@@............@@@@@@@@..@@__\n");
    printf("______@@@@####@@....@@@@@@@@..@@########....@@\n");
    printf("____@@########__@@..........@@__########....@@\n");
    printf("____@@######@@##__@@@@@@@@@@____########....@@\n");
    printf("__@@####@@@@@@####______________@@@@@@@@@@@@__\n");
    printf("__@@......@@@@##########__####@@______________\n");
    printf("__@@......@@@@##########__####@@______________\n");
    printf("__@@......@@@@##########__####@@______________\n");
    printf("____@@@@@@__@@________________@@______________\n");
    printf("__________@@##################@@______________\n");
    printf("________@@##########@@##########@@____________\n");
    printf("______@@@@########@@__@@########@@@@__________\n");
    printf("__@@@@##########@@______@@##########@@@@______\n");
    printf("@@##############@@______@@##############@@____\n");
    printf("@@@@@@@@@@@@@@@@@@______@@@@@@@@@@@@@@@@@@____\n");
    printf("______________________________________________\n");
    printf("______________PLEASE STAND BY_________________\n");

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

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
    Sleep(1000);         // 1 second delay

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

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

int wmain()
{
	PrintVaultBoy();

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