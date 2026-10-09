# Screenshot-inspired concept guide

The parent task inspected three screenshots and supplied the concept inventory below. This execution environment exposed no Library read/materialization action for those source files, so it could not verify their local pixels or preserve the image bytes in conversation logs. The app teaches illustrative equivalents from the verified inventory; it does not embed or reproduce proprietary artwork or claim an exact third-party HWND implementation.

Source references supplied by the parent:

- `image(20261009-070738).png`: `libfile_ee395cf447d88191953f8821cc0722e2`
- `image(20261009-070740).png`: `libfile_770d9374b21c8191b479f177b734c325`
- `image(20261009-070741).png`: `libfile_b07a30f8894481918ed599f7f44ab6f3`

## NVIDIA Control Panel / G-SYNC

| Observed concept                  | Representative Win32 implementation                     | Where to try it                         |
| --------------------------------- | ------------------------------------------------------- | --------------------------------------- |
| Caption and window buttons        | WS_OVERLAPPEDWINDOW, system menu, WM_SYSCOMMAND           | Main window and Window chrome topic     |
| Menu and navigation toolbar       | HMENU, ToolbarWindow32, separator, TBN_DROPDOWN          | Main menu/toolbar                       |
| Navigation tree / lines / icons   | SysTreeView32, TVS_HASLINES, HIMAGELIST                   | Main tree and Tree/report topic         |
| Vertical divider / split pane     | Application-defined child layout and mouse handling     | Drag main divider / Alt+Left/Right      |
| Heading/banner / information icon | Static labels/icons plus application composition        | Navigation/display layout               |
| Text and hyperlinks               | Static / SysLink                                        | Navigation/display layout               |
| Check/radio/disabled states       | Button styles, EnableWindow, BM_SETCHECK                 | Buttons and Navigation/display layout   |
| Selectable monitor image tiles    | Original WM_PAINT surface with app selection state       | Navigation/display layout               |
| Window scrollbars / sizing grip   | WS_HSCROLL / WS_VSCROLL, SBARS_SIZEGRIP                   | Resize gallery small; status corner     |

Tiles, splitters and formatted banners are application-defined concepts. A screenshot alone cannot establish whether the original used owner drawing, custom drawing, a framework or a composite of native controls. No GPU/display configuration calls are made.

## Mouse Properties

The Mouse property sheet topic opens a genuine PropertySheetW container with two original pages. It demonstrates a tab selector, Motion/Snap To/Visibility group boxes, trackbars with ticks and endpoint labels, independent checkboxes and dependent enabled state. The manager supplies OK, Cancel and Apply; changing a value enables Apply. The screenshot's Buttons/Pointers/Pointer Options/Wheel/Hardware pages are recognized as property pages, but this app does not replicate all five OS configuration pages or invoke the real system mouse properties.

The conceptual APIs are PROPSHEETHEADERW, PROPSHEETPAGEW, PropSheet_Changed, PSN_APPLY and PSN_RESET; page dialogs use #32770. A group box is a Button style, not a separate container that enforces logic. Native [property sheets](https://learn.microsoft.com/en-us/windows/win32/controls/property-sheets) implement page switching and button behavior; the application defines its own setting semantics.

## HWiNFO

The main gallery's menu, toolbar, tree, divider and status footer illustrate the shell of a hardware-information app. Sensors/details uses a native report list with Sensor/Current/Minimum/Maximum headers, stock icons and application-managed CPU hierarchy. Double-click or Enter expands/collapses its dummy group. NM_CUSTOMDRAW supplies alternate row colors using system colors.

The native Row and Cell radio buttons compare two working behaviors. Row uses LVS_EX_FULLROWSELECT and native selection rendering. Cell retains the native report list/header and adds one application-managed active cell, LVM_SUBITEMHITTEST mouse handling, arrow-key navigation and NM_CUSTOMDRAW subitem colors. The selected-value label names the sensor, column and value. Blank/header space preserves the active cell. Selection follows stable dummy-sensor IDs through row-index changes; a hidden CPU child falls back to its visible group, retaining its column. DPI/theme changes preserve the cell state and controls.

The System Summary example composes group boxes, labels, read-only Edit fields and a ComboBox with dummy data. Floating Sensor Status and Active Clock buttons create real owned top-level tool windows. The clock uses native progress bars plus numeric labels to illustrate horizontal meters. Values are illustrative and never read actual sensors.

A hierarchical multicolumn sensor tree-grid, cell selection, rich feature labels, docking panels and charts are not universal stock Win32 controls. A native list view gives report rows, images and headers; the app adds group expansion, striping and single-cell state. This build now demonstrates a bounded row-versus-cell comparison. It has no cell editing, multiselection, spreadsheet formulas or custom cell-level accessibility provider. The floating sensor example retains native row selection. The original HWiNFO implementation remains uncertain. Microsoft's [subitem hit testing](https://learn.microsoft.com/en-us/windows/win32/controls/lvm-subitemhittest), [list-view custom drawing](https://learn.microsoft.com/en-us/windows/win32/controls/nm-customdraw-list-view) and [list views](https://learn.microsoft.com/en-us/windows/win32/controls/list-view-controls) explain the primitives.
