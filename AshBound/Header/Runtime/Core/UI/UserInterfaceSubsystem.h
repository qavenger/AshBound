#pragma once

#include <memory>
#include <vector>

#include "Runtime/Core/UI/UserInterface.h"
#include "Runtime/Core/Platform/PlatformWindowHandle.h"

class Viewport;
class PlatformViewport;
class IRHIDevice;
class IRHICommandList;

class UserInterfaceSubsystem
{
public:
	void Initialize(IRHIDevice* device, RHIEnum::Format backbufferFormat);
	void Shutdown();

	void RegisterViewport(Viewport* viewport);
	void UnregisterViewport(PlatformWindowHandle window);

	bool HandleMessage(const UserInterfaceMessage& message);
	void RenderAll(IRHICommandList* commandList);

	void ReinitializeAll(IRHIDevice* device, RHIEnum::Format backbufferFormat);
	RHIEnum::Format GetBackbufferFormat() const { return m_backbufferFormat; }

private:
	struct Entry
	{
		PlatformWindowHandle window = {};
		Viewport* viewport = nullptr;
		PlatformViewport* platformViewport = nullptr;
		std::unique_ptr<UserInterface> ui;
	};

	Entry* FindEntry(PlatformWindowHandle window);
	void InitializeEntry(Entry& entry);

	IRHIDevice* m_device = nullptr;
	RHIEnum::Format m_backbufferFormat = RHIEnum::Format::R10G10B10A2_UNORM;
	std::vector<Entry> m_entries;
};
