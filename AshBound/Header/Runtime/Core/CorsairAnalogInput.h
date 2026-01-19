#pragma once

#include <cstdint>
#include <string>
#include <vector>

class CorsairAnalogInput
{
public:
	static bool Init();
	static void Shutdown();
	static void Update();

	static bool GetAnalogValue(uint32_t keyCode, float& outValue);
	static void SetKeyOffset(uint32_t keyCode, int32_t reportOffset);
	static void EnableReportDiffLog(bool enable);

	static bool IsConnected();
	static const std::vector<uint8_t>& GetLastReport();
	static std::wstring GetDevicePath();
};
