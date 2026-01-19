#pragma once

#include <cstddef>
#include <functional>

struct PlatformMonitorHandle
{
	void* handle = nullptr;

	bool IsValid() const { return handle != nullptr; }
};

inline bool operator==(const PlatformMonitorHandle& lhs, const PlatformMonitorHandle& rhs)
{
	return lhs.handle == rhs.handle;
}

inline bool operator!=(const PlatformMonitorHandle& lhs, const PlatformMonitorHandle& rhs)
{
	return !(lhs == rhs);
}

namespace std
{
	template<>
	struct hash<PlatformMonitorHandle>
	{
		size_t operator()(const PlatformMonitorHandle& value) const noexcept
		{
			return std::hash<void*>{}(value.handle);
		}
	};
}
