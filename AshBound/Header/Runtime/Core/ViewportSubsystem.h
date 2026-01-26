#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Runtime/Core/Viewport.h"
#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"
#include "Runtime/Core/Delegates/Delegate.h"

class DisplaySubsystem;
class UserInterfaceSubsystem;

class ViewportSubsystem
{
public:
	void Initialize(DisplaySubsystem* displaySubsystem, UserInterfaceSubsystem* uiSubsystem);
	void Shutdown();

	Viewport* CreateViewport(int width, int height, const std::wstring& title);
	bool DestroyViewport(Viewport* viewport);

	bool ProcessMessages();

	std::vector<PlatformWindowHandle> GetViewportWindows() const;
	Viewport* GetViewportByWindow(PlatformWindowHandle window) const;

private:
	void RegisterCallbacks();
	void UnregisterCallbacks();

	void OnDisplayStateChanged(const DisplayStateChangedInfo& info);
	void OnColorManagementChanged(const ColorManagementChangedInfo& info);

	/// Notify all viewports about display state change
	void NotifyViewportsDisplayStateChanged(const DisplayStateChangedInfo& info);

	/// Notify all viewports about color management change
	void NotifyViewportsColorManagementChanged(const ColorManagementChangedInfo& info);

	DisplaySubsystem* m_displaySubsystem = nullptr;
	UserInterfaceSubsystem* m_uiSubsystem = nullptr;
	std::vector<std::unique_ptr<Viewport>> m_viewports;
	std::vector<size_t> m_freeSlots;

	DelegateHandle m_displayStateChangedHandle = DelegateHandle::Invalid();
	DelegateHandle m_colorManagementChangedHandle = DelegateHandle::Invalid();
};
