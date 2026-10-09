# Verification

The project was built on Windows 11 x64 using the existing MSVC 19.51.36260 toolchain and Windows SDK 10.0.26100.0, with no installation, network dependency, push or system setting change. Both Release and Debug builds passed hidden native-control audits. The direct MSVC Build.ps1 path is verified; an initial CMake/Ninja compiler probe stalled and was stopped, so CMake is provided for IDE use but is not claimed verified here.

The last verified ready snapshot contains authoritative IndependentLaunch.txt and GuiAudit.txt reports in Verification. The executable was launched from a different working directory to verify that resources are embedded and no source-tree or runtime-package dependency is needed. BuildInfo.json records the source commit and executable SHA-256. All snapshot files are read-only and ready folders are never overwritten by the scripts.

The subsequent six review corrections were built and audited while the machine was locked. Both Release and Debug hidden audits passed, now creating 1,735 controls. The corrected candidate includes IndependentLaunch.txt only; it does not inherit historical GUI captures or claim a visible GUI pass. Ready publication now requires a fresh GUI audit and both root launcher checks. The last verified LatestReady pointer is preserved until those checks succeed. See ReviewCorrections.md for the exact fixes and pending acceptance work.

## Checks performed

- All 14 topics and expected native classes create successfully. The original verified audit created 1,626 specimen controls and recreated all topics across eight additional cycles (112 topic recreations). The current correction audit adds targeted fixtures for a total of 1,735 controls.
- The embedded manifest establishes Per-Monitor V2 awareness; Comctl32 DllGetVersion confirms v6. Theme activation and application theming are reported separately. Windows 11 themed buttons, command links, editors, trees, reports, tabs, rebar and calendars were visually inspected in app-only captures.
- Three-state/disabled/checked button states, command-link notes, slider-to-progress notifications, progress error state, dependent enabled state and list-view report/icon switching are checked through native messages.
- Dialog resources 101/102/103 are present; a native property page can be created and released. The Shell Common Item Dialog COM object can be created without showing it.
- Tab-order lookup resolves without changing focus. Actual focus traversal was not injected.
- Scroll position and 96/144/192 DPI font/layout refresh are tested. Shared icon sizes are also verified against synthetic DPI. Current regressions check that presentation refresh retains HWNDs, edited buffers, selection, logical scrolling and control values. Real monitor transitions remain untested.
- Body-font validity, shared image-list survival, tooltip registration cleanup and stable GDI/USER object counts are checked after repeated navigation. Visual inspection found font/image ownership and rebar alignment issues; those were fixed before publication.
- Visible test runs validate the exact NE18NZ2 friendly display identity and the entire test-window rectangle before showing/capturing. They use WS_EX_NOACTIVATE and SW_SHOWNOACTIVATE, inject no mouse/keyboard input and leave the foreground HWND unchanged. Captures use PrintWindow on this application's own HWND, not a desktop screenshot. Test windows close on completion.

## Explicitly unverified

The corrected root CMD/PowerShell launchers' actual visibility remains pending an unlocked, cleared NE18NZ2 slot. Interactive modal dialog cancellation, Common Item Dialog acceptance, real property-sheet Apply/Cancel flow, actual keyboard focus/mnemonic traversal, owned floating windows' user interaction, real monitor DPI transitions, accessibility inspection tools, high contrast, alternate locales/themes and every style/message permutation require manual verification. Opening modal dialogs could activate windows, so the assistant intentionally did not automate those flows on the user's desktop. Their cancellation handlers and side-effect boundaries were reviewed in code.

File dialogs return paths only; no application file-open/write action follows them. FOS_NOCHANGEDIR and FOS_DONTADDTORECENT are set. Color/font apply only to a temporary sample; Find/Replace modifies a temporary buffer; the property sheet has no system-configuration calls. Print/page setup is excluded entirely.

Source screenshot pixels could not be materialized in this execution environment. ScreenshotGuide.md preserves the parent's verified conceptual inventory and source identifiers; local visual checks cover the generated gallery, not exact replicas of the source applications.
