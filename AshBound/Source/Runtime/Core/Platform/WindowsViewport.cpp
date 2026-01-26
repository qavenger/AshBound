#include "Runtime/Core/Platform/WindowsViewport.h"

#include "Runtime/Core/Viewport.h"
#include "Runtime/Core/Platform/WindowsMonitorHandle.h"
#include "Runtime/Core/Platform/WindowsWindowHandle.h"

WindowsViewport::WindowsViewport(Viewport* owner)
	: PlatformViewport(owner)
{
}

bool WindowsViewport::Initialize(int width, int height, const std::wstring& title)
{
	HINSTANCE hInstance = GetModuleHandleW(nullptr);

	WNDCLASSEXW wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = m_className.c_str();

	RegisterClassExW(&wc);

	RECT wr = { 0, 0, width, height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	m_hWnd = CreateWindowExW(
		0, m_className.c_str(), title.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		wr.right - wr.left, wr.bottom - wr.top,
		nullptr, nullptr, hInstance,
		this
	);

	if (!m_hWnd)
	{
		return false;
	}

	ShowWindow(m_hWnd, SW_SHOW);
	m_lastMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

	RECT windowRect{};
	RECT clientRect{};
	if (GetWindowRect(m_hWnd, &windowRect))
	{
		m_lastWindowPos.x = windowRect.left;
		m_lastWindowPos.y = windowRect.top;
	}
	if (GetClientRect(m_hWnd, &clientRect))
	{
		m_lastClientWidth = clientRect.right - clientRect.left;
		m_lastClientHeight = clientRect.bottom - clientRect.top;
	}

	if (m_owner)
	{
		m_owner->HandleNativeCreated(m_lastClientWidth, m_lastClientHeight);
	}
	return true;
}

bool WindowsViewport::ProcessMessages()
{
	MSG msg;
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return true;
}

PlatformWindowHandle WindowsViewport::GetWindowHandle() const
{
	return WindowsWindowHandle{ m_hWnd }.ToPlatformHandle();
}

void WindowsViewport::RefreshMonitorState()
{
	UpdateMonitorChange(m_hWnd);
}

LRESULT CALLBACK WindowsViewport::HandleMsgSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		WindowsViewport* const pWnd = static_cast<WindowsViewport*>(pCreate->lpCreateParams);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowsViewport::HandleMsgRedirect));

		return pWnd->HandleMsg(hwnd, msg, wParam, lParam);
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WindowsViewport::HandleMsgRedirect(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WindowsViewport* const pWnd = reinterpret_cast<WindowsViewport*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	return pWnd->HandleMsg(hwnd, msg, wParam, lParam);
}

LRESULT WindowsViewport::HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_owner && m_owner->HandlePlatformMessage(msg, static_cast<uintptr_t>(wParam), static_cast<intptr_t>(lParam)))
	{
		return true;
	}
	switch (msg)
	{
	case WM_ENTERSIZEMOVE:
		m_pendingSizeMove = true;
		break;
	case WM_EXITSIZEMOVE:
		HandleExitSizeMove(hwnd);
		m_pendingSizeMove = false;
		break;
	case WM_DISPLAYCHANGE:
	{
		if (m_owner)
		{
			const int bitsPerPixel = static_cast<int>(wParam);
			const int width = static_cast<int>(LOWORD(lParam));
			const int height = static_cast<int>(HIWORD(lParam));
			m_owner->HandleNativeDisplayConfigurationChanged(width, height, bitsPerPixel);
		}
		break;
	}
	case WM_DEVICECHANGE:
		if (m_owner)
		{
			m_owner->HandleNativeDisplayDevicesChanged(static_cast<unsigned int>(wParam),
				static_cast<uintptr_t>(wParam),
				static_cast<intptr_t>(lParam));
		}
		break;
	case WM_SETTINGCHANGE:
		if (m_owner)
		{
			m_owner->HandleNativeDisplaySettingsChanged(static_cast<uintptr_t>(wParam),
				static_cast<intptr_t>(lParam));
		}
		break;
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED && !m_pendingSizeMove)
		{
			HandleSizeChanged(hwnd);
		}
		m_pendingSizeMove = true;
		break;
	case WM_MOVE:
	case WM_WINDOWPOSCHANGED:
	case WM_DPICHANGED:
		m_pendingSizeMove = true;
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void WindowsViewport::UpdateMonitorChange(HWND hwnd)
{
	const HMONITOR currentMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	if (currentMonitor != m_lastMonitor && m_owner)
	{
		const PlatformMonitorHandlePtr previous = WindowsMonitorHandle::Create(m_lastMonitor);
		const PlatformMonitorHandlePtr current = WindowsMonitorHandle::Create(currentMonitor);
		m_lastMonitor = currentMonitor;
		m_owner->HandleNativeWindowDisplayChanged(previous, current);
	}
}

void WindowsViewport::HandleExitSizeMove(HWND hwnd)
{
	RECT windowRect{};
	RECT clientRect{};
	if (!GetWindowRect(hwnd, &windowRect) || !GetClientRect(hwnd, &clientRect))
	{
		return;
	}

	const POINT newPos{ windowRect.left, windowRect.top };
	int newWidth = clientRect.right - clientRect.left;
	int newHeight = clientRect.bottom - clientRect.top;

	constexpr int kMinClientSize = 320;
	const int shortSide = (newWidth < newHeight) ? newWidth : newHeight;
	if (shortSide < kMinClientSize)
	{
		const int prevWidth = (m_lastClientWidth > 0) ? m_lastClientWidth : newWidth;
		const int prevHeight = (m_lastClientHeight > 0) ? m_lastClientHeight : newHeight;
		const double aspect = (prevHeight > 0) ? (static_cast<double>(prevWidth) / prevHeight) : 1.0;

		const bool widthIsShort = newWidth <= newHeight;
		const int targetShort = kMinClientSize;
		int targetWidth = 0;
		int targetHeight = 0;
		if (widthIsShort)
		{
			targetWidth = targetShort;
			targetHeight = static_cast<int>(targetWidth / aspect);
		}
		else
		{
			targetHeight = targetShort;
			targetWidth = static_cast<int>(targetHeight * aspect);
		}

		if (targetWidth < kMinClientSize)
		{
			targetWidth = kMinClientSize;
		}
		if (targetHeight < kMinClientSize)
		{
			targetHeight = kMinClientSize;
		}

		RECT targetRect{ 0, 0, targetWidth, targetHeight };
		const DWORD style = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_STYLE));
		const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_EXSTYLE));
		AdjustWindowRectEx(&targetRect, style, FALSE, exStyle);

		const int windowWidth = targetRect.right - targetRect.left;
		const int windowHeight = targetRect.bottom - targetRect.top;
		SetWindowPos(hwnd, nullptr, windowRect.left, windowRect.top, windowWidth, windowHeight,
			SWP_NOZORDER | SWP_NOACTIVATE);

		newWidth = targetWidth;
		newHeight = targetHeight;
	}

	const bool positionChanged = newPos.x != m_lastWindowPos.x || newPos.y != m_lastWindowPos.y;
	const bool sizeChanged = newWidth != m_lastClientWidth || newHeight != m_lastClientHeight;

	if (sizeChanged && m_owner)
	{
		m_owner->HandleNativeWindowSizeChanged(newWidth, newHeight);
		m_lastClientWidth = newWidth;
		m_lastClientHeight = newHeight;
	}

	if (positionChanged && m_owner)
	{
		m_owner->HandleNativeWindowMoved(newPos.x, newPos.y);
		m_lastWindowPos = newPos;
	}

	if (sizeChanged || positionChanged || m_pendingSizeMove)
	{
		UpdateMonitorChange(hwnd);
	}
}

void WindowsViewport::HandleSizeChanged(HWND hwnd)
{
	RECT clientRect{};
	if (!GetClientRect(hwnd, &clientRect))
	{
		return;
	}

	const int newWidth = clientRect.right - clientRect.left;
	const int newHeight = clientRect.bottom - clientRect.top;
	const bool sizeChanged = newWidth != m_lastClientWidth || newHeight != m_lastClientHeight;
	if (sizeChanged && m_owner)
	{
		m_owner->HandleNativeWindowSizeChanged(newWidth, newHeight);
		m_lastClientWidth = newWidth;
		m_lastClientHeight = newHeight;
		UpdateMonitorChange(hwnd);
	}
}
