#pragma once
#include "Runtime/Core/Input.h"
#include "Runtime/Core/HighPrecisionClock.h"
#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/DisplaySubsystem.h"
#include "Runtime/Core/UI/UserInterfaceSubsystem.h"
#include "Runtime/Core/ViewportSubsystem.h"
#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/Delegates/Delegate.h"
#include "Runtime/Rendering/SceneRenderer.h"
#include "Runtime/Engine/Scene.h"

class Engine
{
public:
	Engine() = default;
	~Engine() = default;

	bool Initialize();
	bool PostInitialize();

	void Run();
	void Destroy();
	double GetRuntimeSeconds() const;
	double GetDeltaSeconds() const;
	bool SetWorkingColorSpaceOverride(WorkingColorSpacePreset preset,
		const WorkingColorSpaceDefinition* customDefinition = nullptr);
	bool RequestBackbufferBitDepth(uint32_t bitDepth);
	Scene& GetScene();
	const Scene& GetScene() const;
	const DisplaySubsystem& GetDisplaySubsystem() const { return m_displaySubsystem; }

protected:
	void Update();
	void Render();
	void RegisterEnhancedInputCallbacks();
	void RegisterRendererCallbacks();
	void UnregisterRendererCallbacks();
	void HandleViewportChanged(const ViewportChangedInfo& info);
	void ApplyDeferredViewportChanges();
	void ProcessPendingSwapChainRebuild();
	bool BuildRendererInitInfo(RHIRendererInitInfo& outInfo) const;
	RHIEnum::ColorSpace ComputeSwapChainColorSpace(const Viewport* viewport) const;
	RHIEnum::Format ResolveSwapChainFormat(RHIEnum::ColorSpace colorSpace) const;
	void UpdateViewportLuminanceFromSwapChain(Viewport* viewport);

private:
	bool IsRunning;
	DisplaySubsystem m_displaySubsystem;
	UserInterfaceSubsystem m_userInterfaceSubsystem;
	ViewportSubsystem m_viewportSubsystem;
	Viewport* m_mainViewport = nullptr;
	SceneRenderer m_sceneRenderer;
	Scene m_scene;
	HighPrecisionClock m_runtimeClock;
	double m_cachedRuntimeSeconds = 0.0;
	double m_cachedDeltaSeconds = 0.0;
	DelegateHandle m_viewportChangedHandle = DelegateHandle::Invalid();
	bool m_ignoreViewportChanged = false;
	bool m_deferViewportChanges = false;
	bool m_hasDeferredViewportChange = false;
	ViewportChangedInfo m_deferredViewportChange = {};
	bool m_hasPendingSwapChainRebuild = false;
	uint32_t m_pendingSwapChainWidth = 0;
	uint32_t m_pendingSwapChainHeight = 0;
	RHIEnum::Format m_pendingSwapChainFormat = RHIEnum::Format::Unknown;
	RHIEnum::ColorSpace m_pendingSwapChainColorSpace = RHIEnum::ColorSpace::SDR_G22_P709;
};

extern Engine* gEngine;