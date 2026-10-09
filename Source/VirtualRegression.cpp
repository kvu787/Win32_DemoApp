#include "App.h"
#include <algorithm>
#include <array>
#include <numeric>
#include <ostream>
#include <set>
#include <stdexcept>

namespace
{
std::wstring Class(HWND window)
{
    wchar_t text[128]{};
    GetClassNameW(window, text, 128);
    return text;
}
int Children(HWND window)
{
    int count = 0;
    EnumChildWindows(
        window,
        [](HWND, LPARAM data) -> BOOL
        {
            ++*reinterpret_cast<int *>(data);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&count));
    return count;
}
} // namespace
void App::VirtualAudit(std::ostream &out)
{
    const auto check = [&](bool okay, const char *name)
    {
        out << (okay ? "PASS owner-data: " : "FAIL owner-data: ") << name << '\n';
        if (!okay)
            throw std::runtime_error(name);
    };
    const auto savedOrder = virtualOrder_;
    const int savedSelected = virtualSelected_, savedColumn = virtualSortColumn_;
    const bool savedAscending = virtualAscending_;
    const UINT initialDpi = dpi_;
    const HWND focus = GetFocus();
    Build(14);
    HWND list = GetDlgItem(panel_, 1401);
    const HWND populated = GetDlgItem(panel_, 1402), header = ListView_GetHeader(list);
    const auto style = GetWindowLongPtrW(list, GWL_STYLE);
    check(Class(list) == WC_LISTVIEWW && Class(header) == WC_HEADERW &&
              (style & LVS_TYPEMASK) == LVS_REPORT && (style & LVS_OWNERDATA) &&
              !(style & (LVS_OWNERDRAWFIXED | LVS_SORTASCENDING | LVS_SORTDESCENDING)) &&
              Header_GetItemCount(header) == 3,
          "native LVS_OWNERDATA report/header uses native drawing, not owner drawing");
    check(!(GetWindowLongPtrW(populated, GWL_STYLE) & LVS_OWNERDATA) &&
              ListView_GetItemCount(populated) == 3 && Class(populated) == WC_LISTVIEWW,
          "ordinary comparison stores its three sample rows in a separate native list");
    const auto permutation = [&]
    {
        const std::set<int> unique(virtualOrder_.begin(), virtualOrder_.end());
        return unique.size() == VirtualRows && *unique.begin() == 0 && *unique.rbegin() == VirtualRows - 1;
    };
    check(virtualOrder_.size() == VirtualRows && ListView_GetItemCount(list) == VirtualRows && permutation(),
          "model and native count agree on 1,000 distinct bounded IDs");
    const int children = Children(list);
    bool counts = children < 5;
    virtualUpdating_ = true;
    for (int count : {1, 100, VirtualRows})
    {
        ListView_SetItemCountEx(list, count, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
        counts &= ListView_GetItemCount(list) == count && Children(list) == children;
    }
    virtualUpdating_ = false;
    VirtualBind(list);
    check(counts, "native count changes allocate no HWND per row; model is an ID map");
    wchar_t nativeText[64]{};
    LVITEMW native{};
    native.mask = LVIF_TEXT;
    native.iItem = 999;
    native.iSubItem = 2;
    native.pszText = nativeText;
    native.cchTextMax = 64;
    const auto requests = virtualRequests_;
    const BOOL obtained = ListView_GetItem(list, &native);
    check(obtained && virtualRequests_ > requests &&
              std::wstring(nativeText) == VirtualText(VirtualId(999), 2),
          "LVM_GETITEM causes real LVN_GETDISPINFO retrieval for a distant row");
    const auto request = [&](int row, int column, wchar_t *buffer, int capacity, UINT mask = LVIF_TEXT)
    {
        NMLVDISPINFOW info{};
        info.hdr = {list, 1401, LVN_GETDISPINFOW};
        info.item.mask = mask;
        info.item.iItem = row;
        info.item.iSubItem = column;
        info.item.pszText = buffer;
        info.item.cchTextMax = capacity;
        SendMessageW(panel_, WM_NOTIFY, 1401, reinterpret_cast<LPARAM>(&info));
    };
    bool textOkay = true;
    for (int row = 0; row < VirtualRows; ++row)
        for (int column = 0; column < 3; ++column)
        {
            std::array<wchar_t, 66> buffer{};
            buffer.front() = buffer.back() = L'#';
            request(row, column, buffer.data() + 1, 64);
            textOkay &= std::wstring(buffer.data() + 1) == VirtualText(VirtualId(row), column) &&
                        buffer.front() == L'#' && buffer.back() == L'#';
        }
    check(textOkay, "all 3,000 valid row/subitem requests return bounded generated text");
    std::array<wchar_t, 5> small{L'#', L'x', L'x', L'x', L'#'};
    request(0, 0, small.data() + 1, 3);
    check(small[0] == L'#' && small[1] == L'D' && small[2] == L'u' && small[3] == 0 && small[4] == L'#',
          "short callback buffer is truncated and terminated without overwriting guards");
    small[1] = L'x';
    request(0, 0, small.data() + 1, 1);
    check(small[1] == 0 && small[0] == L'#' && small[4] == L'#', "one-character buffer contains only NUL");
    small[1] = L'x';
    request(0, 0, small.data() + 1, 0);
    request(0, 0, nullptr, 8);
    request(0, 0, LPSTR_TEXTCALLBACKW, 8);
    const auto beforeNoText = virtualRequests_;
    request(0, 0, small.data() + 1, 3, LVIF_PARAM);
    check(small[1] == L'x' && virtualRequests_ == beforeNoText,
          "zero/null/sentinel buffers and requests without LVIF_TEXT are safe");
    const auto rejected = virtualRejected_;
    bool invalid = true;
    for (auto [row, column] : {std::pair{-1, 0}, {1000, 0}, {0, -1}, {0, 3}})
    {
        small[1] = L'x';
        request(row, column, small.data() + 1, 3);
        invalid &= small[1] == 0 && small[0] == L'#' && small[4] == L'#';
    }
    const int id = virtualOrder_[0];
    virtualOrder_[0] = VirtualRows;
    small[1] = L'x';
    request(0, 0, small.data() + 1, 3);
    invalid &= small[1] == 0;
    virtualOrder_[0] = id;
    check(invalid && virtualRejected_ == rejected + 5,
          "invalid rows/subitems/model IDs produce empty text without out-of-range access");
    ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(list, 123, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    const int selectedId = VirtualId(123);
    check(virtualSelected_ == selectedId, "actual native selection notification maps index to stable ID");
    const auto sort = [&](int column)
    {
        NMLISTVIEW click{};
        click.hdr = {list, 1401, LVN_COLUMNCLICK};
        click.iSubItem = column;
        SendMessageW(panel_, WM_NOTIFY, 1401, reinterpret_cast<LPARAM>(&click));
    };
    bool sorted = true;
    for (int column = 0; column < 3; ++column)
        for (int direction = 0; direction < 2; ++direction)
        {
            sort(column);
            for (int row = 1; row < VirtualRows; ++row)
            {
                const auto a = VirtualText(VirtualId(row - 1), column);
                const auto b = VirtualText(VirtualId(row), column);
                const int comparison = column == 2 ? std::stoi(a) - std::stoi(b) : a.compare(b);
                sorted &= virtualAscending_ ? comparison <= 0 : comparison >= 0;
            }
            HDITEMW heading{};
            heading.mask = HDI_FORMAT;
            Header_GetItem(header, column, &heading);
            sorted &= permutation() && virtualSelected_ == selectedId &&
                      VirtualId(ListView_GetNextItem(list, -1, LVNI_SELECTED)) == selectedId &&
                      bool(heading.fmt & (virtualAscending_ ? HDF_SORTUP : HDF_SORTDOWN));
        }
    check(sorted, "native column notifications sort all keys both ways and retain selected record/glyph");
    VirtualSelect(list, -1);
    sort(1);
    check(virtualSelected_ == -1 && ListView_GetSelectedCount(list) == 0,
          "sorting preserves the absence of native selection");
    bool endpoints = true;
    for (int button : {1405, 1406})
    {
        SendMessageW(panel_, WM_COMMAND, MAKEWPARAM(button, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(panel_, button)));
        const int row = button == 1405 ? 0 : VirtualRows - 1;
        endpoints &=
            ListView_GetNextItem(list, -1, LVNI_SELECTED) == row && virtualSelected_ == VirtualId(row);
    }
    check(endpoints, "first/last native buttons reach endpoints after sorting");
    const auto order = virtualOrder_;
    const int selected = virtualSelected_, column = virtualSortColumn_;
    const bool ascending = virtualAscending_;
    RefreshPresentation(144);
    SendMessageW(window_, WM_THEMECHANGED, 0, 0);
    RefreshPresentation(192);
    RefreshPresentation(initialDpi);
    check(GetDlgItem(panel_, 1401) == list && ListView_GetHeader(list) == header &&
              ListView_GetItemCount(list) == VirtualRows && virtualOrder_ == order &&
              virtualSelected_ == selected && virtualSortColumn_ == column && virtualAscending_ == ascending,
          "DPI/theme refresh retains native HWNDs, count, model order and selection identity");
    const auto gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    const auto user = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    bool repeated = true;
    for (int cycle = 0; cycle < 16; ++cycle)
    {
        Build(0);
        Build(14);
        list = GetDlgItem(panel_, 1401);
        repeated &= virtualOrder_ == order && virtualSelected_ == selected &&
                    VirtualId(ListView_GetNextItem(list, -1, LVNI_SELECTED)) == selected &&
                    ListView_GetItemCount(list) == VirtualRows && Children(list) == children;
    }
    check(repeated && GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS) <= gdi + 2 &&
              GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS) <= user + 2,
          "16 repeated topic visits preserve the model and bounded GDI/USER/window resources");
    if (options_.visible)
    {
        Capture(options_.report.parent_path() / L"VirtualOwnerData.bmp");
        scrollY_ = 250;
        LayoutChildren();
        Capture(options_.report.parent_path() / L"VirtualOwnerDataDetails.bmp");
        scrollY_ = 0;
        LayoutChildren();
    }
    check(GetFocus() == focus && fatal_.empty(),
          "callback tests do not change thread focus or report errors");
    virtualOrder_ = savedOrder;
    virtualSelected_ = savedSelected;
    virtualSortColumn_ = savedColumn;
    virtualAscending_ = savedAscending;
    Build(0);
    out << "LIMIT owner-data: 1,000 generated IDs; no filtering/editing/provider/cache/type-ahead demo. "
           "Physical keyboard/focus interaction is unverified; no input injection.\n";
}
