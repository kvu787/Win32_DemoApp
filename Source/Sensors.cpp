#include "App.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <windowsx.h>

namespace
{
std::wstring CellText(HWND list, int row, int column)
{
    wchar_t text[256]{};
    ListView_GetItemText(list, row, column, text, 256);
    return text;
}
constexpr const wchar_t *SensorColumns[]{L"Sensor", L"Current", L"Minimum", L"Maximum"};
} // namespace
LPARAM App::SensorKey(HWND list, int row) const
{
    LVITEMW item{};
    item.mask = LVIF_PARAM;
    item.iItem = row;
    return row >= 0 && ListView_GetItem(list, &item) ? item.lParam : -1;
}
int App::SensorIndex(HWND list, LPARAM key) const
{
    for (int row = 0; row < ListView_GetItemCount(list); ++row)
        if (SensorKey(list, row) == key)
            return row;
    return -1;
}
bool App::SensorRect(HWND list, int row, int column, RECT &rect) const
{
    const HWND header = ListView_GetHeader(list);
    RECT bounds{}, heading{};
    if (row < 0 || row >= ListView_GetItemCount(list) || column < 0 ||
        column >= Header_GetItemCount(header) || !ListView_GetItemRect(list, row, &bounds, LVIR_BOUNDS) ||
        !Header_GetItemRect(header, column, &heading))
        return false;
    MapWindowPoints(header, list, reinterpret_cast<POINT *>(&heading), 2);
    rect = {heading.left, bounds.top, heading.right, bounds.bottom};
    return true;
}
App::SensorCell App::SensorHit(HWND list, POINT point) const
{
    RECT client{};
    GetClientRect(list, &client);
    if (!PtInRect(&client, point))
        return {};
    LVHITTESTINFO hit{};
    hit.pt = point;
    const int row = int(SendMessageW(list, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&hit)));
    RECT cell{};
    if (row < 0 || !SensorRect(list, row, hit.iSubItem, cell) || !PtInRect(&cell, point))
        return {};
    return {row, hit.iSubItem};
}
void App::SensorLabel(HWND list)
{
    const HWND label = GetDlgItem(panel_, 1212);
    if (!label)
        return;
    int row = SensorIndex(list, sensorKey_);
    if (!sensorCellMode_)
        row = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    std::wstring text;
    if (row < 0)
        text = L"Native row: none selected.";
    else if (sensorCellMode_)
    {
        const auto value = CellText(list, row, sensorColumn_);
        text = L"App-managed cell: " + CellText(list, row, 0) + L" / " + SensorColumns[sensorColumn_] +
               L" = " + (value.empty() ? L"(empty group cell)" : value);
    }
    else
        text = L"Native row: " + CellText(list, row, 0) + L" | Current " + CellText(list, row, 1) +
               L" | Minimum " + CellText(list, row, 2) + L" | Maximum " + CellText(list, row, 3);
    SetWindowTextW(label, text.c_str());
}
void App::SensorSelect(HWND list, int row, int column)
{
    if (row < 0 || row >= ListView_GetItemCount(list) || column < 0 || column >= 4)
        return;
    sensorKey_ = SensorKey(list, row);
    sensorColumn_ = column;
    sensorUpdating_ = true;
    ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(list, row, LVIS_FOCUSED | (sensorCellMode_ ? 0 : LVIS_SELECTED),
                          LVIS_SELECTED | LVIS_FOCUSED);
    sensorUpdating_ = false;
    ListView_EnsureVisible(list, row, FALSE);
    RECT cell{}, client{};
    GetClientRect(list, &client);
    if (sensorCellMode_ && SensorRect(list, row, column, cell))
    {
        const int dx = cell.left < 0 ? cell.left : std::max(0L, cell.right - client.right);
        if (dx)
            ListView_Scroll(list, dx, 0);
    }
    SensorLabel(list);
    InvalidateRect(list, nullptr, FALSE);
}
void App::SensorRestore(HWND list)
{
    int row = SensorIndex(list, sensorKey_);
    if (row < 0)
        row = SensorIndex(list, 100); // A collapsed child falls back to its visible group.
    if (row >= 0)
        SensorSelect(list, row, std::clamp(sensorColumn_, 0, 3));
}
void App::SensorMode(bool cell)
{
    const HWND list = GetDlgItem(panel_, 1201);
    if (!list)
        return;
    sensorCellMode_ = cell;
    CheckRadioButton(panel_, 1210, 1211, cell ? 1211 : 1210);
    ListView_SetExtendedListViewStyleEx(list, LVS_EX_FULLROWSELECT, cell ? 0 : LVS_EX_FULLROWSELECT);
    SensorRestore(list);
}
void App::SensorToggle(HWND list)
{
    if (sensorKey_ != 100)
        return;
    sensorExpanded_ = !sensorExpanded_;
    PopulateReport(list, true);
    Event(L"Application-managed sensor group " +
          std::wstring(sensorExpanded_ ? L"expanded." : L"collapsed."));
}
bool App::SensorKeyDown(HWND list, UINT key)
{
    if (!sensorCellMode_)
        return false;
    int row = SensorIndex(list, sensorKey_), column = sensorColumn_;
    if (row < 0)
        return false;
    switch (key)
    {
    case VK_LEFT:
        column = std::max(0, column - 1);
        break;
    case VK_RIGHT:
        column = std::min(3, column + 1);
        break;
    case VK_UP:
        row = std::max(0, row - 1);
        break;
    case VK_DOWN:
        row = std::min(ListView_GetItemCount(list) - 1, row + 1);
        break;
    case VK_RETURN:
        SensorToggle(list);
        return true;
    case VK_SPACE:
        break;
    default:
        return false;
    }
    SensorSelect(list, row, column);
    return true;
}
void App::SensorMouse(HWND list, POINT point, bool doubleClick)
{
    const auto cell = SensorHit(list, point);
    if (cell.row < 0)
        return; // Blank/header hits leave the active cell unchanged.
    if (!options_.test)
        SetFocus(list);
    SensorSelect(list, cell.row, cell.column);
    if (doubleClick)
        SensorToggle(list);
    else
        Event(L"LVM_SUBITEMHITTEST selected an application-managed sensor cell.");
}
LRESULT App::SensorDraw(NMLVCUSTOMDRAW &draw)
{
    if (draw.nmcd.dwDrawStage == CDDS_PREPAINT)
        return CDRF_NOTIFYITEMDRAW;
    if (draw.nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
        draw.clrText = GetSysColor(COLOR_WINDOWTEXT);
        draw.clrTextBk = draw.nmcd.dwItemSpec % 2 ? GetSysColor(COLOR_BTNFACE) : GetSysColor(COLOR_WINDOW);
        return sensorCellMode_ ? CDRF_NOTIFYSUBITEMDRAW | CDRF_SKIPPOSTPAINT : CDRF_NEWFONT;
    }
    if (sensorCellMode_ && draw.nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM))
    {
        const bool active = SensorKey(draw.nmcd.hdr.hwndFrom, int(draw.nmcd.dwItemSpec)) == sensorKey_ &&
                            draw.iSubItem == sensorColumn_;
        draw.clrText = GetSysColor(active ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
        draw.clrTextBk =
            GetSysColor(active ? COLOR_HIGHLIGHT : (draw.nmcd.dwItemSpec % 2 ? COLOR_BTNFACE : COLOR_WINDOW));
        return CDRF_NEWFONT;
    }
    return CDRF_DODEFAULT;
}
LRESULT CALLBACK App::SensorProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data)
{
    auto self = reinterpret_cast<App *>(data);
    try
    {
        if (m == WM_NCDESTROY)
            RemoveWindowSubclass(h, SensorProc, id);
        if (self->sensorCellMode_)
        {
            if (m == WM_LBUTTONDOWN || m == WM_LBUTTONDBLCLK)
            {
                self->SensorMouse(h, {GET_X_LPARAM(l), GET_Y_LPARAM(l)}, m == WM_LBUTTONDBLCLK);
                return 0;
            }
            if (m == WM_LBUTTONUP || (m == WM_CHAR && (w == L'\r' || w == L' ')))
                return 0;
            if (m == WM_KEYDOWN && self->SensorKeyDown(h, UINT(w)))
                return 0;
            if (m == WM_GETDLGCODE)
                return DefSubclassProc(h, m, w, l) | DLGC_WANTARROWS |
                       (w == VK_RETURN ? DLGC_WANTMESSAGE : 0);
        }
    }
    catch (const std::exception &error)
    {
        self->fatal_ = std::wstring(error.what(), error.what() + strlen(error.what()));
        return 0;
    }
    return DefSubclassProc(h, m, w, l);
}
