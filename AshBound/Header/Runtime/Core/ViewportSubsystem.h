#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Runtime/Core/Viewport.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"

class ViewportSubsystem
{
public:
	void Initialize();
	void Shutdown();

	Viewport* CreateViewport(int width, int height, const std::wstring& title);
	bool DestroyViewport(Viewport* viewport);

	bool ProcessMessages();

	std::vector<PlatformWindowHandle> GetViewportWindows() const;
	Viewport* GetViewportByWindow(PlatformWindowHandle window) const;
	void RefreshViewportMonitor(PlatformWindowHandle window);
	void UpdateViewportDisplaySnapshot(PlatformWindowHandle window, const DisplayInfo& displayInfo);

private:
	std::vector<std::unique_ptr<Viewport>> m_viewports;
	std::vector<size_t> m_freeSlots;
};
