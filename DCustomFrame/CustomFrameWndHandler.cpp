#include "stdafx.h"
#include "CustomFrameWndHandler.h"

#include <set>
#include <boost/thread/mutex.hpp>

typedef std::set<HWND> ExcludeHWndContainerT;

struct CCustomFrameWndHandlerPrivate
{
	CCustomFrameWndHandlerPrivate(HWND owner_hwnd)
		: m_owner_hwnd(owner_hwnd)
	{}

	int							m_dragging_count = 0;
	LONG						m_dragging_hit_test = HTERROR;
	CPoint						m_dragging_start_pt;
	CRect						m_dragging_start_rect;
	HWND						m_owner_hwnd;
	HWND						m_dragging_hwnd = NULL;
	HRGN						m_temp_rgn = NULL;
	HRGN						m_frame_rgn = NULL;
	boost::mutex				m_rgn_mutex;
	MINMAXINFO					m_min_max_info;
	ExcludeHWndContainerT		m_exclude_hwnds;
};

CCustomFrameWndHandler::CCustomFrameWndHandler(HWND owner_hwnd)
	: m_private(new CCustomFrameWndHandlerPrivate(owner_hwnd))
{
}

CCustomFrameWndHandler::~CCustomFrameWndHandler()
{
	if (m_private->m_temp_rgn)
		::DeleteObject(m_private->m_temp_rgn);
	if (m_private->m_frame_rgn)
		::DeleteObject(m_private->m_frame_rgn);
	delete m_private;
}

MINMAXINFO CCustomFrameWndHandler::GetMinMaxInfo()
{
	return m_private->m_min_max_info;
}

HWND CCustomFrameWndHandler::GetOwnerHwnd()
{
	return m_private->m_owner_hwnd;
}

void CCustomFrameWndHandler::SetMinMaxInfo(MINMAXINFO info)
{
	m_private->m_min_max_info = info;
}

void CCustomFrameWndHandler::SetFrameRects( 
	const CRect& left_frame_rect, const CRect& top_frame_rect, 
	const CRect& right_frame_rect, const CRect& bottom_frame_rect)
{
	if (!m_private->m_temp_rgn || !m_private->m_frame_rgn)
	{
		m_private->m_temp_rgn = ::CreateRectRgn(0, 0, 0, 0);
		m_private->m_frame_rgn = ::CreateRectRgn(0, 0, 0, 0);
	}

	::SetRectRgn(m_private->m_temp_rgn, 0, 0, 0, 0);
	HRGN border_rgn = ::CreateRectRgn(0, 0, 0, 0);

	if (!left_frame_rect.IsRectEmpty())
	{
		::SetRectRgn(border_rgn, left_frame_rect.left, left_frame_rect.top,
			left_frame_rect.right, left_frame_rect.bottom);
		::CombineRgn(m_private->m_temp_rgn, m_private->m_temp_rgn, border_rgn, RGN_OR);
	}

	if (!top_frame_rect.IsRectEmpty())
	{
		::SetRectRgn(border_rgn, top_frame_rect.left, top_frame_rect.top,
			top_frame_rect.right, top_frame_rect.bottom);
		::CombineRgn(m_private->m_temp_rgn, m_private->m_temp_rgn, border_rgn, RGN_OR);
	}

	if (!right_frame_rect.IsRectEmpty())
	{
		::SetRectRgn(border_rgn, right_frame_rect.left, right_frame_rect.top,
			right_frame_rect.right, right_frame_rect.bottom);
		::CombineRgn(m_private->m_temp_rgn, m_private->m_temp_rgn, border_rgn, RGN_OR);
	}

	if (!bottom_frame_rect.IsRectEmpty())
	{
		::SetRectRgn(border_rgn, bottom_frame_rect.left, bottom_frame_rect.top,
			bottom_frame_rect.right, bottom_frame_rect.bottom);
		::CombineRgn(m_private->m_temp_rgn, m_private->m_temp_rgn, border_rgn, RGN_OR);
	}

	::DeleteObject(border_rgn);
	boost::unique_lock<boost::mutex> lock(m_private->m_rgn_mutex);
	std::swap(m_private->m_temp_rgn, m_private->m_frame_rgn);
}

void CCustomFrameWndHandler::ExcludeHWnd(HWND hwnd)
{
	m_private->m_exclude_hwnds.insert(hwnd);
}

LRESULT CCustomFrameWndHandler::SubThreadHandle(HWND hwnd, 
	UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC original_proc)
{
	if (!IsHandlable(hwnd))
		return original_proc(hwnd, uMsg, wParam, lParam);

	if (WM_MOUSEMOVE == uMsg)
	{
		CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		::ClientToScreen(hwnd, &pt);

		if (HTERROR != m_private->m_dragging_hit_test)
		{
			SetWindowPosByMouseMove(pt);
			return FALSE;
		}
		else if (ScreenPtInFrameBorder(pt))
		{
			SetCursorByPos(NULL, pt);
			return FALSE;
		}
	}
	else if (WM_SETCURSOR == uMsg)
	{
		// return TRUE to halt further processing or FALSE to continue.
		if (HTERROR != m_private->m_dragging_hit_test)
		{
			SetCursorByHit(0, m_private->m_dragging_hit_test);
			return TRUE;
		}
		else
		{
			DWORD dw = ::GetMessagePos();
			POINT pt = { GET_X_LPARAM(dw), GET_Y_LPARAM(dw) };
			if (ScreenPtInFrameBorder(pt))
			{
				SetCursorByPos(0, pt);
				return TRUE;
			}
		}
	}
	else if (WM_LBUTTONDOWN == uMsg)
	{
		CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		::ClientToScreen(hwnd, &pt);
		if (ScreenPtInFrameBorder(pt))
		{
			LONG hit = HTERROR;
			if (SetCursorByPos(NULL, pt, &hit))
			{
				::SetCapture(hwnd);
				m_private->m_dragging_hwnd = hwnd;
				m_private->m_dragging_hit_test = hit;
				m_private->m_dragging_start_pt = pt;
				::GetWindowRect(m_private->m_owner_hwnd, 
					&m_private->m_dragging_start_rect);
				return FALSE;
			}
		}
	}
	else if (WM_LBUTTONUP == uMsg)
	{
		if (HTERROR != m_private->m_dragging_hit_test)
		{
			::ReleaseCapture();
			m_private->m_dragging_hwnd = NULL;
			m_private->m_dragging_hit_test = HTERROR;
			m_private->m_dragging_start_pt.x = 
				m_private->m_dragging_start_pt.y = 0;
			return FALSE;
		}
	}

	return original_proc(hwnd, uMsg, wParam, lParam);
}

void CCustomFrameWndHandler::SetWindowPosByMouseMove(CPoint pt)
{
	// Direct return let subthread message loop has chance to do some works,
	// for example: ui update.

	++m_private->m_dragging_count;
	m_private->m_dragging_count = m_private->m_dragging_count % 5; 	
	if (0 == m_private->m_dragging_count)
		return;

	CRect origin_top_wnd_rect, new_top_wnd_rect;
	::GetWindowRect(m_private->m_owner_hwnd, origin_top_wnd_rect);
	new_top_wnd_rect = origin_top_wnd_rect;

	switch (m_private->m_dragging_hit_test)
	{
	case HTLEFT:
	{
		new_top_wnd_rect.left = pt.x;
		if (new_top_wnd_rect.Width() < m_private->m_min_max_info.ptMinTrackSize.x)
			new_top_wnd_rect.left = new_top_wnd_rect.right - m_private->m_min_max_info.ptMinTrackSize.x;
		if (new_top_wnd_rect.Width() > m_private->m_min_max_info.ptMaxSize.x)
			new_top_wnd_rect.left = new_top_wnd_rect.right - m_private->m_min_max_info.ptMaxSize.x;
	}
	break;
	case HTRIGHT:
	{
		new_top_wnd_rect.right = pt.x;
		if (new_top_wnd_rect.Width() < m_private->m_min_max_info.ptMinTrackSize.x)
			new_top_wnd_rect.right = new_top_wnd_rect.left + m_private->m_min_max_info.ptMinTrackSize.x;
		if (new_top_wnd_rect.Width() > m_private->m_min_max_info.ptMaxSize.x)
			new_top_wnd_rect.right = new_top_wnd_rect.left + m_private->m_min_max_info.ptMaxSize.x;
	}
	break;
	case HTTOP:
	{	
		new_top_wnd_rect.top = pt.y;
		if (new_top_wnd_rect.Height() < m_private->m_min_max_info.ptMinTrackSize.y)
			new_top_wnd_rect.top = new_top_wnd_rect.bottom - m_private->m_min_max_info.ptMinTrackSize.y;
		if (new_top_wnd_rect.Height() > m_private->m_min_max_info.ptMaxSize.y)
			new_top_wnd_rect.top = new_top_wnd_rect.bottom - m_private->m_min_max_info.ptMaxSize.y;
	}
	break;
	case HTBOTTOM:
	{
		new_top_wnd_rect.bottom = pt.y;
		if (new_top_wnd_rect.Height() < m_private->m_min_max_info.ptMinTrackSize.y)
			new_top_wnd_rect.bottom = new_top_wnd_rect.top + m_private->m_min_max_info.ptMinTrackSize.y;
		if (new_top_wnd_rect.Height() > m_private->m_min_max_info.ptMaxSize.y)
			new_top_wnd_rect.bottom = new_top_wnd_rect.top + m_private->m_min_max_info.ptMaxSize.y;
	}
	break;
	case HTTOPLEFT:
	{
		new_top_wnd_rect.left = pt.x;
		new_top_wnd_rect.top = pt.y;
		if (new_top_wnd_rect.Height() < m_private->m_min_max_info.ptMinTrackSize.y)
			new_top_wnd_rect.top = new_top_wnd_rect.bottom - m_private->m_min_max_info.ptMinTrackSize.y;
		if (new_top_wnd_rect.Height() > m_private->m_min_max_info.ptMaxSize.y)
			new_top_wnd_rect.top = new_top_wnd_rect.bottom - m_private->m_min_max_info.ptMaxSize.y;
		if (new_top_wnd_rect.Width() < m_private->m_min_max_info.ptMinTrackSize.x)
			new_top_wnd_rect.left = new_top_wnd_rect.right - m_private->m_min_max_info.ptMinTrackSize.x;
		if (new_top_wnd_rect.Width() > m_private->m_min_max_info.ptMaxSize.x)
			new_top_wnd_rect.left = new_top_wnd_rect.right - m_private->m_min_max_info.ptMaxSize.x;
	}
	break;
	case HTBOTTOMRIGHT:
	{
		new_top_wnd_rect.right = pt.x;
		new_top_wnd_rect.bottom = pt.y;
		if (new_top_wnd_rect.Height() < m_private->m_min_max_info.ptMinTrackSize.y)
			new_top_wnd_rect.bottom = new_top_wnd_rect.top + m_private->m_min_max_info.ptMinTrackSize.y;
		if (new_top_wnd_rect.Height() > m_private->m_min_max_info.ptMaxSize.y)
			new_top_wnd_rect.bottom = new_top_wnd_rect.top + m_private->m_min_max_info.ptMaxSize.y;
		if (new_top_wnd_rect.Width() < m_private->m_min_max_info.ptMinTrackSize.x)
			new_top_wnd_rect.right = new_top_wnd_rect.left + m_private->m_min_max_info.ptMinTrackSize.x;
		if (new_top_wnd_rect.Width() > m_private->m_min_max_info.ptMaxSize.x)
			new_top_wnd_rect.right = new_top_wnd_rect.left + m_private->m_min_max_info.ptMaxSize.x;
	}
	break;
	case HTTOPRIGHT:
	{
		new_top_wnd_rect.right = pt.x;
		new_top_wnd_rect.top = pt.y;
		if (new_top_wnd_rect.Height() < m_private->m_min_max_info.ptMinTrackSize.y)
			new_top_wnd_rect.top = new_top_wnd_rect.bottom - m_private->m_min_max_info.ptMinTrackSize.y;
		if (new_top_wnd_rect.Height() > m_private->m_min_max_info.ptMaxSize.y)
			new_top_wnd_rect.top = new_top_wnd_rect.bottom - m_private->m_min_max_info.ptMaxSize.y;
		if (new_top_wnd_rect.Width() < m_private->m_min_max_info.ptMinTrackSize.x)
			new_top_wnd_rect.right = new_top_wnd_rect.left + m_private->m_min_max_info.ptMinTrackSize.x;
		if (new_top_wnd_rect.Width() > m_private->m_min_max_info.ptMaxSize.x)
			new_top_wnd_rect.right = new_top_wnd_rect.left + m_private->m_min_max_info.ptMaxSize.x;
	}
	break;
	case HTBOTTOMLEFT:
	{	
		new_top_wnd_rect.left = pt.x;
		new_top_wnd_rect.bottom = pt.y;
		if (new_top_wnd_rect.Height() < m_private->m_min_max_info.ptMinTrackSize.y)
			new_top_wnd_rect.bottom = new_top_wnd_rect.top + m_private->m_min_max_info.ptMinTrackSize.y;
		if (new_top_wnd_rect.Height() > m_private->m_min_max_info.ptMaxSize.y)
			new_top_wnd_rect.bottom = new_top_wnd_rect.top + m_private->m_min_max_info.ptMaxSize.y;
		if (new_top_wnd_rect.Width() < m_private->m_min_max_info.ptMinTrackSize.x)
			new_top_wnd_rect.left = new_top_wnd_rect.right - m_private->m_min_max_info.ptMinTrackSize.x;
		if (new_top_wnd_rect.Width() > m_private->m_min_max_info.ptMaxSize.x)
			new_top_wnd_rect.left = new_top_wnd_rect.right - m_private->m_min_max_info.ptMaxSize.x;
	}
	break;
	case HTCAPTION:
	{
		CPoint offset = pt - m_private->m_dragging_start_pt;
		new_top_wnd_rect = m_private->m_dragging_start_rect;
		new_top_wnd_rect.OffsetRect(offset);
	}
	break;
	}

	if (new_top_wnd_rect != origin_top_wnd_rect)
	{
		::SetWindowPos(m_private->m_owner_hwnd, NULL, 
			new_top_wnd_rect.left, new_top_wnd_rect.top,
			new_top_wnd_rect.Width(), new_top_wnd_rect.Height(), SWP_NOZORDER);
	}
}

LRESULT CCustomFrameWndHandler::MainThreadHandle(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC original_proc)
{
	if (!IsHandlable(hwnd))
		return original_proc(hwnd, uMsg, wParam, lParam);

	if (WM_NCHITTEST == uMsg)
	{
		// no lock needed here.
		CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		if (::PtInRegion(m_private->m_frame_rgn, pt.x, pt.y))
		{
			return HTTRANSPARENT;
		}
	}

	return original_proc(hwnd, uMsg, wParam, lParam);
}

BOOL CCustomFrameWndHandler::IsChildWindow(HWND hwnd)
{
	do 
	{
		if (hwnd == m_private->m_owner_hwnd)
			return TRUE;
		// popup window should be toplevel.
		if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_POPUP)
			return FALSE;
	} while (hwnd = ::GetParent(hwnd));

	return FALSE;
}

BOOL CCustomFrameWndHandler::IsHandlable(HWND hwnd)
{
	if (m_private->m_owner_hwnd == hwnd)
		return FALSE;
	if (m_private->m_exclude_hwnds.find(hwnd) != m_private->m_exclude_hwnds.end())
		return FALSE;
	return TRUE;
}

BOOL CCustomFrameWndHandler::ScreenPtInFrameBorder(CPoint pt)
{
	boost::unique_lock<boost::mutex> lock(m_private->m_rgn_mutex);
	return ::PtInRegion(m_private->m_frame_rgn, pt.x, pt.y);
}

BOOL CCustomFrameWndHandler::SetCursorByPos(HINSTANCE instance, CPoint pt, LONG* hit)
{
	LONG owner_hit = ::SendMessage(m_private->m_owner_hwnd, 
		WM_NCHITTEST, NULL, MAKELPARAM(pt.x, pt.y));
	if (hit) *hit = owner_hit;
	if (IsHitHandlable(owner_hit))
	{
		SetCursorByHit(instance, owner_hit);
		return TRUE; 
	}
	return FALSE;
}

BOOL CCustomFrameWndHandler::IsHitHandlable(LONG hit)
{
	return (HTCAPTION == hit) || (HTLEFT <= hit && hit <= HTBOTTOMRIGHT);
}

LRESULT CCustomFrameWndHandler::SetCursorByHit(HINSTANCE instance, LONG hit)
{
	switch (hit)
	{
	case HTLEFT:
	case HTRIGHT:
		return (LRESULT)SetCursor(LoadCursorA(instance, (LPSTR)IDC_SIZEWE));
	case HTTOP:
	case HTBOTTOM:
		return (LRESULT)SetCursor(LoadCursorA(instance, (LPSTR)IDC_SIZENS));
	case HTTOPLEFT:
	case HTBOTTOMRIGHT:
		return (LRESULT)SetCursor(LoadCursorA(instance, (LPSTR)IDC_SIZENWSE));
	case HTTOPRIGHT:
	case HTBOTTOMLEFT:
		return (LRESULT)SetCursor(LoadCursorA(instance, (LPSTR)IDC_SIZENESW));
	case HTCLIENT:
	case HTERROR:
		break;
	}
	return FALSE;
}
