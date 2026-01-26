#pragma once

#include <functional>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include "Runtime/Core/Delegates/Delegate.h"
#include "Runtime/Core/ColorManagement/Color.h"
#include "Runtime/Core/ColorManagement/ColorSpace.h"
#include "Runtime/Rendering/RHI/RHIEnum.h"

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
	ScRGB,
	PQ,
	Raw
};

enum class OutputGamut
{
	SRGB,
	DisplayP3,
	SCRGB,
	BT2020
};

enum class ColorManagementPriority
{
	PreferDisplay,
	PreferConfig
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
	static uint32_t GetBackbufferBitDepth();
	static Math::Matrix3 GetWorkingToOutputMatrix(OutputGamut outputGamut);
	static OutputGamut GetOutputGamut();
	static EOTF GetOutputEotf();

	static bool SetWorkingColorSpaceOverride(WorkingColorSpacePreset preset,
		const WorkingColorSpaceDefinition* customDefinition = nullptr,
		const std::wstring& configPath = L"Config/ColorManagement.ini");
	static bool SetOutputGamutOverride(OutputGamut outputGamut,
		const std::wstring& configPath = L"Config/ColorManagement.ini");
	static bool SetOutputEotfOverride(EOTF eotf,
		const std::wstring& configPath = L"Config/ColorManagement.ini");
	static bool SetBackbufferBitDepth(uint32_t bitDepth,
		const std::wstring& configPath = L"Config/ColorManagement.ini");

	struct SwapChainColorPlan
	{
		RHIEnum::Format format = RHIEnum::Format::R10G10B10A2_UNORM;
		RHIEnum::ColorSpace colorSpace = RHIEnum::ColorSpace::SDR_G22_P709;
	};

	static SwapChainColorPlan BuildSwapChainColorPlan(ColorManagementPriority priority,
		bool hdrSupported);

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
