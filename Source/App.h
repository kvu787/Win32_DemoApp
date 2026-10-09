#pragma once
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>
struct Display
{
    RECT work{};
    RECT bounds{};
    std::wstring device;
    std::wstring name;
};
Display SelectDisplay(bool test);
struct Options
{
    bool test = false;
    bool visible = false;
    bool launcher = false;
    std::filesystem::path report;
};
class App
{
  public:
    App(HINSTANCE instance, Options options, Display display);
    ~App();
    int Run();
    int Audit();

  private:
    struct Child
    {
        HWND hwnd;
        int x, y, w, h;
    };
    HINSTANCE instance_;
    Options options_;
    Display display_;
    HWND window_ = nullptr, tree_ = nullptr, panel_ = nullptr, heading_ = nullptr, info_ = nullptr,
         doc_ = nullptr;
    HWND toolbar_ = nullptr, status_ = nullptr, log_ = nullptr, tooltip_ = nullptr, find_ = nullptr;
    HFONT font_ = nullptr, bold_ = nullptr;
    HFONT demoFont_ = nullptr;
    COLORREF demoColor_ = RGB(0, 100, 200);
    HIMAGELIST images_ = nullptr;
    HMODULE rich_ = nullptr;
    UINT dpi_ = 96, findMessage_ = 0;
    int page_ = 0, scrollX_ = 0, scrollY_ = 0, height_ = 640, split_ = 245, sortColumn_ = 0;
    bool sortingAscending_ = true, dragging_ = false, navigating_ = false, sensorExpanded_ = true;
    bool dpiGeometryPending_ = false;
    bool sensorCellMode_ = false, sensorUpdating_ = false;
    LPARAM sensorKey_ = 100;
    int sensorColumn_ = 0;
    static constexpr int VirtualRows = 1000;
    static constexpr UINT VirtualStatsMessage = WM_APP + 31;
    std::vector<int> virtualOrder_;
    int virtualSelected_ = 0, virtualSortColumn_ = 0;
    bool virtualAscending_ = true, virtualUpdating_ = false, virtualStatsPending_ = false;
    unsigned long long virtualRequests_ = 0, virtualRejected_ = 0;
    int creationCount_ = 0;
    std::vector<Child> children_;
    struct SpecimenFont
    {
        HWND hwnd;
        HFONT font;
    };
    std::vector<SpecimenFont> specimenFonts_;
    std::vector<HWND> floating_;
    std::vector<HTREEITEM> topics_;
    std::wstring fatal_, lastEvent_;
    FINDREPLACEW findData_{};
    wchar_t findText_[128] = L"demo", replaceText_[128] = L"sample";
    COLORREF customColors_[16]{};
    int S(int n) const;
    HWND Add(const wchar_t *cls, const wchar_t *text, DWORD style, int id, int x, int y, int w, int h,
             DWORD ex = 0, HWND parent = nullptr);
    void Label(const wchar_t *text, int x, int y, int w = 840, int h = 25);
    void Button(const wchar_t *text, int id, int x, int y, int w = 210, DWORD style = BS_PUSHBUTTON);
    void Build(int page);
    void Layout();
    void LayoutChildren();
    void MakeFonts();
    void RefreshPresentation(UINT dpi, const RECT *suggested = nullptr);
    void RefreshImages();
    bool UpdatePreview();
    void AcceptDemoColor(COLORREF color);
    void AcceptDemoFont(const LOGFONTW &description);
    void HandleFind(const FINDREPLACEW &request);
    bool SplitterHit(int logicalX) const;
    void ShowMain();
    void RegressionAudit(std::ostream &report);
    void SensorAudit(std::ostream &report);
    void VirtualAudit(std::ostream &report);
    std::wstring VirtualText(int id, int column) const;
    int VirtualId(int row) const;
    void VirtualBind(HWND list);
    void VirtualSelect(HWND list, int row);
    void VirtualSort(HWND list, int column);
    void VirtualLabels();
    bool VirtualNotify(NMHDR *notification, LRESULT &result);
    struct SensorCell
    {
        int row = -1, column = -1;
    };
    LPARAM SensorKey(HWND list, int row) const;
    int SensorIndex(HWND list, LPARAM key) const;
    bool SensorRect(HWND list, int row, int column, RECT &rect) const;
    SensorCell SensorHit(HWND list, POINT point) const;
    void SensorMode(bool cell);
    void SensorSelect(HWND list, int row, int column);
    void SensorRestore(HWND list);
    void SensorLabel(HWND list);
    bool SensorKeyDown(HWND list, UINT key);
    void SensorMouse(HWND list, POINT point, bool doubleClick);
    void SensorToggle(HWND list);
    LRESULT SensorDraw(NMLVCUSTOMDRAW &draw);
    static LRESULT CALLBACK SensorProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    void Event(const std::wstring &event);
    void Command(int id, int notification, HWND control);
    void Notify(NMHDR *notification);
    void OpenDialog(int id);
    void PropertyDemo();
    void FloatingDemo(int kind);
    void ContextMenu(HWND source, POINT point);
    void PopulateReport(HWND list, bool sensors);
    void Capture(const std::filesystem::path &path);
    LRESULT MainMessage(HWND, UINT, WPARAM, LPARAM);
    LRESULT PanelMessage(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK MainProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK PanelProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK SurfaceProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK FloatingProc(HWND, UINT, WPARAM, LPARAM);
};
