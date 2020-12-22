#include "stdafx.h"
#include "CustomFrameHook.h"

#include <WinUser.h>
#include <set>

#include <boost/thread/win32/recursive_mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/tss.hpp>

static const TCHAR kOriginWndProcProp[] = L"kOriginWndProcProp";

namespace
{
	void UninstallThreadHHook(HHOOK hhook)
	{
		UnhookWindowsHookEx(hhook);
	}

	typedef std::map<HWND, CCustomFrameWndHandlerPtr>			HWNDToHandlerMapT;
	typedef std::set<CCustomFrameHook::CHookMessageFilter*>		SubThreadMsgFilterContainerT;

	static DWORD												g_main_thread_id = 0;
	static boost::thread_specific_ptr<HHOOK__>					g_thread_hhook(UninstallThreadHHook);

    // g_hwnd_handlers
    // add: main thread
    // remove: main thread
    // use: main/sub thread
    // So remove in OnFinalMessage should be safe enough.
	static HWNDToHandlerMapT									g_hwnd_handlers;
	static CCustomFrameWndHandlerPtr							g_primary_handler;
	static SubThreadMsgFilterContainerT							g_sub_thread_filter;
	static boost::recursive_mutex								g_filter_mutex;

	BOOL PreTranslateSubThreadMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		assert(GetCurrentThreadId() != g_main_thread_id);
		boost::unique_lock<boost::recursive_mutex> filter_lock(g_filter_mutex);
		if (!g_sub_thread_filter.size())
			return FALSE;

		MSG msg;
		msg.hwnd = hwnd;
		msg.message = uMsg;
		msg.wParam = wParam;
		msg.lParam = lParam;

		SubThreadMsgFilterContainerT::iterator it_filter = g_sub_thread_filter.begin();
		for (; it_filter != g_sub_thread_filter.end(); ++it_filter)
		{
			if ((*it_filter)->CrBrowserMainPreTranslateMessage(&msg))
				return TRUE;
		}

		return FALSE;
	}
}

CCustomFrameHook::CCustomFrameHook()
{
}

CCustomFrameHook::~CCustomFrameHook()
{
}

BOOL CCustomFrameHook::InstallHook()
{
	DWORD thread_id = GetCurrentThreadId();
	if (0 == g_main_thread_id)
		g_main_thread_id = thread_id;
	HHOOK hhook = SetWindowsHookEx(WH_CALLWNDPROC, 
		HookWndProcCallback, NULL, thread_id);
	g_thread_hhook.reset(hhook);
	return !!hhook;
}

CCustomFrameWndHandlerPtr CCustomFrameHook::AddTopHWndHandler(HWND hwnd)
{
	assert(GetCurrentThreadId() == g_main_thread_id || g_main_thread_id == 0);

	CCustomFrameWndHandlerPtr handler;
	HWNDToHandlerMapT::iterator it_handler = g_hwnd_handlers.find(hwnd);
	if (it_handler == g_hwnd_handlers.end())
	{
		handler = std::make_shared<CCustomFrameWndHandler>(hwnd);
		if (!g_primary_handler)
			g_primary_handler = handler;
		else
			g_hwnd_handlers.insert(std::make_pair(hwnd, handler));
	}
	else
	{
		handler = it_handler->second;
	}
	return handler;
}

void CCustomFrameHook::RemoveTopWndHandler(HWND hwnd)
{
	assert(GetCurrentThreadId() == g_main_thread_id || g_main_thread_id == 0);

	if (g_primary_handler && g_primary_handler->GetOwnerHwnd() == hwnd)
		g_primary_handler.reset();
	else
	{
		HWNDToHandlerMapT::iterator it_handler = g_hwnd_handlers.find(hwnd);
		if (it_handler != g_hwnd_handlers.end())
			g_hwnd_handlers.erase(it_handler);
	}
}

BOOL CCustomFrameHook::ShouldHookWindow(HWND hwnd)
{
	HWNDToHandlerMapT::iterator it_handler = g_hwnd_handlers.begin();
	for (; it_handler != g_hwnd_handlers.end(); ++it_handler)
	{
		if (it_handler->second->IsChildWindow(hwnd))
			return TRUE;
	}

	if (g_primary_handler && g_primary_handler->IsChildWindow(hwnd))
		return TRUE;

	return FALSE;
}

LRESULT CALLBACK CCustomFrameHook::HookWndProcCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	assert(g_main_thread_id);

	DWORD thread_id = GetCurrentThreadId();

	if (nCode == HC_ACTION)
	{
		CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;

		if (WM_CREATE == cwp->message)
		{
			// HWND which is not owned by handlers shouldn't be hooked.
			
			if (ShouldHookWindow(cwp->hwnd))
			{
				WNDPROC old_proc = (WNDPROC)::SetWindowLongPtr(
					cwp->hwnd, GWL_WNDPROC,
					reinterpret_cast<LONG_PTR>(thread_id == g_main_thread_id ? 
						MainThreadHookWndProc : SubThreadHookWndProc));
				::SetProp(cwp->hwnd, kOriginWndProcProp, reinterpret_cast<HANDLE>(old_proc));
			}
		}
	}

	return CallNextHookEx(g_thread_hhook.get(), nCode, wParam, lParam);
}

LRESULT CALLBACK CCustomFrameHook::SubThreadHookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(g_main_thread_id != GetCurrentThreadId());

	if (PreTranslateSubThreadMsg(hwnd, uMsg, wParam, lParam))
		return FALSE;

	WNDPROC old_proc = reinterpret_cast<WNDPROC>(::GetProp(hwnd, kOriginWndProcProp));

	{
		HWNDToHandlerMapT::iterator it_handler = g_hwnd_handlers.begin();
		for (; it_handler != g_hwnd_handlers.end(); ++it_handler)
		{
			if (it_handler->second->IsChildWindow(hwnd))
				return it_handler->second->SubThreadHandle(
					hwnd, uMsg, wParam, lParam, old_proc);
		}
		if (g_primary_handler && g_primary_handler->IsChildWindow(hwnd))
			return g_primary_handler->SubThreadHandle(
				hwnd, uMsg, wParam, lParam, old_proc);
	}

	return old_proc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CCustomFrameHook::MainThreadHookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(g_main_thread_id == GetCurrentThreadId());

	WNDPROC old_proc = reinterpret_cast<WNDPROC>(::GetProp(hwnd, kOriginWndProcProp));

	HWNDToHandlerMapT::iterator it_handler = g_hwnd_handlers.begin();
	for (; it_handler != g_hwnd_handlers.end(); ++it_handler)
	{
		if (it_handler->second->IsChildWindow(hwnd))
			return it_handler->second->MainThreadHandle(
				hwnd, uMsg, wParam, lParam, old_proc);
	}
	if (g_primary_handler && g_primary_handler->IsChildWindow(hwnd))
		return g_primary_handler->MainThreadHandle(
			hwnd, uMsg, wParam, lParam, old_proc);
	
	return old_proc(hwnd, uMsg, wParam, lParam);
}

BOOL CCustomFrameHook::InstallSubThreadMsgFilter(CHookMessageFilter* filter)
{
	assert(GetCurrentThreadId() == g_main_thread_id);

	boost::unique_lock<boost::recursive_mutex> filter_lock(g_filter_mutex);
	SubThreadMsgFilterContainerT::iterator it_filter = g_sub_thread_filter.find(filter);
	if (it_filter == g_sub_thread_filter.end())
	{
		g_sub_thread_filter.insert(filter);
		return TRUE;
	}
	return FALSE;
}

BOOL CCustomFrameHook::UninstallSubThreadMsgFilter(CHookMessageFilter* filter)
{
	assert(GetCurrentThreadId() == g_main_thread_id);

	boost::unique_lock<boost::recursive_mutex> filter_lock(g_filter_mutex);
	SubThreadMsgFilterContainerT::iterator it_filter = g_sub_thread_filter.find(filter);
	if (it_filter != g_sub_thread_filter.end())
	{
		g_sub_thread_filter.erase(it_filter);
		return TRUE;
	}
	return FALSE;
}
