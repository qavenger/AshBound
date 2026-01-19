#pragma once

#include <Windows.h>
#include <string>

#include "Runtime/Core/Platform/PlatformViewport.h"

class WindowsViewport final : public PlatformViewport
{
public:
	explicit WindowsViewport(Viewport* owner);

	bool Initialize(int width, int height, const std::wstring& title) override;
	bool ProcessMessages() override;
	PlatformWindowHandle GetWindowHandle() const override;
	void RefreshMonitorState() override;

private:
	static LRESULT CALLBACK HandleMsgSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HandleMsgRedirect(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LRESULT HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void UpdateMonitorChange(HWND hwnd);
	void HandleExitSizeMove(HWND hwnd);

private:
	HWND m_hWnd = nullptr;
	std::wstring m_className = L"Viewport";
	HMONITOR m_lastMonitor = nullptr;
	POINT m_lastWindowPos{ 0, 0 };
	int m_lastClientWidth = 0;
	int m_lastClientHeight = 0;
	bool m_pendingSizeMove = false;
};
