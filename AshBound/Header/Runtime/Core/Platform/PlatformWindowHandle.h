#pragma once

#include <cstddef>
#include <functional>

struct PlatformWindowHandle
{
	void* handle = nullptr;

	bool IsValid() const { return handle != nullptr; }
};

inline bool operator==(const PlatformWindowHandle& lhs, const PlatformWindowHandle& rhs)
{
	return lhs.handle == rhs.handle;
}

inline bool operator!=(const PlatformWindowHandle& lhs, const PlatformWindowHandle& rhs)
{
	return !(lhs == rhs);
}

namespace std
{
	template<>
	struct hash<PlatformWindowHandle>
	{
		size_t operator()(const PlatformWindowHandle& value) const noexcept
		{
			return std::hash<void*>{}(value.handle);
		}
	};
}
