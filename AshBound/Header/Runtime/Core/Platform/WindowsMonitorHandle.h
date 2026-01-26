#pragma once

#include <Windows.h>

#include "Runtime/Core/Platform/PlatformMonitorHandle.h"

/// Windows-specific monitor handle implementation
class WindowsMonitorHandle : public PlatformMonitorHandle
{
public:
	explicit WindowsMonitorHandle(HMONITOR handle = nullptr);

	// PlatformMonitorHandle interface
	bool IsValid() const override;
	void* GetNativeHandle() const override;
	bool Equals(const PlatformMonitorHandle* other) const override;
	size_t GetHash() const override;

	/// Get the native HMONITOR handle
	HMONITOR GetHandle() const { return m_handle; }

	/// Create a WindowsMonitorHandle from a native HMONITOR
	static PlatformMonitorHandlePtr Create(HMONITOR handle);

protected:
	bool TrySetHdrEnabled(bool enable) override;

private:
	HMONITOR m_handle = nullptr;
};
