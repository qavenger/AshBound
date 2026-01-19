#include "Runtime/Core/Viewport.h"

#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/Platform/PlatformViewport.h"

#include <algorithm>

#if defined(_WIN32)
#include "Runtime/Core/Platform/WindowsViewport.h"
#endif

Viewport::Viewport() = default;

Viewport::~Viewport() = default;

bool Viewport::Initialize(int width, int height, const std::wstring& title)
{
#if defined(_WIN32)
	m_platformViewport = std::make_unique<WindowsViewport>(this);
#else
	return false;
#endif

	if (!m_platformViewport->Initialize(width, height, title))
	{
		m_platformViewport.reset();
		return false;
	}
	return true;
}

bool Viewport::ProcessMessages()
{
	return m_platformViewport ? m_platformViewport->ProcessMessages() : true;
}

PlatformWindowHandle Viewport::GetWindowHandle() const
{
	return m_platformViewport ? m_platformViewport->GetWindowHandle() : PlatformWindowHandle{};
}

void Viewport::RefreshMonitorState()
{
	if (m_platformViewport)
	{
		m_platformViewport->RefreshMonitorState();
	}
}

void Viewport::SetGamma(float gamma)
{
	m_gamma = gamma;
	m_viewportInfo.gamma = gamma;
	m_viewportInfo.invGamma = (gamma != 0.0f) ? (1.0f / gamma) : 0.0f;
}

void Viewport::HandleNativeCreated(int width, int height)
{
	UpdateViewportInfo(width, height);
}

void Viewport::HandleNativeWindowSizeChanged(int width, int height)
{
	UpdateViewportInfo(width, height);
	CoreDelegate::BroadcastWindowSizeChanged({ GetWindowHandle(), width, height });
}

void Viewport::HandleNativeWindowMoved(int x, int y)
{
	CoreDelegate::BroadcastWindowMoved({ GetWindowHandle(), x, y });
}

void Viewport::HandleNativeWindowDisplayChanged(PlatformMonitorHandle previous, PlatformMonitorHandle current)
{
	CoreDelegate::BroadcastWindowDisplayChanged({ GetWindowHandle(), previous, current });
}

void Viewport::HandleNativeDisplayConfigurationChanged(int width, int height, int bitsPerPixel)
{
	CoreDelegate::BroadcastDisplayConfigurationChanged({ GetWindowHandle(), width, height, bitsPerPixel });
}

void Viewport::HandleNativeDisplayDevicesChanged(unsigned int event, uintptr_t wParam, intptr_t lParam)
{
	CoreDelegate::BroadcastDisplayDevicesChanged({ GetWindowHandle(), event, wParam, lParam });
}

void Viewport::HandleNativeDisplaySettingsChanged(uintptr_t wParam, intptr_t lParam)
{
	CoreDelegate::BroadcastDisplaySettingsChanged({ GetWindowHandle(), wParam, lParam });
}

void Viewport::UpdateViewportInfo(int width, int height)
{
    const float fWidth = static_cast<float>(width);
    const float fHeight = static_cast<float>(height);
    const float minDim = (std::min)(fWidth, fHeight);
    const float maxDim = (std::max)(fWidth, fHeight);

    m_viewportInfo.width = fWidth;
    m_viewportInfo.height = fHeight;
    m_viewportInfo.invWidth = (fWidth != 0.0f) ? (1.0f / fWidth) : 0.0f;
    m_viewportInfo.invHeight = (fHeight != 0.0f) ? (1.0f / fHeight) : 0.0f;
    m_viewportInfo.aspectRatio = (fHeight != 0.0f) ? (fWidth / fHeight) : 0.0f;
    m_viewportInfo.invAspectRatio = (m_viewportInfo.aspectRatio != 0.0f) ? (1.0f / m_viewportInfo.aspectRatio) : 0.0f;
    m_viewportInfo.centerX = fWidth * 0.5f;
    m_viewportInfo.centerY = fHeight * 0.5f;

    if (ColorManagement::IsInitialized())
    {
        m_viewportInfo.workingColorSpaceId = static_cast<int>(ColorManagement::GetWorkingColorSpacePreset());
    }
}

void Viewport::UpdateDisplayInfo(const DisplayInfo& displayInfo)
{
	const bool hdrActive = displayInfo.hdrActive;
	m_displayInfo = displayInfo;
	m_hasDisplayInfo = true;

	if (ColorManagement::IsInitialized())
	{
		m_viewportInfo.workingColorSpaceId = static_cast<int>(ColorManagement::GetWorkingColorSpacePreset());
	}

	m_viewportInfo.outputGamutId = static_cast<int>(hdrActive
		? WorkingColorSpacePreset::BT2020
		: WorkingColorSpacePreset::SRGB);
	m_viewportInfo.maxLuminance = displayInfo.maxLuminance;
	m_viewportInfo.minLuminanceLog10 = displayInfo.minLuminanceLog10;

	if (hdrActive)
	{
		m_viewportInfo.eotfId = static_cast<int>(EOTF::PQ);
		SetGamma(1.0f);
	}
	else
	{
		m_viewportInfo.eotfId = static_cast<int>(EOTF::SRGB);
		SetGamma(2.2f);
	}
}
