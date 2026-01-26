#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/DisplayInfo.h"
#include "Runtime/Core/Platform/PlatformMonitorHandle.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"

class PlatformViewport;
class DisplaySubsystem;
class UserInterfaceSubsystem;

/// HDR preference for viewport output mapping
enum class HdrPreference
{
	/// Follow display HDR state: if display has HDR active, use HDR output; otherwise SDR
	Auto,
	/// Force SDR output (sRGB, Gamma 2.2) even if display has HDR enabled
	ForceSDR,
	/// Force HDR output mapping and request system to enable HDR (will restore on engine shutdown)
	ForceHDR,
	/// Force HDR output mapping (PQ + BT2020) without requesting system HDR (for native PQ displays)
	ForceConfig
};

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
	int outputGamutId = static_cast<int>(OutputGamut::SRGB);
	float maxLuminance = 0.0f;
	float minLuminanceLog10 = 0.0f;
	float sdrWhiteLevelNits = 0.0f;
	int eotfId = static_cast<int>(EOTF::Gamma);
	float gamma = 2.2f;
	float invGamma = 1.0f / 2.2f;
};

class Viewport {
public:
    Viewport();
    ~Viewport();

	/// Initialize the viewport with a platform window
	bool Initialize(int width, int height, const std::wstring& title, DisplaySubsystem* displaySubsystem);

	/// Process platform messages. Returns false if window should close.
	bool ProcessMessages();

	/// Get the platform window handle
	PlatformWindowHandle GetWindowHandle() const;

	/// Get the platform viewport implementation
	PlatformViewport* GetPlatformViewport() const;

	/// Get current viewport information
	const ViewportInfo& GetViewportInfo() const { return m_viewportInfo; }

	/// Get cached display information (may be nullptr if not yet set)
	const DisplayInfo* GetDisplayInfo() const { return m_hasDisplayInfo ? &m_displayInfo : nullptr; }

	/// Get/Set gamma value
	float GetGamma() const { return m_gamma; }
	void SetGamma(float gamma);

	/// Get/Set HDR preference
	HdrPreference GetHdrPreference() const { return m_hdrPreference; }
	void SetHdrPreference(HdrPreference preference);

	/// Request system to enable HDR on the current display
	/// @param bForceHdrOutput If true, also sets HdrPreference to ForceHDR; otherwise sets to Auto
	void RequestHdr(bool bForceHdrOutput = false);

	/// Set UI subsystem for message forwarding.
	void SetUserInterfaceSubsystem(UserInterfaceSubsystem* uiSubsystem);

	/// Handle platform messages (forward to UI subsystem).
	bool HandlePlatformMessage(uint32_t message, uintptr_t wParam, intptr_t lParam);

	/// Update luminance from swapchain query (DXGI output).
	void UpdateLuminanceFromSwapChain(float maxLuminanceNits, float minLuminanceNits);
	/// Clear swapchain luminance override (e.g., when display changes).
	void ClearSwapChainLuminanceOverride();

	// ========================================================================
	// Callbacks from ViewportSubsystem (do not call directly)
	// ========================================================================

	/// Called when display state changes (from ViewportSubsystem)
	void OnDisplayStateChanged(const DisplayStateChangedInfo& info);

	/// Called when color management settings change (from ViewportSubsystem)
	void OnColorManagementChanged(const ColorManagementChangedInfo& info);

	// ========================================================================
	// Platform callbacks (from PlatformViewport)
	// ========================================================================

	void HandleNativeCreated(int width, int height);
	void HandleNativeWindowSizeChanged(int width, int height);
	void HandleNativeWindowMoved(int x, int y);
	void HandleNativeWindowDisplayChanged(const PlatformMonitorHandlePtr& previous, const PlatformMonitorHandlePtr& current);
	void HandleNativeDisplayConfigurationChanged(int width, int height, int bitsPerPixel);
	void HandleNativeDisplayDevicesChanged(unsigned int event, uintptr_t wParam, intptr_t lParam);
	void HandleNativeDisplaySettingsChanged(uintptr_t wParam, intptr_t lParam);

private:
	void UpdateViewportInfo(int width, int height);
	void UpdateOutputMappingParameters();
	void RefreshDisplayInfoFromSubsystem();
	void BroadcastViewportChanged(ViewportChangeType changeType);

private:
	std::unique_ptr<PlatformViewport> m_platformViewport;
	DisplaySubsystem* m_displaySubsystem = nullptr;
	ViewportInfo m_viewportInfo;
	DisplayInfo m_displayInfo;
	bool m_hasDisplayInfo = false;
	float m_gamma = 2.2f;
	HdrPreference m_hdrPreference = HdrPreference::Auto;
	bool m_hasSwapChainLuminance = false;
	float m_swapChainMaxLuminance = 0.0f;
	float m_swapChainMinLuminanceLog10 = 0.0f;
	UserInterfaceSubsystem* m_uiSubsystem = nullptr;
	int m_lastLoggedOutputGamutId = -1;
	int m_lastLoggedEotfId = -1;
	float m_lastLoggedMaxLuminance = -1.0f;
	bool m_lastLoggedHdrActive = false;
	HdrPreference m_lastLoggedHdrPreference = HdrPreference::Auto;
};
