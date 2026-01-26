#ifndef COLOR_MANAGEMENT_HLSL
#define COLOR_MANAGEMENT_HLSL
#include "ViewParameters.hlsl"
#define WHITEPOINT uint
static const WHITEPOINT WhitePoint_D65 = 0;
static const WHITEPOINT WhitePoint_D60 = 1;
static const WHITEPOINT WhitePoint_DCI = 2;


static const float3x3 Bradford_MAT =
{
	0.895100, 0.266400, -0.161400,
	-0.750200, 1.713500, 0.036700,
	0.038900, -0.068500, 1.029600,
};

static const float3x3 Bradford_INV =
{
	0.9869929, -0.1470543, 0.1599627,
	0.4323053, 0.5183603, 0.0492912,
	-0.0085287, 0.0400428, 0.9684867,
};

static const float3x3 CAT02_MAT =
{
	0.732800, 0.429600, -0.162400,
	-0.703600, 1.697500, 0.006100,
	0.003000, 0.013600, 0.983400,
};

static const float3x3 CAT02_INV =
{
	1.096124, -0.278869, 0.182745,
	0.454369, 0.473533, 0.072098,
	-0.009628, -0.005698, 1.015326,
};


struct FColorSpace
{
	WHITEPOINT White;
    float3x3 XYZtoRGB;
    float3x3 RGBtoXYZ;
};

// Matrices are specified in row-major order and intended for mul(M, v).
static const float3x3 AP0_2_XYZ_MAT =
{
	0.9525523959, 0.0000000000, 0.0000936786,
	0.3439664498, 0.7281660966, -0.0721325464,
	0.0000000000, 0.0000000000, 1.0088251844,
};

static const float3x3 XYZ_2_AP0_MAT =
{
	 1.0498110175, 0.0000000000, -0.0000974845,
	-0.4959030231, 1.3733130458, 0.0982400361,
	 0.0000000000, 0.0000000000, 0.9912520182,
};

static const float3x3 AP1_2_XYZ_MAT =
{
	 0.6624541811, 0.1340042065, 0.1561876870,
	 0.2722287168, 0.6740817658, 0.0536895174,
	-0.0055746495, 0.0040607335, 1.0103391003,
};

static const float3x3 XYZ_2_AP1_MAT =
{
	 1.6410233797, -0.3248032942, -0.2364246952,
	-0.6636628587,  1.6153315917,  0.0167563477,
	 0.0117218943, -0.0082844420,  0.9883948585,
};

static const float3x3 AP0_2_AP1_MAT =
{
	 1.4514393161, -0.2365107469, -0.2149285693,
	-0.0765537734,  1.1762296998, -0.0996759264,
	 0.0083161484, -0.0060324498,  0.9977163014,
};

static const float3x3 AP1_2_AP0_MAT =
{
	 0.6954522414,  0.1406786965,  0.1638690622,
	 0.0447945634,  0.8596711185,  0.0955343182,
	-0.0055258826,  0.0040252103,  1.0015006723,
};

static const float3 AP1_RGB2Y =
{
	0.2722287168,
	0.6740817658,
	0.0536895174
};

// REC 709 primaries
static const float3x3 XYZ_2_sRGB_MAT =
{
	 3.2409699419, -1.5373831776, -0.4986107603,
	-0.9692436363,  1.8759675015,  0.0415550574,
	 0.0556300797, -0.2039769589,  1.0569715142,
};

static const float3x3 sRGB_2_XYZ_MAT =
{
	0.4123907993, 0.3575843394, 0.1804807884,
	0.2126390059, 0.7151686788, 0.0721923154,
	0.0193308187, 0.1191947798, 0.9505321522,
};

// REC 2020 primaries
static const float3x3 XYZ_2_Rec2020_MAT =
{
	 1.7166511880, -0.3556707838, -0.2533662814,
	-0.6666843518,  1.6164812366,  0.0157685458,
	 0.0176398574, -0.0427706133,  0.9421031212,
};

static const float3x3 Rec2020_2_XYZ_MAT =
{
	0.6369580483, 0.1446169036, 0.1688809752,
	0.2627002120, 0.6779980715, 0.0593017165,
	0.0000000000, 0.0280726930, 1.0609850577,
};

// P3, D65 primaries
static const float3x3 XYZ_2_P3D65_MAT =
{
	 2.4934969119, -0.9313836179, -0.4027107845,
	-0.8294889696,  1.7626640603,  0.0236246858,
	 0.0358458302, -0.0761723893,  0.9568845240,
};

static const float3x3 P3D65_2_XYZ_MAT =
{
	0.4865709486, 0.2656676932, 0.1982172852,
	0.2289745641, 0.6917385218, 0.0792869141,
	0.0000000000, 0.0451133819, 1.0439443689,
};

// Bradford chromatic adaptation transforms between ACES white point (D60) and sRGB white point (D65)
static const float3x3 D65_2_D60_CAT =
{
	 1.0130349146, 0.0061052578, -0.0149709436,
	 0.0076982301, 0.9981633521, -0.0050320385,
	-0.0028413174, 0.0046851567,  0.9245061375,
};

static const float3x3 D60_2_D65_CAT =
{
	 0.9872240087, -0.0061132286, 0.0159532883,
	-0.0075983718,  1.0018614847, 0.0053300358,
	 0.0030725771, -0.0050959615, 1.0816806031,
};

static const float3x3 sRGB_2_AP1_MAT =
	mul(XYZ_2_AP1_MAT, mul(D65_2_D60_CAT, sRGB_2_XYZ_MAT));

static const uint TonemapMode_TCamApprox = 0;
static const uint TonemapMode_ACES = 1;
static const uint TonemapMode_Reinhard = 2;
static const uint TonemapMode_Hable = 3;
static const uint TonemapMode_Filmic = 4;
static const uint TonemapMode_Neutral = 5;

bool WorkingUsesD60(uint workingColorSpaceId)
{
	return (workingColorSpaceId == WORKING_COLORSPACE_AP1) ||
		(workingColorSpaceId == WORKING_COLORSPACE_AP0);
}

float3 WorkingToXYZ(float3 color, uint workingColorSpaceId)
{
	switch (workingColorSpaceId)
	{
	case WORKING_COLORSPACE_P3D65:
		return mul(P3D65_2_XYZ_MAT, color);
	case WORKING_COLORSPACE_BT2020:
		return mul(Rec2020_2_XYZ_MAT, color);
	case WORKING_COLORSPACE_AP1:
		return mul(AP1_2_XYZ_MAT, color);
	case WORKING_COLORSPACE_AP0:
		return mul(AP0_2_XYZ_MAT, color);
	default:
		return mul(sRGB_2_XYZ_MAT, color);
	}
}

float3 XYZToWorking(float3 xyz, uint workingColorSpaceId)
{
	switch (workingColorSpaceId)
	{
	case WORKING_COLORSPACE_P3D65:
		return mul(XYZ_2_P3D65_MAT, xyz);
	case WORKING_COLORSPACE_BT2020:
		return mul(XYZ_2_Rec2020_MAT, xyz);
	case WORKING_COLORSPACE_AP1:
		return mul(XYZ_2_AP1_MAT, xyz);
	case WORKING_COLORSPACE_AP0:
		return mul(XYZ_2_AP0_MAT, xyz);
	default:
		return mul(XYZ_2_sRGB_MAT, xyz);
	}
}

float3 ConvertWorkingSpace(float3 color, uint fromColorSpaceId, uint toColorSpaceId)
{
	if (fromColorSpaceId == toColorSpaceId)
	{
		return color;
	}

	float3 xyz = WorkingToXYZ(color, fromColorSpaceId);
	const bool fromD60 = WorkingUsesD60(fromColorSpaceId);
	const bool toD60 = WorkingUsesD60(toColorSpaceId);
	if (fromD60 && !toD60)
	{
		xyz = mul(D60_2_D65_CAT, xyz);
	}
	else if (!fromD60 && toD60)
	{
		xyz = mul(D65_2_D60_CAT, xyz);
	}
	return XYZToWorking(xyz, toColorSpaceId);
}

static const float3x3 CAM16_2_XYZ_MAT =
{
	 2.0512756811, -1.1400313439,  0.0887556628,
	 0.4269389763,  0.7005835277, -0.1275225040,
	-0.0174712779, -0.0384725929,  1.0589468739
};

static const float3x3 XYZ_2_CAM16_MAT =
{
	 0.3640744835,  0.5947008156, 0.04110127349,
	-0.2222450987,  1.0738554823, 0.14794533610,
	-0.0020676190,  0.0488260453, 0.95038755696
};

// SMPTE ST 2084 (PQ) transfer functions. Input/Output are normalized (0-1).
static const float PQ_m1 = 2610.0 / 16384.0;
static const float PQ_m2 = 2523.0 / 32.0;
static const float PQ_c1 = 3424.0 / 4096.0;
static const float PQ_c2 = 2413.0 / 128.0;
static const float PQ_c3 = 2392.0 / 128.0;
static const float C = 10000.0;

float LinearToPQ(float x)
{
    x = max(x, 0.0) * (1. / C);
	float xm = pow(x, PQ_m1);
	float num = PQ_c1 + PQ_c2 * xm;
	float den = 1.0 + PQ_c3 * xm;
	return pow(num / den, PQ_m2);
}

float PQToLinear(float x)
{
	x = max(x, 0.0);
	float xp = pow(x, 1.0 / PQ_m2);
	float num = max(xp - PQ_c1, 0.0);
	float den = PQ_c2 - PQ_c3 * xp;
	return pow(num / den, 1.0 / PQ_m1) * C;
}

float3 LinearToPQ(float3 v)
{
    float3 x = max(v, 0.0) * (1. / C);
	float3 xm = pow(x, PQ_m1);
	float3 num = PQ_c1 + PQ_c2 * xm;
	float3 den = 1.0 + PQ_c3 * xm;
	return pow(num / den, PQ_m2);
}

float3 PQToLinear(float3 v)
{
	float3 x = max(v, 0.0);
	float3 xp = pow(x, 1.0 / PQ_m2);
	float3 num = max(xp - PQ_c1, 0.0);
	float3 den = PQ_c2 - PQ_c3 * xp;
	return pow(num / den, 1.0 / PQ_m1) * C;
}

float3 LinearToSRGB(float3 v)
{
	const float3 cut = step(v, float3(0.0031308, 0.0031308, 0.0031308));
	const float3 low = v * 12.92;
	const float3 high = 1.055 * pow(max(v, 0.0), 1.0 / 2.4) - 0.055;
	return lerp(high, low, cut);
}

float3 LinearToGamma(float3 v, float invGamma)
{
	return pow(max(v, 0.0), invGamma);
}

float3 EncodeByEotf(float3 linearColor, uint eotfId, float invGamma, float maxLuminance)
{
	if (eotfId == 3) // ScRGB
	{
		// scRGB is linear; scale by reference white (80 nits).
		return linearColor * (max(maxLuminance, 1.0) / 80.0);
	}
	else if (eotfId == 1) // SRGB
	{
		return LinearToSRGB(linearColor);
	}
	else if (eotfId == 2) // Gamma
	{
		return LinearToGamma(linearColor, invGamma);
	}
	else if (eotfId == 4) // PQ
	{
		return LinearToPQ(linearColor * max(maxLuminance, 1.0));
	}
	// Linear/ScRGB/Raw fallback
	return linearColor;
}

float3 ApplyExposure(float3 color, float exposure)
{
	return color * exposure;
}

float3 TonemapReinhard(float3 color)
{
	return color / (1.0 + color);
}

float3 TonemapReinhardExtended(float3 color, float whitePoint)
{
	const float w2 = whitePoint * whitePoint;
	return (color * (1.0 + color / w2)) / (1.0 + color);
}

float3 TonemapFilmicNarkowicz(float3 color)
{
	const float3 x = max(color - 0.004, 0.0);
	return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

float3 TonemapHable(float3 color)
{
	const float A = 0.15;
	const float B = 0.50;
	const float Cc = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;
	return ((color * (A * color + Cc * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

float3 TonemapHableWithWhite(float3 color, float whitePoint)
{
	const float3 mapped = TonemapHable(color);
	const float whiteScale = 1.0 / TonemapHable(float3(whitePoint, whitePoint, whitePoint)).r;
	return mapped * whiteScale;
}

float3 TonemapACESFit(float3 color)
{
	// ACES fitted curve (RRT+ODT approximation).
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float3 ACESHighlightDesat(float3 color)
{
	const float luma = dot(color, AP1_RGB2Y);
	const float t = saturate((luma - 1.0) / 3.0);
	return lerp(color, float3(luma, luma, luma), t);
}

float RgbToSaturation(float3 rgb)
{
	const float maxv = max(max(rgb.x, rgb.y), rgb.z);
	const float minv = min(min(rgb.x, rgb.y), rgb.z);
	return (max(maxv, 1e-6) - max(minv, 1e-6)) / max(maxv, 1e-2);
}

float RgbToHue(float3 rgb)
{
	if (rgb.x == rgb.y && rgb.y == rgb.z)
	{
		return 0.0;
	}
	const float hue = degrees(atan2(sqrt(3.0) * (rgb.y - rgb.z), 2.0 * rgb.x - rgb.y - rgb.z));
	return (hue < 0.0) ? (hue + 360.0) : hue;
}

float RgbToYC(float3 rgb, float ycRadiusWeight)
{
	const float r = rgb.x;
	const float g = rgb.y;
	const float b = rgb.z;
	const float chroma = sqrt(b * (b - g) + g * (g - r) + r * (r - b));
	return (r + g + b + ycRadiusWeight * chroma) / 3.0;
}

float SigmoidShaper(float x)
{
	const float t = max(1.0 - abs(x * 0.5), 0.0);
	const float y = 1.0 + sign(x) * (1.0 - t * t);
	return y * 0.5;
}

float GlowFwd(float ycIn, float glowGain, float glowMid)
{
	if (ycIn <= (2.0 / 3.0) * glowMid)
	{
		return glowGain;
	}
	if (ycIn >= 2.0 * glowMid)
	{
		return 0.0;
	}
	return glowGain * (glowMid / ycIn - 0.5);
}

float CenterHue(float hue, float centerH)
{
	float hueCentered = hue - centerH;
	if (hueCentered < -180.0)
	{
		hueCentered += 360.0;
	}
	else if (hueCentered > 180.0)
	{
		hueCentered -= 360.0;
	}
	return hueCentered;
}

float Smoothstep01(float x)
{
	const float t = saturate(x);
	return t * t * (3.0 - 2.0 * t);
}

float3 ACESRRTAndODTFit(float3 color)
{
	const float3 a = color * (color + 0.0245786) - 0.000090537;
	const float3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
	return a / b;
}

float3 TonemapACESUE5_AP1(float3 color)
{
	// UE5-style ACES RRT/ODT with glow, red modifier, and saturation controls (linear AP1 in).
	const float3 ap0 = mul(AP1_2_AP0_MAT, color);

	const float rrtSaturation = 0.96;
	const float odtSaturation = 0.93;
	const float ycRadiusWeight = 1.75;

	// Glow module.
	const float saturation = RgbToSaturation(ap0);
	const float ycIn = RgbToYC(ap0, ycRadiusWeight);
	const float s = SigmoidShaper((saturation - 0.4) / 0.2);
	const float addedGlow = 1.0 + GlowFwd(ycIn, 0.05 * s, 0.08);
	float3 colorAp0 = ap0 * addedGlow;

	// Red modifier.
	const float rrtRedScale = 0.82;
	const float rrtRedPivot = 0.03;
	const float rrtRedHue = 0.0;
	const float rrtRedWidth = 135.0;
	const float hue = RgbToHue(colorAp0);
	const float centeredHue = CenterHue(hue, rrtRedHue);
	float hueWeight = Smoothstep01(1.0 - abs(2.0 * centeredHue / rrtRedWidth));
	hueWeight *= hueWeight;
	colorAp0.x += hueWeight * saturation * (rrtRedPivot - colorAp0.x) * (1.0 - rrtRedScale);

	// Back to AP1.
	float3 colorAp1 = mul(AP0_2_AP1_MAT, colorAp0);
	colorAp1 = max(colorAp1, 0.0);

	// Pre-saturation (RRT).
	const float3 lumaAp1 = dot(colorAp1, AP1_RGB2Y).xxx;
	colorAp1 = lerp(lumaAp1, colorAp1, rrtSaturation);

	// RRT + ODT fit curve.
	float3 tone = ACESRRTAndODTFit(colorAp1);

	// Post-saturation (ODT).
	const float3 toneLuma = dot(tone, AP1_RGB2Y).xxx;
	tone = lerp(toneLuma, tone, odtSaturation);
	return saturate(tone);
}

// HDR-aware ACES tonemap: takes MaxLuminance, BlackPoint (normalized), and MidGrey into account.
// - maxLuminance: display peak luminance in nits (e.g. 1000)
// - blackPoint: normalized black level (minNits / maxNits)
// - midGrey: scene linear value that should map to 18% output (e.g. sdrWhite * 0.18 / maxNits)
float3 TonemapACESUE5_AP1_HDR(float3 color, float maxLuminance, float blackPoint, float midGrey)
{
	// Reference luminance for SDR (100 nits) and mid-grey (18%).
	const float sdrReference = 100.0;
	const float midGreyTarget = 0.18;

	// 1. Normalize input to SDR-equivalent range so ACES curve works correctly.
	//    Scene 0.18 (mid-grey) should map to ~0.18 input to the curve.
	float3 normalized = color;
	if (midGrey > 0.0)
	{
		// Scale so that midGrey input becomes 0.18.
		normalized = color * (midGreyTarget / midGrey);
	}

	// 2. Apply black point offset (lift blacks).
	normalized = max(normalized - blackPoint, 0.0);

	// 3. Convert to AP1 and run through ACES RRT+ODT.
	const float3 ap0 = mul(AP1_2_AP0_MAT, normalized);

	const float rrtSaturation = 0.96;
	const float odtSaturation = 0.93;
	const float ycRadiusWeight = 1.75;

	// Glow module.
	const float saturation = RgbToSaturation(ap0);
	const float ycIn = RgbToYC(ap0, ycRadiusWeight);
	const float s = SigmoidShaper((saturation - 0.4) / 0.2);
	const float addedGlow = 1.0 + GlowFwd(ycIn, 0.05 * s, 0.08);
	float3 colorAp0 = ap0 * addedGlow;

	// Red modifier.
	const float rrtRedScale = 0.82;
	const float rrtRedPivot = 0.03;
	const float rrtRedHue = 0.0;
	const float rrtRedWidth = 135.0;
	const float hue = RgbToHue(colorAp0);
	const float centeredHue = CenterHue(hue, rrtRedHue);
	float hueWeight = Smoothstep01(1.0 - abs(2.0 * centeredHue / rrtRedWidth));
	hueWeight *= hueWeight;
	colorAp0.x += hueWeight * saturation * (rrtRedPivot - colorAp0.x) * (1.0 - rrtRedScale);

	// Back to AP1.
	float3 colorAp1 = mul(AP0_2_AP1_MAT, colorAp0);
	colorAp1 = max(colorAp1, 0.0);

	// Pre-saturation (RRT).
	const float3 lumaAp1 = dot(colorAp1, AP1_RGB2Y).xxx;
	colorAp1 = lerp(lumaAp1, colorAp1, rrtSaturation);

	// RRT + ODT fit curve (outputs 0-1 for SDR).
	float3 tone = ACESRRTAndODTFit(colorAp1);

	// Post-saturation (ODT).
	const float3 toneLuma = dot(tone, AP1_RGB2Y).xxx;
	tone = lerp(toneLuma, tone, odtSaturation);

	// 4. Expand output range for HDR displays.
	//    SDR ACES outputs 0-1 (maps to 0-100 nits).
	//    For HDR, scale up to use the full display range.
	const float hdrHeadroom = maxLuminance / sdrReference;
	tone = tone * hdrHeadroom;

	// Clamp to valid range (0 to maxLuminance normalized to 1).
	return saturate(tone / hdrHeadroom) * hdrHeadroom;
}

float3 TonemapNeutral(float3 color, float whitePoint)
{
	const float3 x = color / (whitePoint + color);
	return x;
}

float3 TonemapLiftGammaGain(float3 color, float lift, float gamma, float gain)
{
	color = color + lift;
	color = pow(max(color, 0.0), 1.0 / max(gamma, 1e-4));
	return color * gain;
}

float3 TonemapSaturation(float3 color, float saturation)
{
	const float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
	return lerp(float3(luma, luma, luma), color, saturation);
}

float3 TonemapTCamSimplified(float3 color, TonemapParams params)
{
	float3 c = ApplyExposure(color, params.Exposure);
	c = TonemapFilmicNarkowicz(c);
	c = TonemapSaturation(c, params.Saturation);
	return c;
}

float3 TonemapTCamApprox(float3 color, TonemapParams params)
{
	float3 c = ApplyExposure(color, params.Exposure);
	c = max(c - params.BlackPoint, 0.0);
	c *= (params.MidGrey > 0.0) ? (0.18 / params.MidGrey) : 1.0;
	c /= max(params.WhitePoint, 1e-4);

	const float toeLen = max(params.ToeLength, 1e-4);
	const float shoulderLen = max(params.ShoulderLength, 1e-4);
	const float toeStrength = saturate(params.ToeStrength);
	const float shoulderStrength = saturate(params.ShoulderStrength);
	const float shoulderAngle = saturate(params.ShoulderAngle);

	const float3 toeCurve = c / (c + toeLen);
	const float3 shoulderCurve = 1.0 - exp(-c * shoulderLen);
	const float3 t = lerp(c, toeCurve, toeStrength);
	const float3 s = lerp(c, shoulderCurve, shoulderStrength);
	float3 shaped = lerp(t, s, shoulderAngle);
	shaped = pow(max(shaped, 0.0), 1.0 / max(params.Contrast, 1e-4));

	shaped = TonemapLiftGammaGain(shaped, params.Lift, params.Gamma, params.Gain);
	shaped = TonemapSaturation(shaped, params.Saturation);
	return shaped;
}

float3 ApplyTonemap(float3 color, TonemapParams params, uint tonemapMode, uint workingColorSpaceId)
{
	float3 ap1 = ConvertWorkingSpace(color, workingColorSpaceId, WORKING_COLORSPACE_AP1);

	float3 mapped = TonemapACESUE5_AP1(ap1);

	return ConvertWorkingSpace(mapped, WORKING_COLORSPACE_AP1, workingColorSpaceId);
}

// ACEScct transfer functions. Input/Output are normalized (0-1).
float LinearToACEScct(float x)
{
	const float a = 10.5402377416545;
	const float b = 0.0729055341958355;
	if (x <= 0.0078125)
	{
		return a * x + b;
	}
	return (log2(x) + 9.72) / 17.52;
}

float ACEScctToLinear(float x)
{
	const float a = 10.5402377416545;
	const float b = 0.0729055341958355;
	if (x <= 0.155251141552511)
	{
		return (x - b) / a;
	}
	return exp2(x * 17.52 - 9.72);
}

float3 LinearToACEScct(float3 v)
{
	const float a = 10.5402377416545;
	const float b = 0.0729055341958355;
	float3 linearMask = step(v, 0.0078125);
	float3 linearPart = a * v + b;
	float3 logPart = (log2(v) + 9.72) / 17.52;
	return lerp(logPart, linearPart, linearMask);
}

float3 ACEScctToLinear(float3 v)
{
	const float a = 10.5402377416545;
	const float b = 0.0729055341958355;
	float3 linearMask = step(v, 0.155251141552511);
	float3 linearPart = (v - b) / a;
	float3 expPart = exp2(v * 17.52 - 9.72);
	return lerp(expPart, linearPart, linearMask);
}

#endif
