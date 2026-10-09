#include "Gallery.h"
const std::vector<Page> &Pages()
{
    static const std::vector<Page> pages{
        {L"Start here / coverage", L"Native Win32 reference",
         L"USER32 / COMCTL32 v6 / COMDLG32 / Shell / application composition",
         L"Select a topic in the tree. Every specimen is a real HWND unless marked custom. Read its class "
         L"and API names above the live area. The event log explains notifications. Tab / Shift+Tab "
         L"navigate; Alt activates menus and mnemonics; F6 switches navigation/content; Ctrl+Left/Right "
         L"changes topic.\r\n\r\nThis is broad coverage, not every Win32 GUI API or every style permutation. "
         L"The inventory states exclusions. Windows 11 supplies the current theme. No third-party "
         L"screenshot's exact internal implementation can be inferred from pixels.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info"},
        {L"Buttons and states", L"USER32 (v6 adds extended button styles)",
         L"Button / BS_PUSHBUTTON, BS_DEFPUSHBUTTON, BS_AUTOCHECKBOX, BS_AUTO3STATE, BS_AUTORADIOBUTTON, "
         L"BS_GROUPBOX, BS_SPLITBUTTON, BS_COMMANDLINK / BN_CLICKED, BCN_DROPDOWN",
         L"Push buttons execute commands. Checkboxes represent independent booleans; a three-state box can "
         L"be indeterminate. Radio buttons select one item within a WS_GROUP. Group boxes organize related "
         L"controls. Split buttons have a separate menu arrow; command links have a descriptive note. "
         L"Disabled controls stay visible but are excluded from keyboard navigation.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/buttons"},
        {L"Text, icons and editors", L"USER32 + Rich Edit library",
         L"Static / SS_LEFT, SS_ICON, SS_ETCHEDHORZ; Edit / ES_PASSWORD, ES_MULTILINE, ES_READONLY; "
         L"RICHEDIT50W / Msftedit.dll / EM_SETCHARFORMAT",
         L"Static labels and icons present information. Edit controls accept plain text, selection, "
         L"clipboard operations and native context menus. Cue banners explain empty fields. Password "
         L"controls conceal presentation only. Rich Edit adds formatting. These fields contain dummy text; "
         L"no data is saved.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/edit-controls"},
        {L"Lists and choices", L"USER32 + common controls",
         L"ListBox / LBS_NOTIFY, LBS_EXTENDEDSEL; ComboBox / CBS_DROPDOWNLIST, CBS_DROPDOWN, CBS_SIMPLE; "
         L"ComboBoxEx32 / CBEM_INSERTITEM; msctls_hotkey32 / HKM_GETHOTKEY; SysIPAddress32 / IPM_GETADDRESS",
         L"A list box shows choices in place. Combo boxes combine a selection/edit field with a list; "
         L"dropdown-list is not editable, dropdown is editable, simple always shows its list. ComboBoxEx "
         L"adds item icons. Hot key and IP address controls implement specialized segmented input.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/comboboxes"},
        {L"Tree, report and header", L"Common controls",
         L"SysTreeView32 / TVM_INSERTITEM, TVN_SELCHANGED; SysListView32 / LVS_REPORT, LVM_INSERTCOLUMN, "
         L"LVN_COLUMNCLICK; SysHeader32 / HDM_INSERTITEM",
         L"Tree views represent hierarchy with expand/collapse glyphs, optional connector lines, icons and "
         L"checkboxes. A report list view provides selectable rows and resizable columns using a header "
         L"control. Click a column to sort this dummy report; use the view selector to compare report, list "
         L"and icon layouts. A header by itself is a control too.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/list-view-controls"},
        {L"Ranges and scrolling", L"USER32 + common controls",
         L"msctls_trackbar32 / TBM_SETRANGE, WM_HSCROLL; msctls_updown32 / UDM_SETBUDDY, UDM_SETRANGE32; "
         L"msctls_progress32 / PBM_SETPOS, PBM_SETSTATE; ScrollBar / SBS_HORZ, SBS_VERT / SetScrollInfo",
         L"Trackbars (sliders) choose values within a range and can show ticks or selection ranges. Up-down "
         L"controls (spin buttons) adjust a buddy edit. Progress bars report determinate or marquee activity "
         L"and normal, paused or error state. Scrollbars expose a viewport position. The gallery also has "
         L"actual horizontal and vertical scrolling when resized small.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/trackbar-controls"},
        {L"Dates, calendar and animation", L"Common controls",
         L"SysDateTimePick32 / DTS_SHORTDATEFORMAT, DTS_TIMEFORMAT, DTS_SHOWNONE; SysMonthCal32 / "
         L"MCS_MULTISELECT; SysAnimate32 / ACS_AUTOPLAY, ACM_OPEN",
         L"Date/time pickers offer localized date or time input; a checkbox can represent no date. Month "
         L"calendars show days with single/range selection. Animation controls play silent AVI clips; this "
         L"one uses an original tiny uncompressed demo clip embedded as a resource. This legacy control is "
         L"not a modern media player.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/date-and-time-picker-controls"},
        {L"Tabs, toolbars and containers", L"Common controls",
         L"SysTabControl32 / TCN_SELCHANGE; ToolbarWindow32 / TB_ADDBUTTONS; ReBarWindow32 / RB_INSERTBAND; "
         L"SysPager / PGM_SETCHILD; tooltips_class32 / TTM_ADDTOOL; SysLink / NM_CLICK",
         L"Tabs select application-managed pages. A toolbar contains command buttons, separators and "
         L"dropdowns; a rebar hosts movable bands. A pager scrolls an oversized child toolbar. Tooltips "
         L"provide short hover or keyboard help. SysLink provides keyboard-accessible hyperlinks. The main "
         L"footer is a msctls_statusbar32 with a sizing grip.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/common-control-window-classes"},
        {L"Native dialogs", L"Common dialogs + Shell + common controls",
         L"IFileOpenDialog / IFileSaveDialog / FOS_PICKFOLDERS; ChooseColorW; ChooseFontW; FindTextW / "
         L"ReplaceTextW; MessageBoxW; TaskDialogIndirect; PropertySheetW",
         L"These open genuine native dialogs owned by this app. File/save/folder dialogs return a path only: "
         L"this app never opens or writes that path. Color/font choices alter only sample UI. Find/replace "
         L"searches a local demo buffer. Cancel is safe. Print dialogs are inventoried separately and "
         L"intentionally excluded to avoid invoking printer drivers or submitting jobs.",
         L"https://learn.microsoft.com/en-us/windows/win32/dlgbox/common-dialog-box-library"},
        {L"Menus and window chrome", L"USER32 + DWM",
         L"HMENU / CreateMenu, TrackPopupMenuEx; WS_OVERLAPPEDWINDOW; WM_SYSCOMMAND; DrawMenuBar; modeless "
         L"owned windows",
         L"The title bar, window icon, system menu, minimize/maximize/close buttons, resize border and menus "
         L"are operating-system window chrome. The main menu supports Alt and keyboard navigation; "
         L"right-click the example area for a popup with checked, radio, disabled and submenu items. "
         L"Floating owned windows demonstrate tool-window chrome without modifying other apps.",
         L"https://learn.microsoft.com/en-us/windows/win32/menurc/menus"},
        {L"Navigation / display layout", L"Screenshot-inspired application composition",
         L"Tree view + toolbar + native controls; app-defined splitter; custom monitor tiles / WM_PAINT; "
         L"SysLink; WS_HSCROLL, WS_VSCROLL",
         L"Inspired by the described G-SYNC screenshot: native tree navigation, toolbar "
         L"arrows/dropdown/separators, icon heading, hyperlinks, checkbox/radio states, a split pane and "
         L"selectable monitor tiles. Drag the main divider or use Alt+Left/Right to resize it. Tiles and the "
         L"banner are original application drawing; Win32 does not provide a universal monitor-tile or "
         L"banner control. This demo changes no display settings.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/tree-view-controls"},
        {L"Mouse property sheet layout", L"Screenshot-inspired native property sheet",
         L"PropertySheetW / PROPSHEETHEADERW / PROPSHEETPAGEW / #32770; SysTabControl32; PSN_APPLY, "
         L"PSN_RESET; PropSheet_Changed",
         L"Open a real tabbed property sheet with Motion, Snap To and Visibility group boxes, trackbars, "
         L"checkboxes and OK/Cancel/Apply. A checkbox enables a dependent slider; editing enables Apply. All "
         L"settings belong only to the temporary demo, never to Windows mouse configuration. The "
         L"screenshot's five mouse tabs are OS-specific pages; this gallery illustrates their container and "
         L"control concepts with two original pages.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/property-sheets"},
        {L"Sensors / details layout", L"Screenshot-inspired native/custom composition",
         L"SysListView32 / LVS_REPORT / LVS_EX_FULLROWSELECT / LVM_SUBITEMHITTEST / NM_CUSTOMDRAW; owned tool "
         L"windows; msctls_progress32",
         L"Inspired by HWiNFO: report columns, icons, read-only summary fields, a combo box and floating "
         L"meters. Compare native Row selection with an app-managed single active Cell: click any of four "
         L"columns or use arrow keys, then read the selected value. Hit testing/custom drawing extend the "
         L"native report list; cell state and hierarchy belong to the app. Win32 has no stock spreadsheet "
         L"control. Double-click or Enter toggles the dummy CPU group. No editing/multiselection. The "
         L"original app's actual implementation remains uncertain.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/custom-draw"},
        {L"Custom drawing / platform", L"Platform features and application drawing",
         L"WM_PAINT / GDI; WM_DPICHANGED; SystemParametersInfoForDpi; IsDialogMessage; MSAA/UI Automation; "
         L"DWM; WM_THEMECHANGED",
         L"Owner drawing/custom drawing extends native controls; fully custom surfaces must implement "
         L"interaction and accessibility themselves. This page's illustration is explicitly custom GDI. "
         L"System message fonts, system colors, Common Controls v6, and Per-Monitor V2 awareness are active "
         L"throughout. Clipboard, drag/drop, IME, accessibility, touch, Ribbon, WebView and Direct2D are "
         L"broader platform topics, not additional universal USER32 controls; see the exclusions inventory.",
         L"https://learn.microsoft.com/en-us/windows/win32/controls/user-controls-intro"}};
    return pages;
}
const std::vector<InventoryItem> &Inventory()
{
    static const std::vector<InventoryItem> items{
        {L"Buttons (push/check/radio/group/split/command)", L"Button",
         L"Live variants + disabled/indeterminate"},
        {L"Labels, icon, etched separator", L"Static", L"Live"},
        {L"Plain text/password/multiline/read-only", L"Edit", L"Live"},
        {L"List box / three combo variants", L"ListBox / ComboBox", L"Live"},
        {L"Standalone scrollbars", L"ScrollBar", L"Live horizontal + vertical"},
        {L"Formatted editor", L"RICHEDIT50W", L"Live; Msftedit.dll"},
        {L"Icon combo", L"ComboBoxEx32", L"Live"},
        {L"Tree / report list / header", L"SysTreeView32 / SysListView32 / SysHeader32", L"Live"},
        {L"Slider / spin / progress", L"msctls_trackbar32 / msctls_updown32 / msctls_progress32",
         L"Live including marquee/states"},
        {L"Date/time / month calendar", L"SysDateTimePick32 / SysMonthCal32", L"Live"},
        {L"AVI animation", L"SysAnimate32", L"Live embedded original clip"},
        {L"Tab / toolbar / rebar / pager", L"SysTabControl32 / ToolbarWindow32 / ReBarWindow32 / SysPager",
         L"Live"},
        {L"Tooltip / hyperlink / status", L"tooltips_class32 / SysLink / msctls_statusbar32", L"Live"},
        {L"Hotkey / IPv4", L"msctls_hotkey32 / SysIPAddress32", L"Live"},
        {L"Property sheet", L"#32770 + SysTabControl32", L"Live two original property pages"},
        {L"File open / save / folder picker", L"IFileOpenDialog / IFileSaveDialog", L"Live; paths only"},
        {L"Color / font / find / replace", L"ChooseColor / ChooseFont / FindText / ReplaceText",
         L"Live; temporary state"},
        {L"Message box / task dialog", L"MessageBox / TaskDialogIndirect", L"Live"},
        {L"Menus / title bar / system menu", L"HMENU / WS_OVERLAPPEDWINDOW", L"Live"},
        {L"Split panes / tiles / sensor hierarchy", L"App-defined composition",
         L"Live; explicitly custom where needed"},
        {L"Native row / single active sensor cell", L"SysListView32 + hit test/custom draw",
         L"Live; single cell is app-managed"},
        {L"Print / page setup / print job", L"PrintDlgEx / PageSetupDlg",
         L"Documented only; no printer interaction"},
        {L"Wizard / TaskDialog advanced combinations", L"PSH_WIZARD / advanced TASKDIALOGCONFIG",
         L"Documented only"},
        {L"Flat scrollbars / drag list", L"InitializeFlatSB / MakeDragList",
         L"Documented only; legacy helpers"},
        {L"Owner-draw permutations / virtual list", L"WM_DRAWITEM / LVS_OWNERDATA",
         L"Documented only; custom draw shown"},
        {L"Ribbon / Explorer browser / ActiveX", L"UI Ribbon / IExplorerBrowser / COM hosting",
         L"Documented only; separate frameworks"},
        {L"Tray / clipboard / OLE drag/drop / IME", L"Shell_NotifyIcon / clipboard / IDropTarget / IME",
         L"Documented only; avoid system side effects"},
        {L"Direct2D / DirectWrite / DirectComposition", L"Graphics APIs",
         L"Documented only; GDI illustration shown"},
        {L"Accessibility / touch / pen / gestures", L"MSAA / UIA / WM_POINTER",
         L"Native defaults; no custom provider or touch demo"},
        {L"All styles / locales / high contrast", L"Cross-cutting variants",
         L"Not exhaustive; manual testing remains"}};
    return items;
}
