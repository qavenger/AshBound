#pragma once

#include "Runtime/Core/Math/Matrix3.h"
#include "Runtime/Core/Math/Vector3.h"

enum class ChromaticAdaptationMethod
{
	Bradford,
	CAT02
};

struct Chromaticity
{
	float x = 0.0f;
	float y = 0.0f;

	bool IsValid() const;
};

struct ColorPrimaries
{
	Chromaticity red;
	Chromaticity green;
	Chromaticity blue;

	bool IsValid() const;
};

struct ColorSpace
{
	ColorPrimaries primaries;
	Chromaticity whitePoint;
	Math::Matrix3 RGBtoXYZ = {};
	Math::Matrix3 XYZtoRGB = {};

	bool RecomputeMatrices();
	static ColorSpace CreateSRGB();
};

Math::Matrix3 ComputeChromaticAdaptationMatrix(
	const Chromaticity& sourceWhite,
	const Chromaticity& targetWhite,
	ChromaticAdaptationMethod method = ChromaticAdaptationMethod::CAT02);
