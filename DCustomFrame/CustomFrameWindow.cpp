#include "stdafx.h"
#include "CustomFrameWindow.h"

namespace
{
	/* Vista is major: 6, minor: 0 */
	BOOL IsVistaOrGreater()
	{
		OSVERSIONINFOEX osver = { 0 };
		ULONGLONG condMask = 0;
		osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		osver.dwMajorVersion = 6;
		VER_SET_CONDITION(condMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
		return VerifyVersionInfo(&osver, VER_MAJORVERSION, condMask);
	}
}

std::unique_ptr<CDWMAPIInvoker> gDWMAPIInvoker = std::make_unique<CDWMAPIInvoker>();

CDWMAPIInvoker::CDWMAPIInvoker()
	: m_pDwmDefWindowProc(NULL)
	, m_pDwmIsCompositionEnabled(NULL)
	, m_pDwmExtendFrameIntoClientArea(NULL)
	, m_pDwmGetWindowAttribute(NULL)
{
	if (HMODULE dwm_module = LoadSystemLibrary(L"dwmapi.dll"))
	{
		m_pDwmDefWindowProc = LoadAPI<pFuncDwmDefWindowProc>(dwm_module, "DwmDefWindowProc");
		m_pDwmIsCompositionEnabled = LoadAPI<pFuncDwmIsCompositionEnabled>(dwm_module, "DwmIsCompositionEnabled");
		m_pDwmExtendFrameIntoClientArea = LoadAPI<pFuncDwmExtendFrameIntoClientArea>(dwm_module, "DwmExtendFrameIntoClientArea");
		m_pDwmGetWindowAttribute = LoadAPI<pFuncDwmGetWindowAttribute>(dwm_module, "DwmGetWindowAttribute");
		m_pDwmSetWindowAttribute = LoadAPI<pFuncDwmSetWindowAttribute>(dwm_module, "DwmSetWindowAttribute");
	}
}

HMODULE CDWMAPIInvoker::LoadSystemLibrary(const WCHAR* dll_name)
{
	WCHAR dll_path[MAX_PATH];
	UINT result = GetSystemDirectoryW(dll_path, MAX_PATH);
	if (!result || result >= MAX_PATH)
		return NULL;
	BOOL ok = PathAppendW(dll_path, dll_name);
	if (!ok)
		return NULL;
	return LoadLibraryW(dll_path);
}

BOOL CDWMAPIInvoker::IsCompositionEnabled()
{
	if (!m_pDwmIsCompositionEnabled)
		return FALSE;
	BOOL is_enabled;
	if (SUCCEEDED(m_pDwmIsCompositionEnabled(&is_enabled)))
		return is_enabled;
	return FALSE;
}

BOOL CDWMAPIInvoker::DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
{
	if (!m_pDwmDefWindowProc)
		return FALSE;
	return m_pDwmDefWindowProc(hwnd, msg, wParam, lParam, plResult);
}

HRESULT CDWMAPIInvoker::ExtendFrameIntoClientArea(HWND hwnd, const MARGINS* pMarInset)
{
	if (!m_pDwmExtendFrameIntoClientArea)
		return E_NOTIMPL;
	return m_pDwmExtendFrameIntoClientArea(hwnd, pMarInset);
}

HRESULT CDWMAPIInvoker::GetWindowAttribute(HWND hwnd, DWORD dwAttribute, void* pvAttribute, DWORD cbAttribute)
{
	if (!m_pDwmGetWindowAttribute)
		return E_NOTIMPL;
	return m_pDwmGetWindowAttribute(hwnd, dwAttribute, pvAttribute, cbAttribute);
}

HRESULT CDWMAPIInvoker::SetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute)
{
	if (!m_pDwmSetWindowAttribute)
		return E_NOTIMPL;
	return m_pDwmSetWindowAttribute(hwnd, dwAttribute, pvAttribute, cbAttribute);
}
