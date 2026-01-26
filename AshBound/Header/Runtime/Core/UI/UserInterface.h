#pragma once

#include <cstdint>
#include <memory>

#include "Runtime/Core/Platform/PlatformWindowHandle.h"
#include "Runtime/Rendering/RHI/RHIEnum.h"

class PlatformViewport;
class IRHIDevice;
class IRHICommandList;
class Viewport;

struct UserInterfaceInitInfo
{
	PlatformViewport* viewport = nullptr;
	IRHIDevice* device = nullptr;
	RHIEnum::Format backbufferFormat = RHIEnum::Format::R10G10B10A2_UNORM;
};

struct UserInterfaceRenderInfo
{
	IRHICommandList* commandList = nullptr;
	Viewport* viewport = nullptr;
};

struct UserInterfaceMessage
{
	PlatformWindowHandle window = {};
	uint32_t message = 0;
	uintptr_t wParam = 0;
	intptr_t lParam = 0;
};

class UserInterface
{
public:
	virtual ~UserInterface() = default;

	virtual bool Initialize(const UserInterfaceInitInfo& info) = 0;
	virtual void Shutdown() = 0;
	virtual void Render(const UserInterfaceRenderInfo& info) = 0;
	virtual bool HandleMessage(const UserInterfaceMessage& message) = 0;
};

std::unique_ptr<UserInterface> CreateUserInterfaceForPlatform();
