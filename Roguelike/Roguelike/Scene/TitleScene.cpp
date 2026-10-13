#include "TitleScene.h"
#include "SceneManager.h"
#include "SaveSelectScene.h"
#include "SettingsScene.h"
#include "MonsterData.h"
#include "UpdateLog.h"
#include "Input.h"
#include "Sound.h"
#include "Effects.h"
#include "DrawUtil.h"
#include "Font.h"
#include "ButtonGuide.h"
#include "Unit.h"
#include<DxLib.h>
#include<cmath>
#include<algorithm>

using namespace MathUtil;

namespace
{
	const wchar_t* kItems[] = { L"はじめる", L"せってい", L"おわる" };
	constexpr int kItemNum = static_cast<int>(std::size(kItems));

	//地平線と おかの たかさ
	constexpr float kHorizonY = 470.0f;
	constexpr float kGroundY = 575.0f;

	//きまった ばらつき(星の位置など)
	float Hash(int i)
	{
		const unsigned int h = static_cast<unsigned int>(i) * 2654435761u;
		return static_cast<float>((h >> 8) % 10000) / 10000.0f;
	}

	//山なみ
	void Mountains(float baseY, float height, float step, int seed, unsigned int color)
	{
		float px = -40.0f;
		float py = baseY - height * Hash(seed);
		for (int k = 1; px < Game::kScreenWidth + 40.0f; k++)
		{
			const float nx = px + step * (0.6f + Hash(seed + k * 7) * 0.8f);
			const float ny = baseY - height * (0.2f + Hash(seed + k * 13) * 0.8f);
			DrawQuadrangleAA(px, py, nx, ny, nx, kGroundY + 20.0f, px, kGroundY + 20.0f, color, TRUE);
			px = nx;
			py = ny;
		}
	}
}

TitleScene::TitleScene(SceneManager& manager) :
	Scene(manager)
{
}

void TitleScene::Init()
{
	Sound::GetInstance().PlayBGM(BGM::Title);
}

void TitleScene::Update()
{
	time_ += Game::kFrameTime;

	//ほたる
	if (RandI(0, 7) == 0)
	{
		Particle p;
		p.pos = { RandF(80.0f, 1200.0f), RandF(kGroundY - 20.0f, kGroundY + 100.0f) };
		p.vel = { RandF(-15.0f, 15.0f), RandF(-30.0f, -10.0f) };
		p.life = p.maxLife = RandF(1.5f, 3.0f);
		p.size = RandF(3.0f, 5.0f);
		p.endSize = p.size;
		p.color = { 200, 255, 140 };
		p.isAdd = true;
		Effects::GetInstance().Add(p);
	}

	if (isDecided_) return;
	Input& input = Input::GetInstance();
	if (input.IsAnyTriggered(Button::Up) || input.IsAnyTriggered(Button::Left)) { cursor_ = (cursor_ + kItemNum - 1) % kItemNum; Sound::GetInstance().PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down) || input.IsAnyTriggered(Button::Right)) { cursor_ = (cursor_ + 1) % kItemNum; Sound::GetInstance().PlaySE(SE::Cursor); }

	if (input.IsAnyTriggered(Button::Decide))
	{
		Sound::GetInstance().PlaySE(SE::Decide);
		isDecided_ = true;
		switch (cursor_)
		{
		case 0: sceneManager_.ChangeSceneWithFade(std::make_shared<SaveSelectScene>(sceneManager_)); break;
		case 1: sceneManager_.ChangeSceneWithFade(std::make_shared<SettingsScene>(sceneManager_, SettingsScene::Return::Title)); break;
		default: Game::isExitRequested = true; break;
		}
	}
}

void TitleScene::DrawBackground()
{
	//よぞら
	DrawUtil::Gradient(0, 0, Game::kScreenWidth, static_cast<int>(kHorizonY), { 12, 14, 40 }, { 70, 55, 110 });
	DrawUtil::Gradient(0, static_cast<int>(kHorizonY), Game::kScreenWidth, Game::kScreenHeight, { 70, 55, 110 }, { 30, 25, 50 });

	//ほし
	for (int i = 0; i < 90; i++)
	{
		const float x = Hash(i * 3 + 1) * Game::kScreenWidth;
		const float y = Hash(i * 3 + 2) * (kHorizonY - 60.0f);
		const float tw = 0.5f + 0.5f * std::sin(time_ * (1.5f + Hash(i) * 2.0f) + i);
		const int a = static_cast<int>(80 + 170 * tw * (1.0f - y / kHorizonY));
		SetDrawBlendMode(DX_BLENDMODE_ADD, a);
		DrawCircleAA(x, y, 1.0f + Hash(i * 5) * 1.6f, 6, GetColor(255, 250, 230), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//ながれぼし
	const float shoot = std::fmod(time_, 7.0f);
	if (shoot < 0.8f)
	{
		const float t = shoot / 0.8f;
		const float sx = 300.0f + t * 380.0f;
		const float sy = 60.0f + t * 120.0f;
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255 * (1.0f - t)));
		DrawLineAA(sx - 70.0f, sy - 22.0f, sx, sy, GetColor(255, 240, 200), 2.5f);
		DrawCircleAA(sx, sy, 3.0f, 8, GetColor(255, 255, 240), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//つき
	SetDrawBlendMode(DX_BLENDMODE_ADD, 50);
	DrawCircleAA(1080.0f, 110.0f, 95.0f, 40, GetColor(255, 230, 170), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawCircleAA(1080.0f, 110.0f, 50.0f, 40, GetColor(255, 245, 210), TRUE);
	DrawCircleAA(1066.0f, 98.0f, 9.0f, 16, GetColor(235, 220, 180), TRUE);
	DrawCircleAA(1094.0f, 122.0f, 12.0f, 16, GetColor(235, 220, 180), TRUE);
	DrawCircleAA(1072.0f, 132.0f, 5.0f, 12, GetColor(235, 220, 180), TRUE);

	//山なみ と とおくの とう
	Mountains(kHorizonY - 10.0f, 150.0f, 160.0f, 11, GetColor(60, 50, 95));
	{
		const float tx = 215.0f;
		const float ty = kHorizonY - 120.0f;
		DrawBoxAA(tx - 18.0f, ty, tx + 18.0f, kHorizonY, GetColor(45, 38, 70), TRUE);
		DrawTriangleAA(tx - 26.0f, ty, tx + 26.0f, ty, tx, ty - 46.0f, GetColor(45, 38, 70), TRUE);
		const float glow = 0.6f + 0.4f * std::sin(time_ * 2.0f);
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(200 * glow));
		DrawBoxAA(tx - 5.0f, ty + 16.0f, tx + 5.0f, ty + 30.0f, GetColor(255, 180, 80), TRUE);
		DrawCircleAA(tx, ty + 23.0f, 16.0f, 12, GetColor(120, 60, 20), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	Mountains(kHorizonY + 25.0f, 110.0f, 130.0f, 37, GetColor(42, 36, 72));

	//おか
	DrawOvalAA(640.0f, kGroundY + 170.0f, 900.0f, 210.0f, 60, GetColor(36, 70, 52), TRUE);
	DrawOvalAA(640.0f, kGroundY + 176.0f, 880.0f, 200.0f, 60, GetColor(28, 56, 42), TRUE);
	for (int i = 0; i < 40; i++)
	{
		const float gx = 60.0f + Hash(i * 11 + 3) * 1160.0f;
		const float dx = (gx - 640.0f) / 900.0f;
		const float gy = kGroundY + 170.0f - 210.0f * std::sqrt(std::max(0.0f, 1.0f - dx * dx)) + 14.0f + Hash(i * 7) * 30.0f;
		const float sway = std::sin(time_ * 2.0f + i) * 2.0f;
		DrawLineAA(gx, gy, gx - 3.0f + sway, gy - 10.0f, GetColor(60, 110, 70), 2.0f);
		DrawLineAA(gx, gy, gx + 4.0f + sway, gy - 8.0f, GetColor(60, 110, 70), 2.0f);
	}
}

void TitleScene::DrawLogo()
{
	const float pop = EaseOutBack(Clamp01(time_ * 1.4f));


	//リボン
	const float ry = 112.0f;
	const float rw = 250.0f * pop;
	const unsigned int ribbonDark = GetColor(130, 30, 40);
	DrawTriangleAA(640.0f - rw - 40.0f, ry - 18.0f, 640.0f - rw, ry - 26.0f, 640.0f - rw, ry + 30.0f, ribbonDark, TRUE);
	DrawTriangleAA(640.0f - rw - 40.0f, ry - 18.0f, 640.0f - rw - 20.0f, ry + 4.0f, 640.0f - rw - 40.0f, ry + 26.0f, ribbonDark, TRUE);
	DrawTriangleAA(640.0f + rw + 40.0f, ry - 18.0f, 640.0f + rw, ry - 26.0f, 640.0f + rw, ry + 30.0f, ribbonDark, TRUE);
	DrawTriangleAA(640.0f + rw + 40.0f, ry - 18.0f, 640.0f + rw + 20.0f, ry + 4.0f, 640.0f + rw + 40.0f, ry + 26.0f, ribbonDark, TRUE);
	DrawBoxAA(640.0f - rw, ry - 30.0f, 640.0f + rw, ry + 30.0f, GetColor(190, 45, 55), TRUE);
	DrawLineAA(640.0f - rw, ry - 24.0f, 640.0f + rw, ry - 24.0f, GetColor(240, 200, 110), 2.0f);
	DrawLineAA(640.0f - rw, ry + 24.0f, 640.0f + rw, ry + 24.0f, GetColor(240, 200, 110), 2.0f);
	Font::DrawCenterScaled(640.0f, ry, pop, L"まものつかいの", GetColor(255, 245, 220), FontSize::Large, GetColor(90, 20, 30));

	//ぼうけん
	const float my = 225.0f;
	Font::DrawCenterScaled(646.0f, my + 8.0f, pop, L"ぼうけん", GetColor(60, 25, 10), FontSize::Giant, GetColor(60, 25, 10));
	Font::DrawCenterScaled(640.0f, my, pop, L"ぼうけん", GetColor(255, 215, 80), FontSize::Giant, GetColor(120, 50, 15));

	//きらめき
	for (int k = 0; k < 3; k++)
	{
		const float cycle = time_ * 0.7f + k * 0.33f;
		const float ph = std::fmod(cycle, 1.0f);
		if (ph > 0.25f) continue;
		const int seed = static_cast<int>(cycle) * 3 + k;
		const float sx = 400.0f + Hash(seed * 17 + 5) * 480.0f;
		const float sy = my - 50.0f + Hash(seed * 31 + 9) * 100.0f;
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255 * (1.0f - ph / 0.25f)));
		DrawUtil::Star(sx, sy, 10.0f + 6.0f * std::sin(ph * 12.0f), time_ * 3.0f, GetColor(255, 250, 220));
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	Font::DrawCenter(640.0f, 335.0f, L"～ まものと ともに ふかき めいきゅうへ ～", GetColor(230, 220, 255), FontSize::Small, GetColor(30, 20, 40));
}

void TitleScene::Draw()
{
	DrawBackground();
	DrawLogo();

	//まものつかいと なかまたち
	const float baseY = kGroundY;
	UnitDraw::Hero({ 640.0f, baseY - 4.0f }, 60.0f, time_, 1.0f, false);
	const int ids[4] = { 0, 1, 3, 15 };
	const float xs[4] = { 430.0f, 530.0f, 760.0f, 860.0f };
	for (int i = 0; i < 4; i++)
	{
		const float hop = std::fabs(std::sin(time_ * 2.5f + i * 1.3f)) * 6.0f;
		UnitDraw::Monster({ xs[i], baseY + 10.0f - hop }, 40.0f, MonsterTable::Get(ids[i]), time_, xs[i] < 640.0f ? 1.0f : -1.0f, false);
	}

	Effects::GetInstance().Draw();

	//メニュー(よこ ならび)
	const unsigned int edge = GetColor(20, 15, 30);
	for (int i = 0; i < kItemNum; i++)
	{
		const float x = 640.0f + (i - 1) * 230.0f;
		const float y = 640.0f;
		const bool isSel = i == cursor_;
		const float bob = isSel ? std::sin(time_ * 6.0f) * 2.0f : 0.0f;
		DrawUtil::Panel(x - 95.0f, y - 28.0f + bob, x + 95.0f, y + 28.0f + bob, isSel ? Game::Rgb{ 120, 70, 40 } : Game::Rgb{ 30, 26, 50 }, isSel ? 245 : 190,
			isSel ? Game::Rgb{ 255, 220, 120 } : Game::Rgb{ 100, 90, 130 });
		Font::DrawCenter(x, y + bob, kItems[i], isSel ? GetColor(255, 245, 190) : GetColor(190, 180, 210), FontSize::Medium, edge);
	}
	ButtonGuide::DrawCenter(640.0f, 698.0f, L"{LR}：えらぶ　{A}：けってい", GetColor(170, 160, 200), FontSize::Small, edge);
	const std::wstring ver = L"ver" + UpdateLog::Latest();
	Font::DrawFit(1265.0f - Font::Width(ver, FontSize::Small), 686.0f, ver, GetColor(140, 130, 170), FontSize::Small, edge, 200.0f);
}
