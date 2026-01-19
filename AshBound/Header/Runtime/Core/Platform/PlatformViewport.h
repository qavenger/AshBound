#pragma once

#include <string>

#include "Runtime/Core/Platform/PlatformWindowHandle.h"

class Viewport;

class PlatformViewport
{
public:
	explicit PlatformViewport(Viewport* owner);
	virtual ~PlatformViewport() = default;

	virtual bool Initialize(int width, int height, const std::wstring& title) = 0;
	virtual bool ProcessMessages() = 0;
	virtual PlatformWindowHandle GetWindowHandle() const = 0;
	virtual void RefreshMonitorState() = 0;

protected:
	Viewport* m_owner = nullptr;
};
