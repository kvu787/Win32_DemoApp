# Review corrections

Six independent review findings were corrected after the first verified snapshot. The machine remained locked throughout this correction pass. Hidden native-control checks passed in Release and Debug; visible acceptance remains pending. No desktop, focus, cursor, keyboard or security manipulation was performed.

1. The root PowerShell launcher no longer specifies a hidden startup window. Both root launchers expose a restricted NE18NZ2 test mode which uses the real first-show path and asserts that the main window is visible and not minimized. Visible tests refuse a locked/unavailable input desktop. These actual launcher tests have not yet run.
2. Color/font preview control 1011 and the ordinary modal-dialog example now belong to the Native dialogs topic. Preview updates return before calling any window API when the preview is absent. Hidden tests verify color rendering, the selected font and navigation persistence; actual common-dialog acceptance/cancellation is pending.
3. System-font refresh updates all descendants of live owned floating windows before deleting the old font. Composite controls retain separate font ownership. Tests create three hidden owned-window fixtures, refresh twice and verify live font handles.
4. DPI/theme/settings refresh updates fonts, icons, column widths and layout in place. It retains specimen HWNDs rather than rebuilding the topic. Tests retain edit/password/multiline text, edit selection, logical scroll position, checkbox/slider values and combo selection across synthetic presentation changes.
5. Find/Replace searches from the current selection, supports up/down direction, wrapping, Match Case and Whole Word, and replaces only a selected matching occurrence. Replace All scans original nonoverlapping matches and supports empty replacement. Pure search cases and the real native edit handler are tested without opening a modeless dialog.
6. Layout stores the splitter's clamped logical position. Painting/layout and hit testing use that same position. Tests cover both clamp extremes and a narrow hidden-window resize.

The current audit creates 1,735 native controls, includes 112 repeated topic recreations and checks stable GDI/USER counts. Its last line explicitly distinguishes hidden noninteractive verification from GUI verification. Audit reports, source commit and executable SHA-256 are copied into each immutable candidate.

`PublishBuild.ps1 -Candidate` labels the build as unverified for GUI behavior and leaves LatestReady.json untouched. A new ready snapshot requires `-GuiAudit` on an unlocked, cleared NE18NZ2 slot, then GUI rendering and both root-launcher visibility checks. No historical captures are copied into a correction candidate.

Pending checks include the corrected launchers' actual visible startup, native dialog acceptance/cancellation, keyboard focus traversal, owned-window interaction and real DPI monitor transitions. The original inventory's explained-only and omitted variants remain explicit; this correction pass does not expand coverage claims.
