#pragma once

#include <atlwin.h>

#include "CustomFrameWindow.h"
#include "MainView.h"

class CMainFrame
	: public CWindowImpl<CMainFrame>
	, public CCustomFrameWindow<CMainFrame>
	, public CDoubleBufferImpl<CMainFrame>
{
public:
	DECLARE_WND_CLASS(L"CMainFrame")
	BEGIN_MSG_MAP(CMainFrame)
		CCustomFrameWindow::ProcessWindowMessage(
			hWnd, uMsg, wParam, lParam, lResult, bHandled);
		if (bHandled)
			return TRUE;

		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
		MESSAGE_HANDLER(WM_CROSS_THREAD_MSG, OnCrossThreadMsg)
		CHAIN_MSG_MAP(CDoubleBufferImpl<CMainFrame>)

		REFLECT_NOTIFICATIONS();
		DEFAULT_REFLECTION_HANDLER();
	END_MSG_MAP()

	CMainFrame();
	~CMainFrame();

	void DoPaint(CDCHandle /*dc*/);

protected:
	void OnFinalMessage(_In_ HWND /*hWnd*/) final;

private:
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCrossThreadMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	void UpdateLayout();

	CMainView*	m_main_view;
};

