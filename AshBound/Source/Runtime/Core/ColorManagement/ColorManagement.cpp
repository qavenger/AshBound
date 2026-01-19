#include "Runtime/Core/ColorManagement/ColorManagement.h"
#include "Runtime/Core/Log.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cwctype>

DEFINE_LOG_CATEGORY(LogColorManagement, Info)

namespace
{
	struct ParsedColorSpace
	{
		ColorPrimaries primaries;
		Chromaticity whitePoint;
		bool hasRed = false;
		bool hasGreen = false;
		bool hasBlue = false;
		bool hasWhite = false;

		bool IsComplete() const
		{
			return hasRed && hasGreen && hasBlue && hasWhite;
		}
	};

	WorkingColorSpaceDefinition ToDefinition(const ParsedColorSpace& entry)
	{
		WorkingColorSpaceDefinition definition;
		definition.primaries = entry.primaries;
		definition.whitePoint = entry.whitePoint;
		return definition;
	}

	std::wstring Trim(const std::wstring& value)
	{
		const wchar_t* whitespace = L" \t\r\n";
		const size_t start = value.find_first_not_of(whitespace);
		if (start == std::wstring::npos)
		{
			return L"";
		}
		const size_t end = value.find_last_not_of(whitespace);
		return value.substr(start, end - start + 1);
	}

	std::wstring ToLower(const std::wstring& value)
	{
		std::wstring result = value;
		for (wchar_t& ch : result)
		{
			ch = static_cast<wchar_t>(std::towlower(ch));
		}
		return result;
	}

	bool TryParseChromaticity(const std::wstring& value, Chromaticity& out)
	{
		std::wstring normalized = value;
		for (wchar_t& ch : normalized)
		{
			if (ch == L',' || ch == L';')
			{
				ch = L' ';
			}
		}

		std::wistringstream stream(normalized);
		float x = 0.0f;
		float y = 0.0f;
		if (!(stream >> x >> y))
		{
			return false;
		}

		out.x = x;
		out.y = y;
		return true;
	}

	bool TryParseAdaptationMethod(const std::wstring& value, ChromaticAdaptationMethod& out)
	{
		const std::wstring lowered = ToLower(Trim(value));
		if (lowered == L"bradford")
		{
			out = ChromaticAdaptationMethod::Bradford;
			return true;
		}
		if (lowered == L"cat02" || lowered == L"cat 02" || lowered == L"cat-02")
		{
			out = ChromaticAdaptationMethod::CAT02;
			return true;
		}
		return false;
	}

	std::wstring AdaptationToString(ChromaticAdaptationMethod method)
	{
		switch (method)
		{
		case ChromaticAdaptationMethod::Bradford:
			return L"Bradford";
		case ChromaticAdaptationMethod::CAT02:
		default:
			return L"CAT02";
		}
	}

	bool TryParsePresetName(const std::wstring& name, WorkingColorSpacePreset& out)
	{
		const std::wstring lowered = ToLower(Trim(name));
		if (lowered == L"srgb")
		{
			out = WorkingColorSpacePreset::SRGB;
			return true;
		}
		if (lowered == L"p3d65")
		{
			out = WorkingColorSpacePreset::P3D65;
			return true;
		}
		if (lowered == L"bt2020")
		{
			out = WorkingColorSpacePreset::BT2020;
			return true;
		}
		if (lowered == L"ap1")
		{
			out = WorkingColorSpacePreset::AP1;
			return true;
		}
		if (lowered == L"ap0")
		{
			out = WorkingColorSpacePreset::AP0;
			return true;
		}
		if (lowered == L"custom")
		{
			out = WorkingColorSpacePreset::Custom;
			return true;
		}
		return false;
	}

	WorkingColorSpaceDefinition GetPresetDefinition(WorkingColorSpacePreset preset)
	{
		WorkingColorSpaceDefinition definition;
		switch (preset)
		{
		case WorkingColorSpacePreset::SRGB:
			definition.primaries.red = { 0.64f, 0.33f };
			definition.primaries.green = { 0.30f, 0.60f };
			definition.primaries.blue = { 0.15f, 0.06f };
			definition.whitePoint = { 0.3127f, 0.3290f };
			break;
		case WorkingColorSpacePreset::P3D65:
			definition.primaries.red = { 0.6800f, 0.3200f };
			definition.primaries.green = { 0.2650f, 0.6900f };
			definition.primaries.blue = { 0.1500f, 0.0600f };
			definition.whitePoint = { 0.3127f, 0.3290f };
			break;
		case WorkingColorSpacePreset::BT2020:
			definition.primaries.red = { 0.7080f, 0.2920f };
			definition.primaries.green = { 0.1700f, 0.7970f };
			definition.primaries.blue = { 0.1310f, 0.0460f };
			definition.whitePoint = { 0.3127f, 0.3290f };
			break;
		case WorkingColorSpacePreset::AP1:
			definition.primaries.red = { 0.7130f, 0.2930f };
			definition.primaries.green = { 0.1650f, 0.8300f };
			definition.primaries.blue = { 0.1280f, 0.0440f };
			definition.whitePoint = { 0.32168f, 0.33767f };
			break;
		case WorkingColorSpacePreset::AP0:
			definition.primaries.red = { 0.7347f, 0.2653f };
			definition.primaries.green = { 0.0000f, 1.0000f };
			definition.primaries.blue = { 0.0001f, -0.0770f };
			definition.whitePoint = { 0.32168f, 0.33767f };
			break;
		case WorkingColorSpacePreset::Custom:
		default:
			definition = GetPresetDefinition(WorkingColorSpacePreset::SRGB);
			break;
		}
		return definition;
	}

	bool LoadWorkingColorSpaceFromConfig(
		const std::wstring& path,
		ParsedColorSpace& workingSpace,
		std::wstring& workingSpaceName,
		ChromaticAdaptationMethod& adaptation)
	{
		if (!std::filesystem::exists(path))
		{
			return false;
		}

		std::wifstream file(path);
		if (!file.is_open())
		{
			return false;
		}

		std::wstring sectionRaw;
		std::wstring line;
		while (std::getline(file, line))
		{
			const std::wstring trimmed = Trim(line);
			if (trimmed.empty() || trimmed[0] == L';' || trimmed[0] == L'#')
			{
				continue;
			}

			if (trimmed.front() == L'[' && trimmed.back() == L']')
			{
				sectionRaw = Trim(trimmed.substr(1, trimmed.size() - 2));
				continue;
			}

			const size_t eqPos = trimmed.find(L'=');
			if (eqPos == std::wstring::npos)
			{
				continue;
			}

			const std::wstring key = Trim(trimmed.substr(0, eqPos));
			const std::wstring value = Trim(trimmed.substr(eqPos + 1));
			const std::wstring keyLower = ToLower(key);
			const std::wstring sectionLower = ToLower(sectionRaw);

			if (sectionLower == L"colormanagement")
			{
				if (keyLower == L"workingspace" || keyLower == L"workingcolorspace" || keyLower == L"defaultspace" || keyLower == L"space")
				{
					workingSpaceName = value;
				}
				else if (keyLower == L"adaptation" || keyLower == L"chromaticadaptation")
				{
					TryParseAdaptationMethod(value, adaptation);
				}
				else if (keyLower == L"red" || keyLower == L"r")
				{
					workingSpace.hasRed = TryParseChromaticity(value, workingSpace.primaries.red);
				}
				else if (keyLower == L"green" || keyLower == L"g")
				{
					workingSpace.hasGreen = TryParseChromaticity(value, workingSpace.primaries.green);
				}
				else if (keyLower == L"blue" || keyLower == L"b")
				{
					workingSpace.hasBlue = TryParseChromaticity(value, workingSpace.primaries.blue);
				}
				else if (keyLower == L"white" || keyLower == L"whitepoint")
				{
					workingSpace.hasWhite = TryParseChromaticity(value, workingSpace.whitePoint);
				}
				continue;
			}
		}

		return true;
	}

	bool BuildColorSpaceFromConfig(
		const ParsedColorSpace& workingSpace,
		const std::wstring& workingSpaceName,
		ColorSpace& outSpace,
		WorkingColorSpacePreset& outPreset)
	{
		if (!workingSpaceName.empty())
		{
			WorkingColorSpacePreset preset = WorkingColorSpacePreset::SRGB;
			if (TryParsePresetName(workingSpaceName, preset))
			{
				WorkingColorSpaceDefinition definition;
				const bool hasCustomDefinition = workingSpace.IsComplete();
				if (preset == WorkingColorSpacePreset::Custom)
				{
					if (!hasCustomDefinition)
					{
						return false;
					}
					definition = ToDefinition(workingSpace);
				}
				else if (hasCustomDefinition)
				{
					definition = ToDefinition(workingSpace);
				}
				else
				{
					definition = GetPresetDefinition(preset);
				}

				if (!definition.IsValid())
				{
					return false;
				}

				outSpace.primaries = definition.primaries;
				outSpace.whitePoint = definition.whitePoint;
				if (outSpace.RecomputeMatrices())
				{
					outPreset = preset;
					return true;
				}
			}
			else
			{
				return false;
			}
		}

		if (workingSpace.IsComplete())
		{
			outSpace.primaries = workingSpace.primaries;
			outSpace.whitePoint = workingSpace.whitePoint;
			if (outSpace.RecomputeMatrices())
			{
				outPreset = workingSpaceName.empty() ? WorkingColorSpacePreset::Custom : outPreset;
				return true;
			}
		}

		const WorkingColorSpaceDefinition srgb = GetPresetDefinition(WorkingColorSpacePreset::SRGB);
		if (srgb.IsValid())
		{
			outSpace.primaries = srgb.primaries;
			outSpace.whitePoint = srgb.whitePoint;
			if (outSpace.RecomputeMatrices())
			{
				outPreset = WorkingColorSpacePreset::SRGB;
				return true;
			}
		}

		return false;
	}

	std::wstring FormatChromaticity(const Chromaticity& chroma)
	{
		std::wostringstream stream;
		stream << std::fixed << std::setprecision(6) << chroma.x << L", " << chroma.y;
		return stream.str();
	}

	bool WriteWorkingColorSpaceConfig(const std::wstring& path, const std::wstring& spaceName,
		const WorkingColorSpaceDefinition& definition, ChromaticAdaptationMethod adaptation)
	{
		std::vector<std::wstring> output;
		output.reserve(7);
		output.push_back(L"[ColorManagement]");
		output.push_back(L"WorkingColorSpace = " + spaceName);
		output.push_back(L"Red = " + FormatChromaticity(definition.primaries.red));
		output.push_back(L"Green = " + FormatChromaticity(definition.primaries.green));
		output.push_back(L"Blue = " + FormatChromaticity(definition.primaries.blue));
		output.push_back(L"White = " + FormatChromaticity(definition.whitePoint));

		std::wofstream outputFile(path, std::ios::trunc);
		if (!outputFile.is_open())
		{
			return false;
		}

		for (size_t i = 0; i < output.size(); ++i)
		{
			outputFile << output[i];
			if (i + 1 < output.size())
			{
				outputFile << L"\n";
			}
		}
		return true;
	}

	bool IsNearlyIdentity(const Math::Matrix3& matrix, float epsilon)
	{
		for (int row = 0; row < 3; ++row)
		{
			for (int col = 0; col < 3; ++col)
			{
				const float target = (row == col) ? 1.0f : 0.0f;
				if (std::abs(matrix.m[row][col] - target) > epsilon)
				{
					return false;
				}
			}
		}
		return true;
	}

	Math::Vector3 LinearColorToVector3(const LinearColor& color)
	{
		return Math::Vector3(color.r, color.g, color.b);
	}

	LinearColor Vector3ToLinearColor(const Math::Vector3& value, float alpha)
	{
		return LinearColor(value.x, value.y, value.z, alpha);
	}

	Math::Matrix3 ComputeSRGBToWorkingMatrix(const ColorSpace& workingSpace, ChromaticAdaptationMethod method)
	{
		const ColorSpace srgb = ColorSpace::CreateSRGB();
		const Math::Matrix3 adaptation = ComputeChromaticAdaptationMatrix(
			srgb.whitePoint,
			workingSpace.whitePoint,
			method);
		return workingSpace.XYZtoRGB * (adaptation * srgb.RGBtoXYZ);
	}

	LinearColor TransformLinearColorToWorkingSpace(const LinearColor& color, const ColorSpace& sourceSpace)
	{
		const ColorSpace& workingSpace = ColorManagement::GetWorkingColorSpace();
		const ChromaticAdaptationMethod method = ColorManagement::GetDefaultAdaptationMethod();

		const Math::Vector3 rgb = LinearColorToVector3(color);
		const Math::Vector3 xyz = sourceSpace.RGBtoXYZ.TransformVector(rgb);
		const Math::Matrix3 adaptation = ComputeChromaticAdaptationMatrix(
			sourceSpace.whitePoint,
			workingSpace.whitePoint,
			method);
		const Math::Vector3 adaptedXYZ = adaptation.TransformVector(xyz);
		const Math::Vector3 workingRGB = workingSpace.XYZtoRGB.TransformVector(adaptedXYZ);

		return Vector3ToLinearColor(workingRGB, color.a);
	}

}

namespace
{
	struct WorkingColorSpaceCallbackEntry
	{
		ColorManagement::WorkingColorSpaceCallbackHandle handle = ColorManagement::InvalidWorkingColorSpaceCallbackHandle;
		ColorManagement::WorkingColorSpaceChangedCallback callback;
	};

	bool s_initialized = false;
	ColorSpace s_workingColorSpace = {};
	WorkingColorSpacePreset s_workingPreset = WorkingColorSpacePreset::SRGB;
	ChromaticAdaptationMethod s_defaultAdaptationMethod = ChromaticAdaptationMethod::CAT02;
	Math::Matrix3 s_srgbToWorkingMatrix = Math::Matrix3::Identity();
	std::vector<WorkingColorSpaceCallbackEntry> s_colorSpaceChangedCallbacks;
}

void ColorManagement::InitializeDefault()
{
	s_workingColorSpace = ColorSpace::CreateSRGB();
	s_workingPreset = WorkingColorSpacePreset::SRGB;
	s_defaultAdaptationMethod = ChromaticAdaptationMethod::CAT02;
	s_srgbToWorkingMatrix = Math::Matrix3::Identity();
}

void ColorManagement::InitializeFromConfig(const std::wstring& configPath)
{
	if (s_initialized)
	{
		return;
	}
	s_initialized = true;

	ParsedColorSpace workingSpaceConfig;
	std::wstring workingSpaceName;
	ChromaticAdaptationMethod adaptation = ChromaticAdaptationMethod::CAT02;

	if (!LoadWorkingColorSpaceFromConfig(configPath, workingSpaceConfig, workingSpaceName, adaptation))
	{
		InitializeDefault();
		LOG(LogColorManagement, Warning, L"ColorManagement: Config not found or failed to load: %s. Using sRGB.", configPath.c_str());
		return;
	}

	ColorSpace workingSpace = {};
	WorkingColorSpacePreset resolvedPreset = WorkingColorSpacePreset::SRGB;
	if (!BuildColorSpaceFromConfig(workingSpaceConfig, workingSpaceName, workingSpace, resolvedPreset))
	{
		InitializeDefault();
		LOG(LogColorManagement, Warning, L"ColorManagement: Invalid config. Using sRGB.");
		return;
	}

	s_workingColorSpace = workingSpace;
	s_workingPreset = resolvedPreset;
	s_defaultAdaptationMethod = adaptation;
	s_srgbToWorkingMatrix = ComputeSRGBToWorkingMatrix(s_workingColorSpace, s_defaultAdaptationMethod);

	LOG(LogColorManagement, Info, L"ColorManagement: Working space = %s",
		WorkingColorSpacePresetToString(s_workingPreset).c_str());
	LOG(LogColorManagement, Info, L"ColorManagement: RGB->XYZ = %s", s_workingColorSpace.RGBtoXYZ.ToString().c_str());
	LOG(LogColorManagement, Info, L"ColorManagement: XYZ->RGB = %s", s_workingColorSpace.XYZtoRGB.ToString().c_str());

	const Math::Matrix3 sanity = s_workingColorSpace.RGBtoXYZ * s_workingColorSpace.XYZtoRGB;
	if (!IsNearlyIdentity(sanity, 1e-3f))
	{
		LOG(LogColorManagement, Warning, L"ColorManagement: Matrix inversion check failed for %s.",
			WorkingColorSpacePresetToString(s_workingPreset).c_str());
	}
}

bool ColorManagement::IsInitialized()
{
	return s_initialized;
}

const ColorSpace& ColorManagement::GetWorkingColorSpace()
{
	if (!s_initialized)
	{
		InitializeDefault();
	}
	return s_workingColorSpace;
}

std::wstring ColorManagement::GetWorkingColorSpaceName()
{
	if (!s_initialized)
	{
		InitializeDefault();
	}
	return WorkingColorSpacePresetToString(s_workingPreset);
}

WorkingColorSpacePreset ColorManagement::GetWorkingColorSpacePreset()
{
	if (!s_initialized)
	{
		InitializeDefault();
	}
	return s_workingPreset;
}

ChromaticAdaptationMethod ColorManagement::GetDefaultAdaptationMethod()
{
	if (!s_initialized)
	{
		InitializeDefault();
	}
	return s_defaultAdaptationMethod;
}

std::wstring ColorManagement::WorkingColorSpacePresetToString(WorkingColorSpacePreset preset)
{
	switch (preset)
	{
	case WorkingColorSpacePreset::SRGB:
		return L"SRGB";
	case WorkingColorSpacePreset::P3D65:
		return L"P3D65";
	case WorkingColorSpacePreset::BT2020:
		return L"BT2020";
	case WorkingColorSpacePreset::AP1:
		return L"AP1";
	case WorkingColorSpacePreset::AP0:
		return L"AP0";
	case WorkingColorSpacePreset::Custom:
		return L"Custom";
	default:
		return L"SRGB";
	}
}

bool WorkingColorSpaceDefinition::IsValid() const
{
	return primaries.IsValid() && whitePoint.IsValid();
}

bool ColorManagement::SetWorkingColorSpaceOverride(WorkingColorSpacePreset preset,
	const WorkingColorSpaceDefinition* customDefinition,
	const std::wstring& configPath)
{
	if (!s_initialized)
	{
		InitializeFromConfig(configPath);
	}

	WorkingColorSpaceDefinition definition;
	if (preset == WorkingColorSpacePreset::Custom)
	{
		if (!customDefinition || !customDefinition->IsValid())
		{
			LOG(LogColorManagement, Warning, L"ColorManagement: Custom color space definition invalid.");
			return false;
		}
		definition = *customDefinition;
	}
	else
	{
		definition = GetPresetDefinition(preset);
		if (!definition.IsValid())
		{
			LOG(LogColorManagement, Warning, L"ColorManagement: Preset definition invalid.");
			return false;
		}
	}

	ColorSpace newSpace;
	newSpace.primaries = definition.primaries;
	newSpace.whitePoint = definition.whitePoint;
	if (!newSpace.RecomputeMatrices())
	{
		LOG(LogColorManagement, Warning, L"ColorManagement: Failed to compute matrices for override.");
		return false;
	}

	s_workingColorSpace = newSpace;
	s_workingPreset = preset;
	s_srgbToWorkingMatrix = ComputeSRGBToWorkingMatrix(s_workingColorSpace, s_defaultAdaptationMethod);

	if (!WriteWorkingColorSpaceConfig(configPath,
		WorkingColorSpacePresetToString(preset), definition, s_defaultAdaptationMethod))
	{
		LOG(LogColorManagement, Warning, L"ColorManagement: Failed to write config to %s.", configPath.c_str());
	}

	for (const auto& entry : s_colorSpaceChangedCallbacks)
	{
		if (entry.callback)
		{
			entry.callback(s_workingColorSpace, WorkingColorSpacePresetToString(s_workingPreset));
		}
	}

	LOG(LogColorManagement, Info, L"ColorManagement: Working space overridden to %s. Restart required to persist changes.",
		WorkingColorSpacePresetToString(s_workingPreset).c_str());
	return true;
}

LinearColor ColorManagement::SRGBToWorkingColorSpace(const LinearColor& color)
{
	if (s_workingPreset == WorkingColorSpacePreset::SRGB)
	{
		return color;
	}

	const Math::Vector3 rgb = LinearColorToVector3(color);
	const Math::Vector3 workingRGB = s_srgbToWorkingMatrix.TransformVector(rgb);
	return Vector3ToLinearColor(workingRGB, color.a);
}

Color ColorManagement::SRGBToWorkingColorSpace(const Color& color)
{
	if (s_workingPreset == WorkingColorSpacePreset::SRGB)
	{
		return color;
	}

	const LinearColor linear = color.ToLinear(true);
	const LinearColor transformed = SRGBToWorkingColorSpace(linear);
	return transformed.ToColor(true);
}

LinearColor ColorManagement::TransformColorToWorkingColorSpace(const LinearColor& color, const ColorSpace& sourceSpace)
{
	return TransformLinearColorToWorkingSpace(color, sourceSpace);
}

Color ColorManagement::TransformColorToWorkingColorSpace(const Color& color, const ColorSpace& sourceSpace)
{
	const LinearColor linear = color.ToLinear(true);
	const LinearColor transformed = TransformLinearColorToWorkingSpace(linear, sourceSpace);
	return transformed.ToColor(true);
}

ColorManagement::WorkingColorSpaceCallbackHandle ColorManagement::AddWorkingColorSpaceChangedCallback(
	WorkingColorSpaceChangedCallback callback)
{
	if (!callback)
	{
		return InvalidWorkingColorSpaceCallbackHandle;
	}
	const WorkingColorSpaceCallbackHandle handle = callback.GetHandle();
	if (!handle.IsValid())
	{
		return InvalidWorkingColorSpaceCallbackHandle;
	}
	s_colorSpaceChangedCallbacks.push_back({ handle, std::move(callback) });
	return handle;
}

bool ColorManagement::RemoveWorkingColorSpaceChangedCallback(WorkingColorSpaceCallbackHandle handle)
{
	if (!handle.IsValid())
	{
		return false;
	}

	const size_t before = s_colorSpaceChangedCallbacks.size();
	s_colorSpaceChangedCallbacks.erase(
		std::remove_if(s_colorSpaceChangedCallbacks.begin(), s_colorSpaceChangedCallbacks.end(),
			[&](const WorkingColorSpaceCallbackEntry& entry)
			{
				return entry.handle == handle;
			}),
		s_colorSpaceChangedCallbacks.end());
	return s_colorSpaceChangedCallbacks.size() != before;
}

bool ColorManagement::RemoveWorkingColorSpaceChangedCallback(WorkingColorSpaceChangedCallback callback)
{
	if (!callback)
	{
		return false;
	}

	const size_t before = s_colorSpaceChangedCallbacks.size();
	s_colorSpaceChangedCallbacks.erase(
		std::remove_if(s_colorSpaceChangedCallbacks.begin(), s_colorSpaceChangedCallbacks.end(),
			[&](const WorkingColorSpaceCallbackEntry& entry)
			{
				return entry.callback == callback;
			}),
		s_colorSpaceChangedCallbacks.end());
	return s_colorSpaceChangedCallbacks.size() != before;
}

void ColorManagement::ClearWorkingColorSpaceChangedCallbacks()
{
	s_colorSpaceChangedCallbacks.clear();
}
