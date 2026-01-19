#pragma once

#include <cstdint>

struct LinearColor;

struct Color
{
	std::uint8_t r = 0;
	std::uint8_t g = 0;
	std::uint8_t b = 0;
	std::uint8_t a = 255;

	Color() = default;
	Color(std::uint8_t inR, std::uint8_t inG, std::uint8_t inB, std::uint8_t inA = 255);

	LinearColor ToLinear(bool useSRGB = true) const;

	static Color Clamp(const Color& value, std::uint8_t minValue = 0, std::uint8_t maxValue = 255);
	static Color Lerp(const Color& a, const Color& b, float t);

	Color& operator+=(const Color& rhs);
	Color& operator-=(const Color& rhs);
	Color& operator*=(const Color& rhs);
	Color& operator*=(float scalar);
};

Color operator+(Color lhs, const Color& rhs);
Color operator-(Color lhs, const Color& rhs);
Color operator*(Color lhs, const Color& rhs);
Color operator*(Color lhs, float scalar);

struct LinearColor
{
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 1.0f;

	LinearColor() = default;
	LinearColor(float inR, float inG, float inB, float inA = 1.0f);

	Color ToColor(bool useSRGB = true) const;

	static LinearColor Clamp(const LinearColor& value, float minValue = 0.0f, float maxValue = 1.0f);
	static LinearColor Saturate(const LinearColor& value);
	static LinearColor Lerp(const LinearColor& a, const LinearColor& b, float t);

	LinearColor& operator+=(const LinearColor& rhs);
	LinearColor& operator-=(const LinearColor& rhs);
	LinearColor& operator*=(const LinearColor& rhs);
	LinearColor& operator*=(float scalar);
	LinearColor& operator/=(float scalar);
};

LinearColor operator+(LinearColor lhs, const LinearColor& rhs);
LinearColor operator-(LinearColor lhs, const LinearColor& rhs);
LinearColor operator*(LinearColor lhs, const LinearColor& rhs);
LinearColor operator*(LinearColor lhs, float scalar);
LinearColor operator*(float scalar, LinearColor rhs);
LinearColor operator/(LinearColor lhs, float scalar);
