#pragma once
#include<cmath>
#include<random>
#include<algorithm>

struct Vec2
{
	float x = 0.0f;
	float y = 0.0f;

	Vec2() = default;
	Vec2(float inX, float inY) : x(inX), y(inY) {}

	Vec2 operator+(const Vec2& v) const { return { x + v.x, y + v.y }; }
	Vec2 operator-(const Vec2& v) const { return { x - v.x, y - v.y }; }
	Vec2 operator*(float s) const { return { x * s, y * s }; }
	Vec2 operator/(float s) const { return { x / s, y / s }; }
	Vec2 operator-() const { return { -x, -y }; }
	Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
	Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
	Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

	float Length() const { return std::sqrt(x * x + y * y); }
	float Dot(const Vec2& v) const { return x * v.x + y * v.y; }

	Vec2 Normalized() const
	{
		float len = Length();
		if (len <= 0.0001f) return { 0.0f, 0.0f };
		return { x / len, y / len };
	}
};

namespace MathUtil
{
	constexpr float kPi = 3.14159265f;

	inline std::mt19937& Rng()
	{
		static std::mt19937 rng(std::random_device{}());
		return rng;
	}

	inline float RandF(float min, float max)
	{
		std::uniform_real_distribution<float> dist(min, max);
		return dist(Rng());
	}

	//min以上max以下
	inline int RandI(int min, int max)
	{
		std::uniform_int_distribution<int> dist(min, max);
		return dist(Rng());
	}

	inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
	inline float Clamp01(float t) { return std::clamp(t, 0.0f, 1.0f); }

	inline float EaseOutCubic(float t) { t = Clamp01(t); return 1.0f - std::pow(1.0f - t, 3.0f); }
	inline float EaseInCubic(float t) { t = Clamp01(t); return t * t * t; }

	inline float EaseOutBack(float t)
	{
		t = Clamp01(t);
		const float c1 = 1.70158f;
		const float c3 = c1 + 1.0f;
		return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
	}

	inline float EaseOutElastic(float t)
	{
		t = Clamp01(t);
		if (t <= 0.0f || t >= 1.0f) return t;
		const float c4 = (2.0f * kPi) / 3.0f;
		return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
	}
}
