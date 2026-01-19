#include "Runtime/Core/HidDebugTool.h"

#include "Runtime/Core/Log.h"

#include <Windows.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <vector>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace
{
	DEFINE_LOG_CATEGORY(LogHID, Info)

	std::wstring ToHex(uint16_t value)
	{
		wchar_t buffer[8] = {};
		swprintf_s(buffer, L"0x%04X", value);
		return buffer;
	}

	void LogLine(const std::wstring& message)
	{
		LOG(LogHID, Info, message);
	}

	std::wstring SafeWide(const wchar_t* value)
	{
		return value ? std::wstring(value) : L"";
	}
}

void HidDebugTool::DumpConnectedDevices()
{
	LogLine(L"--- HID Device Dump ---");

	GUID hidGuid = {};
	HidD_GetHidGuid(&hidGuid);

	HDEVINFO deviceInfo = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (deviceInfo == INVALID_HANDLE_VALUE)
	{
		LogLine(L"HID: SetupDiGetClassDevs failed.");
		return;
	}

	SP_DEVICE_INTERFACE_DATA interfaceData = {};
	interfaceData.cbSize = sizeof(interfaceData);

	for (DWORD index = 0; SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, &hidGuid, index, &interfaceData); ++index)
	{
		DWORD requiredSize = 0;
		SetupDiGetDeviceInterfaceDetailW(deviceInfo, &interfaceData, nullptr, 0, &requiredSize, nullptr);
		if (requiredSize == 0)
		{
			continue;
		}

		std::vector<uint8_t> detailBuffer(requiredSize);
		auto* detailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detailBuffer.data());
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

		if (!SetupDiGetDeviceInterfaceDetailW(deviceInfo, &interfaceData, detailData, requiredSize, nullptr, nullptr))
		{
			continue;
		}

		const std::wstring devicePath = SafeWide(detailData->DevicePath);

		HANDLE deviceHandle = CreateFileW(devicePath.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (deviceHandle == INVALID_HANDLE_VALUE)
		{
			LogLine(L"HID: Failed to open " + devicePath);
			continue;
		}

		HIDD_ATTRIBUTES attributes = {};
		attributes.Size = sizeof(attributes);
		if (HidD_GetAttributes(deviceHandle, &attributes))
		{
			LogLine(L"HID Device: " + devicePath);
			LogLine(L"  VID: " + ToHex(attributes.VendorID) + L" PID: " + ToHex(attributes.ProductID));
		}
		else
		{
			LogLine(L"HID Device: " + devicePath);
		}

		PHIDP_PREPARSED_DATA preparsedData = nullptr;
		if (HidD_GetPreparsedData(deviceHandle, &preparsedData))
		{
			HIDP_CAPS caps = {};
			if (HidP_GetCaps(preparsedData, &caps) == HIDP_STATUS_SUCCESS)
			{
				LogLine(L"  UsagePage: " + ToHex(caps.UsagePage) + L" Usage: " + ToHex(caps.Usage));
				LogLine(L"  InputReportBytes: " + std::to_wstring(caps.InputReportByteLength));
				LogLine(L"  OutputReportBytes: " + std::to_wstring(caps.OutputReportByteLength));
				LogLine(L"  FeatureReportBytes: " + std::to_wstring(caps.FeatureReportByteLength));

				USHORT valueCount = caps.NumberInputValueCaps;
				if (valueCount > 0)
				{
					std::vector<HIDP_VALUE_CAPS> valueCaps(valueCount);
					if (HidP_GetValueCaps(HidP_Input, valueCaps.data(), &valueCount, preparsedData) == HIDP_STATUS_SUCCESS)
					{
						LogLine(L"  Input Value Caps:");
						for (USHORT i = 0; i < valueCount; ++i)
						{
							const HIDP_VALUE_CAPS& cap = valueCaps[i];
							LogLine(L"    UsagePage=" + ToHex(cap.UsagePage) +
								L" Usage=" + ToHex(cap.Range.UsageMin) +
								L" UsageMax=" + ToHex(cap.Range.UsageMax) +
								L" LogicalMin=" + std::to_wstring(cap.LogicalMin) +
								L" LogicalMax=" + std::to_wstring(cap.LogicalMax));
						}
					}
				}
			}
			HidD_FreePreparsedData(preparsedData);
		}

		CloseHandle(deviceHandle);
	}

	SetupDiDestroyDeviceInfoList(deviceInfo);
	LogLine(L"--- HID Device Dump End ---");
}
