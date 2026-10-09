#include "App.h"
#include <algorithm>
#include <numeric>
#include <string_view>

namespace
{
constexpr const wchar_t *Categories[]{L"Alpha", L"Beta", L"Gamma", L"Delta"};
int GeneratedValue(int id)
{
    return id * 37 % 1000;
}
} // namespace
std::wstring App::VirtualText(int id, int column) const
{
    if (id < 0 || id >= VirtualRows || column < 0 || column >= 3)
        return {};
    if (column == 0)
    {
        auto number = std::to_wstring(id + 1);
        return L"Dummy " + std::wstring(4 - number.size(), L'0') + number;
    }
    return column == 1 ? Categories[id % 4] : std::to_wstring(GeneratedValue(id));
}
int App::VirtualId(int row) const
{
    if (row < 0 || size_t(row) >= virtualOrder_.size())
        return -1;
    const int id = virtualOrder_[row];
    return id >= 0 && id < VirtualRows ? id : -1;
}
void App::VirtualLabels()
{
    const HWND list = GetDlgItem(panel_, 1401), stats = GetDlgItem(panel_, 1403),
               value = GetDlgItem(panel_, 1404);
    if (!list || !stats || !value)
        return;
    const std::wstring text = L"Model IDs: " + std::to_wstring(virtualOrder_.size()) +
                              L" / native item count: " + std::to_wstring(ListView_GetItemCount(list)) +
                              L" / LVN_GETDISPINFO text requests: " + std::to_wstring(virtualRequests_) +
                              L" / rejected requests: " + std::to_wstring(virtualRejected_) +
                              L" (requests may repeat; this is not a benchmark)";
    SetWindowTextW(stats, text.c_str());
    const std::wstring selected = virtualSelected_ < 0
                                      ? L"Native row selection: none."
                                      : L"Native row selection: " + VirtualText(virtualSelected_, 0) +
                                            L" / " + VirtualText(virtualSelected_, 1) +
                                            L" / generated value " + VirtualText(virtualSelected_, 2) +
                                            L". Stable ID is retained when model order changes.";
    SetWindowTextW(value, selected.c_str());
}
void App::VirtualSelect(HWND list, int row)
{
    if (row != -1 && (VirtualId(row) < 0 || row >= ListView_GetItemCount(list)))
        return;
    virtualSelected_ = VirtualId(row);
    virtualUpdating_ = true;
    ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (row >= 0)
    {
        ListView_SetItemState(list, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(list, row, FALSE);
    }
    virtualUpdating_ = false;
    VirtualLabels();
}
void App::VirtualBind(HWND list)
{
    if (virtualOrder_.empty())
    {
        virtualOrder_.resize(VirtualRows);
        std::iota(virtualOrder_.begin(), virtualOrder_.end(), 0);
    }
    virtualUpdating_ = true;
    ListView_SetItemCountEx(list, int(virtualOrder_.size()), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    virtualUpdating_ = false;
    const auto selected = std::find(virtualOrder_.begin(), virtualOrder_.end(), virtualSelected_);
    VirtualSelect(list, selected == virtualOrder_.end() ? -1 : int(selected - virtualOrder_.begin()));
    const HWND header = ListView_GetHeader(list);
    for (int column = 0; column < 3; ++column)
    {
        HDITEMW item{};
        item.mask = HDI_FORMAT;
        Header_GetItem(header, column, &item);
        item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (column == virtualSortColumn_)
            item.fmt |= virtualAscending_ ? HDF_SORTUP : HDF_SORTDOWN;
        Header_SetItem(header, column, &item);
    }
    InvalidateRect(list, nullptr, FALSE);
    VirtualLabels();
}
void App::VirtualSort(HWND list, int column)
{
    if (column < 0 || column >= 3)
        return;
    if (column == virtualSortColumn_)
        virtualAscending_ = !virtualAscending_;
    else
    {
        virtualSortColumn_ = column;
        virtualAscending_ = true;
    }
    virtualSelected_ = VirtualId(ListView_GetNextItem(list, -1, LVNI_SELECTED));
    virtualUpdating_ = true;
    ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    std::sort(virtualOrder_.begin(), virtualOrder_.end(),
              [&](int left, int right)
              {
                  int comparison = 0;
                  if (column == 1)
                  {
                      const int order =
                          std::wstring_view(Categories[left % 4]).compare(Categories[right % 4]);
                      comparison = order < 0 ? -1 : order > 0 ? 1 : 0;
                  }
                  else
                  {
                      const int a = column == 0 ? left : GeneratedValue(left);
                      const int b = column == 0 ? right : GeneratedValue(right);
                      comparison = a < b ? -1 : a > b ? 1 : 0;
                  }
                  return comparison ? (virtualAscending_ ? comparison < 0 : comparison > 0) : left < right;
              });
    virtualUpdating_ = false;
    VirtualBind(list);
    Event(
        L"Owner-data list: application sorted its stable-ID map; native selection follows the same record.");
}
bool App::VirtualNotify(NMHDR *notification, LRESULT &result)
{
    const HWND list = GetDlgItem(panel_, 1401);
    if (!list || notification->hwndFrom != list)
        return false;
    result = 0;
    if (notification->code == LVN_GETDISPINFOW)
    {
        auto &item = reinterpret_cast<NMLVDISPINFOW *>(notification)->item;
        if (item.mask & LVIF_TEXT)
        {
            ++virtualRequests_;
            const int id = VirtualId(item.iItem);
            const bool valid = id >= 0 && item.iSubItem >= 0 && item.iSubItem < 3;
            if (!valid)
                ++virtualRejected_;
            if (item.pszText && item.pszText != LPSTR_TEXTCALLBACKW && item.cchTextMax > 0)
            {
                const auto text = valid ? VirtualText(id, item.iSubItem) : std::wstring{};
                const size_t count = std::min(text.size(), size_t(item.cchTextMax - 1));
                std::copy_n(text.data(), count, item.pszText);
                item.pszText[count] = L'\0';
            }
            if (!virtualStatsPending_)
            {
                virtualStatsPending_ = true;
                PostMessageW(window_, VirtualStatsMessage, 0, 0); // Update outside the display callback.
            }
        }
        if (item.mask & LVIF_IMAGE)
            item.iImage = -1;
        return true;
    }
    if (notification->code == LVN_COLUMNCLICK)
    {
        VirtualSort(list, reinterpret_cast<NMLISTVIEW *>(notification)->iSubItem);
        return true;
    }
    if (notification->code == LVN_ITEMCHANGED || notification->code == LVN_ODSTATECHANGED)
    {
        if (!virtualUpdating_ && !navigating_)
        {
            virtualSelected_ = VirtualId(ListView_GetNextItem(list, -1, LVNI_SELECTED));
            VirtualLabels();
        }
        return true;
    }
    return false;
}
