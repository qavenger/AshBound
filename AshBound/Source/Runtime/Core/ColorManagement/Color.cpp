#include "Runtime/Core/ColorManagement/Color.h"

#include <algorithm>
#include <cmath>

namespace
{
	float ClampFloat(float value, float minValue, float maxValue)
	{
		return std::max(minValue, std::min(maxValue, value));
	}

	std::uint8_t ClampByte(int value)
	{
		if (value < 0)
		{
			return 0;
		}
		if (value > 255)
		{
			return 255;
		}
		return static_cast<std::uint8_t>(value);
	}

	float SRGBToLinearChannel(float value)
	{
		if (value <= 0.04045f)
		{
			return value / 12.92f;
		}
		return std::pow((value + 0.055f) / 1.055f, 2.4f);
	}

	float LinearToSRGBChannel(float value)
	{
		if (value <= 0.0031308f)
		{
			return 12.92f * value;
		}
		return 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
	}

	float ByteToFloat(std::uint8_t value)
	{
		return static_cast<float>(value) / 255.0f;
	}

	std::uint8_t FloatToByte(float value)
	{
		const float scaled = ClampFloat(value, 0.0f, 1.0f) * 255.0f;
		return ClampByte(static_cast<int>(scaled + 0.5f));
	}
}

Color::Color(std::uint8_t inR, std::uint8_t inG, std::uint8_t inB, std::uint8_t inA)
	: r(inR)
	, g(inG)
	, b(inB)
	, a(inA)
{
}

LinearColor Color::ToLinear(bool useSRGB) const
{
	const float rf = ByteToFloat(r);
	const float gf = ByteToFloat(g);
	const float bf = ByteToFloat(b);
	const float af = ByteToFloat(a);

	if (!useSRGB)
	{
		return LinearColor(rf, gf, bf, af);
	}

	return LinearColor(
		SRGBToLinearChannel(rf),
		SRGBToLinearChannel(gf),
		SRGBToLinearChannel(bf),
		af);
}

Color Color::Clamp(const Color& value, std::uint8_t minValue, std::uint8_t maxValue)
{
	Color result;
	result.r = static_cast<std::uint8_t>(std::max(minValue, std::min(maxValue, value.r)));
	result.g = static_cast<std::uint8_t>(std::max(minValue, std::min(maxValue, value.g)));
	result.b = static_cast<std::uint8_t>(std::max(minValue, std::min(maxValue, value.b)));
	result.a = static_cast<std::uint8_t>(std::max(minValue, std::min(maxValue, value.a)));
	return result;
}

Color Color::Lerp(const Color& a, const Color& b, float t)
{
	const float clamped = ClampFloat(t, 0.0f, 1.0f);
	const float r = static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * clamped;
	const float g = static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * clamped;
	const float bVal = static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * clamped;
	const float aVal = static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * clamped;
	return Color(
		ClampByte(static_cast<int>(r + 0.5f)),
		ClampByte(static_cast<int>(g + 0.5f)),
		ClampByte(static_cast<int>(bVal + 0.5f)),
		ClampByte(static_cast<int>(aVal + 0.5f)));
}

Color& Color::operator+=(const Color& rhs)
{
	r = ClampByte(static_cast<int>(r) + rhs.r);
	g = ClampByte(static_cast<int>(g) + rhs.g);
	b = ClampByte(static_cast<int>(b) + rhs.b);
	a = ClampByte(static_cast<int>(a) + rhs.a);
	return *this;
}

Color& Color::operator-=(const Color& rhs)
{
	r = ClampByte(static_cast<int>(r) - rhs.r);
	g = ClampByte(static_cast<int>(g) - rhs.g);
	b = ClampByte(static_cast<int>(b) - rhs.b);
	a = ClampByte(static_cast<int>(a) - rhs.a);
	return *this;
}

Color& Color::operator*=(const Color& rhs)
{
	r = ClampByte(static_cast<int>((static_cast<int>(r) * rhs.r + 127) / 255));
	g = ClampByte(static_cast<int>((static_cast<int>(g) * rhs.g + 127) / 255));
	b = ClampByte(static_cast<int>((static_cast<int>(b) * rhs.b + 127) / 255));
	a = ClampByte(static_cast<int>((static_cast<int>(a) * rhs.a + 127) / 255));
	return *this;
}

Color& Color::operator*=(float scalar)
{
	const float clamped = ClampFloat(scalar, 0.0f, 1.0f);
	r = ClampByte(static_cast<int>(static_cast<float>(r) * clamped + 0.5f));
	g = ClampByte(static_cast<int>(static_cast<float>(g) * clamped + 0.5f));
	b = ClampByte(static_cast<int>(static_cast<float>(b) * clamped + 0.5f));
	a = ClampByte(static_cast<int>(static_cast<float>(a) * clamped + 0.5f));
	return *this;
}

Color operator+(Color lhs, const Color& rhs)
{
	lhs += rhs;
	return lhs;
}

Color operator-(Color lhs, const Color& rhs)
{
	lhs -= rhs;
	return lhs;
}

Color operator*(Color lhs, const Color& rhs)
{
	lhs *= rhs;
	return lhs;
}

Color operator*(Color lhs, float scalar)
{
	lhs *= scalar;
	return lhs;
}

LinearColor::LinearColor(float inR, float inG, float inB, float inA)
	: r(inR)
	, g(inG)
	, b(inB)
	, a(inA)
{
}

Color LinearColor::ToColor(bool useSRGB) const
{
	float rr = r;
	float gg = g;
	float bb = b;

	if (useSRGB)
	{
		rr = LinearToSRGBChannel(ClampFloat(rr, 0.0f, 1.0f));
		gg = LinearToSRGBChannel(ClampFloat(gg, 0.0f, 1.0f));
		bb = LinearToSRGBChannel(ClampFloat(bb, 0.0f, 1.0f));
	}

	return Color(
		FloatToByte(rr),
		FloatToByte(gg),
		FloatToByte(bb),
		FloatToByte(a));
}

LinearColor LinearColor::Clamp(const LinearColor& value, float minValue, float maxValue)
{
	return LinearColor(
		ClampFloat(value.r, minValue, maxValue),
		ClampFloat(value.g, minValue, maxValue),
		ClampFloat(value.b, minValue, maxValue),
		ClampFloat(value.a, minValue, maxValue));
}

LinearColor LinearColor::Saturate(const LinearColor& value)
{
	return Clamp(value, 0.0f, 1.0f);
}

LinearColor LinearColor::Lerp(const LinearColor& a, const LinearColor& b, float t)
{
	const float clamped = ClampFloat(t, 0.0f, 1.0f);
	return LinearColor(
		a.r + (b.r - a.r) * clamped,
		a.g + (b.g - a.g) * clamped,
		a.b + (b.b - a.b) * clamped,
		a.a + (b.a - a.a) * clamped);
}

LinearColor& LinearColor::operator+=(const LinearColor& rhs)
{
	r += rhs.r;
	g += rhs.g;
	b += rhs.b;
	a += rhs.a;
	return *this;
}

LinearColor& LinearColor::operator-=(const LinearColor& rhs)
{
	r -= rhs.r;
	g -= rhs.g;
	b -= rhs.b;
	a -= rhs.a;
	return *this;
}

LinearColor& LinearColor::operator*=(const LinearColor& rhs)
{
	r *= rhs.r;
	g *= rhs.g;
	b *= rhs.b;
	a *= rhs.a;
	return *this;
}

LinearColor& LinearColor::operator*=(float scalar)
{
	r *= scalar;
	g *= scalar;
	b *= scalar;
	a *= scalar;
	return *this;
}

LinearColor& LinearColor::operator/=(float scalar)
{
	const float inv = 1.0f / scalar;
	r *= inv;
	g *= inv;
	b *= inv;
	a *= inv;
	return *this;
}

LinearColor operator+(LinearColor lhs, const LinearColor& rhs)
{
	lhs += rhs;
	return lhs;
}

LinearColor operator-(LinearColor lhs, const LinearColor& rhs)
{
	lhs -= rhs;
	return lhs;
}

LinearColor operator*(LinearColor lhs, const LinearColor& rhs)
{
	lhs *= rhs;
	return lhs;
}

LinearColor operator*(LinearColor lhs, float scalar)
{
	lhs *= scalar;
	return lhs;
}

LinearColor operator*(float scalar, LinearColor rhs)
{
	rhs *= scalar;
	return rhs;
}

LinearColor operator/(LinearColor lhs, float scalar)
{
	lhs /= scalar;
	return lhs;
}
