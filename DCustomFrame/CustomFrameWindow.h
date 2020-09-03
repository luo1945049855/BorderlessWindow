#pragma once

#include <Uxtheme.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <memory>

#include "CustomFrameHook.h"

#define NON_CLIENT_BAND 1

////////////////////////////////////////////////////////////////////////////////////////////////////
/// <summary>	A cdwmapi. </summary>
///
/// <remarks>	dayo, 2019/8/30. </remarks>
////////////////////////////////////////////////////////////////////////////////////////////////////

class CDWMAPIInvoker
{
public:
    /// dwmapi.dll
    typedef BOOL(WINAPI *pFuncDwmDefWindowProc)(HWND hwnd, UINT msg,
                                                WPARAM wParam, LPARAM lParam,
                                                LRESULT *plResult);
    typedef HRESULT(WINAPI *pFuncDwmIsCompositionEnabled)(BOOL *pfEnabled);
    typedef HRESULT(WINAPI *pFuncDwmExtendFrameIntoClientArea)(
        HWND hwnd, const MARGINS *pMarInset);
    typedef HRESULT(WINAPI *pFuncDwmGetWindowAttribute)(HWND hwnd,
                                                        DWORD dwAttribute,
                                                        void *pvAttribute,
                                                        DWORD cbAttribute);
    typedef HRESULT(WINAPI *pFuncDwmSetWindowAttribute)(HWND hwnd,
                                                        DWORD dwAttribute,
                                                        LPCVOID pvAttribute,
                                                        DWORD cbAttribute);

    CDWMAPIInvoker();

    HMODULE LoadSystemLibrary(const WCHAR *dll_name);

    template <typename FuncType>
    FuncType LoadAPI(HMODULE module, const char *func_name)
    {
        return (FuncType)GetProcAddress(module, func_name);
    }

    BOOL IsCompositionEnabled();
    BOOL DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                       LRESULT *plResult);
    HRESULT ExtendFrameIntoClientArea(HWND hwnd, const MARGINS *pMarInset);
    HRESULT GetWindowAttribute(HWND hwnd, DWORD dwAttribute, void *pvAttribute,
                               DWORD cbAttribute);
    HRESULT SetWindowAttribute(HWND hwnd, DWORD dwAttribute,
                               LPCVOID pvAttribute, DWORD cbAttribute);

private:
    pFuncDwmDefWindowProc m_pDwmDefWindowProc;
    pFuncDwmIsCompositionEnabled m_pDwmIsCompositionEnabled;
    pFuncDwmExtendFrameIntoClientArea m_pDwmExtendFrameIntoClientArea;
    pFuncDwmGetWindowAttribute m_pDwmGetWindowAttribute;
    pFuncDwmSetWindowAttribute m_pDwmSetWindowAttribute;
};
extern std::unique_ptr<CDWMAPIInvoker> gDWMAPIInvoker;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// <summary>	Form for viewing the c custom frame. </summary>
///
/// <remarks>	dayo, 2019/8/30. </remarks>
////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> class CCustomFrameWindow
{
public:
    // we cannot just use WS_POPUP style
    // WS_THICKFRAME: without this the window cannot be resized and so aero
    // snap, de-maximizing and minimizing won't work WS_SYSMENU: enables the
    // context menu with the move, close, maximize, minize... commands (shift +
    // right-click on the task bar item) WS_CAPTION: enables aero minimize
    // animation/transition WS_MAXIMIZEBOX, WS_MINIMIZEBOX: enable
    // minimize/maximize

    enum Style
    {
        kStyleWindow = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION |
                       WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                       WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        kStyleAeroBorderless = WS_POPUP | WS_THICKFRAME | WS_CAPTION |
                               WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX |
                               WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        kStyleBasicBorderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU |
                                WS_MAXIMIZEBOX | WS_MINIMIZEBOX |
                                WS_CLIPCHILDREN | WS_CLIPSIBLINGS
    };

    inline HWND SelfWnd() { return static_cast<T *>(this)->m_hWnd; }

    CCustomFrameWindow(int caption_height)
        : m_trace_mouse_event(FALSE), m_caption_height(caption_height)
    {
    }

    void ProcessWindowMessage(_In_ HWND hWnd, _In_ UINT uMsg,
                              _In_ WPARAM wParam, _In_ LPARAM lParam,
                              _Inout_ LRESULT &lResult, _Inout_ BOOL &bHandled)
    {
        if (gDWMAPIInvoker->DefWindowProc(hWnd, uMsg, wParam, lParam, &lResult))
        {
            bHandled = TRUE;
            return;
        }

        bHandled = FALSE;

        switch (uMsg)
        {
        case WM_CREATE:
            lResult = OnCreate(uMsg, wParam, lParam, bHandled);
            break;
        case WM_SIZE:
            lResult = OnSize(uMsg, wParam, lParam, bHandled);
            break;
        case WM_DESTROY:
            lResult = OnDestroy(uMsg, wParam, lParam, bHandled);
            break;
        case WM_WINDOWPOSCHANGED:
            lResult = OnWndPosChanged(uMsg, wParam, lParam, bHandled);
            break;
        case WM_NCACTIVATE:
            lResult = OnNCActivate(uMsg, wParam, lParam, bHandled);
            break;
        case WM_NCCALCSIZE:
            lResult = OnNCCalSize(uMsg, wParam, lParam, bHandled);
            break;
        case WM_NCPAINT:
            lResult = OnNCPaint(uMsg, wParam, lParam, bHandled);
            break;
        case WM_NCHITTEST:
            lResult = OnNCHitTest(uMsg, wParam, lParam, bHandled);
            break;
        case WM_ERASEBKGND:
            lResult = OnEraseBkGnd(uMsg, wParam, lParam, bHandled);
            break;
        default:
            break;
        }
    }

private:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (gDWMAPIInvoker->IsCompositionEnabled())
            ::SetWindowLongPtr(SelfWnd(), GWL_STYLE,
                               static_cast<LONG>(kStyleAeroBorderless));
        else
            ::SetWindowLongPtr(SelfWnd(), GWL_STYLE,
                               static_cast<LONG>(kStyleBasicBorderless));

        if (gDWMAPIInvoker->IsCompositionEnabled())
        {
            MARGINS margins = {1, 1, 1, 1};
            gDWMAPIInvoker->ExtendFrameIntoClientArea(SelfWnd(), &margins);
        }

        m_frame_handler = CCustomFrameHook::AddTopHWndHandler(SelfWnd());
        UpdateCustomFrame();

        return FALSE;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        UpdateCustomFrame();
        return FALSE;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        m_frame_handler.reset();
        CCustomFrameHook::RemoveTopWndHandler(SelfWnd());
        return FALSE;
    }

    LRESULT OnWndPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam,
                            BOOL &bHandled)
    {
        // may be triggered before m_frame_handler being created.
        if (!m_frame_handler)
            return FALSE;
        LPWINDOWPOS window_pos = (LPWINDOWPOS)lParam;
        CRect new_window_rect =
            CRect(window_pos->x, window_pos->y, window_pos->x + window_pos->cx,
                  window_pos->y + window_pos->cy);
        if (!m_pre_window_rect.IsRectEmpty() &&
            m_pre_window_rect == new_window_rect)
            return FALSE;
        m_pre_window_rect = new_window_rect;

        CRect rect_window;
        ::GetClientRect(SelfWnd(), rect_window);
        ::MapWindowPoints(SelfWnd(), HWND_DESKTOP, (LPPOINT)&rect_window, 2);

        int frame_thickness = GetFrameThickness();
        CRect top_frame, left_frame, right_frame, bottom_frame;
        top_frame = CRect(rect_window.left, rect_window.top, rect_window.right,
                          rect_window.top + m_caption_height);
        if (!::IsZoomed(SelfWnd()))
        {
            top_frame =
                CRect(rect_window.left, rect_window.top, rect_window.right,
                      rect_window.top + max(m_caption_height, frame_thickness));
            left_frame =
                CRect(rect_window.left, rect_window.top,
                      rect_window.left + frame_thickness, rect_window.bottom);
            right_frame =
                CRect(rect_window.right - frame_thickness, rect_window.top,
                      rect_window.right, rect_window.bottom);
            bottom_frame =
                CRect(rect_window.left, rect_window.bottom - frame_thickness,
                      rect_window.right, rect_window.bottom);
        }
        m_frame_handler->SetFrameRects(left_frame, top_frame, right_frame,
                                       bottom_frame);

        return FALSE;
    }

    LRESULT OnNCActivate(UINT uMsg, WPARAM wParam, LPARAM lParam,
                         BOOL &bHandled)
    {
        if (!gDWMAPIInvoker->IsCompositionEnabled())
        {
            // Prevents window frame reappearing on window activation
            // in "basic" theme, where no aero shadow is present.
            bHandled = TRUE;
            return TRUE;
        }

        // In Windows8, this needs the default process.

        return FALSE;
    }

    LRESULT OnNCCalSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        // In order to have custom caption, we have to include its area in the
        // client rectangle. Let DefWindowProc calculate the client rectangle.

        RECT *r = wParam == TRUE ? &((NCCALCSIZE_PARAMS *)lParam)->rgrc[0]
                                 : (RECT *)lParam;
        CRect rect_window = *r;

        if (::IsZoomed(SelfWnd()))
        {
            if (gDWMAPIInvoker->IsCompositionEnabled())
            {
                int frame_thickness = GetFrameThickness();
                rect_window.InflateRect(-frame_thickness, -frame_thickness);
            }
            else
            {
                CRect rect_work, rect_monitor;
                MonitorRectFromWindow(SelfWnd(), rect_work, rect_monitor);
                rect_window = rect_work;
            }

            if (NeedsNonClientBandHack(SelfWnd()))
            {
                rect_window.bottom -= NON_CLIENT_BAND;
            }
        }

        *r = rect_window;

        bHandled = TRUE;
        return FALSE;
    }

    LRESULT OnNCPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (!gDWMAPIInvoker->IsCompositionEnabled())  // Needed in xp.
            bHandled = TRUE;
        return FALSE;
    }

    LRESULT OnNCHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        bHandled = TRUE;
        CRect window_rect;
        CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        ::GetWindowRect(SelfWnd(), window_rect);
        if (!window_rect.PtInRect(pt))
            return HTNOWHERE;

        enum RegionMask
        {
            kRegionMaskClient = 0x0000,
            kRegionMaskLeft = 0x0001,
            kRegionMaskTop = kRegionMaskLeft << 1,
            kRegionMaskRight = kRegionMaskLeft << 2,
            kRegionMaskBottom = kRegionMaskLeft << 3,
        };

        UINT result = 0;
        if (IsResizable())
        {
            int frame_thickness = GetFrameThickness();
            result =
                kRegionMaskLeft *
                    (pt.x < (window_rect.left + frame_thickness)) |
                kRegionMaskTop * (pt.y < (window_rect.top + frame_thickness)) |
                kRegionMaskRight *
                    (pt.x >= (window_rect.right - frame_thickness)) |
                kRegionMaskBottom *
                    (pt.y >= (window_rect.bottom - frame_thickness));
        }

        switch (result)
        {
        case kRegionMaskLeft:
            return HTLEFT;
            break;
        case kRegionMaskTop:
            return HTTOP;
            break;
        case kRegionMaskRight:
            return HTRIGHT;
            break;
        case kRegionMaskBottom:
            return HTBOTTOM;
            break;
        case kRegionMaskLeft | kRegionMaskTop:
            return HTTOPLEFT;
            break;
        case kRegionMaskTop | kRegionMaskRight:
            return HTTOPRIGHT;
            break;
        case kRegionMaskRight | kRegionMaskBottom:
            return HTBOTTOMRIGHT;
            break;
        case kRegionMaskBottom | kRegionMaskLeft:
            return HTBOTTOMLEFT;
            break;
        case kRegionMaskClient:
        {
            ::GetClientRect(SelfWnd(), window_rect);
            ::MapWindowPoints(SelfWnd(), HWND_DESKTOP, (LPPOINT)&window_rect,
                              2);
            if (pt.y < window_rect.top + m_caption_height)
                return HTCAPTION;
            return HTCLIENT;
        }
        break;
        }
        return HTNOWHERE;
    }

    LRESULT OnEraseBkGnd(UINT uMsg, WPARAM wParam, LPARAM lParam,
                         BOOL &bHandled)
    {
        bHandled = TRUE;  // Needed to prevent resize flicker.
        return TRUE;
    }

    void UpdateCustomFrame()
    {
        CRect client_rect;
        ::GetClientRect(SelfWnd(), client_rect);
        m_custom_caption_rect = client_rect;
        m_custom_caption_rect.bottom =
            m_custom_caption_rect.top + m_caption_height;
        m_custom_client_rect = client_rect;
        m_custom_client_rect.top = m_custom_caption_rect.bottom;
    }

protected:
    inline CRect GetCustomCaptionRect() { return m_custom_caption_rect; }
    inline CRect GetCustomClientRect() { return m_custom_client_rect; }

    void RemoveResizable()
    {
        LONG_PTR style = ::GetWindowLongPtr(SelfWnd(), GWL_STYLE);
        LONG_PTR remove_style =
            (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
        if (style & remove_style)
            ::SetWindowLongPtr(SelfWnd(), GWL_STYLE, style & (~remove_style));
    }

    void AddResizable()
    {
        LONG_PTR style = ::GetWindowLongPtr(SelfWnd(), GWL_STYLE);
        LONG_PTR add_style = (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
        ::SetWindowLongPtr(SelfWnd(), GWL_STYLE, style | add_style);
    }

    BOOL IsResizable()
    {
        return (::GetWindowLongPtr(SelfWnd(), GWL_STYLE) & WS_THICKFRAME);
    }

    BOOL IsDraggable()
    {
        return (::GetWindowLongPtr(SelfWnd(), GWL_STYLE) & WS_CAPTION);
    }

    void SetCaptionHeight(int caption_height)
    {
        m_caption_height = caption_height;
    }
    int GetCaptionHeight() { return m_caption_height; }

    int GetFrameThickness()
    {
        return ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
    }

    int GetBorderThickness()
    {
        return ::GetSystemMetrics(SM_CXBORDER);
    }

    static inline bool NeedsNonClientBandHack(HWND hwnd)
    {
        return IsZoomed(hwnd);
    }

    static BOOL MonitorRectFromWindow(HWND hwnd, CRect &rect_work,
                                      CRect &rect_monitor)
    {
        rect_monitor =
            CRect(CPoint(0, 0), CSize(::GetSystemMetrics(SM_CXSCREEN),
                                      ::GetSystemMetrics(SM_CYSCREEN)));
        rect_work = rect_monitor;

        HMODULE hUser32 = ::GetModuleHandleA("USER32.DLL");
        if (hUser32 != NULL)
        {
            typedef HMONITOR(WINAPI * FN_MonitorFromWindow)(HWND hWnd,
                                                            DWORD dwFlags);
            typedef BOOL(WINAPI * FN_GetMonitorInfo)(HMONITOR hMonitor,
                                                     LPMONITORINFO lpmi);
            FN_MonitorFromWindow pfnMonitorFromWindow =
                (FN_MonitorFromWindow)::GetProcAddress(hUser32,
                                                       "MonitorFromWindow");
            FN_GetMonitorInfo pfnGetMonitorInfo =
                (FN_GetMonitorInfo)::GetProcAddress(hUser32, "GetMonitorInfoA");
            if (pfnMonitorFromWindow != NULL && pfnGetMonitorInfo != NULL)
            {
                MONITORINFO mi;
                HMONITOR hMonitor =
                    pfnMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                mi.cbSize = sizeof(mi);
                pfnGetMonitorInfo(hMonitor, &mi);
                rect_monitor = mi.rcMonitor;
                rect_work = mi.rcWork;
                return TRUE;
            }
        }
        return FALSE;
    }

    int m_caption_height;
    BOOL m_trace_mouse_event;
    CRect m_custom_caption_rect;
    CRect m_custom_client_rect;
    CRect m_pre_window_rect;
    CCustomFrameWndHandlerPtr m_frame_handler;
};