# Review corrections

The initial locked-desktop status below is historical. Current coverage/tests and remaining manual checks are recorded in Verification.md and the final repository Checkpoint.md. A ready snapshot has fresh GUI and actual root launcher reports; hidden candidates do not.

Six independent review findings were corrected after the first verified snapshot. The machine remained locked throughout this correction pass. Hidden native-control checks passed in Release and Debug; visible acceptance remains pending. No desktop, focus, cursor, keyboard or security manipulation was performed.

1. The root PowerShell launcher no longer specifies a hidden startup window. Both root launchers expose a restricted NE18NZ2 test mode which uses the real first-show path and asserts that the main window is visible and not minimized. Visible tests refuse a locked/unavailable input desktop. These actual launcher tests have not yet run.
2. Color/font preview control 1011 and the ordinary modal-dialog example now belong to the Native dialogs topic. Preview updates return before calling any window API when the preview is absent. Hidden tests verify color rendering, the selected font and navigation persistence; actual common-dialog acceptance/cancellation is pending.
3. System-font refresh updates all descendants of live owned floating windows before deleting the old font. Composite controls retain separate font ownership. Tests create three hidden owned-window fixtures, refresh twice and verify live font handles.
4. DPI/theme/settings refresh updates fonts, icons, column widths and layout in place. It retains specimen HWNDs rather than rebuilding the topic. Tests retain edit/password/multiline text, edit selection, logical scroll position, checkbox/slider values and combo selection across synthetic presentation changes.
5. Find/Replace searches from the current selection, supports up/down direction, wrapping, Match Case and Whole Word, and replaces only a selected matching occurrence. Replace All scans original nonoverlapping matches and supports empty replacement. Pure search cases and the real native edit handler are tested without opening a modeless dialog.
6. Layout stores the splitter's clamped logical position. Painting/layout and hit testing use that same position. Tests cover both clamp extremes and a narrow hidden-window resize.

The audit includes 112 repeated topic recreations and checks stable GDI/USER counts. Its last line explicitly distinguishes hidden noninteractive verification from GUI verification. Audit reports, source commit and executable SHA-256 are copied into each immutable candidate.

## Bounded follow-up corrections

The rereview found two sequencing defects. The root launcher now resolves an explicit test-build override before reading LatestReady.json. Tools/TestLauncher.ps1 runs the real script in a unique temporary fixture with Start-Process mocked. It verifies the first-build missing-pointer case, malformed irrelevant pointers, normal pointer selection and rejected overrides/reports. It does not read or change the real pointer or launch a native process.

WM_DPICHANGED now sets the new DPI for minimum tracking dimensions, applies the suggested rectangle while deferring synchronous WM_SIZE layout, and then refreshes presentation using the final dimensions. Hidden tests send actual 96-to-192 and 192-to-96 messages through the application's window procedure twice. They retain the 650 logical-pixel splitter, horizontal scroll 140, vertical scroll 200, native edit/password/multiline HWNDs, text and edit selection. The calling thread's focus identity remains unchanged; it is empty in this hidden fixture, so actual focused keyboard interaction remains unverified. Suggested oversized synthetic rectangles are never used by visible test mode.

Before fixing the code, the new tests reproduced the missing-pointer exception and the DPI handler's splitter/scroll loss. The launcher report is saved separately as Verification/LauncherRegression.txt in new candidates. No visible checks were run while the desktop was locked.

`PublishBuild.ps1 -Candidate` labels the build as unverified for GUI behavior and leaves LatestReady.json untouched. A new ready snapshot requires `-GuiAudit` on an unlocked, cleared NE18NZ2 slot, then GUI rendering and both root-launcher visibility checks. No historical captures are copied into a correction candidate.

Pending checks include the corrected launchers' actual visible startup, native dialog acceptance/cancellation, keyboard focus traversal, owned-window interaction and real DPI monitor transitions. The original inventory's explained-only and omitted variants remain explicit; this correction pass does not expand coverage claims.
