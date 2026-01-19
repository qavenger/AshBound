#pragma once

#include <functional>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include "Runtime/Core/Delegates/Delegate.h"
#include "Runtime/Core/ColorManagement/Color.h"
#include "Runtime/Core/ColorManagement/ColorSpace.h"

enum class WorkingColorSpacePreset
{
	SRGB,
	P3D65,
	BT2020,
	AP1,
	AP0,
	Custom
};

enum class EOTF
{
	Linear,
	SRGB,
	Gamma,
	PQ,
	Raw
};

struct WorkingColorSpaceDefinition
{
	ColorPrimaries primaries;
	Chromaticity whitePoint;

	bool IsValid() const;
};

class ColorManagement
{
public:
	using WorkingColorSpaceCallbackHandle = DelegateHandle;
	static constexpr WorkingColorSpaceCallbackHandle InvalidWorkingColorSpaceCallbackHandle = DelegateHandle::Invalid();
	using WorkingColorSpaceChangedCallback = Delegate<void(const ColorSpace&, const std::wstring&)>;

	static void InitializeFromConfig(const std::wstring& configPath = L"Config/ColorManagement.ini");
	static bool IsInitialized();

	static const ColorSpace& GetWorkingColorSpace();
	static std::wstring GetWorkingColorSpaceName();
	static WorkingColorSpacePreset GetWorkingColorSpacePreset();
	static std::wstring WorkingColorSpacePresetToString(WorkingColorSpacePreset preset);
	static ChromaticAdaptationMethod GetDefaultAdaptationMethod();

	static bool SetWorkingColorSpaceOverride(WorkingColorSpacePreset preset,
		const WorkingColorSpaceDefinition* customDefinition = nullptr,
		const std::wstring& configPath = L"Config/ColorManagement.ini");

	static LinearColor SRGBToWorkingColorSpace(const LinearColor& color);
	static Color SRGBToWorkingColorSpace(const Color& color);
	static LinearColor TransformColorToWorkingColorSpace(const LinearColor& color, const ColorSpace& sourceSpace);
	static Color TransformColorToWorkingColorSpace(const Color& color, const ColorSpace& sourceSpace);

	static WorkingColorSpaceCallbackHandle AddWorkingColorSpaceChangedCallback(WorkingColorSpaceChangedCallback callback);
	static bool RemoveWorkingColorSpaceChangedCallback(WorkingColorSpaceCallbackHandle handle);
	static bool RemoveWorkingColorSpaceChangedCallback(WorkingColorSpaceChangedCallback callback);
	static void ClearWorkingColorSpaceChangedCallbacks();

private:
	static void InitializeDefault();
};
