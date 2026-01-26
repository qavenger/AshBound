#pragma once

#include "Runtime/Core/CoreDelegate.h"
#include "Runtime/Core/Input.h"

class ViewportSubsystem;
struct ViewportInfo;

class EngineTest
{
public:
	static void Init(ViewportSubsystem* viewportSubsystem);
	static void Shutdown();

private:
	static void HandleViewportChanged(const ViewportChangedInfo& info);
	static void HandleBackbufferToggle(const EnhancedTriggerEventInfo& info);
	static void LogViewportInfo(const wchar_t* eventName, const ViewportInfo& info, PlatformWindowHandle window);

	static ViewportSubsystem* s_viewportSubsystem;
	static DelegateHandle s_viewportChangedHandle;
	static EnhancedTriggerEventCallbackHandle s_backbufferToggleHandle;
};
