#include "Runtime/Core/ViewportSubsystem.h"
#include "Runtime/Core/Log.h"

DEFINE_LOG_CATEGORY(LogViewportSubsystem, Info)

void ViewportSubsystem::Initialize()
{
}

void ViewportSubsystem::Shutdown()
{
	m_viewports.clear();
	m_freeSlots.clear();
}

Viewport* ViewportSubsystem::CreateViewport(int width, int height, const std::wstring& title)
{
	auto viewport = std::make_unique<Viewport>();
	if (!viewport->Initialize(width, height, title))
	{
		return nullptr;
	}

	if (!m_freeSlots.empty())
	{
		const size_t slot = m_freeSlots.back();
		m_freeSlots.pop_back();
		m_viewports[slot] = std::move(viewport);
		return m_viewports[slot].get();
	}

	m_viewports.push_back(std::move(viewport));
	return m_viewports.back().get();
}

bool ViewportSubsystem::DestroyViewport(Viewport* viewport)
{
	if (!viewport)
	{
		return false;
	}

	for (size_t i = 0; i < m_viewports.size(); ++i)
	{
		if (m_viewports[i].get() == viewport)
		{
			m_viewports[i].reset();
			m_freeSlots.push_back(i);
			return true;
		}
	}

	return false;
}

bool ViewportSubsystem::ProcessMessages()
{
	for (const auto& viewport : m_viewports)
	{
		if (viewport)
		{
			if (!viewport->ProcessMessages())
			{
				return false;
			}
		}
	}
	return true;
}

std::vector<PlatformWindowHandle> ViewportSubsystem::GetViewportWindows() const
{
	std::vector<PlatformWindowHandle> windows;
	windows.reserve(m_viewports.size());
	for (const auto& viewport : m_viewports)
	{
		if (viewport)
		{
			windows.push_back(viewport->GetWindowHandle());
		}
	}
	return windows;
}

Viewport* ViewportSubsystem::GetViewportByWindow(PlatformWindowHandle window) const
{
	for (const auto& viewport : m_viewports)
	{
		if (viewport && viewport->GetWindowHandle() == window)
		{
			return viewport.get();
		}
	}
	return nullptr;
}

void ViewportSubsystem::RefreshViewportMonitor(PlatformWindowHandle window)
{
	Viewport* viewport = GetViewportByWindow(window);
	if (viewport)
	{
		viewport->RefreshMonitorState();
	}
}

void ViewportSubsystem::UpdateViewportDisplaySnapshot(PlatformWindowHandle window, const DisplayInfo& displayInfo)
{
	Viewport* viewport = GetViewportByWindow(window);
	if (!viewport)
	{
		return;
	}

	const DisplayInfo* previousInfo = viewport->GetDisplayInfo();
	const bool changed = !previousInfo || previousInfo->monitor != displayInfo.monitor;
	viewport->UpdateDisplayInfo(displayInfo);

	if (changed)
	{
		const RECT& area = displayInfo.monitorRect;
		const RECT& work = displayInfo.workRect;
		LOG(LogViewportSubsystem, Info,
			L"Window(%p) moved to Display(%s) Friendly(%s) Area(%ld,%ld,%ld,%ld) Work(%ld,%ld,%ld,%ld) Primary(%d) Active(%d) DPI(%u,%u) HDR(Supported=%d Active=%d)",
			window.handle,
			displayInfo.deviceName.c_str(),
			displayInfo.friendlyName.empty() ? L"N/A" : displayInfo.friendlyName.c_str(),
			area.left, area.top, area.right, area.bottom,
			work.left, work.top, work.right, work.bottom,
			displayInfo.isPrimary ? 1 : 0,
			displayInfo.isActive ? 1 : 0,
			displayInfo.dpiX,
			displayInfo.dpiY,
			displayInfo.hdrSupported ? 1 : 0,
			displayInfo.hdrActive ? 1 : 0);
	}
}
