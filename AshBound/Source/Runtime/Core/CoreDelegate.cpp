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

	std::vector<DelegateEntry<CoreDelegate::WindowSizeChangedCallback>> s_windowSizeChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::WindowMovedCallback>> s_windowMovedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::WindowDisplayChangedCallback>> s_windowDisplayChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::DisplayConfigurationChangedCallback>> s_displayConfigurationChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::DisplayDevicesChangedCallback>> s_displayDevicesChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::DisplaySettingsChangedCallback>> s_displaySettingsChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::HdrStateChangedCallback>> s_hdrStateChangedCallbacks;
	std::vector<DelegateEntry<CoreDelegate::DisplayCacheRefreshedCallback>> s_displayCacheRefreshedCallbacks;
}

DelegateHandle CoreDelegate::AddWindowSizeChangedCallback(WindowSizeChangedCallback callback)
{
	return AddDelegate(s_windowSizeChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveWindowSizeChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_windowSizeChangedCallbacks, handle);
}

void CoreDelegate::ClearWindowSizeChangedCallbacks()
{
	s_windowSizeChangedCallbacks.clear();
}

DelegateHandle CoreDelegate::AddWindowMovedCallback(WindowMovedCallback callback)
{
	return AddDelegate(s_windowMovedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveWindowMovedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_windowMovedCallbacks, handle);
}

void CoreDelegate::ClearWindowMovedCallbacks()
{
	s_windowMovedCallbacks.clear();
}

DelegateHandle CoreDelegate::AddWindowDisplayChangedCallback(WindowDisplayChangedCallback callback)
{
	return AddDelegate(s_windowDisplayChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveWindowDisplayChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_windowDisplayChangedCallbacks, handle);
}

void CoreDelegate::ClearWindowDisplayChangedCallbacks()
{
	s_windowDisplayChangedCallbacks.clear();
}

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

DelegateHandle CoreDelegate::AddHdrStateChangedCallback(HdrStateChangedCallback callback)
{
	return AddDelegate(s_hdrStateChangedCallbacks, std::move(callback));
}

bool CoreDelegate::RemoveHdrStateChangedCallback(const DelegateHandle& handle)
{
	return RemoveDelegate(s_hdrStateChangedCallbacks, handle);
}

void CoreDelegate::ClearHdrStateChangedCallbacks()
{
	s_hdrStateChangedCallbacks.clear();
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

void CoreDelegate::BroadcastWindowSizeChanged(const WindowSizeChangedInfo& info)
{
	BroadcastDelegates(s_windowSizeChangedCallbacks, info);
}

void CoreDelegate::BroadcastWindowMoved(const WindowMovedInfo& info)
{
	BroadcastDelegates(s_windowMovedCallbacks, info);
}

void CoreDelegate::BroadcastWindowDisplayChanged(const WindowDisplayChangedInfo& info)
{
	BroadcastDelegates(s_windowDisplayChangedCallbacks, info);
}

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

void CoreDelegate::BroadcastHdrStateChanged(const HdrStateChangedInfo& info)
{
	BroadcastDelegates(s_hdrStateChangedCallbacks, info);
}

void CoreDelegate::BroadcastDisplayCacheRefreshed(const DisplayCacheRefreshedInfo& info)
{
	BroadcastDelegates(s_displayCacheRefreshedCallbacks, info);
}
