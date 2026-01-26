#include "Runtime/Core/UI/UserInterfaceSubsystem.h"

#include "Runtime/Core/Viewport.h"
#include "Runtime/Core/Platform/PlatformViewport.h"
#include "Runtime/Rendering/RHI/RHICommandList.h"
#include "Runtime/Rendering/RHI/RHIDevice.h"

void UserInterfaceSubsystem::Initialize(IRHIDevice* device, RHIEnum::Format backbufferFormat)
{
	m_device = device;
	m_backbufferFormat = backbufferFormat;
	for (auto& entry : m_entries)
	{
		if (entry.ui && entry.platformViewport)
		{
			InitializeEntry(entry);
		}
	}
}

void UserInterfaceSubsystem::Shutdown()
{
	for (auto& entry : m_entries)
	{
		if (entry.ui)
		{
			entry.ui->Shutdown();
		}
	}
	m_entries.clear();
	m_device = nullptr;
}

void UserInterfaceSubsystem::RegisterViewport(Viewport* viewport)
{
	if (!viewport)
	{
		return;
	}
	PlatformWindowHandle window = viewport->GetWindowHandle();
	if (!window.IsValid())
	{
		return;
	}

	Entry* existing = FindEntry(window);
	if (existing)
	{
		existing->viewport = viewport;
		existing->platformViewport = viewport->GetPlatformViewport();
		return;
	}

	Entry entry{};
	entry.window = window;
	entry.viewport = viewport;
	entry.platformViewport = viewport->GetPlatformViewport();
	entry.ui = CreateUserInterfaceForPlatform();

	if (entry.ui && m_device && entry.platformViewport)
	{
		InitializeEntry(entry);
	}

	viewport->SetUserInterfaceSubsystem(this);
	m_entries.push_back(std::move(entry));
}

void UserInterfaceSubsystem::UnregisterViewport(PlatformWindowHandle window)
{
	if (!window.IsValid())
	{
		return;
	}
	for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
	{
		if (it->window == window)
		{
			if (it->ui)
			{
				it->ui->Shutdown();
			}
			m_entries.erase(it);
			break;
		}
	}
}

bool UserInterfaceSubsystem::HandleMessage(const UserInterfaceMessage& message)
{
	Entry* entry = FindEntry(message.window);
	if (!entry || !entry->ui)
	{
		return false;
	}
	return entry->ui->HandleMessage(message);
}

void UserInterfaceSubsystem::RenderAll(IRHICommandList* commandList)
{
	if (!commandList)
	{
		return;
	}
	for (auto& entry : m_entries)
	{
		if (!entry.ui || !entry.viewport)
		{
			continue;
		}
		UserInterfaceRenderInfo info{};
		info.commandList = commandList;
		info.viewport = entry.viewport;
		entry.ui->Render(info);
	}
}

void UserInterfaceSubsystem::ReinitializeAll(IRHIDevice* device, RHIEnum::Format backbufferFormat)
{
	m_device = device;
	m_backbufferFormat = backbufferFormat;
	for (auto& entry : m_entries)
	{
		if (!entry.ui || !entry.platformViewport || !m_device)
		{
			continue;
		}
		entry.ui->Shutdown();
		InitializeEntry(entry);
	}
}

UserInterfaceSubsystem::Entry* UserInterfaceSubsystem::FindEntry(PlatformWindowHandle window)
{
	for (auto& entry : m_entries)
	{
		if (entry.window == window)
		{
			return &entry;
		}
	}
	return nullptr;
}

void UserInterfaceSubsystem::InitializeEntry(Entry& entry)
{
	UserInterfaceInitInfo initInfo{};
	initInfo.viewport = entry.platformViewport;
	initInfo.device = m_device;
	initInfo.backbufferFormat = m_backbufferFormat;
	entry.ui->Initialize(initInfo);
}
