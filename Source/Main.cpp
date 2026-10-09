#include "App.h"
#include <algorithm>
#include <objbase.h>
#include <fstream>
#include <shellapi.h>
#include <stdexcept>
Display SelectDisplay(bool test)
{
    std::wstring wanted;
    if (test)
    {
        auto input = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
        if (!input)
            throw std::runtime_error("Unlocked input desktop unavailable: visible GUI tests refused.");
        wchar_t desktop[128]{};
        DWORD required = 0;
        const BOOL readable = GetUserObjectInformationW(input, UOI_NAME, desktop, sizeof(desktop), &required);
        CloseDesktop(input);
        if (!readable || wcscmp(desktop, L"Default") != 0)
            throw std::runtime_error("Input desktop is locked or unavailable: visible GUI tests refused.");
        UINT paths = 0, modes = 0;
        if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &paths, &modes) != ERROR_SUCCESS)
            throw std::runtime_error("Display enumeration failed.");
        std::vector<DISPLAYCONFIG_PATH_INFO> p(paths);
        std::vector<DISPLAYCONFIG_MODE_INFO> m(modes);
        if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &paths, p.data(), &modes, m.data(), nullptr) !=
            ERROR_SUCCESS)
            throw std::runtime_error("Display identity query failed.");
        for (UINT i = 0; i < paths; ++i)
        {
            DISPLAYCONFIG_TARGET_DEVICE_NAME target{};
            target.header = {DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME, sizeof(target),
                             p[i].targetInfo.adapterId, p[i].targetInfo.id};
            DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
            source.header = {DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME, sizeof(source),
                             p[i].sourceInfo.adapterId, p[i].sourceInfo.id};
            if (DisplayConfigGetDeviceInfo(&target.header) == ERROR_SUCCESS &&
                DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS &&
                std::wstring(target.monitorFriendlyDeviceName) == L"NE18NZ2")
                wanted = source.viewGdiDeviceName;
        }
        if (wanted.empty())
            throw std::runtime_error(
                "NE18NZ2 unavailable: visible GUI tests refused. Use --self-test for hidden checks.");
    }
    struct State
    {
        bool test;
        std::wstring wanted;
        Display result;
        bool found = false;
    } state{test, wanted};
    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM parameter) -> BOOL
        {
            auto &s = *reinterpret_cast<State *>(parameter);
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            if (GetMonitorInfoW(monitor, &info) &&
                ((s.test && s.wanted == info.szDevice) || (!s.test && (info.dwFlags & MONITORINFOF_PRIMARY))))
            {
                s.result = {info.rcWork, info.rcMonitor, info.szDevice, s.test ? L"NE18NZ2" : L"Primary"};
                s.found = true;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&state));
    if (!state.found)
        throw std::runtime_error("Required monitor unavailable.");
    return state.result;
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    Options options;
    try
    {
        int argc = 0;
        auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        for (int i = 1; i < argc; ++i)
        {
            if (std::wstring(argv[i]) == L"--self-test")
                options.test = true;
            else if (std::wstring(argv[i]) == L"--gui-test")
            {
                options.test = true;
                options.visible = true;
            }
            else if (std::wstring(argv[i]) == L"--launcher-test")
            {
                options.test = true;
                options.visible = true;
                options.launcher = true;
            }
            else if (std::wstring(argv[i]) == L"--report" && i + 1 < argc)
                options.report = argv[++i];
            else
            {
                LocalFree(argv);
                throw std::runtime_error(
                    "Unknown command line. Use --self-test or --gui-test, optionally --report path.");
            }
        }
        LocalFree(argv);
        // Manifest establishes PerMonitorV2 before the first HWND is created.
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr))
            throw std::runtime_error("COM initialization failed.");
        int result;
        {
            App app(instance, options, SelectDisplay(options.visible));
            result = options.test ? app.Audit() : app.Run();
        }
        CoUninitialize();
        return result;
    }
    catch (const std::exception &error)
    {
        if (!options.report.empty())
        {
            std::ofstream out(options.report, std::ios::app);
            out << "FAIL: " << error.what() << '\n';
        }
        if (!options.test)
        {
            std::wstring text(error.what(), error.what() + strlen(error.what()));
            MessageBoxW(nullptr, text.c_str(), L"Win32_DemoApp startup error", MB_OK | MB_ICONERROR);
        }
        return 1;
    }
}
