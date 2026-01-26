#pragma once

#include <cstdint>
#include <vector>

#include "Runtime/Core/Delegates/Delegate.h"
#include "Runtime/Core/DisplayInfo.h"
#include "Runtime/Core/Platform/PlatformMonitorHandle.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"

// ============================================================================
// Platform-level events (from PlatformViewport)
// ============================================================================

struct DisplayConfigurationChangedInfo
{
	PlatformWindowHandle window = {};
	int width = 0;
	int height = 0;
	int bitsPerPixel = 0;
};

struct DisplayDevicesChangedInfo
{
	PlatformWindowHandle window = {};
	unsigned int event = 0;
	uintptr_t wParam = 0;
	intptr_t lParam = 0;
};

struct DisplaySettingsChangedInfo
{
	PlatformWindowHandle window = {};
	uintptr_t wParam = 0;
	intptr_t lParam = 0;
};

// ============================================================================
// DisplaySubsystem events
// ============================================================================

enum class DisplayStateChangeType : uint32_t
{
	None = 0,
	HdrStateChanged = 1u << 0,
	ResolutionChanged = 1u << 1,
	RefreshRateChanged = 1u << 2,
	DeviceAdded = 1u << 3,
	DeviceRemoved = 1u << 4,
	All = 0xFFFFFFFF
};

inline DisplayStateChangeType operator|(DisplayStateChangeType a, DisplayStateChangeType b)
{
	return static_cast<DisplayStateChangeType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline DisplayStateChangeType operator&(DisplayStateChangeType a, DisplayStateChangeType b)
{
	return static_cast<DisplayStateChangeType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct DisplayStateChangedInfo
{
	PlatformMonitorHandlePtr monitor;
	DisplayStateChangeType changeType = DisplayStateChangeType::None;
};

struct DisplayCacheRefreshedInfo
{
	std::vector<DisplayInfo> displays;
};

// ============================================================================
// Viewport events
// ============================================================================

enum class ViewportChangeType : uint32_t
{
	None = 0,
	Size = 1u << 0,
	Position = 1u << 1,
	Display = 1u << 2,
	HdrPreference = 1u << 3,
	WindowMode = 1u << 4,
	Minimized = 1u << 5,
	Maximized = 1u << 6,
	All = 0xFFFFFFFF
};

inline ViewportChangeType operator|(ViewportChangeType a, ViewportChangeType b)
{
	return static_cast<ViewportChangeType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ViewportChangeType operator&(ViewportChangeType a, ViewportChangeType b)
{
	return static_cast<ViewportChangeType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct ViewportChangedInfo
{
	PlatformWindowHandle window = {};
	ViewportChangeType changeType = ViewportChangeType::None;
};

// ============================================================================
// ColorManagement events
// ============================================================================

struct ColorManagementChangedInfo
{
	uint32_t changedMask = 0;
};

enum class ColorManagementChange : uint32_t
{
	WorkingColorSpace = 1u << 0,
	OutputGamut = 1u << 1,
	Eotf = 1u << 2,
	BackbufferBitDepth = 1u << 3
};

class CoreDelegate
{
public:
	// Platform-level callbacks
	using DisplayConfigurationChangedCallback = Delegate<void(const DisplayConfigurationChangedInfo&)>;
	using DisplayDevicesChangedCallback = Delegate<void(const DisplayDevicesChangedInfo&)>;
	using DisplaySettingsChangedCallback = Delegate<void(const DisplaySettingsChangedInfo&)>;

	// DisplaySubsystem callbacks
	using DisplayStateChangedCallback = Delegate<void(const DisplayStateChangedInfo&)>;
	using DisplayCacheRefreshedCallback = Delegate<void(const DisplayCacheRefreshedInfo&)>;

	// Viewport callbacks
	using ViewportChangedCallback = Delegate<void(const ViewportChangedInfo&)>;

	// ColorManagement callbacks
	using ColorManagementChangedCallback = Delegate<void(const ColorManagementChangedInfo&)>;

	// ========================================================================
	// Platform-level events (from PlatformViewport)
	// ========================================================================

	/// callback signature: void(const DisplayConfigurationChangedInfo& info)
	static DelegateHandle AddDisplayConfigurationChangedCallback(DisplayConfigurationChangedCallback callback);
	static bool RemoveDisplayConfigurationChangedCallback(const DelegateHandle& handle);
	static void ClearDisplayConfigurationChangedCallbacks();

	/// callback signature: void(const DisplayDevicesChangedInfo& info)
	static DelegateHandle AddDisplayDevicesChangedCallback(DisplayDevicesChangedCallback callback);
	static bool RemoveDisplayDevicesChangedCallback(const DelegateHandle& handle);
	static void ClearDisplayDevicesChangedCallbacks();

	/// callback signature: void(const DisplaySettingsChangedInfo& info)
	static DelegateHandle AddDisplaySettingsChangedCallback(DisplaySettingsChangedCallback callback);
	static bool RemoveDisplaySettingsChangedCallback(const DelegateHandle& handle);
	static void ClearDisplaySettingsChangedCallbacks();

	// ========================================================================
	// DisplaySubsystem events
	// ========================================================================

	/// callback signature: void(const DisplayStateChangedInfo& info)
	static DelegateHandle AddDisplayStateChangedCallback(DisplayStateChangedCallback callback);
	static bool RemoveDisplayStateChangedCallback(const DelegateHandle& handle);
	static void ClearDisplayStateChangedCallbacks();

	/// callback signature: void(const DisplayCacheRefreshedInfo& info)
	static DelegateHandle AddDisplayCacheRefreshedCallback(DisplayCacheRefreshedCallback callback);
	static bool RemoveDisplayCacheRefreshedCallback(const DelegateHandle& handle);
	static void ClearDisplayCacheRefreshedCallbacks();

	// ========================================================================
	// Viewport events
	// ========================================================================

	/// callback signature: void(const ViewportChangedInfo& info)
	static DelegateHandle AddViewportChangedCallback(ViewportChangedCallback callback);
	static bool RemoveViewportChangedCallback(const DelegateHandle& handle);
	static void ClearViewportChangedCallbacks();

	// ========================================================================
	// ColorManagement events
	// ========================================================================

	/// callback signature: void(const ColorManagementChangedInfo& info)
	static DelegateHandle AddColorManagementChangedCallback(ColorManagementChangedCallback callback);
	static bool RemoveColorManagementChangedCallback(const DelegateHandle& handle);
	static void ClearColorManagementChangedCallbacks();

	// ========================================================================
	// Broadcast methods
	// ========================================================================

	static void BroadcastDisplayConfigurationChanged(const DisplayConfigurationChangedInfo& info);
	static void BroadcastDisplayDevicesChanged(const DisplayDevicesChangedInfo& info);
	static void BroadcastDisplaySettingsChanged(const DisplaySettingsChangedInfo& info);
	static void BroadcastDisplayStateChanged(const DisplayStateChangedInfo& info);
	static void BroadcastDisplayCacheRefreshed(const DisplayCacheRefreshedInfo& info);
	static void BroadcastViewportChanged(const ViewportChangedInfo& info);
	static void BroadcastColorManagementChanged(const ColorManagementChangedInfo& info);
};
