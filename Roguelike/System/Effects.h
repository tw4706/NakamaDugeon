#pragma once
#include"MathUtil.h"
#include"Game.h"
#include"Font.h"
#include<vector>
#include<string>

/// <summary>
/// パーティクルの形
/// </summary>
enum class ParticleShape
{
	Circle,
	Star,
	Confetti,
	Ring,
	Spark,
};

struct Particle
{
	Vec2 pos;
	Vec2 vel;
	float gravity = 0.0f;
	float drag = 0.0f;
	float life = 1.0f;
	float maxLife = 1.0f;
	float size = 10.0f;
	float endSize = 0.0f;
	float angle = 0.0f;
	float spin = 0.0f;
	Game::Rgb color = { 255, 255, 255 };
	ParticleShape shape = ParticleShape::Circle;
	bool isAdd = false;
};

struct TextPopup
{
	Vec2 pos;
	std::wstring text;
	Game::Rgb color;
	FontSize size;
	float life;
	float maxLife;
};

/// <summary>
/// パーティクル・文字ポップ・画面揺れ・フラッシュ
/// </summary>
class Effects
{
public:
	static Effects& GetInstance();

private:
	Effects() = default;
	Effects(const Effects&) = delete;
	Effects& operator=(const Effects&) = delete;

public:
	void Update(float dt);

	/// <summary>
	/// パーティクルと文字の描画
	/// </summary>
	void Draw() const;

	/// <summary>
	/// 画面全体のフラッシュの描画
	/// </summary>
	void DrawFlash() const;

	void Clear();

	void Add(const Particle& p);

	/// <summary>
	/// 放射状に飛び散らせる
	/// </summary>
	void Burst(const Vec2& pos, int count, const Game::Rgb& color, float speedMin, float speedMax,
		float size, float life, ParticleShape shape, float gravity = 0.0f, bool isAdd = true);

	/// <summary>
	/// 広がる輪
	/// </summary>
	void Ring(const Vec2& pos, const Game::Rgb& color, float startSize, float endSize, float life);

	/// <summary>
	/// 紙吹雪(画面上から)
	/// </summary>
	void ConfettiRain(int count);

	/// <summary>
	/// 紙吹雪(指定位置から)
	/// </summary>
	void ConfettiBurst(const Vec2& pos, int count);

	void Smoke(const Vec2& pos, int count, const Game::Rgb& color);

	void Popup(const Vec2& pos, const std::wstring& text, const Game::Rgb& color, FontSize size = FontSize::Medium);

	void Shake(float power, float time);
	void Flash(const Game::Rgb& color, int alpha);

	Vec2 GetShakeOffset() const { return shakeOffset_; }

	/// <summary>
	/// 描画時にずらす量(カメラ)
	/// </summary>
	void SetDrawOffset(const Vec2& offset) { drawOffset_ = offset; }

private:
	std::vector<Particle> particles_;
	std::vector<TextPopup> popups_;

	float shakePower_ = 0.0f;
	float shakeTime_ = 0.0f;
	float shakeMaxTime_ = 0.0f;
	Vec2 shakeOffset_;
	Vec2 drawOffset_;

	Game::Rgb flashColor_ = { 255, 255, 255 };
	float flashAlpha_ = 0.0f;
};
