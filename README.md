# Win32_DemoApp

A C++20 Windows 11 x64 application for learning native Win32 controls and the names used in Microsoft documentation. The executable uses real USER32/common-control windows, Common Controls v6 visual styles, the system message font and a Per-Monitor V2 manifest. No UI framework, webview, external runtime installation or GPU/mouse setting changes are involved.

## Run

Open the executable in a ready folder under `TestBuilds`, or double-click `RunLatestBuild.cmd` at the repository root after publishing a snapshot. A normal user launch opens on the primary monitor. The executable is self-contained apart from Windows' own DLLs. Its C++ runtime, manifest, property-page templates and original AVI are embedded.

Select a topic in the left tree. Read the family, class names, API identifiers and explanation above the live examples. The event log shows control notifications. The start page records live coverage and exclusions. F1 or the documentation button opens the relevant Microsoft reference in your default browser only when you invoke it.

- Tab / Shift+Tab: native control navigation; Space toggles a checkbox; arrow keys operate radio groups, sliders and lists.
- F6: switch between the topic tree and live examples.
- Ctrl+Left / Ctrl+Right: previous/next topic. Alt+Left / Alt+Right: resize the divider.
- Alt+F / Alt+T / Alt+H: menus. Alt+Space: the native system menu. Ctrl+O: safe path-selection demo.
- Right-click or Shift+F10: native context menu. Drag the divider to resize the navigation pane.
- Resize the window or scroll the live area to reach examples outside the viewport. Focused controls are scrolled into view.

File/open/save/folder examples only return a selected path; they never open or write that path. Color/font choices affect a temporary preview. Find/replace operates on a temporary text buffer. The property sheet changes only its own temporary controls. Cancel is designed to leave application/system state unchanged. There are no printer calls or actual hardware controls.

## Build

Use modern PowerShell (`pwsh`) with an existing Visual Studio C++ x64 toolchain and Windows SDK:

```powershell
pwsh -NoProfile -File .\Build.ps1 -Test
pwsh -NoProfile -File .\Build.ps1 -Configuration Debug -Test
```

`Build.ps1` discovers the installed toolchain through vswhere, enters its x64 developer environment, runs `rc.exe` and `cl.exe`, embeds the manifest and links the static C++ runtime. It writes only to `BuildOutput`. It installs nothing. CMakeLists.txt is also provided for IDE integration; the direct script is the verified build path in this environment.

`Tools/GenerateDemoAnimation.ps1` regenerates the original tiny uncompressed AVI if desired. The generated asset is committed, so regeneration is unnecessary for building.

## Verification and ready snapshots

```powershell
pwsh -NoProfile -File .\PublishBuild.ps1
```

Publication requires a clean committed source tree. It builds/tests, copies into a new uniquely named `TestBuilds` directory, verifies the copied executable from a different working directory, records source commit and SHA-256, then marks snapshot files read-only. Ready folders are never overwritten. The only mutable pointer is `TestBuilds/LatestReady.json`. These directories are gitignored.

The executable supports `--self-test --report path` for hidden noninteractive checks. `--gui-test --report path` is restricted to the exact NE18NZ2 monitor identity; it refuses visible testing if that monitor is absent, shows without activation, injects no input and captures only its own window through PrintWindow. Do not use an ordinary launch for assistant testing. The GUI audit exits and closes its windows when complete.

The audit checks control creation/classes, v6 activation, manifest awareness, state changes, dependent enabled state, report/icon view changes, scrolling, synthetic 96/144/192 DPI font/layout reconstruction, repeated navigation and stable GDI/USER object counts. See the copied audit reports and `Documentation/Verification.md` for actual outcomes and limits. Synthetic DPI checks do not replace real monitor-transition testing. Automated dialog tests intentionally do not open modal dialogs on the user's interactive desktop.

## Learn more

Read `Documentation/Inventory.md` for the API glossary, coverage and exclusions, and `Documentation/ScreenshotGuide.md` for the three screenshot-inspired concept mappings. The app illustrates observed concepts with original art and dummy values; it does not claim knowledge of the third-party applications' exact HWND implementations.

Microsoft's [Control Library](https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info), [common-control class list](https://learn.microsoft.com/en-us/windows/win32/controls/common-control-window-classes), and [DPI reference](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows) are the basis for the terminology.
