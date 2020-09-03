#pragma once

#include "CustomFrameWndHandler.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
/// <summary>	A custom frame hook. </summary>
///
/// <remarks>	dayo, 2019/9/29. </remarks>
////////////////////////////////////////////////////////////////////////////////////////////////////

class CCustomFrameHook
{
public:
	class CHookMessageFilter
	{
	public:
		virtual BOOL CrBrowserMainPreTranslateMessage(LPMSG pMsg) = 0;
	};

	CCustomFrameHook();
	~CCustomFrameHook();

	static BOOL InstallHook();
	static BOOL InstallSubThreadMsgFilter(CHookMessageFilter* filter);
	static BOOL UninstallSubThreadMsgFilter(CHookMessageFilter* filter);

	static CCustomFrameWndHandlerPtr AddTopHWndHandler(HWND hwnd);
	static void RemoveTopWndHandler(HWND hwnd);
	static BOOL ShouldHookWindow(HWND hwnd);

	static LRESULT CALLBACK HookWndProcCallback(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK MainThreadHookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK SubThreadHookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
