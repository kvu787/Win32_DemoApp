#include "App.h"
#include "TextSearch.h"
#include <algorithm>
#include <ostream>
#include <stdexcept>

namespace
{
std::wstring Text(HWND window)
{
    const int length = GetWindowTextLengthW(window);
    std::wstring result(size_t(length) + 1, L'\0');
    GetWindowTextW(window, result.data(), length + 1);
    result.resize(length);
    return result;
}
std::pair<DWORD, DWORD> Selection(HWND window)
{
    DWORD start = 0, end = 0;
    SendMessageW(window, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    return {start, end};
}
} // namespace
void App::RegressionAudit(std::ostream &out)
{
    const auto check = [&](bool okay, const char *name)
    {
        out << (okay ? "PASS regression: " : "FAIL regression: ") << name << '\n';
        if (!okay)
            throw std::runtime_error(name);
    };
    using TextSearch::Options;
    const std::wstring source = L"Demo demo democracy DEMO demo";
    auto match = TextSearch::Find(source, L"demo", 0, 0, Options{});
    check(match && match->start == 0, "case-insensitive first find");
    match = TextSearch::Find(source, L"demo", 0, 4, Options{});
    check(match && match->start == 5, "Find Next advances beyond the selected match");
    match = TextSearch::Find(source, L"demo", 25, 29, Options{});
    check(match && match->start == 0 && match->wrapped, "downward search wraps");
    match = TextSearch::Find(source, L"demo", 0, 4, Options{false, false, true, true});
    check(match && match->start == 25 && match->wrapped, "upward search wraps to the last whole word");
    match = TextSearch::Find(source, L"demo", 20, 24, Options{false, false, true, true});
    check(match && match->start == 5 && !match->wrapped, "upward search skips partial-word matches");
    match = TextSearch::Find(source, L"demo", 0, 0, Options{true, true, true, true});
    check(match && match->start == 5, "Match Case respects case");
    match = TextSearch::Find(source, L"demo", 5, 9, Options{true, false, true, true});
    check(match && match->start == 20, "Whole Word skips democracy");
    check(!TextSearch::Find(L"nothing", L"demo", 0, 0, Options{}), "missing find leaves no fabricated match");
    check(!TextSearch::Find(source, L"", 0, 0, Options{}), "empty find is rejected");
    std::wstring buffer = source;
    check(TextSearch::ReplaceAll(buffer, L"demo", L"X", Options{true, false, true, true}) == 4 &&
              buffer == L"X X democracy X X",
          "Replace All honors case/whole-word options");
    buffer = L"aaaa";
    check(TextSearch::ReplaceAll(buffer, L"aa", L"aaa", Options{}) == 2 && buffer == L"aaaaaa",
          "Replace All scans original nonoverlapping matches without replacement loops");
    buffer = L"demo demo";
    check(TextSearch::ReplaceAll(buffer, L"demo", L"", Options{}) == 2 && buffer == L" ",
          "empty replacement deletes matches");
    buffer = L"demo_demo demo";
    check(TextSearch::ReplaceAll(buffer, L"demo", L"X", Options{true, false, true, true}) == 1 &&
              buffer == L"demo_demo X",
          "underscore participates in word boundaries");

    Build(8);
    auto preview = GetDlgItem(panel_, 1011);
    check(preview != nullptr, "dialog topic contains its color/font preview");
    AcceptDemoColor(RGB(60, 90, 120));
    HDC sample = CreateCompatibleDC(nullptr);
    SendMessageW(panel_, WM_CTLCOLORSTATIC, reinterpret_cast<WPARAM>(sample),
                 reinterpret_cast<LPARAM>(preview));
    check(GetTextColor(sample) == RGB(60, 90, 120), "color acceptance reaches the local preview");
    DeleteDC(sample);
    LOGFONTW description{};
    GetObjectW(font_, sizeof(description), &description);
    description.lfWeight = FW_BOLD;
    AcceptDemoFont(description);
    check(reinterpret_cast<HFONT>(SendMessageW(preview, WM_GETFONT, 0, 0)) == demoFont_ &&
              GetObjectType(demoFont_) == OBJ_FONT,
          "font acceptance reaches a live local preview");
    Build(0);
    check(!UpdatePreview(), "missing preview returns before any redraw call");
    AcceptDemoColor(RGB(80, 110, 140));
    AcceptDemoFont(description);
    Build(8);
    check(UpdatePreview() && demoColor_ == RGB(80, 110, 140),
          "accepted preview settings survive topic navigation");

    auto edit = GetDlgItem(panel_, 1010);
    SetWindowTextW(edit, L"demo demo DEMO democracy");
    wchar_t needle[] = L"demo", replacement[] = L"X";
    FINDREPLACEW request{};
    request.lStructSize = sizeof(request);
    request.lpstrFindWhat = needle;
    request.lpstrReplaceWith = replacement;
    request.Flags = FR_FINDNEXT | FR_DOWN | FR_WHOLEWORD;
    SendMessageW(edit, EM_SETSEL, 0, 0);
    HandleFind(request);
    check(Selection(edit) == std::pair<DWORD, DWORD>{0, 4}, "native Find handler selects the first match");
    HandleFind(request);
    check(Selection(edit) == std::pair<DWORD, DWORD>{5, 9}, "native Find handler advances its selection");
    request.Flags = FR_REPLACE | FR_DOWN | FR_WHOLEWORD;
    HandleFind(request);
    check(Text(edit) == L"demo X DEMO democracy" && Selection(edit) == std::pair<DWORD, DWORD>{7, 11},
          "Replace changes the selected occurrence and advances");
    SendMessageW(edit, EM_SETSEL, 17, 19); // Selected text does not match the search term.
    const auto before = Text(edit);
    HandleFind(request);
    check(Text(edit) == before, "Replace never replaces an unrelated selection");
    request.Flags = FR_REPLACEALL | FR_DOWN | FR_MATCHCASE | FR_WHOLEWORD;
    HandleFind(request);
    check(Text(edit) == L"X X DEMO democracy", "native Replace All honors options");
    SetWindowTextW(edit, L"demo demo demo");
    SendMessageW(edit, EM_SETSEL, 5, 9);
    request.Flags = FR_REPLACE | FR_WHOLEWORD;
    HandleFind(request);
    check(Text(edit) == L"demo X demo" && Selection(edit) == std::pair<DWORD, DWORD>{0, 4},
          "upward Replace advances to the previous selected occurrence");

    const UINT original = dpi_;
    Build(2);
    auto typed = GetDlgItem(panel_, 201), password = GetDlgItem(panel_, 202),
         multiline = GetDlgItem(panel_, 203);
    SetWindowTextW(typed, L"keep this input");
    SetWindowTextW(password, L"secret sample");
    SetWindowTextW(multiline, L"one\r\ntwo\r\nthree");
    SendMessageW(typed, EM_SETSEL, 5, 9);
    scrollY_ = 90;
    LayoutChildren();
    const int savedScroll = scrollY_;
    RefreshPresentation(144);
    SendMessageW(window_, WM_THEMECHANGED, 0, 0);
    check(GetDlgItem(panel_, 201) == typed && Text(typed) == L"keep this input" &&
              Text(password) == L"secret sample" && Text(multiline) == L"one\r\ntwo\r\nthree",
          "DPI/theme refresh preserves HWND identity and editable buffers");
    check(Selection(typed) == std::pair<DWORD, DWORD>{5, 9} && scrollY_ == savedScroll,
          "DPI/theme refresh preserves text selection and logical scrolling");
    Build(1);
    auto checkbox = GetDlgItem(panel_, 113);
    SendMessageW(checkbox, BM_SETCHECK, BST_CHECKED, 0);
    RefreshPresentation(192);
    check(GetDlgItem(panel_, 113) == checkbox && SendMessageW(checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED,
          "DPI refresh preserves checkbox state and control identity");
    Build(5);
    auto track = GetDlgItem(panel_, 501);
    SendMessageW(track, TBM_SETPOS, TRUE, 81);
    RefreshPresentation(96);
    check(GetDlgItem(panel_, 501) == track && SendMessageW(track, TBM_GETPOS, 0, 0) == 81,
          "DPI refresh preserves slider values");
    Build(3);
    auto combo = GetDlgItem(panel_, 306);
    SendMessageW(combo, CB_SETCURSEL, 2, 0);
    RefreshPresentation(144);
    RefreshPresentation(192);
    RefreshPresentation(original);
    check(GetDlgItem(panel_, 306) == combo && SendMessageW(combo, CB_GETCURSEL, 0, 0) == 2 &&
              GetObjectType(font_) == OBJ_FONT,
          "composite-control font/DPI refresh preserves selection and body font lifetime");

    const auto initialFloating = floating_.size();
    FloatingDemo(901);
    FloatingDemo(1205);
    FloatingDemo(1206);
    check(floating_.size() == initialFloating + 3,
          "floating regression fixtures created without showing them");
    for (size_t i = initialFloating; i < floating_.size(); ++i)
        check(!IsWindowVisible(floating_[i]), "floating fixture remains hidden");
    MakeFonts();
    MakeFonts();
    struct FontCheck
    {
        bool valid = true;
        int controls = 0;
    } fontCheck;
    for (size_t i = initialFloating; i < floating_.size(); ++i)
        EnumChildWindows(
            floating_[i],
            [](HWND child, LPARAM parameter) -> BOOL
            {
                auto state = reinterpret_cast<FontCheck *>(parameter);
                auto font = reinterpret_cast<HFONT>(SendMessageW(child, WM_GETFONT, 0, 0));
                if (font)
                {
                    ++state->controls;
                    state->valid &= GetObjectType(font) == OBJ_FONT;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&fontCheck));
    check(fontCheck.valid && fontCheck.controls >= 5,
          "owned-window controls retain live fonts after repeated refresh");
    const auto fontsBefore = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    for (int cycle = 0; cycle < 16; ++cycle)
        MakeFonts();
    check(GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS) <= fontsBefore + 2,
          "repeated main/composite/owned font refresh retains stable GDI resources");
    while (floating_.size() > initialFloating)
    {
        DestroyWindow(floating_.back());
        floating_.pop_back();
    }

    split_ = 10000;
    Layout();
    check(SplitterHit(split_) && split_ < 10000,
          "right-clamped splitter remains hittable at its displayed position");
    split_ = -10000;
    Layout();
    check(split_ == 150 && SplitterHit(150), "left-clamped splitter remains hittable");
    RECT previous{};
    GetWindowRect(window_, &previous);
    split_ = 700;
    SetWindowPos(window_, nullptr, previous.left, previous.top, 720, 650, SWP_NOZORDER | SWP_NOACTIVATE);
    Layout();
    RECT treeRectangle{};
    GetWindowRect(tree_, &treeRectangle);
    check(SplitterHit(split_) && abs((treeRectangle.right - treeRectangle.left) - S(split_ - 16)) <= 1,
          "narrow resizing shares the displayed splitter position with hit testing");
    SetWindowPos(window_, nullptr, previous.left, previous.top, previous.right - previous.left,
                 previous.bottom - previous.top, SWP_NOZORDER | SWP_NOACTIVATE);
    split_ = 245;
    RefreshPresentation(original);
    Build(0);
    check(fatal_.empty(), "targeted regressions leave no callback errors");
    out << "PENDING regression: actual root cmd/ps1 launcher visibility on unlocked NE18NZ2; interactive "
           "dialog acceptance/cancellation.\n";
}
