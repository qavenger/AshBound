#pragma once

#include "Runtime/Core/CoreDelegate.h"

class ViewportSubsystem;
struct ViewportInfo;

class EngineTest
{
public:
	static void Init(ViewportSubsystem* viewportSubsystem);
	static void Shutdown();

private:
	static void HandleWindowMoved(const WindowMovedInfo& info);
	static void HandleHdrStateChanged(const HdrStateChangedInfo& info);
	static void LogViewportInfo(const wchar_t* eventName, const ViewportInfo& info, PlatformWindowHandle window);

	static ViewportSubsystem* s_viewportSubsystem;
	static DelegateHandle s_windowMovedHandle;
	static DelegateHandle s_hdrStateHandle;
};
