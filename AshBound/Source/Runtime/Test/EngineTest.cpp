#include "Runtime/Test/EngineTest.h"

#include "Runtime/Core/Log.h"
#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/ViewportSubsystem.h"
#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Engine.h"
#include "Runtime/Engine/Camera.h"
#include "Runtime/Engine/Scene.h"
#include "Runtime/Engine/StaticMesh.h"
#include "Runtime/Engine/StaticMeshComponent.h"
#include "Runtime/Engine/SceneObject.h"
#include "Runtime/Rendering/DX12/DX12Format.h"
#include "Runtime/Rendering/SceneRenderer.h"
#include "Runtime/Rendering/RHI/RHIEnum.h"
#include "Runtime/Core/Platform/WindowsMonitorHandle.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>

DEFINE_LOG_CATEGORY(LogEngineTest, Info)

namespace
{
	StaticMesh s_fullscreenMesh = StaticMesh::CreateFullscreenTriangle();
	bool s_sceneInitialized = false;
	bool s_loggedColorSpaceSupport = false;

	using Microsoft::WRL::ComPtr;

	constexpr wchar_t kColorSpaceWindowClass[] = L"AshBound_ColorSpaceCheck";

	struct ColorSpaceEntry
	{
		DXGI_COLOR_SPACE_TYPE type = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		const wchar_t* name = L"";
	};

	const std::array<ColorSpaceEntry, 3> kColorSpaces = {
		ColorSpaceEntry{ DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, L"G22_P709" },
		ColorSpaceEntry{ DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709, L"G10_P709" },
		ColorSpaceEntry{ DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, L"G2084_P2020" }
	};

	bool RegisterColorSpaceWindowClass(HINSTANCE instance)
	{
		WNDCLASSEXW wc = {};
		wc.cbSize = sizeof(wc);
		wc.lpfnWndProc = DefWindowProcW;
		wc.hInstance = instance;
		wc.lpszClassName = kColorSpaceWindowClass;
		return RegisterClassExW(&wc) != 0;
	}

	HWND CreateHiddenWindowOnMonitor(const RECT& monitorRect)
	{
		HINSTANCE instance = GetModuleHandleW(nullptr);
		static bool s_registered = RegisterColorSpaceWindowClass(instance);
		if (!s_registered)
		{
			return nullptr;
		}

		const int width = 64;
		const int height = 64;
		const int x = monitorRect.left + 1;
		const int y = monitorRect.top + 1;
		return CreateWindowExW(0, kColorSpaceWindowClass, L"ColorSpaceCheck",
			WS_POPUP, x, y, width, height, nullptr, nullptr, instance, nullptr);
	}

	DXGI_FORMAT ResolveBackbufferFormat()
	{
		const uint32_t bitDepth = ColorManagement::GetBackbufferBitDepth();
		const RHIEnum::Format format = bitDepth >= 16 ? RHIEnum::Format::RGBA16_FLOAT : RHIEnum::Format::R10G10B10A2_UNORM;
		return DX12Format::ToDxgiFormat(format);
	}

	bool FindAdapterForMonitor(IDXGIFactory6* factory, HMONITOR monitor, ComPtr<IDXGIAdapter1>& outAdapter)
	{
		if (!factory || !monitor)
		{
			return false;
		}

		for (UINT adapterIndex = 0; ; ++adapterIndex)
		{
			ComPtr<IDXGIAdapter1> adapter;
			if (factory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}

			for (UINT outputIndex = 0; ; ++outputIndex)
			{
				ComPtr<IDXGIOutput> output;
				if (adapter->EnumOutputs(outputIndex, &output) == DXGI_ERROR_NOT_FOUND)
				{
					break;
				}

				DXGI_OUTPUT_DESC desc = {};
				if (SUCCEEDED(output->GetDesc(&desc)) && desc.Monitor == monitor)
				{
					outAdapter = adapter;
					return true;
				}
			}
		}

		return false;
	}

	void CheckColorSpaceSupport()
	{
		const auto& displays = gEngine->GetDisplaySubsystem().GetDisplays();
		if (displays.empty())
		{
			LOG(LogEngineTest, Warning, L"ColorSpaceSupport: no displays found.");
			return;
		}

		ComPtr<IDXGIFactory6> factory;
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))))
		{
			LOG(LogEngineTest, Warning, L"ColorSpaceSupport: failed to create DXGI factory.");
			return;
		}

		const DXGI_FORMAT backbufferFormat = ResolveBackbufferFormat();
		/*
		for (const DisplayInfo& display : displays)
		{
			const HMONITOR monitor = WindowsMonitorHandle::FromPlatformHandle(display.monitor).handle;
			if (!monitor)
			{
				LOG(LogEngineTest, Warning, L"ColorSpaceSupport: invalid monitor for display %s.",
					display.deviceName.c_str());
				continue;
			}

			ComPtr<IDXGIAdapter1> adapter;
			if (!FindAdapterForMonitor(factory.Get(), monitor, adapter))
			{
				LOG(LogEngineTest, Warning, L"ColorSpaceSupport: adapter not found for display %s.",
					display.deviceName.c_str());
				continue;
			}

			ComPtr<ID3D12Device> device;
			if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
			{
				LOG(LogEngineTest, Warning, L"ColorSpaceSupport: failed to create device for display %s.",
					display.deviceName.c_str());
				continue;
			}

			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			ComPtr<ID3D12CommandQueue> queue;
			if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue))))
			{
				LOG(LogEngineTest, Warning, L"ColorSpaceSupport: failed to create command queue for display %s.",
					display.deviceName.c_str());
				continue;
			}

			HWND window = CreateHiddenWindowOnMonitor(display.monitorRect);
			if (!window)
			{
				LOG(LogEngineTest, Warning, L"ColorSpaceSupport: failed to create hidden window for display %s.",
					display.deviceName.c_str());
				continue;
			}

			DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
			swapDesc.Width = 64;
			swapDesc.Height = 64;
			swapDesc.Format = backbufferFormat;
			swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapDesc.BufferCount = 2;
			swapDesc.SampleDesc.Count = 1;
			swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

			ComPtr<IDXGISwapChain1> swapChain1;
			if (FAILED(factory->CreateSwapChainForHwnd(queue.Get(), window, &swapDesc, nullptr, nullptr, &swapChain1)))
			{
				LOG(LogEngineTest, Warning, L"ColorSpaceSupport: failed to create swapchain for display %s.",
					display.deviceName.c_str());
				DestroyWindow(window);
				continue;
			}

			ComPtr<IDXGISwapChain3> swapChain;
			swapChain1.As(&swapChain);

			LOG(LogEngineTest, Info, L"ColorSpaceSupport: Display(%s) Friendly(%s) HDR(Supported=%d Active=%d) Format=%d",
				display.deviceName.c_str(),
				display.friendlyName.empty() ? L"N/A" : display.friendlyName.c_str(),
				display.hdrSupported ? 1 : 0,
				display.hdrActive ? 1 : 0,
				static_cast<int>(backbufferFormat));

			for (const ColorSpaceEntry& entry : kColorSpaces)
			{
				UINT support = 0;
				const HRESULT hr = swapChain->CheckColorSpaceSupport(entry.type, &support);
				const bool present = SUCCEEDED(hr) &&
					(support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT);
				LOG(LogEngineTest, Info, L"  ColorSpace %s: %s",
					entry.name,
					present ? L"Supported" : L"NotSupported");
			}

			DestroyWindow(window);
		}
		*/
	}
}

ViewportSubsystem* EngineTest::s_viewportSubsystem = nullptr;
EnhancedTriggerEventCallbackHandle EngineTest::s_backbufferToggleHandle = {};

void EngineTest::Init(ViewportSubsystem* viewportSubsystem)
{
	s_viewportSubsystem = viewportSubsystem;
	s_backbufferToggleHandle = Input::AddEnhancedTriggerEventCallbackHandleEx(L"ToggleBackbufferBitDepth",
		EnhancedTriggerEvent::Triggered,
		Input::TriggerEventCallback::Create(&EngineTest::HandleBackbufferToggle));

	Input::BindAction(L"ToggleHdrPreference", KeyCode::C);
	Input::AddActionCallback(L"ToggleHdrPreference", ActionEvent::Pressed,
		[]()
		{
			if (!EngineTest::s_viewportSubsystem)
			{
				return;
			}
			const auto windows = EngineTest::s_viewportSubsystem->GetViewportWindows();
			if (windows.empty())
			{
				return;
			}
			HdrPreference newPreference = HdrPreference::Auto;
			for (const auto& window : windows)
			{
				Viewport* viewport = EngineTest::s_viewportSubsystem->GetViewportByWindow(window);
				if (!viewport)
				{
					continue;
				}
				// Cycle: Auto -> ForceSDR -> ForceHDR -> Auto
				const HdrPreference current = viewport->GetHdrPreference();
				if (current == HdrPreference::Auto)
				{
					newPreference = HdrPreference::ForceSDR;
				}
				else if (current == HdrPreference::ForceSDR)
				{
					newPreference = HdrPreference::ForceHDR;
				}
				else
				{
					newPreference = HdrPreference::Auto;
				}
				break;
			}
			for (const auto& window : windows)
			{
				Viewport* viewport = EngineTest::s_viewportSubsystem->GetViewportByWindow(window);
				if (viewport)
				{
					viewport->SetHdrPreference(newPreference);
				}
			}
			const wchar_t* preferenceName = L"Auto";
			if (newPreference == HdrPreference::ForceSDR)
			{
				preferenceName = L"ForceSDR";
			}
			else if (newPreference == HdrPreference::ForceHDR)
			{
				preferenceName = L"ForceHDR";
			}
			LOG(LogEngineTest, Info, L"HdrPreference switched to %s.", preferenceName);
		});

	Input::AddEnhancedTriggerEventCallbackEx(L"ToggleTonemap", EnhancedTriggerEvent::Triggered,
		[](const EnhancedTriggerEventInfo&)
		{
			SceneRenderer::ToggleTonemapEnabled();
			LOG(LogEngineTest, Info, L"Tonemap %s.",
				SceneRenderer::IsTonemapEnabled() ? L"Enabled" : L"Disabled");
		});

	Input::AddEnhancedTriggerEventCallbackEx(L"SwitchWorkingColorSpace", EnhancedTriggerEvent::Triggered,
		[](const EnhancedTriggerEventInfo& info)
		{
			if (!((info.keyCode >= KeyCode::D1 && info.keyCode <= KeyCode::D6) ||
				(info.keyCode >= KeyCode::NumPad1 && info.keyCode <= KeyCode::NumPad6)))
			{
				return;
			}

			if (SceneRenderer::GetTonemapMode() != 1u)
			{
				SceneRenderer::SetTonemapMode(1u);
				LOG(LogEngineTest, Info, L"Tonemap mode locked to ACES.");
			}
		});

	if (!s_loggedColorSpaceSupport)
	{
		s_loggedColorSpaceSupport = true;
		CheckColorSpaceSupport();
	}

	if (!s_sceneInitialized && gEngine)
	{
		Scene& scene = gEngine->GetScene();
		auto& cubeObject = scene.EmplaceObject<SceneObject>();
		cubeObject.GetTransform().SetPosition(Math::Vector3(0.0f, 0.0f, 0.0f));
		scene.CreateComponent<StaticMeshComponent>(cubeObject, &s_fullscreenMesh);

		auto& camera = scene.EmplaceObject<Camera>();
		camera.SetFovDegrees(90.0f);
		camera.GetTransform().SetPosition(Math::Vector3(0.0f, 1.5f, -3.0f));
		camera.LookAt(Math::Vector3(0.0f, 0.0f, 0.0f));

		s_sceneInitialized = true;
	}

}

void EngineTest::Shutdown()
{
	if (s_backbufferToggleHandle.IsValid())
	{
		Input::RemoveEnhancedTriggerEventCallback(s_backbufferToggleHandle);
		s_backbufferToggleHandle = {};
	}
	s_viewportSubsystem = nullptr;
}

void EngineTest::HandleViewportChanged(const ViewportChangedInfo& info)
{
	if (!s_viewportSubsystem)
	{
		return;
	}
	Viewport* viewport = s_viewportSubsystem->GetViewportByWindow(info.window);
	if (!viewport)
	{
		return;
	}

	const wchar_t* eventName = L"ViewportChanged";
	if ((info.changeType & ViewportChangeType::Size) != ViewportChangeType::None)
	{
		eventName = L"ViewportChanged(Size)";
	}
	else if ((info.changeType & ViewportChangeType::Position) != ViewportChangeType::None)
	{
		eventName = L"ViewportChanged(Position)";
	}
	else if ((info.changeType & ViewportChangeType::Display) != ViewportChangeType::None)
	{
		eventName = L"ViewportChanged(Display)";
	}
	else if ((info.changeType & ViewportChangeType::HdrPreference) != ViewportChangeType::None)
	{
		eventName = L"ViewportChanged(HdrPreference)";
	}

	LogViewportInfo(eventName, viewport->GetViewportInfo(), info.window);
}

void EngineTest::HandleBackbufferToggle(const EnhancedTriggerEventInfo& info)
{
	Engine* engine = gEngine;
	if (!engine)
	{
		return;
	}

	const uint32_t current = ColorManagement::GetBackbufferBitDepth();
	const uint32_t next = current == 16u ? 10u : 16u;
	if (engine->RequestBackbufferBitDepth(next))
	{
		LOG(LogEngineTest, Info, L"Backbuffer bit depth switched to %u.", next);
	}
	else
	{
		LOG(LogEngineTest, Warning, L"Backbuffer bit depth switch failed.");
	}
}

void EngineTest::LogViewportInfo(const wchar_t* eventName, const ViewportInfo& info, PlatformWindowHandle window)
{
	LOG(LogEngineTest, Info,
		L"ViewportInfo Event(%s) Window(%p) Size(%.1f,%.1f) Inv(%.6f,%.6f) Aspect(%.6f) InvAspect(%.6f) Center(%.1f,%.1f) WorkingCS(%d) OutputGamut(%d) "
		L"MaxLum(%.3f) MinLumLog10(%.3f) EOTF(%d) Gamma(%.3f) InvGamma(%.3f)",
		eventName,
		window.handle,
		info.width, info.height,
		info.invWidth, info.invHeight,
		info.aspectRatio, info.invAspectRatio,
		info.centerX, info.centerY,
		info.workingColorSpaceId,
		info.outputGamutId,
		info.maxLuminance,
		info.minLuminanceLog10,
		info.eotfId,
		info.gamma,
		info.invGamma);
}
