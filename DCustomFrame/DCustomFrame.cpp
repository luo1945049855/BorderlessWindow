// DCustomFrame.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "MainFrame.h"

CAppModule _Module;

int Run(LPWSTR lpCmdLine, int cCmdShow)
{
	CMessageLoop message_loop;
	_Module.AddMessageLoop(&message_loop);

	RECT init_rect{0, 0, 1000, 800};
	CMainFrame main_frame;
	main_frame.Create(NULL, &init_rect, L"Window Name", WS_VISIBLE | WS_OVERLAPPEDWINDOW, NULL);
	main_frame.ShowWindow(cCmdShow);
	main_frame.CenterWindow();

	int result = message_loop.Run();
	_Module.RemoveMessageLoop();
	return result;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	OleInitialize(NULL);
	_Module.Init(NULL, hInstance);

	try
	{
		Run(lpCmdLine, nCmdShow);
	}
	catch (const std::exception&) {}

	_Module.Term();
	OleUninitialize();

	return 0;
}
