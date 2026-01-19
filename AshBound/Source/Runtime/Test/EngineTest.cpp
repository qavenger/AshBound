#include "Runtime/Test/EngineTest.h"

#include "Runtime/Core/Log.h"
#include "Runtime/Core/ViewportSubsystem.h"

DEFINE_LOG_CATEGORY(LogEngineTest, Info)

ViewportSubsystem* EngineTest::s_viewportSubsystem = nullptr;
DelegateHandle EngineTest::s_windowMovedHandle = DelegateHandle::Invalid();
DelegateHandle EngineTest::s_hdrStateHandle = DelegateHandle::Invalid();

void EngineTest::Init(ViewportSubsystem* viewportSubsystem)
{
	s_viewportSubsystem = viewportSubsystem;
	s_windowMovedHandle = CoreDelegate::AddWindowMovedCallback(
		CoreDelegate::WindowMovedCallback::Create(&EngineTest::HandleWindowMoved));
	s_hdrStateHandle = CoreDelegate::AddHdrStateChangedCallback(
		CoreDelegate::HdrStateChangedCallback::Create(&EngineTest::HandleHdrStateChanged));
}

void EngineTest::Shutdown()
{
	if (s_windowMovedHandle.IsValid())
	{
		CoreDelegate::RemoveWindowMovedCallback(s_windowMovedHandle);
		s_windowMovedHandle = DelegateHandle::Invalid();
	}
	if (s_hdrStateHandle.IsValid())
	{
		CoreDelegate::RemoveHdrStateChangedCallback(s_hdrStateHandle);
		s_hdrStateHandle = DelegateHandle::Invalid();
	}
	s_viewportSubsystem = nullptr;
}

void EngineTest::HandleWindowMoved(const WindowMovedInfo& info)
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
	LogViewportInfo(L"WindowMoved", viewport->GetViewportInfo(), info.window);
}

void EngineTest::HandleHdrStateChanged(const HdrStateChangedInfo& info)
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
	LogViewportInfo(L"HdrStateChanged", viewport->GetViewportInfo(), info.window);
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
