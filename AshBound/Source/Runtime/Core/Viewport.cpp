#include "Runtime/Core/Viewport.h"

#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/DisplaySubsystem.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/UI/UserInterfaceSubsystem.h"
#include "Runtime/Core/Platform/PlatformViewport.h"

#include <algorithm>
#include <cmath>

#if defined(_WIN32)
#include "Runtime/Core/Platform/WindowsViewport.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogViewport, Info)
DEFINE_LOG_CATEGORY(LogViewport, Info)

Viewport::Viewport() = default;

Viewport::~Viewport() = default;

bool Viewport::Initialize(int width, int height, const std::wstring& title, DisplaySubsystem* displaySubsystem)
{
	m_displaySubsystem = displaySubsystem;

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

	// Get initial display info from subsystem
	RefreshDisplayInfoFromSubsystem();

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

PlatformViewport* Viewport::GetPlatformViewport() const
{
	return m_platformViewport.get();
}

void Viewport::SetGamma(float gamma)
{
	m_gamma = gamma;
	m_viewportInfo.gamma = gamma;
	m_viewportInfo.invGamma = (gamma != 0.0f) ? (1.0f / gamma) : 0.0f;
}

void Viewport::SetHdrPreference(HdrPreference preference)
{
	if (m_hdrPreference == preference)
	{
		return;
	}

	m_hdrPreference = preference;
	UpdateOutputMappingParameters();
	BroadcastViewportChanged(ViewportChangeType::HdrPreference);
}

void Viewport::RequestHdr(bool bForceHdrOutput)
{
	if (!m_displaySubsystem || !m_hasDisplayInfo)
	{
		LOG(LogViewport, Warning, L"RequestHdr: No display subsystem or display info available.");
		return;
	}

	// Request system to enable HDR on the current monitor
	const bool success = m_displaySubsystem->TryEnableHdr(m_displayInfo.monitor);

	if (success)
	{
		// Set preference based on parameter
		SetHdrPreference(bForceHdrOutput ? HdrPreference::ForceHDR : HdrPreference::Auto);
	}
	else
	{
		LOG(LogViewport, Warning, L"RequestHdr: Failed to enable HDR on display %s.",
			m_displayInfo.deviceName.c_str());
	}
}

void Viewport::SetUserInterfaceSubsystem(UserInterfaceSubsystem* uiSubsystem)
{
	m_uiSubsystem = uiSubsystem;
}

bool Viewport::HandlePlatformMessage(uint32_t message, uintptr_t wParam, intptr_t lParam)
{
	if (!m_uiSubsystem)
	{
		return false;
	}
	UserInterfaceMessage uiMessage{};
	uiMessage.window = GetWindowHandle();
	uiMessage.message = message;
	uiMessage.wParam = wParam;
	uiMessage.lParam = lParam;
	return m_uiSubsystem->HandleMessage(uiMessage);
}

void Viewport::UpdateLuminanceFromSwapChain(float maxLuminanceNits, float minLuminanceNits)
{
	if (maxLuminanceNits <= 0.0f)
	{
		return;
	}

	m_hasSwapChainLuminance = true;
	m_swapChainMaxLuminance = maxLuminanceNits;
	m_swapChainMinLuminanceLog10 = (minLuminanceNits > 0.0f)
		? static_cast<float>(std::log10(minLuminanceNits))
		: 0.0f;

	UpdateOutputMappingParameters();
	BroadcastViewportChanged(ViewportChangeType::Display);
}

void Viewport::ClearSwapChainLuminanceOverride()
{
	m_hasSwapChainLuminance = false;
	m_swapChainMaxLuminance = 0.0f;
	m_swapChainMinLuminanceLog10 = 0.0f;
}

void Viewport::OnDisplayStateChanged(const DisplayStateChangedInfo& info)
{
	// Check if this affects our current display
	if (!m_hasDisplayInfo || !m_displayInfo.monitor || !info.monitor)
	{
		return;
	}
	if (!m_displayInfo.monitor->Equals(info.monitor.get()))
	{
		return;
	}

	// Check if user manually disabled HDR while we were in ForceHDR mode
	const bool wasHdrActive = m_displayInfo.hdrActive;

	// Refresh display info from subsystem
	RefreshDisplayInfoFromSubsystem();

	const bool isHdrActive = m_displayInfo.hdrActive;

	// Handle ForceHDR degradation: if user manually disabled HDR, degrade to Auto
	if ((info.changeType & DisplayStateChangeType::HdrStateChanged) != DisplayStateChangeType::None)
	{
		if (m_hdrPreference == HdrPreference::ForceHDR && wasHdrActive && !isHdrActive)
		{
			// User manually disabled HDR while we were in ForceHDR mode
			LOG(LogViewport, Info, L"User disabled system HDR while ForceHDR was active. Degrading to Auto.");
			m_hdrPreference = HdrPreference::Auto;
		}
	}

	// Update output mapping based on new display state
	UpdateOutputMappingParameters();

	// Notify listeners that viewport has changed (display state affects output)
	if ((info.changeType & DisplayStateChangeType::HdrStateChanged) != DisplayStateChangeType::None)
	{
		BroadcastViewportChanged(ViewportChangeType::Display);
	}
}

void Viewport::OnColorManagementChanged(const ColorManagementChangedInfo& info)
{
	(void)info;
	UpdateOutputMappingParameters();
	// Color management change affects viewport output, but we don't broadcast here
	// as it's a global setting change, not a viewport-specific change
}

void Viewport::HandleNativeCreated(int width, int height)
{
	// Ensure display info is refreshed before any initial viewport change events.
	if (m_displaySubsystem)
	{
		m_displaySubsystem->RefreshDisplayForWindow(GetWindowHandle(), DisplayStateChangeType::All);
		RefreshDisplayInfoFromSubsystem();
	}
	UpdateViewportInfo(width, height);
	BroadcastViewportChanged(ViewportChangeType::Display);
}

void Viewport::HandleNativeWindowSizeChanged(int width, int height)
{
	UpdateViewportInfo(width, height);
	BroadcastViewportChanged(ViewportChangeType::Size);
}

void Viewport::HandleNativeWindowMoved(int x, int y)
{
	(void)x;
	(void)y;
	BroadcastViewportChanged(ViewportChangeType::Position);
}

void Viewport::HandleNativeWindowDisplayChanged(const PlatformMonitorHandlePtr& previous, const PlatformMonitorHandlePtr& current)
{
	(void)previous;
	(void)current;

	// Window moved to a different display, refresh display info from cache
	ClearSwapChainLuminanceOverride();
	RefreshDisplayInfoFromSubsystem();
	UpdateOutputMappingParameters();
	BroadcastViewportChanged(ViewportChangeType::Display);
}

void Viewport::HandleNativeDisplayConfigurationChanged(int width, int height, int bitsPerPixel)
{
	// Forward to CoreDelegate for DisplaySubsystem to handle
	CoreDelegate::BroadcastDisplayConfigurationChanged({ GetWindowHandle(), width, height, bitsPerPixel });
}

void Viewport::HandleNativeDisplayDevicesChanged(unsigned int event, uintptr_t wParam, intptr_t lParam)
{
	// Forward to CoreDelegate for DisplaySubsystem to handle
	CoreDelegate::BroadcastDisplayDevicesChanged({ GetWindowHandle(), event, wParam, lParam });
}

void Viewport::HandleNativeDisplaySettingsChanged(uintptr_t wParam, intptr_t lParam)
{
	// Forward to CoreDelegate for DisplaySubsystem to handle
	CoreDelegate::BroadcastDisplaySettingsChanged({ GetWindowHandle(), wParam, lParam });
}

void Viewport::UpdateViewportInfo(int width, int height)
{
	const float fWidth = static_cast<float>(width);
	const float fHeight = static_cast<float>(height);

	m_viewportInfo.width = fWidth;
	m_viewportInfo.height = fHeight;
	m_viewportInfo.invWidth = (fWidth != 0.0f) ? (1.0f / fWidth) : 0.0f;
	m_viewportInfo.invHeight = (fHeight != 0.0f) ? (1.0f / fHeight) : 0.0f;
	m_viewportInfo.aspectRatio = (fHeight != 0.0f) ? (fWidth / fHeight) : 0.0f;
	m_viewportInfo.invAspectRatio = (m_viewportInfo.aspectRatio != 0.0f) ? (1.0f / m_viewportInfo.aspectRatio) : 0.0f;
	m_viewportInfo.centerX = fWidth * 0.5f;
	m_viewportInfo.centerY = fHeight * 0.5f;

	UpdateOutputMappingParameters();
}

void Viewport::UpdateOutputMappingParameters()
{
	if (!ColorManagement::IsInitialized())
	{
		ColorManagement::InitializeFromConfig();
	}

	// Determine output parameters based on HdrPreference and display state
	const bool hdrSupported = m_hasDisplayInfo && m_displayInfo.hdrSupported;
	const bool hdrActive = m_hasDisplayInfo && m_displayInfo.hdrActive;

	switch (m_hdrPreference)
	{
	case HdrPreference::ForceSDR:
		// Force SDR output: sRGB gamut, Gamma 2.2
		m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::SRGB);
		m_viewportInfo.eotfId = static_cast<int>(EOTF::Gamma);
		SetGamma(2.2f);
		break;

	case HdrPreference::ForceHDR:
		// Force HDR output mapping + request system HDR if needed
		if (hdrActive)
		{
			// Display has HDR enabled: use scRGB (G10_P709)
			m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::SCRGB);
			m_viewportInfo.eotfId = static_cast<int>(EOTF::ScRGB);
			SetGamma(1.0f);
		}
		else if (hdrSupported && m_displaySubsystem)
		{
			// Display supports HDR but it's not active - request system to enable it
			LOG(LogViewport, Info, L"ForceHDR: Display supports HDR but not active, requesting system to enable.");
			if (m_displaySubsystem->TryEnableHdr(m_displayInfo.monitor))
			{
				// HDR enabled successfully, use scRGB
				m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::SCRGB);
				m_viewportInfo.eotfId = static_cast<int>(EOTF::ScRGB);
				SetGamma(1.0f);
			}
			else
			{
				// Failed to enable system HDR, fall back to PQ/BT.2020
				m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::BT2020);
				m_viewportInfo.eotfId = static_cast<int>(EOTF::PQ);
				SetGamma(1.0f);
			}
		}
		else
		{
			// Display does not support or have HDR: use PQ/BT.2020 (G2084_P2020)
			m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::BT2020);
			m_viewportInfo.eotfId = static_cast<int>(EOTF::PQ);
			SetGamma(1.0f);
		}
		break;

	case HdrPreference::ForceConfig:
		// Force HDR output mapping WITHOUT requesting system HDR
		// For displays with native PQ support that don't need Windows HDR
		if (hdrActive)
		{
			// Display has HDR enabled: use scRGB
			m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::SCRGB);
			m_viewportInfo.eotfId = static_cast<int>(EOTF::ScRGB);
			SetGamma(1.0f);
		}
		else
		{
			// Use PQ/BT.2020 without requesting system HDR
			m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::BT2020);
			m_viewportInfo.eotfId = static_cast<int>(EOTF::PQ);
			SetGamma(1.0f);
		}
		break;

	case HdrPreference::Auto:
	default:
		// Follow display state
		if (hdrActive)
		{
			// Display has HDR enabled: use scRGB
			m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::SCRGB);
			m_viewportInfo.eotfId = static_cast<int>(EOTF::ScRGB);
			SetGamma(1.0f);
		}
		else
		{
			// Display is SDR: use sRGB with gamma
			m_viewportInfo.outputGamutId = static_cast<int>(OutputGamut::SRGB);
			m_viewportInfo.eotfId = static_cast<int>(EOTF::Gamma);
			SetGamma(2.2f);
		}
		break;
	}

	// Update working color space from ColorManagement
	if (ColorManagement::IsInitialized())
	{
		m_viewportInfo.workingColorSpaceId = static_cast<int>(ColorManagement::GetWorkingColorSpacePreset());
	}
	else
	{
		m_viewportInfo.workingColorSpaceId = static_cast<int>(WorkingColorSpacePreset::SRGB);
	}

	// Update luminance info from display
	if (m_hasDisplayInfo)
	{
		m_viewportInfo.maxLuminance = m_displayInfo.maxLuminance;
		m_viewportInfo.minLuminanceLog10 = m_displayInfo.minLuminanceLog10;
		m_viewportInfo.sdrWhiteLevelNits = m_displayInfo.sdrWhiteLevelNits;
	}

	// Prefer swapchain luminance if available.
	if (m_hasSwapChainLuminance && m_swapChainMaxLuminance > 0.0f)
	{
		m_viewportInfo.maxLuminance = m_swapChainMaxLuminance;
		if (m_swapChainMinLuminanceLog10 != 0.0f)
		{
			m_viewportInfo.minLuminanceLog10 = m_swapChainMinLuminanceLog10;
		}
	}

	const bool outputChanged =
		m_lastLoggedOutputGamutId != m_viewportInfo.outputGamutId ||
		m_lastLoggedEotfId != m_viewportInfo.eotfId ||
		m_lastLoggedMaxLuminance != m_viewportInfo.maxLuminance ||
		m_lastLoggedHdrActive != hdrActive ||
		m_lastLoggedHdrPreference != m_hdrPreference;
	if (outputChanged)
	{
		LOG(LogViewport, Info,
			L"Output mapping: Pref=%d HDRActive=%d OutputGamut=%d EOTF=%d MaxLum=%.1f Gamma=%.2f",
			static_cast<int>(m_hdrPreference),
			hdrActive ? 1 : 0,
			m_viewportInfo.outputGamutId,
			m_viewportInfo.eotfId,
			m_viewportInfo.maxLuminance,
			m_viewportInfo.gamma);

		m_lastLoggedOutputGamutId = m_viewportInfo.outputGamutId;
		m_lastLoggedEotfId = m_viewportInfo.eotfId;
		m_lastLoggedMaxLuminance = m_viewportInfo.maxLuminance;
		m_lastLoggedHdrActive = hdrActive;
		m_lastLoggedHdrPreference = m_hdrPreference;
	}
}

void Viewport::RefreshDisplayInfoFromSubsystem()
{
	if (!m_displaySubsystem)
	{
		return;
	}

	const PlatformWindowHandle window = GetWindowHandle();
	if (!window.IsValid())
	{
		return;
	}

	const DisplayInfo* displayInfo = m_displaySubsystem->GetDisplayInfoForWindow(window);
	if (displayInfo)
	{
		bool displayChanged = !m_hasDisplayInfo;
		if (!displayChanged && m_displayInfo.monitor && displayInfo->monitor)
		{
			displayChanged = !m_displayInfo.monitor->Equals(displayInfo->monitor.get());
		}
		else if (!displayChanged)
		{
			displayChanged = (m_displayInfo.monitor != displayInfo->monitor);
		}

		m_displayInfo = *displayInfo;
		m_hasDisplayInfo = true;

		if (displayChanged)
		{
			LOG(LogViewport, Info,
				L"Viewport(%p) using Display(%s) HDR(Supported=%d Active=%d) Luminance(Max=%.1f Min=%.4f SDRWhite=%.1f)",
				window.handle,
				m_displayInfo.deviceName.c_str(),
				m_displayInfo.hdrSupported ? 1 : 0,
				m_displayInfo.hdrActive ? 1 : 0,
				m_displayInfo.maxLuminance,
				m_displayInfo.minLuminanceLog10,
				m_displayInfo.sdrWhiteLevelNits);
		}
	}
}

void Viewport::BroadcastViewportChanged(ViewportChangeType changeType)
{
	const PlatformWindowHandle window = GetWindowHandle();
	if (window.IsValid())
	{
		ViewportChangedInfo info{};
		info.window = window;
		info.changeType = changeType;
		CoreDelegate::BroadcastViewportChanged(info);
	}
}
