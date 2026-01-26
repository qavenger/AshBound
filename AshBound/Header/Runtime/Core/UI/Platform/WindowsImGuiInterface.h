#pragma once

#if defined(_WIN32)
#include "Runtime/Core/UI/UserInterface.h"

#include <d3d12.h>
#include <wrl/client.h>

struct ImGuiContext;

class WindowsImGuiInterface final : public UserInterface
{
public:
	WindowsImGuiInterface() = default;
	~WindowsImGuiInterface() override;

	bool Initialize(const UserInterfaceInitInfo& info) override;
	void Shutdown() override;
	void Render(const UserInterfaceRenderInfo& info) override;
	bool HandleMessage(const UserInterfaceMessage& message) override;

private:
	ImGuiContext* m_context = nullptr;
	PlatformViewport* m_platformViewport = nullptr;
	RHIEnum::Format m_backbufferFormat = RHIEnum::Format::Unknown;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
};
#endif
