#include "Engine.h"
#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/Viewport.h"
#include "Runtime/Rendering/RHI/RHISwapChain.h"
#include "Runtime/Rendering/RHI/RHIEnum.h"
#include <cmath>

#ifdef _DEBUG
#include "DebugConsole.h"
#include "Runtime/Test/EngineTest.h"
#endif // _DEBUG

namespace
{
	const wchar_t* ColorSpaceToString(RHIEnum::ColorSpace colorSpace)
	{
		switch (colorSpace)
		{
		case RHIEnum::ColorSpace::HDR_G10_P709:
			return L"G10_P709";
		case RHIEnum::ColorSpace::HDR_G2084_P2020:
			return L"G2084_P2020";
		case RHIEnum::ColorSpace::SDR_G22_P709:
		default:
			return L"G22_P709";
		}
	}
}

DEFINE_LOG_CATEGORY(LogEngine, Info)

Engine* gEngine = nullptr;

bool Engine::Initialize()
{
	gEngine = this;
#ifdef _DEBUG
	DebugConsole::Init();
	Log::Init();
#endif // _DEBUG

	// Initialize DisplaySubsystem first (it manages display devices)
	m_displaySubsystem.Initialize();

	// Log detected displays
	const auto& displays = m_displaySubsystem.GetDisplays();
	LOG(LogEngine, Info, L"Detected %u display(s).", static_cast<unsigned int>(displays.size()));
	for (const auto& display : displays)
	{
		const RECT& area = display.monitorRect;
		const RECT& work = display.workRect;
		LOG(LogEngine, Info, L"Display(%s) Friendly(%s) Area(%ld,%ld,%ld,%ld) Work(%ld,%ld,%ld,%ld) Primary(%d) DPI(%u,%u) HDR(Supported=%d Active=%d) Modes(%u)",
			display.deviceName.c_str(),
			display.friendlyName.empty() ? L"N/A" : display.friendlyName.c_str(),
			area.left, area.top, area.right, area.bottom,
			work.left, work.top, work.right, work.bottom,
			display.isPrimary ? 1 : 0,
			display.dpiX,
			display.dpiY,
			display.hdrSupported ? 1 : 0,
			display.hdrActive ? 1 : 0,
			static_cast<unsigned int>(display.supportedModes.size()));
	}

	// Initialize ViewportSubsystem with DisplaySubsystem reference
	m_viewportSubsystem.Initialize(&m_displaySubsystem, &m_userInterfaceSubsystem);

	// Create main viewport
	m_mainViewport = m_viewportSubsystem.CreateViewport(1920, 1080, L"Viewport");
	IsRunning = m_mainViewport != nullptr;

	if (IsRunning)
	{
		ColorManagement::InitializeFromConfig();
		SceneRenderer::SetTonemapEnabled(true);

		ColorManagement::AddWorkingColorSpaceChangedCallback(
			ColorManagement::WorkingColorSpaceChangedCallback::Create(
				[](const ColorSpace& space, const std::wstring& name)
				{
					LOG(LogEngine, Info, L"Working color space switched to %s.", name.c_str());
					LOG(LogEngine, Info, L"Working RGB->XYZ = %s", space.RGBtoXYZ.ToString().c_str());
					LOG(LogEngine, Info, L"Working XYZ->RGB = %s", space.XYZtoRGB.ToString().c_str());
				}));

		Input::Init(m_mainViewport->GetWindowHandle());
		Input::LoadEnhancedInputConfig(L"Config/Input");
		Input::SetEnhancedTimeProvider([this]()
		{
			return m_cachedRuntimeSeconds;
		});

#ifdef _DEBUG
		EngineTest::Init(&m_viewportSubsystem);
#endif // _DEBUG

		RegisterEnhancedInputCallbacks();
		if (!PostInitialize())
		{
			IsRunning = false;
		}
	}
	return IsRunning;
}

void Engine::Run()
{
	m_runtimeClock.Reset();
	m_runtimeClock.Tick();
	m_cachedRuntimeSeconds = m_runtimeClock.GetTotalSeconds();
	m_cachedDeltaSeconds = m_runtimeClock.GetDeltaSeconds();

	while (IsRunning)
	{
		if (!m_viewportSubsystem.ProcessMessages())
		{
			IsRunning = false;
			break;
		}
		Update();
		Render();
	}

	Destroy();
}

void Engine::Destroy()
{
#ifdef _DEBUG
	EngineTest::Shutdown();
#endif
	UnregisterRendererCallbacks();
	m_userInterfaceSubsystem.Shutdown();
	m_sceneRenderer.Shutdown();
#ifdef _DEBUG
	Log::Shutdown();
	DebugConsole::Free();
#endif
	Input::Shutdown();
	m_viewportSubsystem.Shutdown();
	m_displaySubsystem.Shutdown();

	if (gEngine == this)
	{
		gEngine = nullptr;
	}
}

void Engine::Update()
{
	m_runtimeClock.Tick();
	m_cachedRuntimeSeconds = m_runtimeClock.GetTotalSeconds();
	m_cachedDeltaSeconds = m_runtimeClock.GetDeltaSeconds();
	Input::Update();
	m_scene.Update(static_cast<float>(m_cachedDeltaSeconds));

#ifdef _DEBUG
	const bool shiftDown = Input::IsKeyDown(KeyCode::Shift) ||
		Input::IsKeyDown(KeyCode::LeftShift) ||
		Input::IsKeyDown(KeyCode::RightShift);
	if (shiftDown && Input::WasKeyPressed(KeyCode::R))
	{
		if (Input::ReloadEnhancedInputConfig())
		{
			RegisterEnhancedInputCallbacks();
			LOG(LogEngine, Info, L"EnhancedInput config reloaded.");
		}
		else
		{
			LOG(LogEngine, Warning, L"EnhancedInput config reload failed.");
		}
	}

	if (Input::WasKeyPressed(KeyCode::F5))
	{
		m_sceneRenderer.RequestShaderReload();
		LOG(LogEngine, Info, L"Shader hot reload requested.");
	}
#endif
}

double Engine::GetRuntimeSeconds() const
{
	return m_cachedRuntimeSeconds;
}

double Engine::GetDeltaSeconds() const
{
	return m_cachedDeltaSeconds;
}

Scene& Engine::GetScene()
{
	return m_scene;
}

const Scene& Engine::GetScene() const
{
	return m_scene;
}

bool Engine::SetWorkingColorSpaceOverride(WorkingColorSpacePreset preset,
	const WorkingColorSpaceDefinition* customDefinition)
{
	return ColorManagement::SetWorkingColorSpaceOverride(preset, customDefinition);
}

bool Engine::RequestBackbufferBitDepth(uint32_t bitDepth)
{
	return m_sceneRenderer.RequestBackbufferBitDepth(bitDepth);
}


bool Engine::PostInitialize()
{
	if (!m_mainViewport)
	{
		return false;
	}

	RHIRendererInitInfo rendererInitInfo;
	if (!BuildRendererInitInfo(rendererInitInfo))
	{
		return false;
	}

	if (!m_sceneRenderer.Initialize(rendererInitInfo))
	{
		return false;
	}
	m_userInterfaceSubsystem.Initialize(m_sceneRenderer.GetDevice(), rendererInitInfo.backbufferFormat);
	UpdateViewportLuminanceFromSwapChain(m_mainViewport);
	RegisterRendererCallbacks();
	return true;
}

void Engine::Render()
{
	ProcessPendingSwapChainRebuild();

	IRHIRenderer* renderer = m_sceneRenderer.GetRenderer();
	if (!renderer)
	{
		return;
	}

	const float clearColor[4] = { 0.07f, 0.07f, 0.09f, 1.0f };
	renderer->BeginFrame(clearColor);
	if (m_mainViewport)
	{
		const ViewportInfo& viewportInfo = m_mainViewport->GetViewportInfo();
		m_sceneRenderer.RenderScene(m_scene, viewportInfo);
		m_deferViewportChanges = true;
		m_sceneRenderer.RenderUserInterfaces(&m_userInterfaceSubsystem);
		m_deferViewportChanges = false;
		ApplyDeferredViewportChanges();
	}
	renderer->EndFrame();
	renderer->Present();
}

void Engine::RegisterEnhancedInputCallbacks()
{
	Input::AddEnhancedTriggerEventCallbackEx(L"Quit", EnhancedTriggerEvent::Triggered,
		[this](const EnhancedTriggerEventInfo& info)
		{
			Log::Write(L"$f", info.heldSeconds);
			IsRunning = false;
		});
}

void Engine::RegisterRendererCallbacks()
{
	m_viewportChangedHandle = CoreDelegate::AddViewportChangedCallback(
		CoreDelegate::ViewportChangedCallback::Create(this, &Engine::HandleViewportChanged));
}

void Engine::UnregisterRendererCallbacks()
{
	if (m_viewportChangedHandle.IsValid())
	{
		CoreDelegate::RemoveViewportChangedCallback(m_viewportChangedHandle);
		m_viewportChangedHandle = DelegateHandle::Invalid();
	}
}

void Engine::HandleViewportChanged(const ViewportChangedInfo& info)
{
	if (m_ignoreViewportChanged)
	{
		return;
	}
	if (!m_mainViewport || info.window != m_mainViewport->GetWindowHandle())
	{
		return;
	}
	if (m_deferViewportChanges)
	{
		m_hasDeferredViewportChange = true;
		m_deferredViewportChange.window = info.window;
		m_deferredViewportChange.changeType = m_deferredViewportChange.changeType | info.changeType;
		return;
	}

	const ViewportChangeType rebuildMask =
		ViewportChangeType::Size |
		ViewportChangeType::Display |
		ViewportChangeType::HdrPreference |
		ViewportChangeType::WindowMode |
		ViewportChangeType::Minimized |
		ViewportChangeType::Maximized;
	if ((info.changeType & rebuildMask) == ViewportChangeType::None)
	{
		return;
	}

	const ViewportInfo& viewportInfo = m_mainViewport->GetViewportInfo();
	const uint32_t width = static_cast<uint32_t>(viewportInfo.width);
	const uint32_t height = static_cast<uint32_t>(viewportInfo.height);

	if (width == 0 || height == 0)
	{
		return;
	}

	// Compute appropriate color space based on HdrPreference and display state
	const RHIEnum::ColorSpace colorSpace = ComputeSwapChainColorSpace(m_mainViewport);
	const RHIEnum::Format format = ResolveSwapChainFormat(colorSpace);

	LOG(LogEngine, Info, L"ViewportChanged: changeType=%u SwapChain pending: Format=%d ColorSpace=%s",
		static_cast<uint32_t>(info.changeType),
		static_cast<int>(format),
		ColorSpaceToString(colorSpace));

	m_pendingSwapChainWidth = width;
	m_pendingSwapChainHeight = height;
	m_pendingSwapChainFormat = format;
	m_pendingSwapChainColorSpace = colorSpace;
	m_hasPendingSwapChainRebuild = true;
	m_sceneRenderer.RequestSwapChainRebuild();
}

void Engine::ApplyDeferredViewportChanges()
{
	if (!m_hasDeferredViewportChange)
	{
		return;
	}
	const ViewportChangedInfo pending = m_deferredViewportChange;
	m_deferredViewportChange = {};
	m_hasDeferredViewportChange = false;
	HandleViewportChanged(pending);
}

void Engine::ProcessPendingSwapChainRebuild()
{
	if (!m_mainViewport)
	{
		return;
	}
	if (!m_sceneRenderer.IsSwapChainRebuildPending())
	{
		return;
	}

	const ViewportInfo& viewportInfo = m_mainViewport->GetViewportInfo();
	const uint32_t width = m_hasPendingSwapChainRebuild
		? m_pendingSwapChainWidth
		: static_cast<uint32_t>(viewportInfo.width);
	const uint32_t height = m_hasPendingSwapChainRebuild
		? m_pendingSwapChainHeight
		: static_cast<uint32_t>(viewportInfo.height);

	if (width == 0 || height == 0)
	{
		return;
	}

	const RHIEnum::ColorSpace colorSpace = m_hasPendingSwapChainRebuild
		? m_pendingSwapChainColorSpace
		: ComputeSwapChainColorSpace(m_mainViewport);
	const RHIEnum::Format format = m_hasPendingSwapChainRebuild
		? m_pendingSwapChainFormat
		: ResolveSwapChainFormat(colorSpace);

	if (m_sceneRenderer.RecreateSwapChain(width, height, format, colorSpace))
	{
		m_hasPendingSwapChainRebuild = false;
		if (format != m_userInterfaceSubsystem.GetBackbufferFormat())
		{
			m_userInterfaceSubsystem.ReinitializeAll(m_sceneRenderer.GetDevice(), format);
		}
		UpdateViewportLuminanceFromSwapChain(m_mainViewport);
		return;
	}

	// HDR swapchain creation failed; roll back to SDR and log the error.
	LOG(LogEngine, Error, L"SwapChain recreate failed (Format=%d ColorSpace=%s). Rolling back to SDR.",
		static_cast<int>(format),
		ColorSpaceToString(colorSpace));

	m_ignoreViewportChanged = true;
	m_mainViewport->SetHdrPreference(HdrPreference::ForceSDR);
	m_ignoreViewportChanged = false;

	const RHIEnum::ColorSpace fallbackColorSpace = RHIEnum::ColorSpace::SDR_G22_P709;
	const RHIEnum::Format fallbackFormat = ResolveSwapChainFormat(fallbackColorSpace);
	if (!m_sceneRenderer.RecreateSwapChain(width, height, fallbackFormat, fallbackColorSpace))
	{
		LOG(LogEngine, Error, L"SwapChain recreate failed after SDR fallback (Format=%d ColorSpace=%s).",
			static_cast<int>(fallbackFormat),
			ColorSpaceToString(fallbackColorSpace));
	}
	else
	{
		m_hasPendingSwapChainRebuild = false;
		if (fallbackFormat != m_userInterfaceSubsystem.GetBackbufferFormat())
		{
			m_userInterfaceSubsystem.ReinitializeAll(m_sceneRenderer.GetDevice(), fallbackFormat);
		}
		UpdateViewportLuminanceFromSwapChain(m_mainViewport);
	}
}

RHIEnum::ColorSpace Engine::ComputeSwapChainColorSpace(const Viewport* viewport) const
{
	if (!viewport)
	{
		return RHIEnum::ColorSpace::SDR_G22_P709;
	}

	const DisplayInfo* displayInfo = viewport->GetDisplayInfo();
	const bool hdrActive = displayInfo && displayInfo->hdrActive;
	const HdrPreference preference = viewport->GetHdrPreference();

	switch (preference)
	{
	case HdrPreference::ForceSDR:
		return RHIEnum::ColorSpace::SDR_G22_P709;

	case HdrPreference::ForceHDR:
		if (hdrActive)
		{
			// Display has Windows HDR enabled: use scRGB (G10_P709)
			return RHIEnum::ColorSpace::HDR_G10_P709;
		}
		else
		{
			// Display does not have Windows HDR enabled: try PQ (G2084_P2020)
			// Note: Creation may fail, renderer will fallback to SDR
			return RHIEnum::ColorSpace::HDR_G2084_P2020;
		}

	case HdrPreference::Auto:
	default:
		if (hdrActive)
		{
			return RHIEnum::ColorSpace::HDR_G10_P709;
		}
		else
		{
			return RHIEnum::ColorSpace::SDR_G22_P709;
		}
	}
}

bool Engine::BuildRendererInitInfo(RHIRendererInitInfo& outInfo) const
{
	if (!m_mainViewport)
	{
		return false;
	}

	const ViewportInfo& viewportInfo = m_mainViewport->GetViewportInfo();
	outInfo.viewport = m_mainViewport->GetPlatformViewport();
	outInfo.width = static_cast<uint32_t>(viewportInfo.width);
	outInfo.height = static_cast<uint32_t>(viewportInfo.height);
	outInfo.enableVsync = true;

	const RHIEnum::ColorSpace colorSpace = ComputeSwapChainColorSpace(m_mainViewport);
	outInfo.colorSpace = colorSpace;
	outInfo.backbufferFormat = ResolveSwapChainFormat(colorSpace);

	LOG(LogEngine, Info, L"Initial SwapChain plan: Format=%d ColorSpace=%s",
		static_cast<int>(outInfo.backbufferFormat),
		ColorSpaceToString(outInfo.colorSpace));

#ifdef _DEBUG
	outInfo.enableDebugLayer = true;
#endif
	return outInfo.viewport != nullptr && outInfo.width > 0 && outInfo.height > 0;
}

RHIEnum::Format Engine::ResolveSwapChainFormat(RHIEnum::ColorSpace colorSpace) const
{
	// scRGB (G10_P709) should use 16-bit float to avoid driver fallback.
	if (colorSpace == RHIEnum::ColorSpace::HDR_G10_P709)
	{
		return RHIEnum::Format::RGBA16_FLOAT;
	}

	const uint32_t bitDepth = ColorManagement::GetBackbufferBitDepth();
	if (bitDepth >= 16)
	{
		return RHIEnum::Format::RGBA16_FLOAT;
	}

	return RHIEnum::Format::R10G10B10A2_UNORM;
}

void Engine::UpdateViewportLuminanceFromSwapChain(Viewport* viewport)
{
	if (!viewport)
	{
		return;
	}
	IRHISwapChain* swapChain = m_sceneRenderer.GetSwapChain();
	if (!swapChain)
	{
		return;
	}
	float maxNits = 0.0f;
	float minNits = 0.0f;
	if (!swapChain->QueryOutputLuminance(maxNits, minNits) || maxNits <= 0.0f)
	{
		return;
	}

	m_ignoreViewportChanged = true;
	viewport->UpdateLuminanceFromSwapChain(maxNits, minNits);
	m_ignoreViewportChanged = false;
}
