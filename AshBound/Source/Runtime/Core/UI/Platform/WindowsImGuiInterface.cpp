#include "Runtime/Core/UI/Platform/WindowsImGuiInterface.h"

#if defined(_WIN32)
#include "Runtime/Core/Viewport.h"
#include "Runtime/Core/Platform/PlatformViewport.h"
#include "Runtime/Rendering/RHI/RHICommandList.h"
#include "Runtime/Rendering/RHI/RHIDevice.h"
#include "Runtime/Rendering/DX12/DX12Format.h"

#include <Windows.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

WindowsImGuiInterface::~WindowsImGuiInterface()
{
	Shutdown();
}

bool WindowsImGuiInterface::Initialize(const UserInterfaceInitInfo& info)
{
	if (m_context || !info.viewport || !info.device)
	{
		return false;
	}

	const PlatformWindowHandle windowHandle = info.viewport->GetWindowHandle();
	HWND hwnd = static_cast<HWND>(windowHandle.handle);
	ID3D12Device* device = static_cast<ID3D12Device*>(info.device->GetNativeDevice());
	if (!hwnd || !device)
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvHeap))))
	{
		return false;
	}

	IMGUI_CHECKVERSION();
	m_context = ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui::SetCurrentContext(m_context);
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device,
		2,
		DX12Format::ToDxgiFormat(info.backbufferFormat),
		m_srvHeap.Get(),
		m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_srvHeap->GetGPUDescriptorHandleForHeapStart());

	m_platformViewport = info.viewport;
	m_backbufferFormat = info.backbufferFormat;
	return true;
}

void WindowsImGuiInterface::Shutdown()
{
	if (!m_context)
	{
		return;
	}
	ImGui::SetCurrentContext(m_context);
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext(m_context);
	m_context = nullptr;
	m_srvHeap.Reset();
	m_platformViewport = nullptr;
	m_backbufferFormat = RHIEnum::Format::Unknown;
}

void WindowsImGuiInterface::Render(const UserInterfaceRenderInfo& info)
{
	if (!m_context || !info.viewport || !info.commandList)
	{
		return;
	}

	ImGui::SetCurrentContext(m_context);
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Viewport");
	const char* items[] = { "Auto", "ForceSDR", "ForceHDR", "ForceConfig" };
	int current = static_cast<int>(info.viewport->GetHdrPreference());
	const int itemCount = static_cast<int>(sizeof(items) / sizeof(items[0]));
	if (ImGui::Combo("HDR Preference", &current, items, itemCount))
	{
		info.viewport->SetHdrPreference(static_cast<HdrPreference>(current));
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("ForceHDR: Request system HDR + HDR output\nForceConfig: HDR output only (no system HDR request)");
	}
	ImGui::End();

	ImGui::Render();
	ID3D12GraphicsCommandList* commandList =
		reinterpret_cast<ID3D12GraphicsCommandList*>(info.commandList->GetNativeList());
	if (!commandList)
	{
		return;
	}
	ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
	commandList->SetDescriptorHeaps(1, heaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool WindowsImGuiInterface::HandleMessage(const UserInterfaceMessage& message)
{
	if (!m_context)
	{
		return false;
	}
	ImGui::SetCurrentContext(m_context);
	HWND hwnd = static_cast<HWND>(message.window.handle);
	return ImGui_ImplWin32_WndProcHandler(hwnd, message.message, message.wParam, message.lParam);
}
#endif
