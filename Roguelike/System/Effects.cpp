#include "Effects.h"
#include "DrawUtil.h"
#include "SaveData.h"
#include "Input.h"
#include<DxLib.h>
#include<cmath>
#include<algorithm>

using namespace MathUtil;

namespace
{
	constexpr size_t kMaxParticle = 3000;

	//紙吹雪の色
	const Game::Rgb kConfettiColors[] =
	{
		{ 255, 80, 80 }, { 80, 160, 255 }, { 90, 220, 110 }, { 255, 220, 60 }, { 255, 130, 220 }, { 255, 160, 60 },
	};

	Game::Rgb RandomConfettiColor()
	{
		return kConfettiColors[RandI(0, static_cast<int>(std::size(kConfettiColors)) - 1)];
	}
}

Effects& Effects::GetInstance()
{
	static Effects instance;
	return instance;
}

void Effects::Update(float dt)
{
	for (auto& p : particles_)
	{
		p.vel.y += p.gravity * dt;
		if (p.drag > 0.0f) p.vel *= std::exp(-p.drag * dt);
		p.pos += p.vel * dt;
		p.angle += p.spin * dt;
		p.life -= dt;
	}
	std::erase_if(particles_, [](const Particle& p) { return p.life <= 0.0f; });

	for (auto& t : popups_)
	{
		t.pos.y -= 50.0f * dt;
		t.life -= dt;
	}
	std::erase_if(popups_, [](const TextPopup& t) { return t.life <= 0.0f; });

	//画面揺れ
	if (shakeTime_ > 0.0f)
	{
		shakeTime_ -= dt;
		const float power = shakePower_ * std::max(0.0f, shakeTime_ / shakeMaxTime_);
		shakeOffset_ = { RandF(-power, power), RandF(-power, power) };
	}
	else
	{
		shakeOffset_ = {};
	}

	flashAlpha_ = std::max(0.0f, flashAlpha_ - 600.0f * dt);
}

void Effects::Draw() const
{
	for (const auto& p : particles_)
	{
		const Vec2 P = p.pos + drawOffset_;
		const float rate = Clamp01(p.life / p.maxLife);
		const float size = Lerp(p.endSize, p.size, rate);
		const int alpha = static_cast<int>(255 * std::min(1.0f, rate * 2.0f));
		const unsigned int color = Game::ToColor(p.color);

		SetDrawBlendMode(p.isAdd ? DX_BLENDMODE_ADD : DX_BLENDMODE_ALPHA, alpha);
		switch (p.shape)
		{
		case ParticleShape::Circle:
			DrawCircleAA(P.x, P.y, size, 16, color, TRUE);
			break;
		case ParticleShape::Star:
			DrawUtil::Star(P.x, P.y, size, p.angle, color);
			break;
		case ParticleShape::Confetti:
		{
			//ひらひら回転
			const float w = size;
			const float h = size * 0.6f * std::fabs(std::cos(p.angle * 1.7f));
			const float c = std::cos(p.angle);
			const float s = std::sin(p.angle);
			auto rot = [&](float x, float y) { return Vec2(P.x + x * c - y * s, P.y + x * s + y * c); };
			const Vec2 a = rot(-w, -h);
			const Vec2 b = rot(w, -h);
			const Vec2 d = rot(w, h);
			const Vec2 e = rot(-w, h);
			DrawQuadrangleAA(a.x, a.y, b.x, b.y, d.x, d.y, e.x, e.y, color, TRUE);
			break;
		}
		case ParticleShape::Ring:
			DrawCircleAA(P.x, P.y, size, 48, color, FALSE, 3.0f + 8.0f * rate);
			break;
		case ParticleShape::Spark:
		{
			const Vec2 dir = p.vel.Normalized();
			const Vec2 tail = P - dir * (size * 3.0f);
			DrawLineAA(tail.x, tail.y, P.x, P.y, color, std::max(1.0f, size * 0.6f));
			break;
		}
		}
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	for (const auto& t : popups_)
	{
		const float rate = t.life / t.maxLife;
		const float appear = Clamp01((t.maxLife - t.life) / 0.2f);
		const float scale = EaseOutBack(appear);
		const int alpha = static_cast<int>(255 * Clamp01(rate * 3.0f));
		Font::DrawCenterAlpha(t.pos.x + drawOffset_.x, t.pos.y + drawOffset_.y, scale, alpha, t.text.c_str(), Game::ToColor(t.color), t.size, GetColor(30, 30, 40));
	}
}

void Effects::DrawFlash() const
{
	if (flashAlpha_ <= 0.0f) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(flashAlpha_));
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, Game::ToColor(flashColor_), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Effects::Clear()
{
	particles_.clear();
	popups_.clear();
	shakeTime_ = 0.0f;
	shakeOffset_ = {};
}

void Effects::Add(const Particle& p)
{
	if (particles_.size() >= kMaxParticle) return;
	particles_.push_back(p);
}

void Effects::Burst(const Vec2& pos, int count, const Game::Rgb& color, float speedMin, float speedMax,
	float size, float life, ParticleShape shape, float gravity, bool isAdd)
{
	for (int i = 0; i < count; i++)
	{
		const float angle = RandF(0.0f, kPi * 2.0f);
		const float speed = RandF(speedMin, speedMax);
		Particle p;
		p.pos = pos;
		p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
		p.gravity = gravity;
		p.drag = 2.0f;
		p.life = p.maxLife = life * RandF(0.7f, 1.2f);
		p.size = size * RandF(0.6f, 1.3f);
		p.endSize = 0.0f;
		p.angle = RandF(0.0f, kPi * 2.0f);
		p.spin = RandF(-8.0f, 8.0f);
		p.color = color;
		p.shape = shape;
		p.isAdd = isAdd;
		if (shape == ParticleShape::Confetti)
		{
			p.color = RandomConfettiColor();
			p.drag = 1.5f;
			p.endSize = p.size;
		}
		Add(p);
	}
}

void Effects::Ring(const Vec2& pos, const Game::Rgb& color, float startSize, float endSize, float life)
{
	Particle p;
	p.pos = pos;
	p.life = p.maxLife = life;
	p.size = endSize;
	p.endSize = startSize;
	p.color = color;
	p.shape = ParticleShape::Ring;
	p.isAdd = true;
	Add(p);
}

void Effects::ConfettiRain(int count)
{
	for (int i = 0; i < count; i++)
	{
		Particle p;
		p.pos = { RandF(0.0f, static_cast<float>(Game::kScreenWidth)), RandF(-80.0f, -10.0f) };
		p.vel = { RandF(-60.0f, 60.0f), RandF(80.0f, 200.0f) };
		p.gravity = 40.0f;
		p.life = p.maxLife = RandF(4.0f, 6.0f);
		p.size = p.endSize = RandF(6.0f, 11.0f);
		p.angle = RandF(0.0f, kPi * 2.0f);
		p.spin = RandF(-6.0f, 6.0f);
		p.color = RandomConfettiColor();
		p.shape = ParticleShape::Confetti;
		Add(p);
	}
}

void Effects::ConfettiBurst(const Vec2& pos, int count)
{
	for (int i = 0; i < count; i++)
	{
		const float angle = RandF(-kPi * 0.9f, -kPi * 0.1f);
		const float speed = RandF(300.0f, 800.0f);
		Particle p;
		p.pos = pos;
		p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
		p.gravity = 500.0f;
		p.drag = 2.0f;
		p.life = p.maxLife = RandF(2.0f, 3.5f);
		p.size = p.endSize = RandF(6.0f, 11.0f);
		p.angle = RandF(0.0f, kPi * 2.0f);
		p.spin = RandF(-8.0f, 8.0f);
		p.color = RandomConfettiColor();
		p.shape = ParticleShape::Confetti;
		Add(p);
	}
}

void Effects::Smoke(const Vec2& pos, int count, const Game::Rgb& color)
{
	for (int i = 0; i < count; i++)
	{
		const float angle = RandF(0.0f, kPi * 2.0f);
		const float speed = RandF(30.0f, 160.0f);
		Particle p;
		p.pos = pos + Vec2(RandF(-10.0f, 10.0f), RandF(-10.0f, 10.0f));
		p.vel = { std::cos(angle) * speed, std::sin(angle) * speed - 40.0f };
		p.drag = 2.5f;
		p.life = p.maxLife = RandF(0.5f, 1.0f);
		p.size = RandF(8.0f, 16.0f);
		p.endSize = p.size * 2.5f;
		p.color = color;
		p.shape = ParticleShape::Circle;
		p.isAdd = false;
		Add(p);
	}
}

void Effects::Popup(const Vec2& pos, const std::wstring& text, const Game::Rgb& color, FontSize size)
{
	popups_.push_back({ pos, text, color, size, 1.0f, 1.0f });
}

void Effects::Shake(float power, float time)
{
	const Settings& st = SaveManager::GetInstance().GetSettings();
	if (st.isVibration) Input::GetInstance().Vibrate(std::clamp(static_cast<int>(power * 60.0f), 200, 1000), static_cast<int>(time * 1000.0f));
	if (!st.isShake) return;
	if (power < shakePower_ * Clamp01(shakeTime_ / std::max(0.001f, shakeMaxTime_))) return;
	shakePower_ = power;
	shakeTime_ = time;
	shakeMaxTime_ = time;
}

void Effects::Flash(const Game::Rgb& color, int alpha)
{
	flashColor_ = color;
	flashAlpha_ = static_cast<float>(alpha);
}
