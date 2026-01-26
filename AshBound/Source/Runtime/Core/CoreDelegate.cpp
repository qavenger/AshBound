#include "Runtime/Core/CoreDelegate.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace
{
	template<typename CallbackType>
	struct DelegateEntry
	{
		DelegateHandle handle;
		CallbackType callback;
	};

	template<typename CallbackType>
	DelegateHandle AddDelegate(std::vector<DelegateEntry<CallbackType>>& list, CallbackType callback)
	{
		if (!callback.IsBound())
		{
			return DelegateHandle::Invalid();
		}
		const DelegateHandle handle = callback.GetHandle();
		list.push_back({ handle, std::move(callback) });
		return handle;
	}

	template<typename CallbackType>
	bool RemoveDelegate(std::vector<DelegateEntry<CallbackType>>& list, const DelegateHandle& handle)
	{
		if (!handle.IsValid())
		{
			return false;
		}
		const auto it = std::remove_if(list.begin(), list.end(),
			[&handle](const auto& entry)
			{
				return entry.handle == handle;
			});
		if (it == list.end())
		{
			return false;
		}
		list.erase(it, list.end());
		return true;
	}

	template<typename CallbackType, typename InfoType>
	void BroadcastDelegates(const std::vector<DelegateEntry<CallbackType>>& list, const InfoType& info)
	{
		for (const auto& entry : list)
		{
			entry.callback.Execute(info);
		}
	}

	// Platform-level event callbacks
	std::vector<DelegateEntry<CoreDelegate::DisplayConfigurationChangedCallback>> s_displayConfigurationChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::DisplayDevicesChangedCallback>> s_displayDevicesChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::DisplaySettingsChangedCallback>> s_displaySettingsChangedCallbacks;

	// DisplaySubsystem event callbacks
	std::vector<DelegateEntry<CoreDelegate::DisplayStateChangedCallback>> s_displayStateChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::DisplayCacheRefreshedCallback>> s_displayCacheRefreshedCallbacks;

	// Viewport event callbacks
	std::vector<DelegateEntry<CoreDelegate::ViewportChangedCallback>> s_viewportChangedCallbacks;

	// ColorManagement event callbacks
	std::vector<DelegateEntry<CoreDelegate::ColorManagementChangedCallback>> s_colorManagementChangedCallbacks;
}

// ============================================================================
// Platform-level events
// ============================================================================

DelegateHandle CoreDelegate::AddDisplayConfigurationChangedCallback(DisplayConfigurationChangedCallback callback)
{
	return AddDelegate(s_displayConfigurationChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveDisplayConfigurationChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_displayConfigurationChangedCallbacks, handle);
}

void CoreDelegate::ClearDisplayConfigurationChangedCallbacks()
{
	s_displayConfigurationChangedCallbacks.clear();
}

DelegateHandle CoreDelegate::AddDisplayDevicesChangedCallback(DisplayDevicesChangedCallback callback)
{
	return AddDelegate(s_displayDevicesChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveDisplayDevicesChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_displayDevicesChangedCallbacks, handle);
}

void CoreDelegate::ClearDisplayDevicesChangedCallbacks()
{
	s_displayDevicesChangedCallbacks.clear();
}

DelegateHandle CoreDelegate::AddDisplaySettingsChangedCallback(DisplaySettingsChangedCallback callback)
{
	return AddDelegate(s_displaySettingsChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveDisplaySettingsChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_displaySettingsChangedCallbacks, handle);
}

void CoreDelegate::ClearDisplaySettingsChangedCallbacks()
{
	s_displaySettingsChangedCallbacks.clear();
}

// ============================================================================
// DisplaySubsystem events
// ============================================================================

DelegateHandle CoreDelegate::AddDisplayStateChangedCallback(DisplayStateChangedCallback callback)
{
	return AddDelegate(s_displayStateChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveDisplayStateChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_displayStateChangedCallbacks, handle);
}

void CoreDelegate::ClearDisplayStateChangedCallbacks()
{
	s_displayStateChangedCallbacks.clear();
}

DelegateHandle CoreDelegate::AddDisplayCacheRefreshedCallback(DisplayCacheRefreshedCallback callback)
{
	return AddDelegate(s_displayCacheRefreshedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveDisplayCacheRefreshedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_displayCacheRefreshedCallbacks, handle);
}

void CoreDelegate::ClearDisplayCacheRefreshedCallbacks()
{
	s_displayCacheRefreshedCallbacks.clear();
}

// ============================================================================
// Viewport events
// ============================================================================

DelegateHandle CoreDelegate::AddViewportChangedCallback(ViewportChangedCallback callback)
{
	return AddDelegate(s_viewportChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveViewportChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_viewportChangedCallbacks, handle);
}

void CoreDelegate::ClearViewportChangedCallbacks()
{
	s_viewportChangedCallbacks.clear();
}

// ============================================================================
// ColorManagement events
// ============================================================================

DelegateHandle CoreDelegate::AddColorManagementChangedCallback(ColorManagementChangedCallback callback)
{
	return AddDelegate(s_colorManagementChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveColorManagementChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_colorManagementChangedCallbacks, handle);
}

void CoreDelegate::ClearColorManagementChangedCallbacks()
{
	s_colorManagementChangedCallbacks.clear();
}

// ============================================================================
// Broadcast methods
// ============================================================================

void CoreDelegate::BroadcastDisplayConfigurationChanged(const DisplayConfigurationChangedInfo& info)
{
	BroadcastDelegates(s_displayConfigurationChangedCallbacks, info);
}

void CoreDelegate::BroadcastDisplayDevicesChanged(const DisplayDevicesChangedInfo& info)
{
	BroadcastDelegates(s_displayDevicesChangedCallbacks, info);
}

void CoreDelegate::BroadcastDisplaySettingsChanged(const DisplaySettingsChangedInfo& info)
{
	BroadcastDelegates(s_displaySettingsChangedCallbacks, info);
}

void CoreDelegate::BroadcastDisplayStateChanged(const DisplayStateChangedInfo& info)
{
	BroadcastDelegates(s_displayStateChangedCallbacks, info);
}

void CoreDelegate::BroadcastDisplayCacheRefreshed(const DisplayCacheRefreshedInfo& info)
{
	BroadcastDelegates(s_displayCacheRefreshedCallbacks, info);
}

void CoreDelegate::BroadcastViewportChanged(const ViewportChangedInfo& info)
{
	BroadcastDelegates(s_viewportChangedCallbacks, info);
}

void CoreDelegate::BroadcastColorManagementChanged(const ColorManagementChangedInfo& info)
{
	BroadcastDelegates(s_colorManagementChangedCallbacks, info);
}
