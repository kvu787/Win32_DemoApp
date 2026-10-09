# Coverage and terminology

This is a broad educational gallery, not a claim to implement every Win32 GUI feature. The in-app start page contains the same live/excluded distinction. Microsoft separates USER32 controls, COMCTL32 common controls and other libraries; checkboxes, radio buttons and group boxes are styles of the Button class, not independent classes. Visual styles can affect both standard and common controls.

| Family / proper term        | Class / central API                                                | Live demonstration                                   |
| --------------------------- | ------------------------------------------------------------------ | ---------------------------------------------------- |
| Push/default/check/radio    | Button / BS_* / BN_CLICKED                                         | Push, default, check, mixed, radio, disabled         |
| Group box                   | Button / BS_GROUPBOX                                               | Buttons, summary and property-page groups            |
| Split button / command link | Button / BS_SPLITBUTTON / BS_COMMANDLINK / BCM_SETNOTE             | Dropdown menu and command-link note                  |
| Static text / icon          | Static / SS_LEFT / SS_ICON / STM_SETICON                           | Labels, system stock icons, etched separator         |
| Plain text editor           | Edit / ES_* / EM_SETCUEBANNER / EN_CHANGE                          | Plain, password, multiline, read-only, cue banner    |
| List box                    | ListBox / LBS_NOTIFY / LBS_EXTENDEDSEL                             | Single and extended selection                        |
| Combo box                   | ComboBox / CBS_SIMPLE / CBS_DROPDOWN / CBS_DROPDOWNLIST            | All three core variants                              |
| Scrollbar                   | ScrollBar / SetScrollInfo / WM_HSCROLL / WM_VSCROLL                | Horizontal/vertical child plus gallery scrollbars    |
| Formatted text editor       | RICHEDIT50W / MSFTEDIT_CLASS / Msftedit.dll / EM_SETCHARFORMAT     | Bold text and native editing                         |
| ComboBoxEx                  | ComboBoxEx32 / CBEM_INSERTITEM / CBEM_SETIMAGELIST                 | Icon choices                                         |
| Tree view                   | SysTreeView32 / TVM_* / TVN_SELCHANGED                             | Hierarchy, lines, icons, checkboxes                  |
| List view                   | SysListView32 / LVS_REPORT / LVM_*                                 | Report/list/icon, headers, sort, checkboxes          |
| Virtual / owner-data list   | SysListView32 / LVS_OWNERDATA / LVM_SETITEMCOUNT / LVN_GETDISPINFO | 1,000 generated rows, sorting and stable selection   |
| Header                      | SysHeader32 / HDM_INSERTITEM                                       | Separate resizable column header                     |
| Trackbar / slider           | msctls_trackbar32 / TBM_*                                          | Tick marks, selection range, progress coupling       |
| Up-down / spin control      | msctls_updown32 / UDM_SETBUDDY / UDM_SETRANGE32                    | Numeric buddy edit                                   |
| Progress bar                | msctls_progress32 / PBM_*                                          | Normal/paused/error, marquee, floating meters        |
| Date and time picker        | SysDateTimePick32 / DTS_*                                          | Optional date and localized time                     |
| Month calendar              | SysMonthCal32 / MCS_MULTISELECT                                    | Range selection and week numbers                     |
| Animation                   | SysAnimate32 / ACM_OPEN / Animate_OpenEx                           | Original embedded silent uncompressed AVI            |
| Tab                         | SysTabControl32 / TCM_* / TCN_SELCHANGE                            | Selection updates sample text; real property sheet   |
| Toolbar                     | ToolbarWindow32 / TB_ADDBUTTONS / TBN_DROPDOWN                     | Back/Forward, separator, dropdown, native tooltips   |
| Rebar                       | ReBarWindow32 / RB_INSERTBAND                                      | Movable toolbar bands                                |
| Pager                       | SysPager / PGM_SETCHILD / PGN_CALCSIZE                             | Scrollable oversized child toolbar                   |
| Status bar                  | msctls_statusbar32 / SB_SETTEXT / SBARS_SIZEGRIP                   | Main status footer and sizing grip                   |
| Tooltip                     | tooltips_class32 / TTM_ADDTOOL                                     | Toolbar tips and hyperlink tooltip                   |
| SysLink / hyperlink         | SysLink / NM_CLICK / NM_RETURN                                     | Keyboard/mouse link to Microsoft docs                |
| Hot key                     | msctls_hotkey32 / HKM_GETHOTKEY                                    | Specialized key-combination entry                    |
| IP address                  | SysIPAddress32 / IPM_SETADDRESS / IPM_GETADDRESS                   | IPv4-style four segmented fields                     |
| Image list                  | HIMAGELIST / ImageList_Create / ImageList_Destroy                  | Supporting icon resource for tree/list/combo         |
| Modal / modeless dialog     | #32770 / DialogBoxParamW / FindTextW / ReplaceTextW                | Ordinary modal template plus modeless Find/Replace   |
| Property sheet / page       | PropertySheetW / PROPSHEETHEADER / PROPSHEETPAGE / PSN_APPLY       | Two real pages, temporary settings, OK/Cancel/Apply  |
| Message box / task dialog   | MessageBoxW / TaskDialogIndirect                                   | Native safe prompts, details and verification        |
| Common Item Dialog          | IFileOpenDialog / IFileSaveDialog / FOS_PICKFOLDERS                | Open/save/folder path selection only                 |
| Color / font dialog         | ChooseColorW / ChooseFontW                                         | Temporary sample preview                             |
| Menu / context menu         | HMENU / CreateMenu / TrackPopupMenuEx                              | Menu bar, submenu, check/radio/disabled, access keys |
| Window chrome               | WS_OVERLAPPEDWINDOW / WS_EX_TOOLWINDOW / WM_SYSCOMMAND             | System caption/buttons, border, owned floating UI    |
| Application composition     | Child HWND layout / WM_PAINT / NM_CUSTOMDRAW                       | Split pane, banner, tiles, sensor groups/striping    |

## Glossary

An HWND identifies a window, including most controls. A window class defines its implementation; style flags choose a variant. The client area belongs to the application's content, while nonclient chrome includes the caption, system menu and borders. An owned window is a separate top-level window associated with an owner; a child window is confined to its parent's client area. A modal dialog blocks interaction with its owner until closed; a modeless dialog remains available beside the owner.

Sensors/details adds a live comparison of native report-row selection and application-managed single-cell selection. Its Button radio selectors, SysListView32 report, SysHeader32 header and Static value label are native windows. LVM_SUBITEMHITTEST and NM_CUSTOMDRAW are native extension points; active-cell state, arrow navigation and hierarchy fallback are application logic. See SensorSelection.md. This is not a stock Win32 spreadsheet control.

WM_COMMAND conveys many standard-control and menu events; WM_NOTIFY carries structured common-control notifications. An access key (the underlined letter from `&` in a label) differs from an accelerator such as Ctrl+O. A disabled control cannot be operated; a read-only editor still supports selection/copy. A marquee is indeterminate progress, not an actual measured percentage. A property sheet contains property pages and a native tab selector. A report list has rows and column headers, but it is not automatically a spreadsheet or tree-grid.

Owner drawing (WM_DRAWITEM) asks the application to paint a control/item. Custom drawing (NM_CUSTOMDRAW) modifies stages of a common control's normal rendering. A fully custom surface uses its own window procedure and painting/interaction. These concepts do not identify the internals of third-party UI from a screenshot.

Owner-data (LVS_OWNERDATA) supplies data on request while Windows draws the list. The live owner-data topic keeps 1,000 stable IDs, generates requested text and sorts its own ID map while preserving native selected-row identity. This differs from owner drawing, custom drawing and the sensor active-cell example. See OwnerData.md.

## Explicit exclusions and limits

The following are explained/inventoried only, not live demonstrations: print/page setup/print jobs (PrintDlgEx, PageSetupDlg); property-sheet wizards; Common Item Dialog multiselect; all advanced task-dialog combinations; all owner-draw styles and bitmap buttons/statics; owner-data filtering, providers, cache hints and type-ahead; legacy MDIClient and drag-list helpers; Ribbon/Explorer browser/ActiveX hosting; tray icons, clipboard mutation, OLE drag/drop, IME-specific flows, touch/pen/gesture input; Direct2D, DirectWrite and DirectComposition; custom UI Automation/MSAA providers; docking/property-grid/tree-grid/spreadsheet frameworks. Native edit controls still offer their ordinary OS text operations.

Flat-scrollbar APIs are unsupported by Comctl32 v6 and remain reference-only. The Native Font control is an invisible localization helper, not a font picker. The IP control is IPv4-style and does not provide IPv6 entry. Animation plays silent uncompressed/BI_RLE8 AVI and is not a general media player. These distinctions are important when deciding what Win32 provides directly.

Not all styles, messages, locales, themes, accessibility modes or permutations are covered. Floating example windows demonstrate fixed sample layouts; the main gallery is the fully scrollable/resizable educational container. Tile selection uses original custom drawing and keyboard interaction, with no custom accessibility provider. The chart is informational dummy data. See Verification.md for tested versus unverified behavior.

Official references: [Control Library](https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info), [class names](https://learn.microsoft.com/en-us/windows/win32/controls/common-control-window-classes), [button styles](https://learn.microsoft.com/en-us/windows/win32/controls/button-styles), [Rich Edit](https://learn.microsoft.com/en-us/windows/win32/controls/using-rich-edit-controls), [Common Item Dialog](https://learn.microsoft.com/en-us/windows/win32/shell/common-file-dialog), [flat scrollbars](https://learn.microsoft.com/en-us/windows/win32/controls/flat-scroll-bars).
