#include "App.h"
#include "Gallery.h"
#include <algorithm>
#include <array>
#include <dwmapi.h>
#include <fstream>
#include <richedit.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <sstream>
#include <stdexcept>
#include <uxtheme.h>
#include <windowsx.h>

namespace
{
constexpr int NavFirst = 5000, NavBack = 6001, NavNext = 6002, NavHome = 6003, NavDrop = 6004;
constexpr int OpenDoc = 6100, Quit = 6101, About = 6102, CopyName = 6103;
std::wstring ClassName(HWND h)
{
    wchar_t text[256]{};
    GetClassNameW(h, text, 256);
    return text;
}
std::string Narrow(const std::wstring &s)
{
    const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
    std::string r(n, ' ');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), r.data(), n, nullptr, nullptr);
    return r;
}
void Pump()
{
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message != WM_QUIT)
            DispatchMessageW(&message);
    }
}
void Columns(HWND list, const std::vector<std::pair<const wchar_t *, int>> &columns)
{
    int i = 0;
    for (auto &c : columns)
    {
        LVCOLUMNW v{};
        v.mask = LVCF_TEXT | LVCF_WIDTH;
        v.pszText = const_cast<wchar_t *>(c.first);
        v.cx = c.second;
        ListView_InsertColumn(list, i++, &v);
    }
}
void Row(HWND list, int i, const std::vector<std::wstring> &values, LPARAM data = 0, int image = -1)
{
    LVITEMW item{};
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = i;
    item.pszText = const_cast<wchar_t *>(values[0].c_str());
    item.lParam = data;
    if (image >= 0)
    {
        item.mask |= LVIF_IMAGE;
        item.iImage = image;
    }
    ListView_InsertItem(list, &item);
    for (int j = 1; j < int(values.size()); ++j)
        ListView_SetItemText(list, i, j, const_cast<wchar_t *>(values[j].c_str()));
}
void FitOwned(HWND h, HWND owner)
{
    RECT r{}, o{};
    GetWindowRect(h, &r);
    GetWindowRect(owner, &o);
    MONITORINFO m{sizeof(m)};
    GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &m);
    const int w = r.right - r.left, hg = r.bottom - r.top;
    const int x = std::clamp(o.left + 40, m.rcWork.left, std::max(m.rcWork.left, m.rcWork.right - w));
    const int y = std::clamp(o.top + 40, m.rcWork.top, std::max(m.rcWork.top, m.rcWork.bottom - hg));
    SetWindowPos(h, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}
UINT_PTR CALLBACK CommonHook(HWND dialog, UINT message, WPARAM, LPARAM)
{
    if (message == WM_INITDIALOG)
        FitOwned(dialog, GetWindow(dialog, GW_OWNER));
    return 0;
}
INT_PTR CALLBACK PropertyProc(HWND dialog, UINT message, WPARAM w, LPARAM l)
{
    if (message == WM_INITDIALOG)
    {
        SendDlgItemMessageW(dialog, 301, TBM_SETRANGE, TRUE, MAKELPARAM(1, 10));
        SendDlgItemMessageW(dialog, 301, TBM_SETPOS, TRUE, 5);
        SendDlgItemMessageW(dialog, 304, TBM_SETRANGE, TRUE, MAKELPARAM(1, 5));
        EnableWindow(GetDlgItem(dialog, 304), FALSE);
        return TRUE;
    }
    if (message == WM_COMMAND && HIWORD(w) == BN_CLICKED)
    {
        if (LOWORD(w) == 303)
            EnableWindow(GetDlgItem(dialog, 304), IsDlgButtonChecked(dialog, 303) == BST_CHECKED);
        PropSheet_Changed(GetParent(dialog), dialog);
        return TRUE;
    }
    if (message == WM_HSCROLL)
    {
        PropSheet_Changed(GetParent(dialog), dialog);
        return TRUE;
    }
    if (message == WM_NOTIFY && reinterpret_cast<NMHDR *>(l)->code == PSN_APPLY)
    {
        SetWindowLongPtrW(dialog, DWLP_MSGRESULT, PSNRET_NOERROR);
        return TRUE;
    }
    return FALSE;
}
HRESULT CALLBACK TaskCallback(HWND h, UINT message, WPARAM, LPARAM, LONG_PTR owner)
{
    if (message == TDN_CREATED)
        FitOwned(h, reinterpret_cast<HWND>(owner));
    return S_OK;
}
INT_PTR CALLBACK OrdinaryDialog(HWND h, UINT m, WPARAM w, LPARAM)
{
    if (m == WM_INITDIALOG)
    {
        FitOwned(h, GetWindow(h, GW_OWNER));
        return TRUE;
    }
    if (m == WM_COMMAND && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL))
    {
        EndDialog(h, LOWORD(w));
        return TRUE;
    }
    return FALSE;
}
} // namespace

int App::S(int n) const
{
    return MulDiv(n, int(dpi_), 96);
}
App::App(HINSTANCE instance, Options options, Display display)
    : instance_(instance), options_(options), display_(display)
{
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_WIN95_CLASSES | ICC_DATE_CLASSES | ICC_USEREX_CLASSES |
                                                    ICC_COOL_CLASSES | ICC_INTERNET_CLASSES |
                                                    ICC_PAGESCROLLER_CLASS | ICC_LINK_CLASS};
    if (!InitCommonControlsEx(&common))
        throw std::runtime_error("InitCommonControlsEx failed.");
    rich_ = LoadLibraryW(L"Msftedit.dll");
    if (!rich_)
        throw std::runtime_error("Msftedit.dll unavailable.");
    auto reg = [&](const wchar_t *cls, WNDPROC proc, HBRUSH background)
    {
        WNDCLASSEXW c{sizeof(c)};
        c.hInstance = instance_;
        c.lpszClassName = cls;
        c.lpfnWndProc = proc;
        c.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        c.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        c.hIconSm = c.hIcon;
        c.hbrBackground = background;
        if (!RegisterClassExW(&c) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("RegisterClassEx failed.");
    };
    reg(L"Win32DemoMain", MainProc, GetSysColorBrush(COLOR_BTNFACE));
    reg(L"Win32DemoPanel", PanelProc, GetSysColorBrush(COLOR_BTNFACE));
    reg(L"Win32DemoSurface", SurfaceProc, GetSysColorBrush(COLOR_WINDOW));
    reg(L"Win32DemoFloat", FloatingProc, GetSysColorBrush(COLOR_BTNFACE));
    const auto &r = display_.work;
    window_ = CreateWindowExW(options_.test ? WS_EX_NOACTIVATE : 0, L"Win32DemoMain",
                              L"Win32_DemoApp - Native controls and terminology",
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, r.left + 16, r.top + 16,
                              std::min(1320L, r.right - r.left - 32), std::min(1000L, r.bottom - r.top - 32),
                              nullptr, nullptr, instance_, this);
    if (!window_)
        throw std::runtime_error("Main window creation failed.");
    dpi_ = GetDpiForWindow(window_);
    MakeFonts();
    auto menu = CreateMenu(), file = CreatePopupMenu(), view = CreatePopupMenu(), help = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, 1000, L"&Open path selection demo...\tCtrl+O");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, Quit, L"E&xit");
    for (int i = 0; i < int(Pages().size()); ++i)
        AppendMenuW(view, MF_STRING, NavFirst + i, Pages()[i].title);
    AppendMenuW(help, MF_STRING, OpenDoc, L"Microsoft &documentation\tF1");
    AppendMenuW(help, MF_STRING, About, L"&About / coverage");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"&File");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(view), L"&Topics");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(help), L"&Help");
    SetMenu(window_, menu);
    toolbar_ = CreateWindowExW(0, TOOLBARCLASSNAMEW, nullptr,
                               WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP, 0, 0, 0, 0,
                               window_, reinterpret_cast<HMENU>(20), instance_, nullptr);
    if (!toolbar_)
        throw std::runtime_error("Toolbar creation failed.");
    SendMessageW(toolbar_, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    TBADDBITMAP bitmaps{HINST_COMMCTRL, IDB_HIST_SMALL_COLOR};
    SendMessageW(toolbar_, TB_ADDBITMAP, 0, reinterpret_cast<LPARAM>(&bitmaps));
    SendMessageW(toolbar_, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
    const wchar_t strings[] = L"Back\0Forward\0Start\0Topics\0\0";
    const auto str = SendMessageW(toolbar_, TB_ADDSTRINGW, 0, reinterpret_cast<LPARAM>(strings));
    TBBUTTON b[5]{};
    b[0] = {HIST_BACK, NavBack, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {}, 0, str};
    b[1] = {HIST_FORWARD, NavNext, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {}, 0, str + 1};
    b[2] = {0, 0, TBSTATE_ENABLED, BTNS_SEP, {}, 0, 0};
    b[3] = {I_IMAGENONE, NavHome, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {}, 0, str + 2};
    b[4] = {I_IMAGENONE, NavDrop, TBSTATE_ENABLED, BTNS_DROPDOWN | BTNS_SHOWTEXT, {}, 0, str + 3};
    SendMessageW(toolbar_, TB_ADDBUTTONS, 5, reinterpret_cast<LPARAM>(b));
    SendMessageW(toolbar_, TB_AUTOSIZE, 0, 0);
    tree_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"Topics",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT |
                                TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
                            0, 0, 0, 0, window_, reinterpret_cast<HMENU>(21), instance_, nullptr);
    heading_ = CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, window_, nullptr,
                               instance_, nullptr);
    info_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL |
                                WS_VSCROLL,
                            0, 0, 0, 0, window_, nullptr, instance_, nullptr);
    doc_ =
        CreateWindowExW(0, L"Button", L"Microsoft &documentation (F1)", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0,
                        0, 0, 0, window_, reinterpret_cast<HMENU>(INT_PTR(OpenDoc)), instance_, nullptr);
    panel_ = CreateWindowExW(WS_EX_CONTROLPARENT, L"Win32DemoPanel", L"Live native examples",
                             WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL, 0, 0, 0, 0,
                             window_, nullptr, instance_, this);
    log_ = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"Edit", L"Event log: interact with an example to see its notification.",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 0, 0,
        0, 0, window_, nullptr, instance_, nullptr);
    status_ = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0,
                              window_, reinterpret_cast<HMENU>(22), instance_, nullptr);
    if (!tree_ || !heading_ || !info_ || !doc_ || !panel_ || !log_ || !status_)
        throw std::runtime_error("Gallery host creation failed.");
    images_ = ImageList_Create(S(16), S(16), ILC_COLOR32 | ILC_MASK, 4, 1);
    if (!images_)
        throw std::runtime_error("ImageList_Create failed.");
    for (auto icon : {IDI_APPLICATION, IDI_INFORMATION, IDI_WARNING, IDI_QUESTION})
        ImageList_AddIcon(images_, LoadIconW(nullptr, icon));
    TreeView_SetImageList(tree_, images_, TVSIL_NORMAL);
    for (int i = 0; i < int(Pages().size()); ++i)
    {
        TVINSERTSTRUCTW t{};
        t.hParent = TVI_ROOT;
        t.hInsertAfter = TVI_LAST;
        t.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        t.item.pszText = const_cast<wchar_t *>(Pages()[i].title);
        t.item.lParam = i;
        t.item.iImage = i == 0 ? 1 : 0;
        t.item.iSelectedImage = t.item.iImage;
        topics_.push_back(TreeView_InsertItem(tree_, &t));
    }
    tooltip_ =
        CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, 0,
                        0, 0, 0, window_, nullptr, instance_, nullptr);
    findMessage_ = RegisterWindowMessageW(FINDMSGSTRINGW);
    MakeFonts();
    Build(0);
    Layout();
    if (!fatal_.empty())
        throw std::runtime_error(Narrow(fatal_));
}
App::~App()
{
    if (find_ && IsWindow(find_))
        DestroyWindow(find_);
    for (auto f : floating_)
        if (IsWindow(f))
            DestroyWindow(f);
    if (window_ && IsWindow(window_))
        DestroyWindow(window_);
    for (auto font : specimenFonts_)
        if (GetObjectType(font.font) == OBJ_FONT)
            DeleteObject(font.font);
    if (images_)
        ImageList_Destroy(images_);
    if (font_)
        DeleteObject(font_);
    if (bold_)
        DeleteObject(bold_);
    if (demoFont_)
        DeleteObject(demoFont_);
    if (rich_)
        FreeLibrary(rich_);
}
void App::MakeFonts()
{
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    if (!SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi_))
        throw std::runtime_error("System UI font metrics unavailable.");
    auto normal = CreateFontIndirectW(&metrics.lfMessageFont);
    metrics.lfMessageFont.lfWeight = FW_BOLD;
    auto bold = CreateFontIndirectW(&metrics.lfMessageFont);
    if (!normal || !bold)
        throw std::runtime_error("Font allocation failed.");
    auto old = font_, oldBold = bold_;
    font_ = normal;
    bold_ = bold;
    EnumChildWindows(
        window_,
        [](HWND h, LPARAM context) -> BOOL
        {
            auto self = reinterpret_cast<App *>(context);
            for (const auto &isolated : self->specimenFonts_)
                if (h == isolated.hwnd || IsChild(isolated.hwnd, h))
                    return TRUE;
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(self->font_), TRUE);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(this));
    if (heading_)
        SendMessageW(heading_, WM_SETFONT, reinterpret_cast<WPARAM>(bold_), TRUE);
    if (old)
        DeleteObject(old);
    if (oldBold)
        DeleteObject(oldBold);
}
HWND App::Add(const wchar_t *cls, const wchar_t *text, DWORD style, int id, int x, int y, int w, int h,
              DWORD ex, HWND parent)
{
    const bool validFontBefore = GetObjectType(font_) == OBJ_FONT;
    HWND target = parent ? parent : panel_;
    auto control = CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, S(w), S(h), target,
                                   reinterpret_cast<HMENU>(INT_PTR(id)), instance_, nullptr);
    if (!control)
        throw std::runtime_error("CreateWindowEx failed for " + Narrow(cls) + " error " +
                                 std::to_string(GetLastError()));
    if (validFontBefore && GetObjectType(font_) != OBJ_FONT)
        throw std::runtime_error("Font invalidated by CreateWindowEx before WM_SETFONT for " + Narrow(cls));
    ++creationCount_;
    HFONT specimenFont = font_;
    // Give specialized composite controls independent fonts to isolate internal lifetimes.
    if (wcscmp(cls, WC_COMBOBOXEXW) == 0 || wcscmp(cls, WC_IPADDRESSW) == 0 ||
        wcscmp(cls, HOTKEY_CLASSW) == 0)
    {
        LOGFONTW description{};
        GetObjectW(font_, sizeof(description), &description);
        specimenFont = CreateFontIndirectW(&description);
        if (!specimenFont)
            throw std::runtime_error("Specimen font allocation failed.");
        specimenFonts_.push_back({control, specimenFont});
    }
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(specimenFont), TRUE);
    if (validFontBefore && GetObjectType(font_) != OBJ_FONT)
        throw std::runtime_error("Font invalidated during creation of " + Narrow(cls));
    if (!parent)
        children_.push_back({control, x, y, w, h});
    return control;
}
void App::Label(const wchar_t *text, int x, int y, int w, int h)
{
    Add(L"Static", text, SS_LEFT, 0, x, y, w, h);
}
void App::Button(const wchar_t *text, int id, int x, int y, int w, DWORD style)
{
    Add(L"Button", text, WS_TABSTOP | style, id, x, y, w, 30);
}
void App::Event(const std::wstring &event)
{
    lastEvent_ = event;
    const auto text = event + L"\r\n";
    SendMessageW(log_, EM_SETSEL, static_cast<WPARAM>(-1), -1);
    SendMessageW(log_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    if (GetWindowTextLengthW(log_) > 10000)
        SetWindowTextW(log_, event.c_str());
    SendMessageW(status_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(event.c_str()));
}
void App::PopulateReport(HWND list, bool sensors)
{
    ListView_DeleteAllItems(list);
    if (sensors)
    {
        Row(list, 0, {sensorExpanded_ ? L"[-] Demo CPU" : L"[+] Demo CPU", L"", L"", L""}, 100, 0);
        if (sensorExpanded_)
        {
            Row(list, 1, {L"    Core temperature", L"47 C", L"41 C", L"62 C"}, 1, 1);
            Row(list, 2, {L"    Core clock", L"3,800 MHz", L"800 MHz", L"4,600 MHz"}, 2, 1);
            Row(list, 3, {L"    Package power", L"28 W", L"12 W", L"65 W"}, 3, 1);
        }
        Row(list, sensorExpanded_ ? 4 : 1, {L"Demo GPU", L"51 C", L"45 C", L"67 C"}, 4, 0);
    }
    else
    {
        Row(list, 0, {L"Button", L"USER32", L"Push / check / radio"}, 0, 0);
        Row(list, 1, {L"Tree view", L"COMCTL32", L"Hierarchy"}, 1, 1);
        Row(list, 2, {L"List view", L"COMCTL32", L"Rows and columns"}, 2, 2);
    }
}
void App::Build(int page)
{
    navigating_ = true;
    // Remove tooltip registrations before destroying their target HWNDs.
    if (tooltip_)
    {
        TOOLINFOW tool{};
        tool.cbSize = sizeof(tool);
        while (SendMessageW(tooltip_, TTM_ENUMTOOLSW, 0, reinterpret_cast<LPARAM>(&tool)))
        {
            SendMessageW(tooltip_, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
            tool.cbSize = sizeof(tool);
        }
    }
    for (auto &c : children_)
    {
        const auto cls = ClassName(c.hwnd);
        const bool fontValid = GetObjectType(font_) == OBJ_FONT;
        DestroyWindow(c.hwnd);
        if (fontValid && GetObjectType(font_) != OBJ_FONT)
            throw std::runtime_error("Body font invalidated by destruction of " + Narrow(cls));
    }
    children_.clear();
    for (auto font : specimenFonts_)
        if (GetObjectType(font.font) == OBJ_FONT)
            DeleteObject(font.font);
    specimenFonts_.clear();
    int imageWidth = 0, imageHeight = 0;
    ImageList_GetIconSize(images_, &imageWidth, &imageHeight);
    if (imageWidth != S(16) || imageHeight != S(16))
    {
        auto next = ImageList_Create(S(16), S(16), ILC_COLOR32 | ILC_MASK, 4, 1);
        if (!next)
            throw std::runtime_error("DPI image-list allocation failed.");
        for (auto icon : {IDI_APPLICATION, IDI_INFORMATION, IDI_WARNING, IDI_QUESTION})
            ImageList_AddIcon(next, LoadIconW(nullptr, icon));
        auto previous = images_;
        images_ = next;
        TreeView_SetImageList(tree_, images_, TVSIL_NORMAL);
        ImageList_Destroy(previous);
    }
    page_ = std::clamp(page, 0, int(Pages().size()) - 1);
    scrollX_ = scrollY_ = 0;
    height_ = 640;
    const auto &p = Pages()[page_];
    SetWindowTextW(heading_, p.title);
    std::wstring details = std::wstring(p.family) + L"\r\n" + p.identifiers + L"\r\n\r\n" + p.explanation;
    SetWindowTextW(info_, details.c_str());
    TreeView_SelectItem(tree_, topics_[page_]);
    CheckMenuRadioItem(GetSubMenu(GetMenu(window_), 1), NavFirst, NavFirst + int(Pages().size()) - 1,
                       NavFirst + page_, MF_BYCOMMAND);
    navigating_ = false;
    switch (page_)
    {
    case 0:
    {
        Label(L"Coverage inventory - 'live' means this build creates and exercises the control", 16, 12);
        auto list = Add(WC_LISTVIEWW, L"Coverage inventory",
                        WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHAREIMAGELISTS, 100, 16, 45, 850, 760,
                        WS_EX_CLIENTEDGE);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        Columns(list, {{L"Concept", S(300)}, {L"Class / API", S(285)}, {L"Coverage", S(260)}});
        int i = 0;
        for (const auto &e : Inventory())
            Row(list, i++, {e.name, e.cls, e.coverage});
        height_ = 830;
        break;
    }
    case 1:
    {
        Label(L"Button styles: ordinary / default / disabled / checkbox / mixed / radio / group", 16, 12);
        Button(L"&Push button", 110, 16, 48);
        Button(L"Default push button", 111, 246, 48, 210, BS_DEFPUSHBUTTON);
        Button(L"Disabled button", 112, 476, 48);
        EnableWindow(GetDlgItem(panel_, 112), FALSE);
        Button(L"&Independent checkbox", 113, 16, 94, 260, BS_AUTOCHECKBOX);
        Button(L"Three-state checkbox", 114, 320, 94, 260, BS_AUTO3STATE);
        SendDlgItemMessageW(panel_, 114, BM_SETCHECK, BST_INDETERMINATE, 0);
        Add(L"Button", L"Radio group / BS_GROUPBOX", BS_GROUPBOX, 0, 16, 144, 620, 98);
        Add(L"Button", L"Choice &A", WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON, 115, 35, 174, 180, 28);
        Add(L"Button", L"Choice &B", WS_TABSTOP | BS_AUTORADIOBUTTON, 116, 240, 174, 180, 28);
        SendDlgItemMessageW(panel_, 115, BM_SETCHECK, BST_CHECKED, 0);
        Button(L"Split button", 117, 16, 270, 220, BS_SPLITBUTTON);
        auto command = Add(L"Button", L"Command link", WS_TABSTOP | BS_COMMANDLINK, 118, 16, 326, 620, 82);
        SendMessageW(command, BCM_SETNOTE, 0,
                     reinterpret_cast<LPARAM>(L"Secondary text explains a higher-level action."));
        Button(L"Button with stock icon", 119, 16, 436, 260);
        SendDlgItemMessageW(panel_, 119, BM_SETIMAGE, IMAGE_ICON,
                            reinterpret_cast<LPARAM>(LoadIconW(nullptr, IDI_INFORMATION)));
        Label(L"Try Space on checkboxes; arrow keys move within the radio group.", 16, 500);
        break;
    }
    case 2:
    {
        Label(L"Static label and stock system icon / etched separator", 16, 12);
        auto icon = Add(L"Static", L"", SS_ICON, 0, 16, 45, 40, 40);
        SendMessageW(icon, STM_SETICON, reinterpret_cast<WPARAM>(LoadIconW(nullptr, IDI_INFORMATION)), 0);
        Add(L"Static", L"", SS_ETCHEDHORZ, 0, 74, 62, 780, 2);
        Label(L"&Single-line Edit with cue banner", 16, 100);
        auto edit = Add(L"Edit", L"", WS_TABSTOP | ES_AUTOHSCROLL, 201, 16, 127, 400, 28, WS_EX_CLIENTEDGE);
        SendMessageW(edit, EM_SETCUEBANNER, FALSE,
                     reinterpret_cast<LPARAM>(L"Type here - EN_CHANGE is logged"));
        Label(L"&Password (presentation only)", 450, 100, 350);
        Add(L"Edit", L"demo", WS_TABSTOP | ES_PASSWORD | ES_AUTOHSCROLL, 202, 450, 127, 350, 28,
            WS_EX_CLIENTEDGE);
        Label(L"Multiline Edit with scrollbars", 16, 177);
        Add(L"Edit",
            L"Native multiline editor.\r\nSelect text, right-click, or try Ctrl+A / Ctrl+C.\r\nThis text "
            L"remains in memory only.",
            WS_TABSTOP | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | WS_VSCROLL | WS_HSCROLL, 203, 16,
            205, 800, 105, WS_EX_CLIENTEDGE);
        Label(L"Read-only Edit / ES_READONLY (still selectable)", 16, 337);
        Add(L"Edit", L"Read-only information; disabled and read-only are different states.",
            WS_TABSTOP | ES_READONLY | ES_AUTOHSCROLL, 204, 16, 365, 800, 28, WS_EX_CLIENTEDGE);
        Label(L"Rich Edit / RICHEDIT50W / Msftedit.dll", 16, 420);
        auto rich = Add(MSFTEDIT_CLASS,
                        L"Rich Edit supports formatting, selection and native editing.\r\nThis first "
                        L"sentence has a bold, colored character format.",
                        WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 205, 16, 452, 800, 120,
                        WS_EX_CLIENTEDGE);
        SendMessageW(rich, EM_SETSEL, 0, 9);
        CHARFORMAT2W format{};
        format.cbSize = sizeof(format);
        format.dwMask = CFM_BOLD;
        format.dwEffects = CFE_BOLD;
        SendMessageW(rich, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
        SendMessageW(rich, EM_SETSEL, 0, 0);
        break;
    }
    case 3:
    {
        Label(L"Single selection ListBox", 16, 12, 300);
        auto lb = Add(L"ListBox", L"", WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL, 301, 16, 45, 275, 135,
                      WS_EX_CLIENTEDGE);
        auto multi = Add(L"ListBox", L"", WS_TABSTOP | LBS_NOTIFY | LBS_EXTENDEDSEL | WS_VSCROLL, 302, 330,
                         45, 275, 135, WS_EX_CLIENTEDGE);
        Label(L"Extended selection (Ctrl / Shift)", 330, 12, 300);
        for (auto s : {L"First item", L"Second item", L"Third item", L"Fourth item"})
        {
            SendMessageW(lb, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
            SendMessageW(multi, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
        }
        int x = 16;
        for (auto style : {CBS_DROPDOWNLIST, CBS_DROPDOWN, CBS_SIMPLE})
        {
            Label(style == CBS_SIMPLE     ? L"CBS_SIMPLE"
                  : style == CBS_DROPDOWN ? L"CBS_DROPDOWN"
                                          : L"CBS_DROPDOWNLIST",
                  x, 215, 260);
            auto combo =
                Add(L"ComboBox", L"", WS_TABSTOP | DWORD(style) | WS_VSCROLL, 303 + x, x, 245, 260, 130);
            for (auto s : {L"Native choice A", L"Native choice B", L"Native choice C"})
                SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
            SendMessageW(combo, CB_SETCURSEL, 0, 0);
            x += 280;
        }
        Label(L"ComboBoxEx with icon image list", 16, 415);
        auto cx = Add(WC_COMBOBOXEXW, L"", WS_TABSTOP | CBS_DROPDOWNLIST, 306, 16, 446, 340, 120);
        SendMessageW(cx, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(images_));
        for (int i = 0; i < 3; ++i)
        {
            COMBOBOXEXITEMW item{};
            item.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
            item.iItem = i;
            item.pszText = const_cast<wchar_t *>(i ? L"Icon choice" : L"Application");
            item.iImage = item.iSelectedImage = i;
            SendMessageW(cx, CBEM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
        }
        SendMessageW(cx, CB_SETCURSEL, 0, 0);
        Label(L"Hot key (press a key combination)", 16, 510, 400);
        Add(HOTKEY_CLASSW, L"", WS_TABSTOP, 307, 16, 542, 340, 30, WS_EX_CLIENTEDGE);
        Label(L"IPv4 address / segmented input", 430, 510, 380);
        auto ip = Add(WC_IPADDRESSW, L"", WS_TABSTOP, 308, 430, 542, 340, 30);
        SendMessageW(ip, IPM_SETADDRESS, 0, MAKEIPADDRESS(192, 168, 1, 42));
        break;
    }
    case 4:
    {
        auto tree = Add(WC_TREEVIEWW, L"",
                        WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_CHECKBOXES, 401,
                        16, 46, 260, 350, WS_EX_CLIENTEDGE);
        Label(L"Tree view / hierarchy", 16, 12, 280);
        TreeView_SetImageList(tree, images_, TVSIL_NORMAL);
        TVINSERTSTRUCTW t{};
        t.hParent = TVI_ROOT;
        t.hInsertAfter = TVI_LAST;
        t.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        t.item.pszText = const_cast<wchar_t *>(L"Control families");
        t.item.iImage = t.item.iSelectedImage = 0;
        auto root = TreeView_InsertItem(tree, &t);
        for (auto s : {L"USER32", L"Common controls", L"Dialogs"})
        {
            t.hParent = root;
            t.item.pszText = const_cast<wchar_t *>(s);
            t.item.iImage = t.item.iSelectedImage = 1;
            TreeView_InsertItem(tree, &t);
        }
        TreeView_Expand(tree, root, TVE_EXPAND);
        Label(L"List view: click header to sort", 304, 12, 520);
        auto list = Add(WC_LISTVIEWW, L"", WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHAREIMAGELISTS, 402,
                        304, 46, 550, 280, WS_EX_CLIENTEDGE);
        ListView_SetExtendedListViewStyle(list,
                                          LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
        ListView_SetImageList(list, images_, LVSIL_SMALL);
        ListView_SetImageList(list, images_, LVSIL_NORMAL);
        Columns(list, {{L"Name", S(180)}, {L"Library", S(140)}, {L"Purpose", S(200)}});
        PopulateReport(list, false);
        Button(L"Report", 403, 304, 350, 165);
        Button(L"List", 404, 490, 350, 165);
        Button(L"Icons", 405, 676, 350, 165);
        Label(L"Standalone header (drag its dividers)", 16, 435);
        auto header = Add(WC_HEADERW, L"", HDS_BUTTONS | HDS_FULLDRAG, 406, 16, 470, 838, 32);
        for (int i = 0; i < 3; ++i)
        {
            HDITEMW item{};
            item.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
            item.cxy = S(275);
            item.fmt = HDF_LEFT | HDF_STRING;
            item.pszText = const_cast<wchar_t *>(i == 0 ? L"Column A" : i == 1 ? L"Column B" : L"Column C");
            Header_InsertItem(header, i, &item);
        }
        break;
    }
    case 5:
    {
        Label(L"Trackbar with ticks and selection range (arrow keys, Page Up/Down)", 16, 12);
        auto track =
            Add(TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_AUTOTICKS | TBS_ENABLESELRANGE, 501, 16, 48, 650, 55);
        SendMessageW(track, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessageW(track, TBM_SETTICFREQ, 10, 0);
        SendMessageW(track, TBM_SETPOS, TRUE, 40);
        SendMessageW(track, TBM_SETSEL, TRUE, MAKELPARAM(20, 60));
        Label(L"0 / minimum", 16, 105, 220);
        Label(L"100 / maximum", 540, 105, 220);
        Label(L"Up-down with buddy Edit", 16, 154);
        auto edit =
            Add(L"Edit", L"42", WS_TABSTOP | ES_NUMBER | ES_RIGHT, 502, 16, 185, 180, 30, WS_EX_CLIENTEDGE);
        auto spin =
            Add(UPDOWN_CLASSW, L"", UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS, 503, 196, 185, 28, 30);
        SendMessageW(spin, UDM_SETBUDDY, reinterpret_cast<WPARAM>(edit), 0);
        SendMessageW(spin, UDM_SETRANGE32, 0, 100);
        SendMessageW(spin, UDM_SETPOS32, 0, 42);
        Label(L"Determinate progress / normal, paused, error", 16, 245);
        auto progress = Add(PROGRESS_CLASSW, L"", 0, 504, 16, 275, 800, 28);
        SendMessageW(progress, PBM_SETPOS, 40, 0);
        Button(L"Normal", 505, 16, 318, 180);
        Button(L"Paused", 506, 216, 318, 180);
        Button(L"Error", 507, 416, 318, 180);
        Label(L"Indeterminate marquee", 16, 375);
        auto marquee = Add(PROGRESS_CLASSW, L"", PBS_MARQUEE, 508, 16, 405, 800, 28);
        SendMessageW(marquee, PBM_SETMARQUEE, TRUE, 45);
        Label(L"ScrollBar child windows (separate from a window's own scrollbars)", 16, 460);
        auto sb = Add(L"ScrollBar", L"", SBS_HORZ | WS_TABSTOP, 509, 16, 496, 680, 24);
        auto sv = Add(L"ScrollBar", L"", SBS_VERT | WS_TABSTOP, 510, 744, 472, 24, 140);
        SCROLLINFO si{sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS, 0, 100, 20, 25, 0};
        SetScrollInfo(sb, SB_CTL, &si, TRUE);
        SetScrollInfo(sv, SB_CTL, &si, TRUE);
        break;
    }
    case 6:
    {
        Label(L"Date picker with optional value / time picker", 16, 12);
        Add(DATETIMEPICK_CLASSW, L"", WS_TABSTOP | DTS_SHOWNONE | DTS_SHORTDATEFORMAT, 601, 16, 50, 360, 30);
        Add(DATETIMEPICK_CLASSW, L"", WS_TABSTOP | DTS_TIMEFORMAT, 602, 425, 50, 300, 30);
        Label(L"Month calendar / multiple-day selection", 16, 125);
        auto calendar =
            Add(MONTHCAL_CLASSW, L"", WS_TABSTOP | MCS_MULTISELECT | MCS_WEEKNUMBERS, 603, 16, 162, 380, 245);
        RECT minimum{};
        MonthCal_GetMinReqRect(calendar, &minimum);
        children_.back().w = std::max(380, MulDiv(minimum.right, 96, dpi_));
        children_.back().h = std::max(245, MulDiv(minimum.bottom, 96, dpi_));
        Label(L"Animation / silent original AVI", 450, 125, 360);
        auto animation =
            Add(ANIMATE_CLASSW, L"", ACS_CENTER | ACS_AUTOPLAY | ACS_TRANSPARENT, 604, 450, 162, 300, 210);
        if (!Animate_OpenEx(animation, instance_, MAKEINTRESOURCEW(201)))
            throw std::runtime_error("Embedded AVI did not open.");
        Label(L"This legacy AVI control is not a video player.", 450, 390, 380, 50);
        break;
    }
    case 7:
    {
        Label(L"Tab control manages selection; this application changes the page text", 16, 12);
        auto tab = Add(WC_TABCONTROLW, L"", WS_TABSTOP, 701, 16, 48, 800, 110);
        for (int i = 0; i < 3; ++i)
        {
            TCITEMW item{};
            item.mask = TCIF_TEXT;
            item.pszText = const_cast<wchar_t *>(i == 0 ? L"General" : i == 1 ? L"Details" : L"Advanced");
            TabCtrl_InsertItem(tab, i, &item);
        }
        Add(L"Static", L"General page selected", 0, 702, 40, 92, 720, 30);
        Label(L"Rebar with two native toolbar bands (drag the grippers)", 16, 192);
        auto rebar = Add(REBARCLASSNAMEW, L"",
                         WS_CLIPCHILDREN | RBS_VARHEIGHT | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
                         703, 16, 228, 800, 86);
        for (int i = 0; i < 2; ++i)
        {
            auto tb = Add(TOOLBARCLASSNAMEW, L"",
                          TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER,
                          710 + i, 0, 0, 350, 35, 0, rebar);
            SendMessageW(tb, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            const wchar_t strings[] = L"Action A\0Action B\0\0";
            auto base = SendMessageW(tb, TB_ADDSTRINGW, 0, reinterpret_cast<LPARAM>(strings));
            TBBUTTON buttons[3]{
                {I_IMAGENONE, 720 + i * 10, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {}, 0, base},
                {0, 0, TBSTATE_ENABLED, BTNS_SEP, {}, 0, 0},
                {I_IMAGENONE, 721 + i * 10, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {}, 0, base + 1}};
            SendMessageW(tb, TB_ADDBUTTONS, 3, reinterpret_cast<LPARAM>(buttons));
            REBARBANDINFOW band{};
            band.cbSize = sizeof(band);
            band.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
            band.fStyle = RBBS_GRIPPERALWAYS;
            band.hwndChild = tb;
            band.cxMinChild = S(290);
            band.cyMinChild = S(35);
            band.cx = S(370);
            SendMessageW(rebar, RB_INSERTBANDW, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&band));
        }
        Label(L"Pager: narrow viewport over a wider toolbar", 16, 352);
        auto pager = Add(WC_PAGESCROLLERW, L"", PGS_HORZ, 704, 16, 387, 420, 42);
        auto wide = Add(TOOLBARCLASSNAMEW, L"", TBSTYLE_FLAT | CCS_NORESIZE | CCS_NOPARENTALIGN, 705, 0, 0,
                        720, 36, 0, pager);
        SendMessageW(wide, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        const wchar_t labels[] = L"One\0Two\0Three\0Four\0Five\0Six\0\0";
        auto base = SendMessageW(wide, TB_ADDSTRINGW, 0, reinterpret_cast<LPARAM>(labels));
        for (int i = 0; i < 6; ++i)
        {
            TBBUTTON b{I_IMAGENONE, 740 + i, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {}, 0, base + i};
            SendMessageW(wide, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&b));
        }
        SendMessageW(pager, PGM_SETCHILD, 0, reinterpret_cast<LPARAM>(wide));
        auto link = Add(
            WC_LINK,
            L"<a href=\"https://learn.microsoft.com/en-us/windows/win32/controls/syslink-overview\">SysLink: "
            L"official Microsoft reference</a>",
            WS_TABSTOP, 706, 16, 475, 800, 32);
        TOOLINFOW tool{};
        tool.cbSize = sizeof(tool);
        tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        tool.hwnd = panel_;
        tool.uId = reinterpret_cast<UINT_PTR>(link);
        tool.lpszText = const_cast<wchar_t *>(L"A native tooltip / tooltips_class32");
        SendMessageW(tooltip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        Label(L"Hover the hyperlink for a tooltip; Tab then Enter follows it.", 16, 530);
        break;
    }
    case 8:
    {
        int y = 18;
        for (auto entry : std::vector<std::pair<int, const wchar_t *>>{{1000, L"File &Open (path only)"},
                                                                       {1001, L"File &Save (path only)"},
                                                                       {1002, L"&Folder picker"},
                                                                       {1003, L"&Color picker"},
                                                                       {1004, L"&Font picker"},
                                                                       {1005, L"&Find text (modeless)"},
                                                                       {1006, L"&Replace text (modeless)"},
                                                                       {1007, L"&Message box"},
                                                                       {1008, L"&Task dialog"},
                                                                       {1009, L"Property &sheet"}})
        {
            Button(entry.second, entry.first, 16, y, 340);
            Label(entry.first <= 1002   ? L"Selecting or cancelling never opens or writes a file."
                  : entry.first <= 1006 ? L"Edits affect temporary demo UI/text only."
                                        : L"Native OS container with safe demo content.",
                  386, y, 450, 32);
            y += 48;
        }
        Add(L"Edit", L"Find and replace demo text. Search for demo. This demo stays in memory.",
            WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL, 1010, 16, 525, 800, 75, WS_EX_CLIENTEDGE);
        break;
    }
    case 9:
    {
        Label(L"Right-click this page (or Shift+F10) for a native context menu.", 16, 12);
        Button(L"&Open owned tool window", 901, 16, 64, 320);
        Button(L"Open owned normal &window", 902, 370, 64, 340);
        Add(L"Button", L"Enabled / checked menu example", WS_TABSTOP | BS_AUTOCHECKBOX, 903, 16, 145, 600,
            30);
        SendDlgItemMessageW(panel_, 903, BM_SETCHECK, BST_CHECKED, 0);
        Label(L"Client area: the app's controls and content. Nonclient area: caption, system menu, "
              L"borders.\r\n\r\nThe status bar's lower-right sizing grip resizes the main "
              L"window.\r\nAlt+Space opens the system menu. Alt+F opens File. Ctrl+O selects a demo "
              L"path.\r\n\r\nWindows, owned windows and child controls are different HWND relationships.",
              16, 220, 820, 210);
        break;
    }
    case 10:
    {
        auto icon = Add(L"Static", L"", SS_ICON, 0, 16, 14, 40, 40);
        SendMessageW(icon, STM_SETICON, reinterpret_cast<WPARAM>(LoadIconW(nullptr, IDI_INFORMATION)), 0);
        Label(L"Display capabilities - original demo banner", 68, 16, 740);
        Add(WC_LINK,
            L"<a "
            L"href=\"https://learn.microsoft.com/en-us/windows/win32/gdi/multiple-display-monitors\">Learn "
            L"about multiple-display monitor APIs</a>",
            WS_TABSTOP, 100, 68, 47, 760, 30);
        Button(L"&Enable demo mode", 1051, 16, 105, 600, BS_AUTOCHECKBOX);
        SendDlgItemMessageW(panel_, 1051, BM_SETCHECK, BST_CHECKED, 0);
        Add(L"Button", L"Windowed mode", WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON, 1052, 42, 148, 320, 30);
        Add(L"Button", L"Fullscreen mode", WS_TABSTOP | BS_AUTORADIOBUTTON, 1053, 390, 148, 320, 30);
        SendDlgItemMessageW(panel_, 1052, BM_SETCHECK, BST_CHECKED, 0);
        Label(L"Select a dummy monitor tile (mouse or Left/Right then Space)", 16, 213);
        auto surface = Add(L"Win32DemoSurface", L"Monitor tiles (custom)", WS_TABSTOP, 1054, 16, 252, 800,
                           215, WS_EX_CLIENTEDGE);
        SetWindowLongPtrW(surface, GWLP_USERDATA, 1);
        Label(L"Information: these original tiles are custom drawing, not a built-in Win32 control.\r\nThe "
              L"main divider is application-defined; native tree controls remain genuine.",
              16, 505, 800, 80);
        break;
    }
    case 11:
    {
        Button(L"&Open native demo properties", 1009, 16, 28, 390);
        Label(L"Property-sheet vocabulary\r\n\r\nProperty sheet = top-level dialog container.\r\nProperty "
              L"page = a child dialog template.\r\nTab = native page selector.\r\nGroup box = Button with "
              L"BS_GROUPBOX.\r\nTrackbar = slider with ticks and optional endpoint labels.\r\nApply = commit "
              L"temporary demo settings without closing.\r\nCancel = discard temporary dialog "
              L"state.\r\n\r\nNo system mouse settings change.",
              16, 95, 820, 360);
        break;
    }
    case 12:
    {
        Label(L"Demo Sensor Status - native report list with app-managed group expansion", 16, 12);
        auto list =
            Add(WC_LISTVIEWW, L"Sensors", WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHAREIMAGELISTS, 1201,
                16, 48, 835, 240, WS_EX_CLIENTEDGE);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetImageList(list, images_, LVSIL_SMALL);
        Columns(list,
                {{L"Sensor", S(365)}, {L"Current", S(150)}, {L"Minimum", S(150)}, {L"Maximum", S(150)}});
        PopulateReport(list, true);
        Add(L"Button", L"System Summary / original dummy data", BS_GROUPBOX, 0, 16, 320, 835, 188);
        Label(L"Processor", 36, 352, 135);
        Add(L"Edit", L"Example CPU - 8 cores", WS_TABSTOP | ES_READONLY, 1202, 182, 349, 400, 27,
            WS_EX_CLIENTEDGE);
        Add(L"Static", L"Color / font preview (temporary)", SS_LEFT, 1011, 16, 625, 800, 50);
        if (demoFont_)
            SendDlgItemMessageW(panel_, 1011, WM_SETFONT, reinterpret_cast<WPARAM>(demoFont_), TRUE);
        Button(L"Ordinary &modal dialog", 1013, 16, 700, 360);
        Label(L"Memory", 36, 394, 135);
        Add(L"Edit", L"32 GB (demo)", WS_TABSTOP | ES_READONLY, 1203, 182, 391, 400, 27, WS_EX_CLIENTEDGE);
        Label(L"Profile", 36, 438, 135);
        auto combo = Add(L"ComboBox", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 1204, 182, 435, 400, 100);
        for (auto s : {L"Balanced (demo)", L"Performance (demo)"})
            SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
        SendMessageW(combo, CB_SETCURSEL, 0, 0);
        Button(L"Floating &Sensor Status", 1205, 16, 548, 390);
        Button(L"Floating Active &Clock", 1206, 436, 548, 390);
        break;
    }
    case 13:
    {
        Label(L"Custom GDI drawing - no built-in chart/splitter/tile control", 16, 12);
        auto surface = Add(L"Win32DemoSurface", L"Custom drawing illustration", 0, 1301, 16, 58, 800, 220,
                           WS_EX_CLIENTEDGE);
        SetWindowLongPtrW(surface, GWLP_USERDATA, 2);
        Label((L"Current HWND DPI: " + std::to_wstring(dpi_) +
               L" / font: SystemParametersInfoForDpi message font\r\n\r\nNative controls expose their "
               L"default accessibility support.\r\nCustom illustrations here are informational; monitor-tile "
               L"interaction also has a focus rectangle and keyboard keys.\r\nA production custom control "
               L"should implement UI Automation/MSAA semantics.\r\n\r\nChanging scale on a monitor sends "
               L"WM_DPICHANGED. The app recreates fonts and scales layout.\r\nTheme/settings changes reread "
               L"fonts and system colors.")
                  .c_str(),
              16, 315, 820, 240);
        break;
    }
    }
    for (const auto &c : children_)
        height_ = std::max(height_, c.y + c.h + 28);
    if (GetObjectType(font_) != OBJ_FONT)
        throw std::runtime_error("Font invalid before Layout on page " + std::to_string(page_));
    Layout();
    if (GetObjectType(font_) != OBJ_FONT)
        throw std::runtime_error("Font invalid after Layout on page " + std::to_string(page_));
    Event(L"Topic: " + std::wstring(p.title));
}
void App::Layout()
{
    if (!panel_)
        return;
    RECT r{};
    GetClientRect(window_, &r);
    const int width = MulDiv(r.right, 96, dpi_), height = MulDiv(r.bottom, 96, dpi_);
    SendMessageW(toolbar_, TB_AUTOSIZE, 0, 0);
    RECT bar{};
    GetWindowRect(toolbar_, &bar);
    const int top = MulDiv(bar.bottom - bar.top, 96, dpi_) + 8;
    const int divider = std::clamp(split_, 150, std::max(150, width - 350));
    auto place = [&](HWND h, int x, int y, int w, int hg)
    { MoveWindow(h, S(x), S(y), S(std::max(1, w)), S(std::max(1, hg)), TRUE); };
    place(tree_, 8, top, divider - 16, height - top - 31);
    place(heading_, divider + 12, top + 4, width - divider - 24, 26);
    place(info_, divider + 12, top + 36, width - divider - 24, 146);
    place(doc_, divider + 12, top + 190, 320, 28);
    place(panel_, divider + 12, top + 230, width - divider - 24, height - top - 354);
    place(log_, divider + 12, height - 110, width - divider - 24, 76);
    SendMessageW(status_, WM_SIZE, 0, 0);
    std::wstring footer = L"Win32 / Common Controls v6 | DPI " + std::to_wstring(dpi_) +
                          L" | F6 navigation/content | Ctrl+Left/Right topics";
    SendMessageW(status_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(footer.c_str()));
    LayoutChildren();
    InvalidateRect(window_, nullptr, TRUE);
}
void App::LayoutChildren()
{
    RECT r{};
    GetClientRect(panel_, &r);
    const int width = MulDiv(r.right, 96, dpi_), height = MulDiv(r.bottom, 96, dpi_);
    scrollX_ = std::clamp(scrollX_, 0, std::max(0, 890 - width));
    scrollY_ = std::clamp(scrollY_, 0, std::max(0, height_ - height));
    SCROLLINFO si{
        sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS, 0, height_ - 1, UINT(std::max(1, height)), scrollY_, 0};
    SetScrollInfo(panel_, SB_VERT, &si, TRUE);
    si.nMax = 889;
    si.nPage = UINT(std::max(1, width));
    si.nPos = scrollX_;
    SetScrollInfo(panel_, SB_HORZ, &si, TRUE);
    for (const auto &c : children_)
        MoveWindow(c.hwnd, S(c.x - scrollX_), S(c.y - scrollY_), S(c.w), S(c.h), TRUE);
}
void App::ContextMenu(HWND source, POINT point)
{
    if (options_.test)
        return;
    if (point.x == -1 && point.y == -1)
    {
        RECT r{};
        GetWindowRect(source, &r);
        point = {r.left + 30, r.top + 30};
    }
    auto menu = CreatePopupMenu(), sub = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, OpenDoc, L"&Microsoft documentation");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | MF_CHECKED, 905, L"&Checked item");
    AppendMenuW(menu, MF_STRING | MF_DISABLED, 906, L"Disabled item");
    AppendMenuW(sub, MF_STRING, 907, L"Option &A");
    AppendMenuW(sub, MF_STRING, 908, L"Option &B");
    CheckMenuRadioItem(sub, 907, 908, 907, MF_BYCOMMAND);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(sub), L"Radio choices");
    int id = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, window_, nullptr);
    DestroyMenu(menu);
    if (id)
        Command(id, 0, nullptr);
}
void App::PropertyDemo()
{
    if (options_.test)
        return;
    PROPSHEETPAGEW pages[2]{};
    for (int i = 0; i < 2; ++i)
    {
        pages[i].dwSize = sizeof(PROPSHEETPAGEW);
        pages[i].hInstance = instance_;
        pages[i].pszTemplate = MAKEINTRESOURCEW(101 + i);
        pages[i].pfnDlgProc = PropertyProc;
    }
    PROPSHEETHEADERW header{};
    header.dwSize = sizeof(header);
    header.dwFlags = PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP;
    header.hwndParent = window_;
    header.hInstance = instance_;
    header.pszCaption = L"Demo Mouse Properties - temporary values only";
    header.nPages = 2;
    header.ppsp = pages;
    PropertySheetW(&header);
    Event(L"PropertySheetW closed; no system settings were changed.");
}
void App::OpenDialog(int id)
{
    if (options_.test)
        return;
    if (id == 1009)
    {
        PropertyDemo();
        return;
    }
    if (id == 1013)
    {
        const auto result = DialogBoxParamW(instance_, MAKEINTRESOURCEW(103), window_, OrdinaryDialog, 0);
        Event(L"DialogBoxParamW returned " + std::to_wstring(result));
        return;
    }
    if (id <= 1002)
    {
        IFileDialog *dialog = nullptr;
        HRESULT hr = id == 1001 ? CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER,
                                                   IID_PPV_ARGS(&dialog))
                                : CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                                   IID_PPV_ARGS(&dialog));
        if (FAILED(hr))
        {
            Event(L"Common Item Dialog could not be created.");
            return;
        }
        DWORD flags = 0;
        dialog->GetOptions(&flags);
        dialog->SetOptions(flags | FOS_NOCHANGEDIR | FOS_DONTADDTORECENT |
                           (id == 1002 ? FOS_PICKFOLDERS : 0));
        dialog->SetTitle(L"Win32 demo: returns a path only; no file is opened or written");
        hr = dialog->Show(window_);
        if (SUCCEEDED(hr))
        {
            IShellItem *item = nullptr;
            if (SUCCEEDED(dialog->GetResult(&item)))
            {
                PWSTR path = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path)))
                {
                    Event(L"Selected path only: " + std::wstring(path));
                    CoTaskMemFree(path);
                }
                item->Release();
            }
        }
        else
            Event(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)
                      ? L"Common Item Dialog cancelled; no side effects."
                      : L"Common Item Dialog failed.");
        dialog->Release();
        return;
    }
    if (id == 1003)
    {
        CHOOSECOLORW color{};
        color.lStructSize = sizeof(color);
        color.hwndOwner = window_;
        color.lpCustColors = customColors_;
        color.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
        color.lpfnHook = CommonHook;
        color.rgbResult = demoColor_;
        if (ChooseColorW(&color))
        {
            demoColor_ = color.rgbResult;
            InvalidateRect(GetDlgItem(panel_, 1011), nullptr, TRUE);
            Event(L"ChooseColorW changed the temporary preview color.");
        }
        else
            Event(L"ChooseColorW cancelled.");
    }
    else if (id == 1004)
    {
        LOGFONTW lf{};
        GetObjectW(font_, sizeof(lf), &lf);
        CHOOSEFONTW font{};
        font.lStructSize = sizeof(font);
        font.hwndOwner = window_;
        font.lpLogFont = &lf;
        font.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_ENABLEHOOK;
        font.lpfnHook = CommonHook;
        if (ChooseFontW(&font))
        {
            auto selected = CreateFontIndirectW(&lf);
            if (!selected)
                throw std::runtime_error("Sample font allocation failed.");
            SendDlgItemMessageW(panel_, 1011, WM_SETFONT, reinterpret_cast<WPARAM>(selected), TRUE);
            if (demoFont_)
                DeleteObject(demoFont_);
            demoFont_ = selected;
            Event(L"ChooseFontW changed the temporary preview font: " + std::wstring(lf.lfFaceName));
        }
        else
            Event(L"ChooseFontW cancelled.");
    }
    else if (id == 1005 || id == 1006)
    {
        if (find_ && IsWindow(find_))
        {
            Event(L"Close the current modeless Find/Replace dialog first.");
            return;
        }
        findData_ = {};
        findData_.lStructSize = sizeof(findData_);
        findData_.hwndOwner = window_;
        findData_.Flags = FR_DOWN | FR_ENABLEHOOK;
        findData_.lpfnHook = CommonHook;
        findData_.lpstrFindWhat = findText_;
        findData_.wFindWhatLen = 128;
        findData_.lpstrReplaceWith = replaceText_;
        findData_.wReplaceWithLen = 128;
        find_ = id == 1005 ? FindTextW(&findData_) : ReplaceTextW(&findData_);
        if (!find_)
            Event(L"Find/Replace dialog creation failed.");
    }
    else if (id == 1007)
    {
        const int result = MessageBoxW(window_,
                                       L"This is a native MessageBoxW.\nOK and Cancel return button "
                                       L"identifiers.\nIt does not save or change anything.",
                                       L"Demo message box", MB_OKCANCEL | MB_ICONINFORMATION);
        Event(L"MessageBoxW returned " + std::to_wstring(result));
    }
    else if (id == 1008)
    {
        TASKDIALOGCONFIG config{};
        config.cbSize = sizeof(config);
        config.hwndParent = window_;
        config.dwFlags =
            TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_EXPAND_FOOTER_AREA;
        config.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
        config.pszWindowTitle = L"Native task dialog";
        config.pszMainIcon = TD_INFORMATION_ICON;
        config.pszMainInstruction = L"TaskDialogIndirect provides richer native prompts";
        config.pszContent = L"This example contains text, an icon, a verification checkbox and expandable "
                            L"details. All values are temporary.";
        config.pszVerificationText = L"Demo verification checkbox";
        config.pszExpandedInformation =
            L"Task dialogs can also include command links, radio buttons, hyperlinks, progress bars and "
            L"callbacks. This gallery demonstrates a representative safe subset.";
        config.pfCallback = TaskCallback;
        config.lpCallbackData = reinterpret_cast<LONG_PTR>(window_);
        int result = 0;
        BOOL check = FALSE;
        auto hr = TaskDialogIndirect(&config, &result, nullptr, &check);
        Event(SUCCEEDED(hr) ? L"TaskDialogIndirect result " + std::to_wstring(result) + L"; verification " +
                                  std::to_wstring(check)
                            : L"Task dialog failed.");
    }
}
void App::Command(int id, int code, HWND control)
{
    // Logging/read-only explanation updates generate edit notifications too.
    if (control && (control == log_ || control == info_))
        return;
    if (id >= NavFirst && id < NavFirst + int(Pages().size()))
    {
        Build(id - NavFirst);
        return;
    }
    if (id == NavBack)
    {
        Build(page_ - 1);
        return;
    }
    if (id == NavNext)
    {
        Build(page_ + 1);
        return;
    }
    if (id == NavHome || id == About)
    {
        Build(0);
        return;
    }
    if (id == Quit)
    {
        DestroyWindow(window_);
        return;
    }
    if (id == OpenDoc)
    {
        if (!options_.test)
            ShellExecuteW(window_, L"open", Pages()[page_].doc, nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if ((id >= 1000 && id <= 1009) || id == 1013)
    {
        OpenDialog(id);
        return;
    }
    if (id == 901 || id == 902 || id == 1205 || id == 1206)
    {
        FloatingDemo(id);
        return;
    }
    if (id == 403 || id == 404 || id == 405)
    {
        SendDlgItemMessageW(panel_, 402, LVM_SETVIEW,
                            id == 403   ? LV_VIEW_DETAILS
                            : id == 404 ? LV_VIEW_LIST
                                        : LV_VIEW_ICON,
                            0);
    }
    if (id >= 505 && id <= 507)
        SendDlgItemMessageW(panel_, 504, PBM_SETSTATE,
                            id == 505   ? PBST_NORMAL
                            : id == 506 ? PBST_PAUSED
                                        : PBST_ERROR,
                            0);
    if (id == 1051)
    {
        bool on = SendDlgItemMessageW(panel_, 1051, BM_GETCHECK, 0, 0) == BST_CHECKED;
        EnableWindow(GetDlgItem(panel_, 1052), on);
        EnableWindow(GetDlgItem(panel_, 1053), on);
    }
    if (control)
        Event(ClassName(control) + L" / id " + std::to_wstring(id) + L" / WM_COMMAND notification " +
              std::to_wstring(code) +
              (ClassName(control) == L"Button" ? L" (BN_CLICKED=0)"
               : ClassName(control) == L"Edit" ? L" (EN_CHANGE=768)"
                                               : L""));
    else
        Event(L"Menu / toolbar command " + std::to_wstring(id));
}
void App::Notify(NMHDR *hdr)
{
    if (hdr->code == NM_CUSTOMDRAW)
        return;
    if (hdr->hwndFrom == tree_ && hdr->code == TVN_SELCHANGEDW && !navigating_)
    {
        Build(int(reinterpret_cast<NMTREEVIEWW *>(hdr)->itemNew.lParam));
        return;
    }
    if (hdr->code == TBN_DROPDOWN || hdr->code == BCN_DROPDOWN)
    {
        if (options_.test)
            return;
        auto menu = CreatePopupMenu();
        if (hdr->hwndFrom == toolbar_)
        {
            for (int i = 0; i < int(Pages().size()); ++i)
                AppendMenuW(menu, MF_STRING, NavFirst + i, Pages()[i].title);
        }
        else
        {
            AppendMenuW(menu, MF_STRING, 1171, L"Demo split action A");
            AppendMenuW(menu, MF_STRING, 1172, L"Demo split action B");
        }
        RECT r{};
        GetWindowRect(hdr->hwndFrom, &r);
        const int id = TrackPopupMenuEx(menu, TPM_RETURNCMD, r.left, r.bottom, window_, nullptr);
        DestroyMenu(menu);
        if (id)
            Command(id, 0, nullptr);
        return;
    }
    if (hdr->code == NM_CLICK || hdr->code == NM_RETURN)
    {
        if (ClassName(hdr->hwndFrom) == WC_LINK)
        {
            auto link = reinterpret_cast<NMLINK *>(hdr);
            if (!options_.test && std::wstring(link->item.szUrl).starts_with(L"https://learn.microsoft.com/"))
                ShellExecuteW(window_, L"open", link->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
    }
    if (hdr->idFrom == 701 && hdr->code == TCN_SELCHANGE)
    {
        std::wstring text = L"Page selected: " + std::to_wstring(TabCtrl_GetCurSel(hdr->hwndFrom));
        SetDlgItemTextW(panel_, 702, text.c_str());
    }
    if (hdr->code == PGN_CALCSIZE)
    {
        auto calc = reinterpret_cast<NMPGCALCSIZE *>(hdr);
        if (calc->dwFlag == PGF_CALCWIDTH)
            calc->iWidth = S(650);
        else
            calc->iHeight = S(36);
        return;
    }
    if (hdr->idFrom == 402 && hdr->code == LVN_COLUMNCLICK)
    {
        sortColumn_ = reinterpret_cast<NMLISTVIEW *>(hdr)->iSubItem;
        sortingAscending_ = !sortingAscending_;
        const auto compare = [](LPARAM a, LPARAM b, LPARAM context) -> int
        {
            auto app = reinterpret_cast<App *>(context);
            static const wchar_t *data[3][3]{{L"Button", L"USER32", L"Push / check / radio"},
                                             {L"Tree view", L"COMCTL32", L"Hierarchy"},
                                             {L"List view", L"COMCTL32", L"Rows and columns"}};
            int order = wcscmp(data[a][app->sortColumn_], data[b][app->sortColumn_]);
            return app->sortingAscending_ ? order : -order;
        };
        ListView_SortItems(hdr->hwndFrom, compare, reinterpret_cast<LPARAM>(this));
    }
    if (hdr->idFrom == 1201 && (hdr->code == NM_DBLCLK || hdr->code == LVN_KEYDOWN))
    {
        if (hdr->code == LVN_KEYDOWN && reinterpret_cast<NMLVKEYDOWN *>(hdr)->wVKey != VK_RETURN)
            return;
        int index = ListView_GetNextItem(hdr->hwndFrom, -1, LVNI_SELECTED);
        LVITEMW item{};
        item.mask = LVIF_PARAM;
        item.iItem = index;
        if (index >= 0 && ListView_GetItem(hdr->hwndFrom, &item) && item.lParam == 100)
        {
            sensorExpanded_ = !sensorExpanded_;
            PopulateReport(hdr->hwndFrom, true);
        }
    }
    const wchar_t *name = hdr->code == LVN_ITEMCHANGED      ? L"LVN_ITEMCHANGED"
                          : hdr->code == LVN_COLUMNCLICK    ? L"LVN_COLUMNCLICK"
                          : hdr->code == TCN_SELCHANGE      ? L"TCN_SELCHANGE"
                          : hdr->code == DTN_DATETIMECHANGE ? L"DTN_DATETIMECHANGE"
                          : hdr->code == MCN_SELCHANGE      ? L"MCN_SELCHANGE"
                          : hdr->code == TVN_SELCHANGEDW    ? L"TVN_SELCHANGEDW"
                          : hdr->code == NM_CLICK           ? L"NM_CLICK"
                          : hdr->code == NM_DBLCLK          ? L"NM_DBLCLK"
                                                            : L"notification";
    Event(ClassName(hdr->hwndFrom) + L" / " + name + L" / WM_NOTIFY code " +
          std::to_wstring(static_cast<int>(hdr->code)));
}
void App::FloatingDemo(int kind)
{
    if (options_.test)
        return;
    auto f = CreateWindowExW(kind == 902 ? 0 : WS_EX_TOOLWINDOW, L"Win32DemoFloat",
                             kind == 1206   ? L"Demo Active Clock"
                             : kind == 1205 ? L"Demo Sensor Status"
                                            : L"Owned demo window",
                             WS_OVERLAPPEDWINDOW, display_.work.left + 80, display_.work.top + 80, S(650),
                             S(390), window_, nullptr, instance_, this);
    if (!f)
    {
        Event(L"Owned window creation failed.");
        return;
    }
    floating_.push_back(f);
    FitOwned(f, window_);
    if (kind == 1205)
    {
        auto list = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"Sensors",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHAREIMAGELISTS, 12,
                                    12, S(600), S(280), f, reinterpret_cast<HMENU>(1201), instance_, nullptr);
        SendMessageW(list, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        Columns(list,
                {{L"Sensor", S(280)}, {L"Current", S(100)}, {L"Minimum", S(100)}, {L"Maximum", S(100)}});
        PopulateReport(list, true);
    }
    else if (kind == 1206)
    {
        for (int i = 0; i < 4; ++i)
        {
            auto label = CreateWindowExW(
                0, L"Static",
                (L"Core " + std::to_wstring(i) + L": " + std::to_wstring(1800 + i * 600) + L" MHz (dummy)")
                    .c_str(),
                WS_CHILD | WS_VISIBLE, 12, S(16 + i * 66), S(580), S(24), f, nullptr, instance_, nullptr);
            SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
            auto progress = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE, 12,
                                            S(43 + i * 66), S(580), S(18), f, nullptr, instance_, nullptr);
            SendMessageW(progress, PBM_SETPOS, 30 + i * 15, 0);
        }
    }
    else
    {
        auto label = CreateWindowExW(
            0, L"Static",
            L"Owned floating window\r\n\r\nClose affects this window only.\r\nWS_EX_TOOLWINDOW uses smaller "
            L"tool-window chrome.\r\nThe main application's HWND is its owner.",
            WS_CHILD | WS_VISIBLE, 16, 16, S(570), S(250), f, nullptr, instance_, nullptr);
        SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    }
    ShowWindow(f, SW_SHOWNORMAL);
}
LRESULT CALLBACK App::FloatingProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    App *self = reinterpret_cast<App *>(GetWindowLongPtrW(h, GWLP_USERDATA));
    if (m == WM_NCCREATE)
    {
        self = static_cast<App *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self && m == WM_NOTIFY)
    {
        auto hdr = reinterpret_cast<NMHDR *>(l);
        if (hdr->code == NM_CUSTOMDRAW && hdr->idFrom == 1201)
        {
            auto draw = reinterpret_cast<NMLVCUSTOMDRAW *>(l);
            if (draw->nmcd.dwDrawStage == CDDS_PREPAINT)
                return CDRF_NOTIFYITEMDRAW;
            if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
            {
                draw->clrTextBk =
                    draw->nmcd.dwItemSpec % 2 ? GetSysColor(COLOR_BTNFACE) : GetSysColor(COLOR_WINDOW);
                return CDRF_NEWFONT;
            }
        }
        if (hdr->idFrom == 1201 && (hdr->code == NM_DBLCLK || hdr->code == LVN_KEYDOWN))
        {
            if (hdr->code != LVN_KEYDOWN || reinterpret_cast<NMLVKEYDOWN *>(hdr)->wVKey == VK_RETURN)
            {
                int item = ListView_GetNextItem(hdr->hwndFrom, -1, LVNI_SELECTED);
                if (item == 0)
                {
                    self->sensorExpanded_ = !self->sensorExpanded_;
                    self->PopulateReport(hdr->hwndFrom, true);
                }
            }
        }
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
LRESULT CALLBACK App::SurfaceProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_GETDLGCODE)
        return DLGC_WANTARROWS;
    if (m == WM_LBUTTONDOWN)
    {
        if (!(GetWindowLongPtrW(GetAncestor(h, GA_ROOT), GWL_EXSTYLE) & WS_EX_NOACTIVATE))
            SetFocus(h);
        RECT r{};
        GetClientRect(h, &r);
        SetWindowLongPtrW(h, GWLP_ID, GET_X_LPARAM(l) > r.right / 2 ? 2 : 1);
        InvalidateRect(h, nullptr, TRUE);
        return 0;
    }
    if (m == WM_KEYDOWN)
    {
        if (w == VK_LEFT || w == VK_RIGHT || w == VK_SPACE)
        {
            SetWindowLongPtrW(h, GWLP_ID, w == VK_LEFT ? 1 : 2);
            InvalidateRect(h, nullptr, TRUE);
            return 0;
        }
    }
    if (m == WM_SETFOCUS || m == WM_KILLFOCUS)
    {
        InvalidateRect(h, nullptr, TRUE);
        return 0;
    }
    if (m == WM_PAINT)
    {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(h, &ps);
        RECT r{};
        GetClientRect(h, &r);
        FillRect(dc, &r, GetSysColorBrush(COLOR_WINDOW));
        auto owner = reinterpret_cast<App *>(GetWindowLongPtrW(GetParent(h), GWLP_USERDATA));
        auto font = owner ? owner->font_ : nullptr;
        auto old = font ? SelectObject(dc, font) : nullptr;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
        int type = int(GetWindowLongPtrW(h, GWLP_USERDATA));
        if (type == 1)
        {
            for (int i = 0; i < 2; ++i)
            {
                RECT tile{20 + i * r.right / 2, 20, (i + 1) * r.right / 2 - 20, r.bottom - 20};
                FillRect(dc, &tile,
                         GetSysColorBrush(GetWindowLongPtrW(h, GWLP_ID) == i + 1 ? COLOR_HIGHLIGHT
                                                                                 : COLOR_BTNFACE));
                RECT screen{tile.left + 20, tile.top + 20, tile.right - 20, tile.bottom - 55};
                FrameRect(dc, &screen, GetSysColorBrush(COLOR_WINDOWTEXT));
                RECT text{tile.left + 15, tile.bottom - 45, tile.right - 15, tile.bottom - 10};
                SetTextColor(dc, GetSysColor(GetWindowLongPtrW(h, GWLP_ID) == i + 1 ? COLOR_HIGHLIGHTTEXT
                                                                                    : COLOR_WINDOWTEXT));
                DrawTextW(dc, i ? L"Demo display 2" : L"Demo display 1", -1, &text,
                          DT_CENTER | DT_SINGLELINE | DT_VCENTER);
                if (GetFocus() == h && GetWindowLongPtrW(h, GWLP_ID) == i + 1)
                    DrawFocusRect(dc, &tile);
            }
        }
        else
        {
            RECT text{16, 10, r.right - 16, 40};
            DrawTextW(dc, L"Original custom chart / WM_PAINT / GDI", -1, &text, DT_LEFT | DT_SINGLELINE);
            for (int i = 0; i < 5; ++i)
            {
                RECT bar{24 + i * (r.right - 48) / 5, 60 + (4 - i) * 18,
                         24 + (i + 1) * (r.right - 48) / 5 - 12, r.bottom - 25};
                FillRect(dc, &bar, GetSysColorBrush(COLOR_HIGHLIGHT));
            }
        }
        if (old)
            SelectObject(dc, old);
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
LRESULT CALLBACK App::MainProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    auto self = reinterpret_cast<App *>(GetWindowLongPtrW(h, GWLP_USERDATA));
    if (m == WM_NCCREATE)
    {
        self = static_cast<App *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self)
        return DefWindowProcW(h, m, w, l);
    try
    {
        return self->MainMessage(h, m, w, l);
    }
    catch (const std::exception &e)
    {
        self->fatal_ = std::wstring(e.what(), e.what() + strlen(e.what()));
        if (self->log_)
            self->Event(L"Error: " + self->fatal_);
        return 0;
    }
}
LRESULT CALLBACK App::PanelProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    auto self = reinterpret_cast<App *>(GetWindowLongPtrW(h, GWLP_USERDATA));
    if (m == WM_NCCREATE)
    {
        self = static_cast<App *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self)
        return DefWindowProcW(h, m, w, l);
    try
    {
        return self->PanelMessage(h, m, w, l);
    }
    catch (const std::exception &e)
    {
        self->fatal_ = std::wstring(e.what(), e.what() + strlen(e.what()));
        return 0;
    }
}
LRESULT App::MainMessage(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == findMessage_)
    {
        auto data = reinterpret_cast<FINDREPLACEW *>(l);
        if (data->Flags & FR_DIALOGTERM)
        {
            find_ = nullptr;
            Event(L"Find/Replace closed.");
        }
        else
        {
            auto edit = GetDlgItem(panel_, 1010);
            if (edit)
            {
                const int length = GetWindowTextLengthW(edit);
                std::wstring text(size_t(length) + 1, L'\0');
                GetWindowTextW(edit, text.data(), length + 1);
                text.resize(length);
                const std::wstring needle = data->lpstrFindWhat;
                const auto at = text.find(needle);
                if (at != std::wstring::npos && !needle.empty())
                {
                    if (data->Flags & (FR_REPLACE | FR_REPLACEALL))
                    {
                        size_t pos = at;
                        do
                        {
                            text.replace(pos, needle.size(), data->lpstrReplaceWith);
                            pos = text.find(needle, pos + wcslen(data->lpstrReplaceWith));
                        } while ((data->Flags & FR_REPLACEALL) && pos != std::wstring::npos);
                        SetWindowTextW(edit, text.c_str());
                    }
                    else
                        SendMessageW(edit, EM_SETSEL, at, at + needle.size());
                    Event(L"Find/Replace operated on the temporary demo buffer.");
                }
                else
                    Event(L"Find text not found.");
            }
        }
        return 0;
    }
    switch (m)
    {
    case WM_SIZE:
        Layout();
        return 0;
    case WM_COMMAND:
        Command(LOWORD(w), HIWORD(w), reinterpret_cast<HWND>(l));
        return 0;
    case WM_NOTIFY:
        Notify(reinterpret_cast<NMHDR *>(l));
        return 0;
    case WM_MOUSEACTIVATE:
        if (options_.test)
            return MA_NOACTIVATE;
        break;
    case WM_LBUTTONDOWN:
    {
        const int x = MulDiv(GET_X_LPARAM(l), 96, dpi_);
        if (abs(x - split_) < 10 && !options_.test)
        {
            dragging_ = true;
            SetCapture(h);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (dragging_)
        {
            split_ = MulDiv(GET_X_LPARAM(l), 96, dpi_);
            Layout();
        }
        return 0;
    case WM_LBUTTONUP:
        if (dragging_)
        {
            dragging_ = false;
            ReleaseCapture();
        }
        return 0;
    case WM_CAPTURECHANGED:
        dragging_ = false;
        return 0;
    case WM_SETCURSOR:
    {
        if (options_.test)
            return TRUE;
        POINT pt{};
        GetCursorPos(&pt);
        ScreenToClient(h, &pt);
        if (abs(MulDiv(pt.x, 96, dpi_) - split_) < 10)
        {
            SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
            return TRUE;
        }
        break;
    }
    case WM_DPICHANGED:
    {
        dpi_ = HIWORD(w);
        MakeFonts();
        auto r = reinterpret_cast<RECT *>(l);
        SetWindowPos(h, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        Build(page_);
        Layout();
        return 0;
    }
    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        MakeFonts();
        Build(page_);
        return 0;
    case WM_GETMINMAXINFO:
    {
        auto mm = reinterpret_cast<MINMAXINFO *>(l);
        mm->ptMinTrackSize = {S(720), S(650)};
        return 0;
    }
    case WM_CONTEXTMENU:
        ContextMenu(h, {GET_X_LPARAM(l), GET_Y_LPARAM(l)});
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
LRESULT App::PanelMessage(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m)
    {
    case WM_CTLCOLORSTATIC:
        if (GetDlgCtrlID(reinterpret_cast<HWND>(l)) == 1011)
        {
            SetTextColor(reinterpret_cast<HDC>(w), demoColor_);
            SetBkColor(reinterpret_cast<HDC>(w), GetSysColor(COLOR_BTNFACE));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }
        break;
    case WM_COMMAND:
        Command(LOWORD(w), HIWORD(w), reinterpret_cast<HWND>(l));
        return 0;
    case WM_NOTIFY:
    {
        auto hdr = reinterpret_cast<NMHDR *>(l);
        if (hdr->idFrom == 1201 && hdr->code == NM_CUSTOMDRAW)
        {
            auto draw = reinterpret_cast<NMLVCUSTOMDRAW *>(l);
            if (draw->nmcd.dwDrawStage == CDDS_PREPAINT)
                return CDRF_NOTIFYITEMDRAW;
            if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
            {
                draw->clrTextBk =
                    draw->nmcd.dwItemSpec % 2 ? GetSysColor(COLOR_BTNFACE) : GetSysColor(COLOR_WINDOW);
                return CDRF_NEWFONT;
            }
        }
        Notify(hdr);
        return 0;
    }
    case WM_CONTEXTMENU:
        ContextMenu(h, {GET_X_LPARAM(l), GET_Y_LPARAM(l)});
        return 0;
    case WM_MOUSEWHEEL:
        scrollY_ -= GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA * 64;
        LayoutChildren();
        return 0;
    case WM_HSCROLL:
    case WM_VSCROLL:
    {
        if (l)
        {
            auto control = reinterpret_cast<HWND>(l);
            if (ClassName(control) == L"ScrollBar")
            {
                SCROLLINFO si{sizeof(si), SIF_ALL};
                GetScrollInfo(control, SB_CTL, &si);
                const int code = LOWORD(w);
                if (code == SB_THUMBTRACK)
                    si.nPos = si.nTrackPos;
                else if (code == SB_LINELEFT)
                    si.nPos -= 1;
                else if (code == SB_LINERIGHT)
                    si.nPos += 1;
                else if (code == SB_PAGELEFT)
                    si.nPos -= int(si.nPage);
                else if (code == SB_PAGERIGHT)
                    si.nPos += int(si.nPage);
                SetScrollInfo(control, SB_CTL, &si, TRUE);
                Event(L"ScrollBar / WM_SCROLL position " + std::to_wstring(si.nPos));
            }
            else
            {
                const auto value = SendMessageW(control, TBM_GETPOS, 0, 0);
                if (GetDlgCtrlID(control) == 501)
                    SendDlgItemMessageW(panel_, 504, PBM_SETPOS, value, 0);
                Event(L"Trackbar / WM_HSCROLL value " + std::to_wstring(value));
            }
            return 0;
        }
        int &pos = m == WM_VSCROLL ? scrollY_ : scrollX_;
        SCROLLINFO si{sizeof(si), SIF_ALL};
        GetScrollInfo(h, m == WM_VSCROLL ? SB_VERT : SB_HORZ, &si);
        switch (LOWORD(w))
        {
        case SB_LINEUP:
            pos -= 24;
            break;
        case SB_LINEDOWN:
            pos += 24;
            break;
        case SB_PAGEUP:
            pos -= int(si.nPage);
            break;
        case SB_PAGEDOWN:
            pos += int(si.nPage);
            break;
        case SB_THUMBTRACK:
            pos = si.nTrackPos;
            break;
        case SB_TOP:
            pos = 0;
            break;
        case SB_BOTTOM:
            pos = si.nMax;
            break;
        }
        LayoutChildren();
        return 0;
    }
    }
    return DefWindowProcW(h, m, w, l);
}
int App::Run()
{
    ShowWindow(window_, SW_SHOWNORMAL);
    UpdateWindow(window_);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        if (msg.message == WM_KEYDOWN)
        {
            const bool ctrl = GetKeyState(VK_CONTROL) < 0;
            if (msg.wParam == VK_F1)
            {
                Command(OpenDoc, 0, nullptr);
                continue;
            }
            if (ctrl && msg.wParam == 'O')
            {
                OpenDialog(1000);
                continue;
            }
            if (ctrl && (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
            {
                Build(page_ + (msg.wParam == VK_LEFT ? -1 : 1));
                continue;
            }
            if (msg.wParam == VK_F6)
            {
                SetFocus(GetFocus() == tree_ ? GetNextDlgTabItem(panel_, nullptr, FALSE) : tree_);
                continue;
            }
        }
        if (msg.message == WM_SYSKEYDOWN && (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
        {
            split_ += msg.wParam == VK_LEFT ? -20 : 20;
            Layout();
            continue;
        }
        if (find_ && IsWindow(find_) && IsDialogMessageW(find_, &msg))
            continue;
        bool consumed = false;
        for (auto f : floating_)
            if (IsWindow(f) && IsDialogMessageW(f, &msg))
            {
                consumed = true;
                break;
            }
        if (consumed)
            continue;
        if (!IsDialogMessageW(window_, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        auto focused = GetFocus();
        if (focused && IsChild(panel_, focused))
        {
            RECT r{};
            GetWindowRect(focused, &r);
            MapWindowPoints(nullptr, panel_, reinterpret_cast<POINT *>(&r), 2);
            RECT p{};
            GetClientRect(panel_, &p);
            if (r.top < 0)
                scrollY_ += MulDiv(r.top, 96, dpi_);
            else if (r.bottom > p.bottom)
                scrollY_ += MulDiv(r.bottom - p.bottom, 96, dpi_);
            if (r.left < 0)
                scrollX_ += MulDiv(r.left, 96, dpi_);
            else if (r.right > p.right)
                scrollX_ += MulDiv(r.right - p.right, 96, dpi_);
            LayoutChildren();
        }
    }
    return int(msg.wParam);
}
void App::Capture(const std::filesystem::path &path)
{
    RECT r{};
    GetWindowRect(window_, &r);
    if (r.left < display_.bounds.left || r.top < display_.bounds.top || r.right > display_.bounds.right ||
        r.bottom > display_.bounds.bottom)
        throw std::runtime_error("Test window escaped NE18NZ2 bounds; capture refused.");
    auto dc = GetDC(window_);
    auto memory = CreateCompatibleDC(dc);
    auto bitmap = CreateCompatibleBitmap(dc, r.right - r.left, r.bottom - r.top);
    auto old = SelectObject(memory, bitmap);
    const BOOL rendered = PrintWindow(window_, memory, 2);
    SelectObject(memory, old);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = r.right - r.left;
    info.bmiHeader.biHeight = -(r.bottom - r.top);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    const DWORD bytes = DWORD(r.right - r.left) * DWORD(r.bottom - r.top) * 4;
    std::vector<unsigned char> pixels(bytes);
    const int rows = GetDIBits(memory, bitmap, 0, r.bottom - r.top, pixels.data(), &info, DIB_RGB_COLORS);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(window_, dc);
    if (!rendered || rows != r.bottom - r.top)
        throw std::runtime_error("Native window rendering failed.");
    BITMAPFILEHEADER header{};
    header.bfType = 0x4d42;
    header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
    header.bfSize = header.bfOffBits + bytes;
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(&header), sizeof(header));
    out.write(reinterpret_cast<const char *>(&info.bmiHeader), sizeof(BITMAPINFOHEADER));
    out.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
    if (!out)
        throw std::runtime_error("Capture report could not be written.");
}
int App::Audit()
{
    std::ofstream out(options_.report.empty() ? std::filesystem::path(L"Audit.txt") : options_.report);
    if (!out)
        throw std::runtime_error("Audit report destination unavailable.");
    auto check = [&](bool okay, const std::string &name)
    {
        out << (okay ? "PASS " : "FAIL ") << name << '\n';
        if (!okay)
            throw std::runtime_error(name);
    };
    HWND foreground = GetForegroundWindow();
    POINT cursor{};
    GetCursorPos(&cursor);
    check(AreDpiAwarenessContextsEqual(GetWindowDpiAwarenessContext(window_),
                                       DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE,
          "Manifest Per-Monitor V2 awareness");
    DLLVERSIONINFO v{sizeof(v)};
    auto version = reinterpret_cast<HRESULT(CALLBACK *)(DLLVERSIONINFO *)>(
        GetProcAddress(GetModuleHandleW(L"comctl32.dll"), "DllGetVersion"));
    const bool versionValid = version && SUCCEEDED(version(&v)) && v.dwMajorVersion >= 6;
    check(versionValid, "Common Controls v6 activation (major " + std::to_string(v.dwMajorVersion) + ")");
    out << "Theme active=" << IsThemeActive() << " app themed=" << IsAppThemed() << " DPI=" << dpi_ << '\n';
    if (options_.visible)
    {
        RECT r{};
        GetWindowRect(window_, &r);
        check(r.left >= display_.bounds.left && r.top >= display_.bounds.top &&
                  r.right <= display_.bounds.right && r.bottom <= display_.bounds.bottom,
              "Window wholly on exact NE18NZ2 identity");
        ShowWindow(window_, SW_SHOWNOACTIVATE);
        UpdateWindow(window_);
        check(GetForegroundWindow() == foreground, "No foreground activation");
    }
    std::vector<std::wstring> classes;
    for (int i = 0; i < int(Pages().size()); ++i)
    {
        Build(i);
        Pump();
        check(GetObjectType(font_) == OBJ_FONT, "Body font valid after page " + std::to_string(i));
        check(fatal_.empty(), "Page " + std::to_string(i) + " creation");
        for (auto c : children_)
        {
            check(IsWindow(c.hwnd), "Live " + Narrow(ClassName(c.hwnd)));
            classes.push_back(ClassName(c.hwnd));
        }
        if (options_.visible)
        {
            UpdateWindow(window_);
            Capture(options_.report.parent_path() / (L"Page" + std::to_wstring(i) + L".bmp"));
        }
    }
    for (auto expected : {L"Button",
                          L"Static",
                          L"Edit",
                          L"ListBox",
                          L"ComboBox",
                          L"ScrollBar",
                          L"RICHEDIT50W",
                          L"ComboBoxEx32",
                          L"msctls_hotkey32",
                          L"SysIPAddress32",
                          L"SysTreeView32",
                          L"SysListView32",
                          L"SysHeader32",
                          L"msctls_trackbar32",
                          L"msctls_updown32",
                          L"msctls_progress32",
                          L"SysDateTimePick32",
                          L"SysMonthCal32",
                          L"SysAnimate32",
                          L"SysTabControl32",
                          L"ReBarWindow32",
                          L"SysPager",
                          L"SysLink"})
        check(std::find(classes.begin(), classes.end(), expected) != classes.end(),
              "Inventory class " + Narrow(expected));
    check(ImageList_GetImageCount(images_) == 4, "Shared image list survives control destruction");
    LOGFONTW verifiedFont{};
    check(GetObjectW(font_, sizeof(verifiedFont), &verifiedFont) == sizeof(verifiedFont) &&
              verifiedFont.lfWeight < FW_BOLD,
          "System body font remains regular after navigation");
    for (int resource : {101, 102, 103})
        check(FindResourceW(instance_, MAKEINTRESOURCEW(resource), RT_DIALOG) != nullptr,
              "Native dialog template " + std::to_string(resource));
    PROPSHEETPAGEW templatePage{};
    templatePage.dwSize = sizeof(templatePage);
    templatePage.hInstance = instance_;
    templatePage.pszTemplate = MAKEINTRESOURCEW(101);
    templatePage.pfnDlgProc = PropertyProc;
    auto propertyPage = CreatePropertySheetPageW(&templatePage);
    check(propertyPage != nullptr, "Property page accepts native template without showing a dialog");
    check(DestroyPropertySheetPage(propertyPage) != FALSE, "Property page resource released");
    IFileDialog *fileDialog = nullptr;
    auto dialogResult =
        CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDialog));
    if (fileDialog)
        fileDialog->Release();
    check(SUCCEEDED(dialogResult), "Shell Common Item Dialog COM object creation; no Show call");
    check(ClassName(toolbar_) == TOOLBARCLASSNAMEW && ClassName(status_) == STATUSCLASSNAMEW &&
              ClassName(tooltip_) == TOOLTIPS_CLASSW,
          "Toolbar, status and tooltip hosts");
    Build(1);
    check(SendDlgItemMessageW(panel_, 114, BM_GETCHECK, 0, 0) == BST_INDETERMINATE,
          "Three-state checkbox indeterminate");
    check(!IsWindowEnabled(GetDlgItem(panel_, 112)), "Disabled button state");
    SendDlgItemMessageW(panel_, 113, BM_SETCHECK, BST_CHECKED, 0);
    check(SendDlgItemMessageW(panel_, 113, BM_GETCHECK, 0, 0) == BST_CHECKED, "Checkbox change");
    SendDlgItemMessageW(panel_, 118, BCM_GETNOTELENGTH, 0, 0);
    check(SendDlgItemMessageW(panel_, 118, BCM_GETNOTELENGTH, 0, 0) > 0, "Command link note accepted by v6");
    check(GetNextDlgTabItem(panel_, nullptr, FALSE) != nullptr,
          "Keyboard tab order resolves without stealing focus");
    Build(5);
    SendDlgItemMessageW(panel_, 501, TBM_SETPOS, TRUE, 73);
    SendMessageW(panel_, WM_HSCROLL, TB_THUMBPOSITION, reinterpret_cast<LPARAM>(GetDlgItem(panel_, 501)));
    check(SendDlgItemMessageW(panel_, 504, PBM_GETPOS, 0, 0) == 73,
          "Trackbar updates progress via WM_HSCROLL");
    Command(507, BN_CLICKED, GetDlgItem(panel_, 507));
    check(SendDlgItemMessageW(panel_, 504, PBM_GETSTATE, 0, 0) == PBST_ERROR, "Progress error state");
    Build(10);
    SendDlgItemMessageW(panel_, 1051, BM_SETCHECK, BST_UNCHECKED, 0);
    Command(1051, BN_CLICKED, GetDlgItem(panel_, 1051));
    check(!IsWindowEnabled(GetDlgItem(panel_, 1052)), "Checkbox disables dependent native controls");
    Build(4);
    Command(405, 0, nullptr);
    check(SendDlgItemMessageW(panel_, 402, LVM_GETVIEW, 0, 0) == LV_VIEW_ICON, "List view icon variant");
    Command(403, 0, nullptr);
    check(SendDlgItemMessageW(panel_, 402, LVM_GETVIEW, 0, 0) == LV_VIEW_DETAILS, "List view report variant");
    const UINT original = dpi_;
    for (UINT d : {96u, 144u, 192u})
    {
        dpi_ = d;
        MakeFonts();
        Build(3);
        MakeFonts();
        Build(4);
        Build(5);
        int iconWidth = 0, iconHeight = 0;
        ImageList_GetIconSize(images_, &iconWidth, &iconHeight);
        check(iconWidth == S(16) && iconHeight == S(16),
              "Image list scales at synthetic DPI " + std::to_string(d));
        check(GetObjectType(font_) == OBJ_FONT, "System font/layout at synthetic DPI " + std::to_string(d));
    }
    dpi_ = original;
    MakeFonts();
    Build(0);
    scrollY_ = 210;
    scrollX_ = 60;
    LayoutChildren();
    SCROLLINFO scroll{sizeof(scroll), SIF_POS};
    GetScrollInfo(panel_, SB_VERT, &scroll);
    check(scroll.nPos > 0, "Gallery scroll position applies");
    for (int i = 0; i < int(Pages().size()); ++i)
    {
        Build(i);
        Pump();
    }
    Build(0);
    Pump();
    const auto gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS),
               user = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    for (int cycle = 0; cycle < 8; ++cycle)
    {
        for (int i = 0; i < int(Pages().size()); ++i)
        {
            Build(i);
            Pump();
        }
        Build(0);
        Pump();
    }
    check(GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS) <= gdi + 2,
          "GDI lifetime stable across 112 page recreations");
    check(GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS) <= user + 2,
          "USER lifetime stable across 112 page recreations");
    check(SendMessageW(tooltip_, TTM_GETTOOLCOUNT, 0, 0) == 0,
          "Tooltip registrations cleaned during navigation");
    check(fatal_.empty(), "No callback errors");
    check(GetForegroundWindow() == foreground, "Foreground HWND unchanged throughout audit");
    POINT end{};
    GetCursorPos(&end);
    out << "Cursor before=" << cursor.x << ',' << cursor.y << " after=" << end.x << ',' << end.y
        << " (no cursor/input injection performed)\n";
    out << "Created controls=" << creationCount_ << "\nGUI mode="
        << (options_.visible ? "NE18NZ2 no activation; app-only PrintWindow captures"
                             : "hidden noninteractive")
        << "\nUNVERIFIED: interactive modal dialog cancellation, actual keyboard focus traversal, real DPI "
           "monitor transitions, accessibility tools, high contrast, all style permutations.\n";
    Build(12);
    if (options_.visible)
    {
        UpdateWindow(window_);
        Capture(options_.report.parent_path() / L"FinalLaunch.bmp");
        ShowWindow(window_, SW_HIDE);
    }
    return 0;
}
