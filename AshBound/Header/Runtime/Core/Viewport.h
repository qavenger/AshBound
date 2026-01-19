#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/DisplayManager.h"
#include "Runtime/Core/Platform/PlatformMonitorHandle.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"

class PlatformViewport;

struct ViewportInfo
{
	float width = 0.0f;
	float height = 0.0f;
	float invWidth = 0.0f;
	float invHeight = 0.0f;
	float aspectRatio = 0.0f;
	float invAspectRatio = 0.0f;
	float centerX = 0.0f;
	float centerY = 0.0f;
	int workingColorSpaceId = static_cast<int>(WorkingColorSpacePreset::SRGB);
	int outputGamutId = static_cast<int>(WorkingColorSpacePreset::SRGB);
	float maxLuminance = 0.0f;
	float minLuminanceLog10 = 0.0f;
	int eotfId = static_cast<int>(EOTF::SRGB);
	float gamma = 2.2f;
	float invGamma = 1.0f / 2.2f;
};

class Viewport {
public:
    Viewport();
    ~Viewport();

    // ??????????
    bool Initialize(int width, int height, const std::wstring& title);

    // ??????????? (??????)
    // ???? false ??????????????????????????
    bool ProcessMessages();

    // ????????? (??????? DirectX/OpenGL)
    PlatformWindowHandle GetWindowHandle() const;
    void RefreshMonitorState();
	const ViewportInfo& GetViewportInfo() const { return m_viewportInfo; }
	const DisplayInfo* GetDisplayInfo() const { return m_hasDisplayInfo ? &m_displayInfo : nullptr; }
	float GetGamma() const { return m_gamma; }
	void SetGamma(float gamma);
	void UpdateDisplayInfo(const DisplayInfo& displayInfo);

	// Platform callbacks
	void HandleNativeCreated(int width, int height);
	void HandleNativeWindowSizeChanged(int width, int height);
	void HandleNativeWindowMoved(int x, int y);
	void HandleNativeWindowDisplayChanged(PlatformMonitorHandle previous, PlatformMonitorHandle current);
	void HandleNativeDisplayConfigurationChanged(int width, int height, int bitsPerPixel);
	void HandleNativeDisplayDevicesChanged(unsigned int event, uintptr_t wParam, intptr_t lParam);
	void HandleNativeDisplaySettingsChanged(uintptr_t wParam, intptr_t lParam);

private:
	void UpdateViewportInfo(int width, int height);

private:
	std::unique_ptr<PlatformViewport> m_platformViewport;
	ViewportInfo m_viewportInfo;
	DisplayInfo m_displayInfo;
	bool m_hasDisplayInfo = false;
	float m_gamma = 2.2f;
};
