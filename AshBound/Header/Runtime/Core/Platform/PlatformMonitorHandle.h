#pragma once

#include <cstddef>
#include <functional>
#include <memory>

class DisplaySubsystem;

/// Abstract base class for platform-specific monitor handles.
/// Provides HDR state management and platform-specific HDR control.
class PlatformMonitorHandle
{
	friend class DisplaySubsystem;

public:
	virtual ~PlatformMonitorHandle() = default;

	/// Check if the monitor handle is valid
	virtual bool IsValid() const = 0;

	/// Get the native platform handle as void*
	virtual void* GetNativeHandle() const = 0;

	/// Compare two monitor handles for equality
	virtual bool Equals(const PlatformMonitorHandle* other) const = 0;

	/// Get hash value for use in containers
	virtual size_t GetHash() const = 0;

	// ========================================================================
	// HDR State (managed by DisplaySubsystem)
	// ========================================================================

	/// Get the initial HDR state when the engine started
	bool GetInitialHdrActive() const { return m_initialHdrActive; }

	/// Check if the engine has modified this monitor's HDR state
	bool IsHdrModifiedByEngine() const { return m_hdrModifiedByEngine; }

protected:
	/// Try to enable/disable HDR on this monitor (platform-specific implementation)
	/// Only callable by DisplaySubsystem (friend)
	virtual bool TrySetHdrEnabled(bool enable) = 0;

	// HDR state management (set by DisplaySubsystem)
	bool m_initialHdrActive = false;
	bool m_initialHdrStateValid = false;
	bool m_hdrModifiedByEngine = false;
};

using PlatformMonitorHandlePtr = std::shared_ptr<PlatformMonitorHandle>;

/// Helper for using PlatformMonitorHandlePtr in containers
struct PlatformMonitorHandlePtrHash
{
	size_t operator()(const PlatformMonitorHandlePtr& ptr) const noexcept
	{
		return ptr ? ptr->GetHash() : 0;
	}
};

struct PlatformMonitorHandlePtrEqual
{
	bool operator()(const PlatformMonitorHandlePtr& lhs, const PlatformMonitorHandlePtr& rhs) const noexcept
	{
		if (!lhs && !rhs) return true;
		if (!lhs || !rhs) return false;
		return lhs->Equals(rhs.get());
	}
};
