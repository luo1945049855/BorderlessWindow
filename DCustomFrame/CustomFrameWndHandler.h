#pragma once

#include <memory>

struct CCustomFrameWndHandlerPrivate;

class CCustomFrameWndHandler
{
public:
	friend class CCustomFrameHook;

	CCustomFrameWndHandler(HWND owner_hwnd);
	~CCustomFrameWndHandler();

	void SetFrameRects(
		const CRect& left_frame_rect, const CRect& top_frame_rect,
		const CRect& right_frame_rect, const CRect& bottom_frame_rect);
	void ExcludeHWnd(HWND hwnd);
	void SetMinMaxInfo(MINMAXINFO info);
	MINMAXINFO GetMinMaxInfo();
	HWND GetOwnerHwnd();

private:
	LRESULT SubThreadHandle(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC original_proc);
	LRESULT MainThreadHandle(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC original_proc);

	BOOL IsChildWindow(HWND hwnd);
	BOOL IsHandlable(HWND hwnd);
	BOOL IsHitHandlable(LONG hit);
	BOOL ScreenPtInFrameBorder(CPoint pt);
	BOOL SetCursorByPos(HINSTANCE instance, CPoint pt, LONG* hit = NULL);
	LRESULT SetCursorByHit(HINSTANCE instance, LONG hit);
	void SetWindowPosByMouseMove(CPoint pt);

private:
	CCustomFrameWndHandlerPrivate* m_private;
};
typedef std::shared_ptr<CCustomFrameWndHandler> CCustomFrameWndHandlerPtr;
