#pragma once

#include <thread>
#include <atlwin.h>
#include <atlframe.h>

extern UINT WM_CROSS_THREAD_MSG;

enum CrossThreadMsgType
{
	kCrossThreadMsgTypeSubHWndCreated,
};

class CMainView 
	: public CWindowImpl<CMainView>
	, public CDoubleBufferImpl<CMainView>
{
public:
	DECLARE_WND_CLASS(L"CMainView")
	BEGIN_MSG_MAP(CMainView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		CHAIN_MSG_MAP(CDoubleBufferImpl<CMainView>);
	END_MSG_MAP()

	CMainView();
	~CMainView();

	void DoPaint(CDCHandle dc);
	void CreateInSubThread(HWND parent, UINT style);

protected:
	void OnFinalMessage(_In_ HWND /*hWnd*/) final;

private:
	void SubThreadFunc(HWND parent, UINT style);

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	std::shared_ptr<std::thread> m_sub_thread;
};

