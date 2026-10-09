#include "App.h"
#include <array>
#include <map>
#include <ostream>
#include <stdexcept>
#include <windowsx.h>

namespace
{
std::wstring Text(HWND window)
{
    const int length = GetWindowTextLengthW(window);
    std::wstring text(size_t(length) + 1, L'\0');
    GetWindowTextW(window, text.data(), length + 1);
    text.resize(length);
    return text;
}
std::wstring Class(HWND window)
{
    wchar_t text[128]{};
    GetClassNameW(window, text, 128);
    return text;
}
std::map<LPARAM, std::array<std::wstring, 4>> Data(HWND list)
{
    std::map<LPARAM, std::array<std::wstring, 4>> result;
    for (int row = 0; row < ListView_GetItemCount(list); ++row)
    {
        LVITEMW item{};
        item.mask = LVIF_PARAM;
        item.iItem = row;
        ListView_GetItem(list, &item);
        for (int column = 0; column < 4; ++column)
        {
            wchar_t text[256]{};
            ListView_GetItemText(list, row, column, text, 256);
            result[item.lParam][column] = text;
        }
    }
    return result;
}
} // namespace
void App::SensorAudit(std::ostream &out)
{
    const auto check = [&](bool okay, const char *name)
    {
        out << (okay ? "PASS sensor: " : "FAIL sensor: ") << name << '\n';
        if (!okay)
            throw std::runtime_error(name);
    };
    const UINT initialDpi = dpi_;
    const auto initialKey = sensorKey_;
    const int initialColumn = sensorColumn_;
    const bool initialMode = sensorCellMode_, initialExpansion = sensorExpanded_;
    const HWND focus = GetFocus();
    sensorKey_ = 100;
    sensorColumn_ = 0;
    sensorCellMode_ = false;
    sensorExpanded_ = true;
    Build(12);
    const HWND list = GetDlgItem(panel_, 1201), header = ListView_GetHeader(list);
    const HWND value = GetDlgItem(panel_, 1212);
    const auto baseline = Data(list);
    const auto style = GetWindowLongPtrW(list, GWL_STYLE);
    check(Class(list) == WC_LISTVIEWW && Class(header) == WC_HEADERW &&
              (style & LVS_TYPEMASK) == LVS_REPORT && (style & LVS_SINGLESEL) &&
              !(style & (LVS_EDITLABELS | LVS_OWNERDRAWFIXED)) && Header_GetItemCount(header) == 4,
          "native single-select report/header stays read-only with four columns");
    check(Class(GetDlgItem(panel_, 1210)) == L"Button" && Class(GetDlgItem(panel_, 1211)) == L"Button" &&
              (GetWindowLongPtrW(GetDlgItem(panel_, 1210), GWL_STYLE) & BS_TYPEMASK) == BS_AUTORADIOBUTTON &&
              (GetWindowLongPtrW(GetDlgItem(panel_, 1211), GWL_STYLE) & BS_TYPEMASK) == BS_AUTORADIOBUTTON &&
              Class(value) == L"Static",
          "mode selectors and selected-value label are native controls");
    check((ListView_GetExtendedListViewStyle(list) & LVS_EX_FULLROWSELECT) &&
              ListView_GetSelectedCount(list) == 1 &&
              SendDlgItemMessageW(panel_, 1210, BM_GETCHECK, 0, 0) == BST_CHECKED,
          "Row mode uses native full-row selection");
    SensorSelect(list, 2, 0);
    MSG rowEnter{};
    rowEnter.hwnd = list;
    rowEnter.message = WM_KEYDOWN;
    rowEnter.wParam = VK_RETURN;
    out << "Sensor Row native Enter dialog code="
        << SendMessageW(list, WM_GETDLGCODE, VK_RETURN, reinterpret_cast<LPARAM>(&rowEnter)) << '\n';
    SensorSelect(list, 0, 0);
    const BOOL rowRouted = IsDialogMessageW(window_, &rowEnter);
    if (!rowRouted)
        DispatchMessageW(&rowEnter);
    out << "Sensor Row Enter route handled=" << rowRouted << " group expanded=" << sensorExpanded_ << '\n';
    check(!sensorExpanded_, "Row Enter reaches the group through the actual dialog routing path");
    const BOOL rowReturnRouted = IsDialogMessageW(window_, &rowEnter);
    if (!rowReturnRouted)
        DispatchMessageW(&rowEnter);
    check(sensorExpanded_, "Row Enter routes a second time and restores the group");
    SensorSelect(list, 2, 0);
    check(Text(value).find(L"Core clock") != std::wstring::npos &&
              Text(value).find(L"3,800 MHz") != std::wstring::npos,
          "native row selection reports its dummy values");
    NMLVCUSTOMDRAW draw{};
    draw.nmcd.hdr = {list, 1201, NM_CUSTOMDRAW};
    draw.nmcd.dwDrawStage = CDDS_ITEMPREPAINT;
    draw.nmcd.dwItemSpec = 2;
    check(SendMessageW(panel_, WM_NOTIFY, 1201, reinterpret_cast<LPARAM>(&draw)) == CDRF_NEWFONT,
          "Row mode custom draw retains native item rendering");

    SendMessageW(panel_, WM_COMMAND, MAKEWPARAM(1211, BN_CLICKED),
                 reinterpret_cast<LPARAM>(GetDlgItem(panel_, 1211)));
    check(sensorCellMode_ && !(ListView_GetExtendedListViewStyle(list) & LVS_EX_FULLROWSELECT) &&
              ListView_GetSelectedCount(list) == 0 && GetWindowLongPtrW(list, GWL_STYLE) == style &&
              SendDlgItemMessageW(panel_, 1211, BM_GETCHECK, 0, 0) == BST_CHECKED,
          "Cell mode keeps the native report class/styles while replacing row selection with app state");
    draw.nmcd.dwDrawStage = CDDS_PREPAINT;
    check(SendMessageW(panel_, WM_NOTIFY, 1201, reinterpret_cast<LPARAM>(&draw)) == CDRF_NOTIFYITEMDRAW,
          "custom draw requests native item stages");
    draw.nmcd.dwDrawStage = CDDS_ITEMPREPAINT;
    check(SendMessageW(panel_, WM_NOTIFY, 1201, reinterpret_cast<LPARAM>(&draw)) ==
              (CDRF_NOTIFYSUBITEMDRAW | CDRF_SKIPPOSTPAINT),
          "Cell mode requests subitem colors and suppresses native row focus painting");
    const wchar_t *columns[]{L"Sensor", L"Current", L"Minimum", L"Maximum"};
    for (int column = 0; column < 4; ++column)
    {
        RECT rect{};
        check(SensorRect(list, 1, column, rect), "native header/item rectangles identify each column");
        POINT point{(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2};
        const auto hit = SensorHit(list, point);
        check(hit.row == 1 && hit.column == column, "LVM_SUBITEMHITTEST resolves each of all four cells");
        SendMessageW(list, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
        SendMessageW(list, WM_LBUTTONUP, 0, MAKELPARAM(point.x, point.y));
        check(sensorKey_ == 1 && sensorColumn_ == column && ListView_GetSelectedCount(list) == 0 &&
                  Text(value).find(columns[column]) != std::wstring::npos &&
                  Text(value).find(baseline.at(1)[column]) != std::wstring::npos,
              "native list subclass mouse handler updates one active cell and the value label");
        for (int row = 0; row < ListView_GetItemCount(list); ++row)
            for (int subitem = 0; subitem < 4; ++subitem)
            {
                draw.nmcd.dwDrawStage = CDDS_ITEMPREPAINT | CDDS_SUBITEM;
                draw.nmcd.dwItemSpec = row;
                draw.iSubItem = subitem;
                const bool active = row == 1 && subitem == column;
                const auto result = SendMessageW(panel_, WM_NOTIFY, 1201, reinterpret_cast<LPARAM>(&draw));
                check(result == CDRF_NEWFONT &&
                          draw.clrTextBk == GetSysColor(active ? COLOR_HIGHLIGHT
                                                               : (row % 2 ? COLOR_BTNFACE : COLOR_WINDOW)) &&
                          draw.clrText == GetSysColor(active ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT),
                      "custom draw highlights exactly the active cell using system colors");
            }
    }
    RECT last{}, client{};
    SensorRect(list, 4, 3, last);
    GetClientRect(list, &client);
    const POINT blankPoints[]{
        {10, 2}, {10, last.bottom + 20}, {last.right + 3, (last.top + last.bottom) / 2}, {-1, last.top + 2}};
    const auto savedLabel = Text(value);
    for (auto point : blankPoints)
    {
        check(SensorHit(list, point).row == -1, "header/below-row/right-margin/outside hits are blank");
        SendMessageW(list, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
        check(sensorKey_ == 1 && sensorColumn_ == 3 && Text(value) == savedLabel,
              "blank mouse messages retain the active cell and label");
    }
    SensorSelect(list, 0, 0);
    SendMessageW(list, WM_KEYDOWN, VK_LEFT, 0);
    SendMessageW(list, WM_KEYDOWN, VK_UP, 0);
    check(sensorKey_ == 100 && sensorColumn_ == 0, "Left/Up clamp at the first cell");
    for (int i = 0; i < 8; ++i)
        SendMessageW(list, WM_KEYDOWN, VK_RIGHT, 0);
    for (int i = 0; i < 8; ++i)
        SendMessageW(list, WM_KEYDOWN, VK_DOWN, 0);
    check(sensorKey_ == 4 && sensorColumn_ == 3 && Text(value).find(L"67 C") != std::wstring::npos,
          "Right/Down move through cells and clamp at the last row/column");
    for (int i = 0; i < 8; ++i)
        SendMessageW(list, WM_KEYDOWN, VK_LEFT, 0);
    for (int i = 0; i < 8; ++i)
        SendMessageW(list, WM_KEYDOWN, VK_UP, 0);
    check(sensorKey_ == 100 && sensorColumn_ == 0 && ListView_GetSelectedCount(list) == 0,
          "reverse arrow navigation returns to the first cell without native row selection");
    check(SendMessageW(list, WM_GETDLGCODE, VK_RIGHT, 0) & DLGC_WANTARROWS,
          "native keyboard dialog-code path keeps arrows for the cell handler");
    SendMessageW(list, WM_KEYDOWN, VK_RETURN, 0);
    check(!sensorExpanded_ && ListView_GetItemCount(list) == 2 && sensorKey_ == 100,
          "cell Enter handler collapses the dummy group with a valid active cell");
    SendMessageW(list, WM_KEYDOWN, VK_RETURN, 0);
    check(sensorExpanded_ && Data(list) == baseline, "cell Enter expands with all original data intact");
    RECT group{};
    SensorRect(list, 0, 0, group);
    SendMessageW(list, WM_LBUTTONDBLCLK, MK_LBUTTON,
                 MAKELPARAM((group.left + group.right) / 2, (group.top + group.bottom) / 2));
    check(!sensorExpanded_, "cell double-click handler toggles the native group row");
    SendMessageW(list, WM_KEYDOWN, VK_RETURN, 0);

    SensorSelect(list, 2, 2);
    sensorExpanded_ = false;
    PopulateReport(list, true);
    check(sensorKey_ == 100 && sensorColumn_ == 2 &&
              Text(value).find(L"(empty group cell)") != std::wstring::npos,
          "a disappearing child falls back to its group and retains the selected column");
    sensorExpanded_ = true;
    PopulateReport(list, true);
    SensorSelect(list, SensorIndex(list, 4), 3);
    const auto gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    const auto user = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    bool invariant = true;
    for (int cycle = 0; cycle < 32; ++cycle)
    {
        sensorExpanded_ = false;
        PopulateReport(list, true);
        invariant &= SensorIndex(list, 4) == 1 && sensorKey_ == 4 && sensorColumn_ == 3 &&
                     Data(list).at(4) == baseline.at(4);
        sensorExpanded_ = true;
        PopulateReport(list, true);
        invariant &=
            SensorIndex(list, 4) == 4 && sensorKey_ == 4 && sensorColumn_ == 3 && Data(list) == baseline;
    }
    check(invariant,
          "32 collapse/expand cycles preserve stable GPU identity, active value and all dummy data");
    check(GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS) <= gdi + 2 &&
              GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS) <= user + 2,
          "repeated cell selection/data repopulation retains stable GDI/USER resources");
    SensorSelect(list, 4, 2);
    SetWindowTextW(GetDlgItem(panel_, 1202), L"retained dummy CPU summary");
    SendDlgItemMessageW(panel_, 1204, CB_SETCURSEL, 1, 0);
    RefreshPresentation(144);
    SendMessageW(window_, WM_THEMECHANGED, 0, 0);
    RefreshPresentation(192);
    RefreshPresentation(initialDpi);
    check(GetDlgItem(panel_, 1201) == list && ListView_GetHeader(list) == header && sensorCellMode_ &&
              sensorKey_ == 4 && sensorColumn_ == 2 && Text(value).find(L"45 C") != std::wstring::npos &&
              Text(GetDlgItem(panel_, 1202)) == L"retained dummy CPU summary" &&
              SendDlgItemMessageW(panel_, 1204, CB_GETCURSEL, 0, 0) == 1,
          "DPI/theme refresh retains cell state, native HWNDs, summary input and profile selection");
    if (options_.visible)
        Capture(options_.report.parent_path() / L"SensorsCellSelection.bmp");
    SendMessageW(panel_, WM_COMMAND, MAKEWPARAM(1210, BN_CLICKED),
                 reinterpret_cast<LPARAM>(GetDlgItem(panel_, 1210)));
    check(!sensorCellMode_ && (ListView_GetExtendedListViewStyle(list) & LVS_EX_FULLROWSELECT) &&
              ListView_GetSelectedCount(list) == 1 && ListView_GetNextItem(list, -1, LVNI_SELECTED) == 4 &&
              SendDlgItemMessageW(panel_, 1210, BM_GETCHECK, 0, 0) == BST_CHECKED &&
              SendDlgItemMessageW(panel_, 1211, BM_GETCHECK, 0, 0) == BST_UNCHECKED,
          "switching back restores native row selection of the same stable sensor");
    if (options_.visible)
        Capture(options_.report.parent_path() / L"SensorsRowSelection.bmp");
    SensorSelect(list, 0, 0);
    NMLISTVIEW click{};
    click.hdr = {list, 1201, NM_DBLCLK};
    SendMessageW(panel_, WM_NOTIFY, 1201, reinterpret_cast<LPARAM>(&click));
    check(!sensorExpanded_ && ListView_GetSelectedCount(list) == 1,
          "Row mode native double-click notification keeps hierarchy behavior");
    SendMessageW(panel_, WM_NOTIFY, 1201, reinterpret_cast<LPARAM>(&click));
    check(sensorExpanded_ && Data(list) == baseline && GetFocus() == focus && fatal_.empty(),
          "sensor handlers preserve data and thread focus without input injection or callback errors");
    sensorKey_ = initialKey;
    sensorColumn_ = initialColumn;
    sensorCellMode_ = initialMode;
    sensorExpanded_ = initialExpansion;
    Build(0);
    if (options_.visible)
        out << "VISUAL sensor: app-only native Row/Cell rendering captures saved for inspection.\n";
    else
        out << "UNVERIFIED sensor: visual cell painting (hidden audit).\n";
    out << "UNVERIFIED sensor: actual mouse/keyboard focus interaction and cell-level accessibility. "
           "Handler messages/custom-draw decisions were tested; no system input injection.\n";
}
