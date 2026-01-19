#ifndef COLOR_MANAGEMENT_HLSL
#define COLOR_MANAGEMENT_HLSL
#define WhitePoint uint
static const WhitePoint WhitePoint_D65 = 0;
static const WhitePoint WhitePoint_D60 = 1;
static const WhitePoint WhitePoint_DCI = 2;


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
	WhitePoint White;
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
