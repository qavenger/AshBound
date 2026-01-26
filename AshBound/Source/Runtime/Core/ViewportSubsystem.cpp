#include "Runtime/Core/ViewportSubsystem.h"
#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/DisplaySubsystem.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/UI/UserInterfaceSubsystem.h"

DEFINE_LOG_CATEGORY(LogViewportSubsystem, Info)

void ViewportSubsystem::Initialize(DisplaySubsystem* displaySubsystem, UserInterfaceSubsystem* uiSubsystem)
{
	m_displaySubsystem = displaySubsystem;
	m_uiSubsystem = uiSubsystem;
	RegisterCallbacks();
}

void ViewportSubsystem::Shutdown()
{
	UnregisterCallbacks();
	m_viewports.clear();
	m_freeSlots.clear();
	m_displaySubsystem = nullptr;
	m_uiSubsystem = nullptr;
}

Viewport* ViewportSubsystem::CreateViewport(int width, int height, const std::wstring& title)
{
	auto viewport = std::make_unique<Viewport>();
	if (!viewport->Initialize(width, height, title, m_displaySubsystem))
	{
		return nullptr;
	}
	if (m_uiSubsystem)
	{
		m_uiSubsystem->RegisterViewport(viewport.get());
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
			if (m_uiSubsystem)
			{
				m_uiSubsystem->UnregisterViewport(viewport->GetWindowHandle());
			}
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

void ViewportSubsystem::RegisterCallbacks()
{
	m_displayStateChangedHandle = CoreDelegate::AddDisplayStateChangedCallback(
		CoreDelegate::DisplayStateChangedCallback::Create(this, &ViewportSubsystem::OnDisplayStateChanged));
	m_colorManagementChangedHandle = CoreDelegate::AddColorManagementChangedCallback(
		CoreDelegate::ColorManagementChangedCallback::Create(this, &ViewportSubsystem::OnColorManagementChanged));
}

void ViewportSubsystem::UnregisterCallbacks()
{
	if (m_displayStateChangedHandle.IsValid())
	{
		CoreDelegate::RemoveDisplayStateChangedCallback(m_displayStateChangedHandle);
		m_displayStateChangedHandle = DelegateHandle::Invalid();
	}

	if (m_colorManagementChangedHandle.IsValid())
	{
		CoreDelegate::RemoveColorManagementChangedCallback(m_colorManagementChangedHandle);
		m_colorManagementChangedHandle = DelegateHandle::Invalid();
	}
}

void ViewportSubsystem::OnDisplayStateChanged(const DisplayStateChangedInfo& info)
{
	NotifyViewportsDisplayStateChanged(info);
}

void ViewportSubsystem::OnColorManagementChanged(const ColorManagementChangedInfo& info)
{
	NotifyViewportsColorManagementChanged(info);
}

void ViewportSubsystem::NotifyViewportsDisplayStateChanged(const DisplayStateChangedInfo& info)
{
	for (const auto& viewport : m_viewports)
	{
		if (viewport)
		{
			viewport->OnDisplayStateChanged(info);
		}
	}
}

void ViewportSubsystem::NotifyViewportsColorManagementChanged(const ColorManagementChangedInfo& info)
{
	for (const auto& viewport : m_viewports)
	{
		if (viewport)
		{
			viewport->OnColorManagementChanged(info);
		}
	}
}
