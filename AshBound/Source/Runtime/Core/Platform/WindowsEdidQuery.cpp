#include "Runtime/Core/Platform/WindowsEdidQuery.h"
#include "Runtime/Core/Log.h"

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <cwctype>
#include <cwchar>
#include <setupapi.h>
#include <devguid.h>
#include <cmath>

#pragma comment(lib, "setupapi.lib")

namespace
{
	const uint8_t kCtaExtensionTag = 0x02;
	const uint8_t kExtendedTag = 0x07;
	const uint8_t kExtendedHdrStaticMetadata = 0x06;
	const uint8_t kExtendedHdrDynamicMetadata = 0x0E;
	const uint8_t kVendorSpecificTag = 0x03;
	const uint8_t kHdrEotfPqBit = 1u << 2;
	const uint8_t kHdr10PlusTypeBit = 1u << 0;

	float DecodeHdrMaxLuminance(uint8_t code)
	{
		if (code == 0)
		{
			return 0.0f;
		}
		return 50.0f * std::pow(2.0f, static_cast<float>(code) / 32.0f);
	}

	float DecodeHdrMinLuminance(uint8_t code, float maxLuminance)
	{
		if (code == 0 || maxLuminance <= 0.0f)
		{
			return 0.0f;
		}
		const float normalized = static_cast<float>(code) / 255.0f;
		return maxLuminance * normalized * normalized * 0.0001f;
	}

	bool StartsWith(const std::wstring& value, const std::wstring& prefix)
	{
		if (value.size() < prefix.size())
		{
			return false;
		}
		return value.compare(0, prefix.size(), prefix) == 0;
	}

	std::wstring ToLower(const std::wstring& value)
	{
		std::wstring lowered = value;
		for (auto& ch : lowered)
		{
			ch = static_cast<wchar_t>(towlower(ch));
		}
		return lowered;
	}

	bool ContainsEnumDisplayPath(const std::wstring& value)
	{
		const std::wstring lowered = ToLower(value);
		return lowered.find(L"\\enum\\display\\") != std::wstring::npos;
	}

	bool EndsWithCaseInsensitive(const std::wstring& value, const std::wstring& suffix)
	{
		if (value.size() < suffix.size())
		{
			return false;
		}
		const std::wstring tail = value.substr(value.size() - suffix.size());
		return ToLower(tail) == ToLower(suffix);
	}

	std::wstring BuildEdidRegistryPath(const std::wstring& deviceId)
	{
		if (deviceId.empty())
		{
			return {};
		}

		std::wstring id = deviceId;
		const std::wstring registryMachinePrefix = L"\\Registry\\Machine\\";
		const std::wstring hklmPrefix = L"HKEY_LOCAL_MACHINE\\";
		const std::wstring hklmShortPrefix = L"HKLM\\";
		if (StartsWith(id, registryMachinePrefix))
		{
			id = id.substr(registryMachinePrefix.size());
		}
		else if (StartsWith(id, hklmPrefix))
		{
			id = id.substr(hklmPrefix.size());
		}
		else if (StartsWith(id, hklmShortPrefix))
		{
			id = id.substr(hklmShortPrefix.size());
		}
		else if (StartsWith(id, L"SYSTEM\\"))
		{
			id = id;
		}
		else
		{
			const std::wstring monitorPrefix = L"MONITOR\\";
			const std::wstring displayPrefix = L"DISPLAY\\";
			if (StartsWith(id, monitorPrefix))
			{
				id = id.substr(monitorPrefix.size());
			}
			else if (StartsWith(id, displayPrefix))
			{
				id = id.substr(displayPrefix.size());
			}

			if (id.empty())
			{
				return {};
			}

			id = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\" + id;
		}

		if (!ContainsEnumDisplayPath(id))
		{
			return {};
		}

		const std::wstring deviceParametersSuffix = L"\\Device Parameters";
		if (!EndsWithCaseInsensitive(id, deviceParametersSuffix))
		{
			id += deviceParametersSuffix;
		}
		return id;
	}

	bool TryReadEdidFromRegistry(const std::wstring& deviceId, std::vector<uint8_t>& outEdid)
	{
		const std::wstring path = BuildEdidRegistryPath(deviceId);
		if (path.empty())
		{
			return false;
		}

		HKEY key = nullptr;
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
		{
			return false;
		}

		DWORD type = 0;
		DWORD size = 0;
		LSTATUS status = RegQueryValueExW(key, L"EDID", nullptr, &type, nullptr, &size);
		if (status != ERROR_SUCCESS || type != REG_BINARY || size < 128)
		{
			RegCloseKey(key);
			return false;
		}

		outEdid.resize(size);
		status = RegQueryValueExW(key, L"EDID", nullptr, &type,
			reinterpret_cast<LPBYTE>(outEdid.data()), &size);
		RegCloseKey(key);
		if (status != ERROR_SUCCESS || type != REG_BINARY)
		{
			outEdid.clear();
			return false;
		}
		return true;
	}

	bool HardwareIdMatchesDevice(const std::wstring& deviceIdLower, const wchar_t* multiSz, DWORD byteSize)
	{
		if (!multiSz || byteSize < sizeof(wchar_t))
		{
			return false;
		}

		const size_t charCount = byteSize / sizeof(wchar_t);
		size_t index = 0;
		while (index < charCount && multiSz[index] != L'\0')
		{
			const wchar_t* start = multiSz + index;
			const size_t len = wcslen(start);
			if (len == 0)
			{
				break;
			}

			const std::wstring entryLower = ToLower(std::wstring(start, len));
			if (!entryLower.empty() && deviceIdLower.find(entryLower) != std::wstring::npos)
			{
				return true;
			}

			index += len + 1;
		}
		return false;
	}

	bool TryReadEdidFromSetupApi(const std::wstring& deviceId, std::vector<uint8_t>& outEdid)
	{
		if (deviceId.empty())
		{
			return false;
		}

		const std::wstring deviceIdLower = ToLower(deviceId);
		HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, nullptr, nullptr, DIGCF_PRESENT);
		if (devInfo == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		bool success = false;
		SP_DEVINFO_DATA devData = {};
		devData.cbSize = sizeof(devData);
		for (DWORD index = 0; SetupDiEnumDeviceInfo(devInfo, index, &devData); ++index)
		{
			DWORD dataType = 0;
			DWORD dataSize = 0;
			SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_HARDWAREID,
				&dataType, nullptr, 0, &dataSize);
			if (dataType != REG_MULTI_SZ || dataSize == 0)
			{
				continue;
			}

			std::vector<wchar_t> buffer(dataSize / sizeof(wchar_t));
			if (!SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_HARDWAREID,
				&dataType, reinterpret_cast<PBYTE>(buffer.data()), dataSize, nullptr))
			{
				continue;
			}

			if (!HardwareIdMatchesDevice(deviceIdLower, buffer.data(), dataSize))
			{
				continue;
			}

			HKEY devKey = SetupDiOpenDevRegKey(devInfo, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
			if (devKey == INVALID_HANDLE_VALUE)
			{
				continue;
			}

			DWORD valueType = 0;
			DWORD valueSize = 0;
			LSTATUS status = RegQueryValueExW(devKey, L"EDID", nullptr, &valueType, nullptr, &valueSize);
			if (status == ERROR_SUCCESS && valueType == REG_BINARY && valueSize >= 128)
			{
				outEdid.resize(valueSize);
				status = RegQueryValueExW(devKey, L"EDID", nullptr, &valueType,
					reinterpret_cast<LPBYTE>(outEdid.data()), &valueSize);
				if (status == ERROR_SUCCESS && valueType == REG_BINARY)
				{
					success = true;
				}
				else
				{
					outEdid.clear();
				}
			}

			RegCloseKey(devKey);
			if (success)
			{
				break;
			}
		}

		SetupDiDestroyDeviceInfoList(devInfo);
		return success;
	}

	void ParseCtaExtension(const uint8_t* data, size_t size, EdidHdrInfo& outInfo)
	{
		if (!data || size < 128)
		{
			return;
		}

		if (data[0] != kCtaExtensionTag)
		{
			return;
		}

		const uint8_t dtdOffset = data[2];
		if (dtdOffset < 4 || dtdOffset > 127)
		{
			return;
		}

		size_t index = 4;
		const size_t end = static_cast<size_t>(dtdOffset);
		while (index < end)
		{
			const uint8_t header = data[index++];
			const uint8_t tag = header >> 5;
			const uint8_t length = header & 0x1F;
			if (length == 0)
			{
				continue;
			}
			if (index + length > end)
			{
				break;
			}

			const uint8_t* payload = data + index;
			if (tag == kExtendedTag && length >= 2)
			{
				const uint8_t extendedTag = payload[0];
				if (extendedTag == kExtendedHdrStaticMetadata)
				{
					const uint8_t eotfFlags = payload[1];
					const uint8_t staticMetadataFlags = (length >= 3) ? payload[2] : 0;
					if ((eotfFlags & kHdrEotfPqBit) != 0)
					{
						outInfo.hdrSupported = true;
					}
					if (length >= 4)
					{
						outInfo.maxLuminance = DecodeHdrMaxLuminance(payload[3]);
					}
					if (length >= 5)
					{
						outInfo.maxFrameAverageLuminance = DecodeHdrMaxLuminance(payload[4]);
					}
					if (length >= 6)
					{
						outInfo.minLuminance = DecodeHdrMinLuminance(payload[5], outInfo.maxLuminance);
					}

					// Log full HDR Static Metadata Data Block details for diagnostics.
					const bool eotfSdr = (eotfFlags & (1u << 0)) != 0;
					const bool eotfHdr = (eotfFlags & (1u << 1)) != 0;
					const bool eotfPq = (eotfFlags & (1u << 2)) != 0;
					const bool eotfHlg = (eotfFlags & (1u << 3)) != 0;
					std::wstring descriptorTypes;
					for (int bit = 0; bit < 4; ++bit)
					{
						if ((staticMetadataFlags & (1u << bit)) == 0)
						{
							continue;
						}
						if (!descriptorTypes.empty())
						{
							descriptorTypes += L",";
						}
						descriptorTypes += L"Type";
						descriptorTypes += std::to_wstring(bit + 1);
					}
					if (descriptorTypes.empty())
					{
						descriptorTypes = L"None";
					}
					LOG(LogTemp, Info,
						L"EDID HDR Static Metadata: EOTF(SDR=%d HDR=%d PQ=%d HLG=%d) StaticDescFlags=0x%02X Types=%s "
						L"MaxLum=%.2f MaxFALL=%.2f MinLum=%.6f",
						eotfSdr ? 1 : 0,
						eotfHdr ? 1 : 0,
						eotfPq ? 1 : 0,
						eotfHlg ? 1 : 0,
						staticMetadataFlags,
						descriptorTypes.c_str(),
						outInfo.maxLuminance,
						outInfo.maxFrameAverageLuminance,
						outInfo.minLuminance);
				}
				else if (extendedTag == kExtendedHdrDynamicMetadata)
				{
					const uint8_t metadataTypes = payload[1];
					if ((metadataTypes & kHdr10PlusTypeBit) != 0)
					{
						outInfo.hdr10PlusSupported = true;
					}
				}
			}
			else if (tag == kVendorSpecificTag && length >= 3)
			{
				if (payload[0] == 0x46 && payload[1] == 0xD0 && payload[2] == 0x00)
				{
					outInfo.dolbyVisionSupported = true;
				}
			}

			index += length;
		}
	}
}

bool WindowsEdidQuery::TryQuery(const std::wstring& deviceKey,
	const std::wstring& deviceId,
	EdidHdrInfo& outInfo)
{
	outInfo = {};

	std::vector<uint8_t> edid;
	if (!TryReadEdidFromRegistry(deviceKey, edid) &&
		!TryReadEdidFromRegistry(deviceId, edid) &&
		!TryReadEdidFromSetupApi(deviceId, edid))
	{
		return false;
	}
	if (edid.size() < 128)
	{
		return false;
	}

	const uint8_t extensionCount = edid[0x7E];
	for (uint8_t i = 0; i < extensionCount; ++i)
	{
		const size_t offset = 128ull * (static_cast<size_t>(i) + 1);
		if (offset + 128 > edid.size())
		{
			break;
		}
		ParseCtaExtension(edid.data() + offset, 128, outInfo);
	}

	return true;
}
