#pragma once
#include "Runtime/Core/Input.h"
#include "Runtime/Core/HighPrecisionClock.h"
#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/DisplaySubsystem.h"
#include "Runtime/Core/ViewportSubsystem.h"

class Engine
{
public:
	Engine() = default;
	~Engine() = default;

	bool Initialize();

	void Run();
	void Destroy();
	double GetRuntimeSeconds() const;
	double GetDeltaSeconds() const;
	bool SetWorkingColorSpaceOverride(WorkingColorSpacePreset preset,
		const WorkingColorSpaceDefinition* customDefinition = nullptr);
protected:
	void Update();
	void Render();
	void RegisterEnhancedInputCallbacks();
private:
	bool IsRunning;
	ViewportSubsystem m_viewportSubsystem;
	DisplaySubsystem m_displaySubsystem;
	Viewport* m_mainViewport = nullptr;
	HighPrecisionClock m_runtimeClock;
	double m_cachedRuntimeSeconds = 0.0;
	double m_cachedDeltaSeconds = 0.0;
};

