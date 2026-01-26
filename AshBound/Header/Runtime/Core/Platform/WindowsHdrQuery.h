#pragma once

#include <Windows.h>

struct WindowsHdrInfo
{
	bool supported = false;
	bool active = false;
	bool activeKnown = false;
	float maxLuminance = 0.0f;
	float minLuminanceLog10 = 0.0f;
	float sdrWhiteLevelNits = 0.0f;
};

class WindowsHdrQuery
{
public:
	static bool TryQuery(HMONITOR monitor, WindowsHdrInfo& outInfo);
};
