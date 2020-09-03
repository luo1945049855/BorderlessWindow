#include "stdafx.h"
#include "MainFrame.h"
#include "CustomFrameHook.h"

static const int kWindowMinWidth = 500;
static const int kWindowMinHeight = 500;
static const int kCaptionHeight = 50;

CMainFrame::CMainFrame()
	: CCustomFrameWindow(kCaptionHeight)
	, m_main_view(NULL)
{
}

CMainFrame::~CMainFrame()
{
	if (m_main_view)
		delete m_main_view;
}

void CMainFrame::DoPaint(CDCHandle dc)
{
	CRect client_rect;
	GetClientRect(&client_rect);
	CBrush brush;
	brush.CreateSolidBrush(RGB(255, 0, 0));
	dc.FillRect(&client_rect, brush);
}

LRESULT CMainFrame::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CCustomFrameHook::InstallHook();

	MINMAXINFO	min_max_info;
	min_max_info.ptMaxSize.x = INT_MAX;
	min_max_info.ptMaxSize.y = INT_MAX;
	min_max_info.ptMinTrackSize.x = kWindowMinWidth;
	min_max_info.ptMinTrackSize.y = kWindowMinHeight;
	m_frame_handler->SetMinMaxInfo(min_max_info);

	m_main_view = new CMainView();
	m_main_view->CreateInSubThread(m_hWnd, WS_CHILDWINDOW | WS_VISIBLE);

	InvalidateRect(NULL, TRUE);
	UpdateLayout();

	return FALSE;
}

LRESULT CMainFrame::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return FALSE;
}

LRESULT CMainFrame::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	UpdateLayout();
	return FALSE;
}

LRESULT CMainFrame::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DestroyWindow();
	PostQuitMessage(0);
	return FALSE;
}

LRESULT CMainFrame::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LPMINMAXINFO lpMinMaxInfo = reinterpret_cast<LPMINMAXINFO>(lParam);
	MONITORINFO monitor_info = {};
	monitor_info.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &monitor_info);

	CRect rect_work = monitor_info.rcWork;
	CRect rect_monitor = monitor_info.rcMonitor;
	rect_work.OffsetRect(-rect_monitor.left, -rect_monitor.top);

	lpMinMaxInfo->ptMaxPosition.x = rect_work.left;
	lpMinMaxInfo->ptMaxPosition.y = rect_work.top;
	lpMinMaxInfo->ptMaxSize.x = rect_work.right;
	lpMinMaxInfo->ptMaxSize.y = rect_work.bottom;

	lpMinMaxInfo->ptMinTrackSize.x = kWindowMinWidth;
	lpMinMaxInfo->ptMinTrackSize.y = kWindowMinHeight;

	return FALSE;
}

LRESULT CMainFrame::OnCrossThreadMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CrossThreadMsgType msg_type = (CrossThreadMsgType)wParam;

	switch (msg_type)
	{
	case kCrossThreadMsgTypeSubHWndCreated:
	{
		CRect view_rect = GetCustomClientRect();
		::SetWindowPos(m_main_view->m_hWnd, m_hWnd, view_rect.left, view_rect.top,
			view_rect.Width(), view_rect.Height(), SWP_SHOWWINDOW);
	}
	break;
	default:
		break;
	}

	return FALSE;
}

void CMainFrame::OnFinalMessage(_In_ HWND /*hWnd*/)
{

}

void CMainFrame::UpdateLayout()
{
	if (m_main_view->m_hWnd)
	{
		CRect view_rect = GetCustomClientRect();
		::MoveWindow(m_main_view->m_hWnd, view_rect.left, view_rect.top,
			view_rect.Width(), view_rect.Height(), TRUE);
	}
}
