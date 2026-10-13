#include "Fade.h"
#include "Game.h"
#include "MathUtil.h"
#include "DrawUtil.h"
#include<DxLib.h>
#include<algorithm>
#include<cmath>

namespace
{
	constexpr float kFadeTime = 0.45f;
	constexpr float kMaxRadius = 760.0f;
	constexpr float kRingThickness = 1700.0f;
}

void Fade::StartFadeIn()
{
	state_ = State::FadeIn;
	rate_ = 1.0f;
}

void Fade::StartFadeOut()
{
	state_ = State::FadeOut;
	rate_ = 0.0f;
}

void Fade::Update(float dt)
{
	if (state_ == State::FadeIn)
	{
		rate_ -= dt / kFadeTime;
		if (rate_ <= 0.0f)
		{
			rate_ = 0.0f;
			state_ = State::None;
		}
	}
	else if (state_ == State::FadeOut)
	{
		rate_ += dt / kFadeTime;
		if (rate_ >= 1.0f)
		{
			rate_ = 1.0f;
			state_ = State::FadeOutEnd;
		}
	}
}

void Fade::Draw() const
{
	if (rate_ <= 0.0f) return;

	const unsigned int color = GetColor(20, 15, 40);
	if (rate_ >= 1.0f)
	{
		DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, color, TRUE);
		return;
	}

	//太い輪で穴の外側を塗る
	const float hole = kMaxRadius * (1.0f - MathUtil::EaseInCubic(rate_));
	const float cx = Game::kScreenWidth * Game::kHalf;
	const float cy = Game::kScreenHeight * Game::kHalf;
	DrawCircleAA(cx, cy, hole + kRingThickness * 0.5f, 96, color, FALSE, kRingThickness);

	//穴のふちの星
	if (hole > 20.0f)
	{
		for (int i = 0; i < 8; i++)
		{
			const float angle = rate_ * 6.0f + MathUtil::kPi * 2.0f * i / 8.0f;
			DrawUtil::Star(cx + std::cos(angle) * hole, cy + std::sin(angle) * hole, 18.0f, angle, GetColor(255, 220, 80));
		}
	}
}
