#include "Runtime/Core/CorsairAnalogInput.h"

#include "Runtime/Core/Log.h"

#include <Windows.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <algorithm>
#include <array>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace
{
	constexpr uint16_t kCorsairVid = 0x1B1C;
	constexpr USHORT kUsagePage = 0xFF42;
	constexpr USHORT kUsage1 = 0x0001;
	constexpr USHORT kUsage2 = 0x0002;
	DEFINE_LOG_CATEGORY(LogCorsairHID, Info)

	std::array<int32_t, 256> g_keyOffsets = {};
	bool g_dumpReportDiff = false;
	bool g_dumpReportRaw = false;
	constexpr uint32_t kRawLogIntervalMs = 200;
	constexpr uint32_t kNoReportLogIntervalMs = 2000;
	constexpr uint32_t kPendingLogIntervalMs = 2000;

	struct DeviceState
	{
		HANDLE handle = INVALID_HANDLE_VALUE;
		HANDLE readEvent = nullptr;
		OVERLAPPED overlapped = {};
		DWORD reportSize = 0;
		std::vector<uint8_t> reportBuffer;
		std::vector<uint8_t> lastReport;
		std::vector<uint8_t> prevReport;
		std::wstring devicePath;
		HIDP_CAPS caps = {};
		bool readPending = false;
		uint64_t lastRawLogTick = 0;
		uint64_t lastReportTick = 0;
		uint64_t lastNoReportLogTick = 0;
		uint64_t pendingSinceTick = 0;
		uint64_t lastPendingLogTick = 0;
		DWORD lastReadError = 0;
		DWORD lastOverlappedError = 0;
	};

	std::vector<DeviceState> g_devices;

	std::wstring ToHex(uint16_t value)
	{
		wchar_t buffer[8] = {};
		swprintf_s(buffer, L"0x%04X", value);
		return buffer;
	}

	bool GetCapsForHandle(HANDLE deviceHandle, HIDP_CAPS& outCaps)
	{
		PHIDP_PREPARSED_DATA preparsedData = nullptr;
		if (!HidD_GetPreparsedData(deviceHandle, &preparsedData))
		{
			return false;
		}

		const NTSTATUS status = HidP_GetCaps(preparsedData, &outCaps);
		HidD_FreePreparsedData(preparsedData);
		return status == HIDP_STATUS_SUCCESS;
	}

	std::wstring DevicePrefix(size_t index)
	{
		return L"Corsair HID#" + std::to_wstring(index);
	}

	void LogReportDiff(DeviceState& device, size_t index)
	{
		if (device.prevReport.size() != device.lastReport.size())
		{
			device.prevReport = device.lastReport;
			return;
		}

		size_t changes = 0;
		std::wstring message = DevicePrefix(index) + L" diff:";
		for (size_t i = 0; i < device.lastReport.size(); ++i)
		{
			if (device.lastReport[i] != device.prevReport[i])
			{
				if (changes < 16)
				{
					message += L" [";
					message += std::to_wstring(i);
					message += L":";
					message += std::to_wstring(device.prevReport[i]);
					message += L"->";
					message += std::to_wstring(device.lastReport[i]);
					message += L"]";
				}
				++changes;
			}
		}

		if (changes > 0)
		{
			if (changes > 16)
			{
				message += L" ... (";
				message += std::to_wstring(changes);
				message += L" changes)";
			}
			LOG(LogCorsairHID, Info, message);
		}

		device.prevReport = device.lastReport;
	}

	void LogReportRaw(DeviceState& device, size_t index)
	{
		if (!g_dumpReportRaw || device.lastReport.empty())
		{
			return;
		}

		const uint64_t now = GetTickCount64();
		if (now - device.lastRawLogTick < kRawLogIntervalMs)
		{
			return;
		}
		device.lastRawLogTick = now;

		const size_t total = device.lastReport.size();
		const size_t maxBytes = 128;
		const size_t shown = min(total, maxBytes);
		std::wstring message = DevicePrefix(index) + L" raw (" + std::to_wstring(total) + L" bytes):\n";
		for (size_t i = 0; i < shown; ++i)
		{
			wchar_t buffer[8] = {};
			swprintf_s(buffer, L"%02X ", device.lastReport[i]);
			message += buffer;
			if ((i + 1) % 16 == 0 && i + 1 < shown)
			{
				message += L"\n";
			}
		}
		if (total > maxBytes)
		{
			message += L"\n...";
		}
		LOG(LogCorsairHID, Info, message);
	}

	bool StartAsyncRead(DeviceState& device, size_t index)
	{
		if (device.handle == INVALID_HANDLE_VALUE || device.reportSize == 0)
		{
			if (device.reportSize == 0)
			{
				LOG(LogCorsairHID, Warning, DevicePrefix(index) + L" report size is 0.");
			}
			return false;
		}

		if (device.reportBuffer.size() != device.reportSize)
		{
			device.reportBuffer.assign(device.reportSize, 0);
		}

		ResetEvent(device.readEvent);
		device.overlapped.hEvent = device.readEvent;
		DWORD bytesRead = 0;
		const BOOL result = ReadFile(device.handle, device.reportBuffer.data(), device.reportSize, &bytesRead, &device.overlapped);
		if (!result)
		{
			const DWORD error = GetLastError();
			if (error == ERROR_IO_PENDING)
			{
				device.readPending = true;
				if (device.pendingSinceTick == 0)
				{
					device.pendingSinceTick = GetTickCount64();
				}
				return true;
			}
			device.lastReadError = error;
			LOG(LogCorsairHID, Warning, DevicePrefix(index) + L" ReadFile failed: " + std::to_wstring(error));
			return false;
		}

		device.lastReport = device.reportBuffer;
		device.lastReportTick = GetTickCount64();
		device.pendingSinceTick = 0;
		device.lastPendingLogTick = 0;
		if (g_dumpReportDiff)
		{
			LogReportDiff(device, index);
		}
		LogReportRaw(device, index);
		device.readPending = false;
		return true;
	}

	bool FinishAsyncRead(DeviceState& device, size_t index)
	{
		if (!device.readPending)
		{
			return false;
		}

		DWORD bytesRead = 0;
		if (!GetOverlappedResult(device.handle, &device.overlapped, &bytesRead, FALSE))
		{
			const DWORD error = GetLastError();
			device.lastOverlappedError = error;
			LOG(LogCorsairHID, Warning, DevicePrefix(index) + L" GetOverlappedResult failed: " + std::to_wstring(error));
			return false;
		}

		device.lastReport = device.reportBuffer;
		device.lastReportTick = GetTickCount64();
		device.pendingSinceTick = 0;
		device.lastPendingLogTick = 0;
		if (g_dumpReportDiff)
		{
			LogReportDiff(device, index);
		}
		LogReportRaw(device, index);
		device.readPending = false;
		return true;
	}

	bool TryGetInputReport(DeviceState& device, size_t index)
	{
		if (device.handle == INVALID_HANDLE_VALUE || device.reportSize == 0)
		{
			return false;
		}

		std::vector<uint8_t> buffer(device.reportSize, 0);
		if (!HidD_GetInputReport(device.handle, buffer.data(), static_cast<ULONG>(buffer.size())))
		{
			return false;
		}

		device.lastReport = std::move(buffer);
		device.lastReportTick = GetTickCount64();
		device.pendingSinceTick = 0;
		device.lastPendingLogTick = 0;
		if (g_dumpReportDiff)
		{
			LogReportDiff(device, index);
		}
		LogReportRaw(device, index);
		return true;
	}

	void CancelPendingRead(DeviceState& device)
	{
		if (!device.readPending || device.handle == INVALID_HANDLE_VALUE)
		{
			return;
		}
		CancelIoEx(device.handle, &device.overlapped);
		device.readPending = false;
		device.pendingSinceTick = 0;
	}

	void SetDefaultOffsets()
	{
		g_keyOffsets.fill(-1);
		g_keyOffsets['W'] = 1;
		g_keyOffsets['A'] = 2;
		g_keyOffsets['S'] = 3;
		g_keyOffsets['D'] = 4;
	}

	void AddCorsairDevice(const std::wstring& devicePath, HANDLE handle, const HIDP_CAPS& caps)
	{
		DeviceState device;
		device.handle = handle;
		device.reportSize = caps.InputReportByteLength;
		device.devicePath = devicePath;
		device.caps = caps;
		device.lastReportTick = GetTickCount64();
		device.readEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (!device.readEvent)
		{
			CloseHandle(handle);
			return;
		}
		device.overlapped = {};
		device.overlapped.hEvent = device.readEvent;
		g_devices.push_back(std::move(device));
	}

	void EnumerateCorsairDevices(bool requireUsage)
	{
		GUID hidGuid = {};
		HidD_GetHidGuid(&hidGuid);

		HDEVINFO deviceInfo = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
		if (deviceInfo == INVALID_HANDLE_VALUE)
		{
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

			const std::wstring devicePath = detailData->DevicePath ? detailData->DevicePath : L"";
			HANDLE handle = CreateFileW(devicePath.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
				nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				continue;
			}

			HIDD_ATTRIBUTES attributes = {};
			attributes.Size = sizeof(attributes);
			if (!HidD_GetAttributes(handle, &attributes) ||
				attributes.VendorID != kCorsairVid)
			{
				CloseHandle(handle);
				continue;
			}

			HIDP_CAPS caps = {};
			if (!GetCapsForHandle(handle, caps))
			{
				CloseHandle(handle);
				continue;
			}

			std::wstring info = L"Corsair HID candidate: VID=" + ToHex(attributes.VendorID);
			info += L" PID=" + ToHex(attributes.ProductID);
			info += L" UsagePage=" + ToHex(caps.UsagePage);
			info += L" Usage=" + ToHex(caps.Usage);
			info += L" InputReport=" + std::to_wstring(caps.InputReportByteLength);
			LOG(LogCorsairHID, Info, info);

			if (requireUsage && (caps.UsagePage != kUsagePage || (caps.Usage != kUsage1 && caps.Usage != kUsage2)))
			{
				CloseHandle(handle);
				continue;
			}

			if (caps.InputReportByteLength == 0)
			{
				LOG(LogCorsairHID, Warning, L"Corsair HID candidate has no input reports.");
				CloseHandle(handle);
				continue;
			}

			AddCorsairDevice(devicePath, handle, caps);
		}

		SetupDiDestroyDeviceInfoList(deviceInfo);
	}
}

bool CorsairAnalogInput::Init()
{
	if (!g_devices.empty())
	{
		return true;
	}

	SetDefaultOffsets();

	EnumerateCorsairDevices(true);
	if (g_devices.empty())
	{
		LOG(LogCorsairHID, Warning, L"Corsair analog HID not found for vendor usage. Falling back to any Corsair HID interface.");
		EnumerateCorsairDevices(false);
	}

	if (g_devices.empty())
	{
		LOG(LogCorsairHID, Warning, L"No Corsair HID interface available.");
		return false;
	}

	for (size_t i = 0; i < g_devices.size(); ++i)
	{
		LOG(LogCorsairHID, Info, DevicePrefix(i) + L" connected: " + g_devices[i].devicePath);
		LOG(LogCorsairHID, Info, DevicePrefix(i) + L" report size: " + std::to_wstring(g_devices[i].reportSize));
		StartAsyncRead(g_devices[i], i);
	}
	return true;
}

void CorsairAnalogInput::Shutdown()
{
	g_dumpReportDiff = false;
	g_dumpReportRaw = false;
	for (auto& device : g_devices)
	{
		if (device.readEvent)
		{
			CloseHandle(device.readEvent);
			device.readEvent = nullptr;
		}
		if (device.handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(device.handle);
			device.handle = INVALID_HANDLE_VALUE;
		}
	}
	g_devices.clear();
}

void CorsairAnalogInput::Update()
{
	if (g_devices.empty())
	{
		return;
	}

	const uint64_t now = GetTickCount64();
	for (size_t i = 0; i < g_devices.size(); ++i)
	{
		DeviceState& device = g_devices[i];
		if (device.readPending)
		{
			if (device.pendingSinceTick == 0)
			{
				device.pendingSinceTick = now;
			}
			if (WaitForSingleObject(device.readEvent, 0) == WAIT_OBJECT_0)
			{
				if (FinishAsyncRead(device, i))
				{
					StartAsyncRead(device, i);
				}
			}
		}
		else
		{
			StartAsyncRead(device, i);
		}

		if (device.lastReportTick == 0)
		{
			device.lastReportTick = now;
		}
		if (now - device.lastReportTick > kNoReportLogIntervalMs &&
			now - device.lastNoReportLogTick > kNoReportLogIntervalMs)
		{
			device.lastNoReportLogTick = now;
			LOG(LogCorsairHID, Warning, DevicePrefix(i) + L": no input reports received.");
		}

		if (device.readPending &&
			now - device.pendingSinceTick > kPendingLogIntervalMs &&
			now - device.lastPendingLogTick > kPendingLogIntervalMs)
		{
			device.lastPendingLogTick = now;
			std::wstring pendingMessage = DevicePrefix(i) + L": read pending for ";
			pendingMessage += std::to_wstring(now - device.pendingSinceTick);
			pendingMessage += L" ms.";
			if (device.lastReadError != 0)
			{
				pendingMessage += L" ReadFile error=";
				pendingMessage += std::to_wstring(device.lastReadError);
				pendingMessage += L".";
			}
			if (device.lastOverlappedError != 0)
			{
				pendingMessage += L" Overlapped error=";
				pendingMessage += std::to_wstring(device.lastOverlappedError);
				pendingMessage += L".";
			}
			LOG(LogCorsairHID, Warning, pendingMessage);

			if (TryGetInputReport(device, i))
			{
				CancelPendingRead(device);
				StartAsyncRead(device, i);
			}
		}
	}
}

bool CorsairAnalogInput::GetAnalogValue(uint32_t keyCode, float& outValue)
{
	if (g_devices.empty())
	{
		return false;
	}

	if (keyCode >= g_keyOffsets.size())
	{
		return false;
	}

	const DeviceState& device = g_devices.front();
	if (device.lastReport.empty())
	{
		return false;
	}

	const int32_t offset = g_keyOffsets[keyCode];
	if (offset < 0 || static_cast<size_t>(offset) >= device.lastReport.size())
	{
		return false;
	}

	outValue = static_cast<float>(device.lastReport[static_cast<size_t>(offset)]) / 255.0f;
	return true;
}

void CorsairAnalogInput::SetKeyOffset(uint32_t keyCode, int32_t reportOffset)
{
	if (keyCode >= g_keyOffsets.size())
	{
		return;
	}
	g_keyOffsets[keyCode] = reportOffset;
}

void CorsairAnalogInput::EnableReportDiffLog(bool enable)
{
	g_dumpReportDiff = enable;
	g_dumpReportRaw = enable;
	if (!enable)
	{
			for (auto& device : g_devices)
			{
				device.prevReport.clear();
			}
	}
	else
	{
		LOG(LogCorsairHID, Info, L"Corsair analog HID logging enabled (diff + raw).");
			for (auto& device : g_devices)
			{
				device.lastRawLogTick = 0;
				device.lastNoReportLogTick = 0;
				device.pendingSinceTick = 0;
				device.lastPendingLogTick = 0;
			}
	}
}

bool CorsairAnalogInput::IsConnected()
{
	return !g_devices.empty();
}

const std::vector<uint8_t>& CorsairAnalogInput::GetLastReport()
{
	static const std::vector<uint8_t> empty;
	if (g_devices.empty())
	{
		return empty;
	}
	return g_devices.front().lastReport;
}

std::wstring CorsairAnalogInput::GetDevicePath()
{
	if (g_devices.empty())
	{
		return L"";
	}
	return g_devices.front().devicePath;
}
