# Borderless Window

Borderless window for windows xp/7/8/10.

## Window Style

* `WS_THICKFRAME`: without this the window cannot be resized and so aero snap, de-maximizing and minimizing won't work
* `WS_SYSMENU`: enables the context menu with the move, close, maximize, minize... commands (shift + right-click on the task bar item)
* `WS_CAPTION`: enables aero minimize animation/transition
* `WS_MAXIMIZEBOX`, WS_MINIMIZEBOX: enable minimize/maximize

we cannot just use WS_POPUP style.

## Window Message

* `WM_NCCALSIZE`: we can remove border by let client rect same to window rect.

* `WM_NCHITTEST`: we have to handle WM_NCHITTEST ourselves to implement dragging and resizing, because there is no caption and border.

Remarks:

To let message WM_NCHITTEST transfer to main window, we have to return HTTRANSPARENT in WM_NCHITTEST handler of child window that covers the main window.

According to [MSDN](https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest)

> `HTTRANSPARENT` (-1)
In a window currently covered by another window in the `same thread` (the message will be sent to underlying windows in the same thread until one of them returns a code that is not HTTRANSPARENT).

If there are too many different child windows, we can use a `hook` to `subclass` all the child windows and intercept their WM_NCHITTEST messages.

IMPORTANT: In [cef](https://bitbucket.org/chromiumembedded/cef) applications, browser windows aren't in the same thread with main window when `CefSettings.multi_thread_message_loop = true` is used. There are two solutions to this problem:

 1. (Not Recommended)Run cef message loop work in main thread by setting `CefSettings.multi_thread_message_loop = false`.
 2. Update main window position and size by handling `WM_SETCURSOR/WM_LBUTTONDOWN/WM_MOUSEMOVE/WM_LBUTTONUP...` [example can be found in my code](./DCustomFrame/CustomFrameWndHandler.cpp).

## Shadow

System drawn shadow will disappear when there is non client area. This can be solved by  [DwmExtendFrameIntoClientArea](https://docs.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmextendframeintoclientarea).

What about Windows XP? There is a widely used solution is attaching a `WS_EX_LAYERED` window to the main window and draw shadow in the layered window.

## Reference

[BorderlessWindow](https://github.com/melak47/BorderlessWindow)
