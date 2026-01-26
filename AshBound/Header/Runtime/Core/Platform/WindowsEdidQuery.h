#pragma once

#include <string>

struct EdidHdrInfo
{
	bool hdrSupported = false;
	bool hdr10PlusSupported = false;
	bool dolbyVisionSupported = false;
	float maxLuminance = 0.0f;
	float maxFrameAverageLuminance = 0.0f;
	float minLuminance = 0.0f;
};

namespace WindowsEdidQuery
{
	bool TryQuery(const std::wstring& deviceKey,
		const std::wstring& deviceId,
		EdidHdrInfo& outInfo);
}
