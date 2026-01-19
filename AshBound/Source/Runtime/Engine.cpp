#include "Engine.h"
#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/DisplayManager.h"
#include "Runtime/Core/Log.h"
#include <Core/CoreDelegate.h>
#include <cmath>

#ifdef _DEBUG
#include "DebugConsole.h"
#include "Runtime/Test/EngineTest.h"
#endif // _DEBUG

DEFINE_LOG_CATEGORY(LogEngine, Info)


bool Engine::Initialize()
{
#ifdef _DEBUG
	DebugConsole::Init();
	Log::Init();
#endif // _DEBUG
	m_viewportSubsystem.Initialize();
	m_mainViewport = m_viewportSubsystem.CreateViewport(1920, 1080, L"Viewport");
	IsRunning = m_mainViewport != nullptr;
	if (IsRunning)
	{
		m_displaySubsystem.Initialize(&m_viewportSubsystem, m_mainViewport->GetWindowHandle());
		const auto& displays = m_displaySubsystem.GetDisplays();
		LOG(LogEngine, Info, L"Detected %u display(s).", static_cast<unsigned int>(displays.size()));
		for (const auto& display : displays)
		{
			const RECT& area = display.monitorRect;
			const RECT& work = display.workRect;
			LOG(LogEngine, Info, L"Display(%s) Friendly(%s) Area(%ld,%ld,%ld,%ld) Work(%ld,%ld,%ld,%ld) Primary(%d) Active(%d) DPI(%u,%u) HDR(Supported=%d Active=%d) Modes(%u)",
				display.deviceName.c_str(),
				display.friendlyName.empty() ? L"N/A" : display.friendlyName.c_str(),
				area.left, area.top, area.right, area.bottom,
				work.left, work.top, work.right, work.bottom,
				display.isPrimary ? 1 : 0,
				display.isActive ? 1 : 0,
				display.dpiX,
				display.dpiY,
				display.hdrSupported ? 1 : 0,
				display.hdrActive ? 1 : 0,
				static_cast<unsigned int>(display.supportedModes.size()));
		}
		ColorManagement::InitializeFromConfig();
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
	Log::Shutdown();
	DebugConsole::Free();
#endif
	Input::Shutdown();
	m_displaySubsystem.Shutdown();
	m_viewportSubsystem.Shutdown();
}

void Engine::Update()
{
	m_runtimeClock.Tick();
	m_cachedRuntimeSeconds = m_runtimeClock.GetTotalSeconds();
	m_cachedDeltaSeconds = m_runtimeClock.GetDeltaSeconds();
	Input::Update();

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

bool Engine::SetWorkingColorSpaceOverride(WorkingColorSpacePreset preset,
	const WorkingColorSpaceDefinition* customDefinition)
{
	return ColorManagement::SetWorkingColorSpaceOverride(preset, customDefinition);
}

void Engine::Render()
{
}

void Engine::RegisterEnhancedInputCallbacks()
{
	Input::AddEnhancedTriggerEventCallbackEx(L"Quit", EnhancedTriggerEvent::Triggered,
		[this](const EnhancedTriggerEventInfo& info)
		{
			Log::Write(L"$f", info.heldSeconds);
			IsRunning = false;
		});

	Input::AddEnhancedTriggerEventCallbackEx(L"SwitchWorkingColorSpace", EnhancedTriggerEvent::Triggered,
		[this](const EnhancedTriggerEventInfo& info)
		{
			WorkingColorSpacePreset preset;
			switch (info.keyCode)
			{
			case KeyCode::D1:
			case KeyCode::NumPad1:
				preset = WorkingColorSpacePreset::SRGB;
				break;
			case KeyCode::D2:
			case KeyCode::NumPad2:
				preset = WorkingColorSpacePreset::P3D65;
				break;
			case KeyCode::D3:
			case KeyCode::NumPad3:
				preset = WorkingColorSpacePreset::BT2020;
				break;
			case KeyCode::D4:
			case KeyCode::NumPad4:
				preset = WorkingColorSpacePreset::AP1;
				break;
			case KeyCode::D5:
			case KeyCode::NumPad5:
				preset = WorkingColorSpacePreset::AP0;
				break;
			default:
				return;
			}
			if (preset == ColorManagement::GetWorkingColorSpacePreset())
			{
				return;
			}
			SetWorkingColorSpaceOverride(preset);
		});

	CoreDelegate::AddWindowSizeChangedCallback([](const WindowSizeChangedInfo& info) {
		DisplayInfo displayInfo{};
		if (DisplayManager::TryGetDisplayInfoFromWindow(info.window, displayInfo))
		{
			const RECT& area = displayInfo.monitorRect;
			const RECT& work = displayInfo.workRect;
			LOG(LogEngine, Info, L"Window(%p) Size(%d, %d) Monitor(%s) Friendly(%s) Area(%ld,%ld,%ld,%ld) Work(%ld,%ld,%ld,%ld) Primary(%d) DPI(%u,%u) HDR(Supported=%d Active=%d)",
				info.window.handle,
				info.width,
				info.height,
				displayInfo.deviceName.c_str(),
				displayInfo.friendlyName.empty() ? L"N/A" : displayInfo.friendlyName.c_str(),
				area.left, area.top, area.right, area.bottom,
				work.left, work.top, work.right, work.bottom,
				displayInfo.isPrimary ? 1 : 0,
				displayInfo.dpiX,
				displayInfo.dpiY,
				displayInfo.hdrSupported ? 1 : 0,
				displayInfo.hdrActive ? 1 : 0);
		}
		else
		{
			LOG(LogEngine, Info, L"Window(%p) Size(%d, %d) Monitor(N/A)", info.window.handle, info.width, info.height);
		}
		});
}
