#include "stdafx.h"
#include "MainView.h"
#include "CustomFrameHook.h"

UINT WM_CROSS_THREAD_MSG = ::RegisterWindowMessage(L"WM_CROSS_THREAD_MSG");

CMainView::CMainView()
{
}


CMainView::~CMainView()
{
	if (m_sub_thread && m_sub_thread->joinable())
		m_sub_thread->join();
}

void CMainView::DoPaint(CDCHandle dc)
{
	CRect client_rect;
	GetClientRect(&client_rect);

	CBrush brush;
	brush.CreateSolidBrush(RGB(0, 255, 0));
	dc.FillRect(&client_rect, brush);
}


void CMainView::CreateInSubThread(HWND parent, UINT style)
{
	m_sub_thread = std::make_shared<std::thread>(
		std::bind(&CMainView::SubThreadFunc, this, parent, style));
}

void CMainView::SubThreadFunc(HWND parent, UINT style)
{
	CCustomFrameHook::InstallHook();
	HWND hwnd = Create(parent, NULL, NULL, style);
	CMessageLoop message_loop;
	message_loop.Run();
}

LRESULT CMainView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::PostMessage(GetParent(), WM_CROSS_THREAD_MSG, kCrossThreadMsgTypeSubHWndCreated, (LPARAM)m_hWnd);

	return FALSE;
}

LRESULT CMainView::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return FALSE;
}

LRESULT CMainView::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return FALSE;
}

void CMainView::OnFinalMessage(_In_ HWND /*hWnd*/)
{
	::PostQuitMessage(0);
}
