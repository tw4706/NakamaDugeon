#include "FishingScene.h"
#include "SceneManager.h"
#include "CampScene.h"
#include "SaveData.h"
#include "FishData.h"
#include "Unit.h"
#include "Input.h"
#include "Sound.h"
#include "Effects.h"
#include "DrawUtil.h"
#include "Font.h"
#include "ButtonGuide.h"
#include<DxLib.h>
#include<cmath>
#include<algorithm>

using namespace MathUtil;

namespace
{
	constexpr float kWaterTop = 250.0f;
	const Vec2 kHeroFeet = { 230.0f, 610.0f };
	constexpr float kHeroSize = 60.0f;
	const Vec2 kRodBase = { 257.0f, 574.0f };

	constexpr float kCastTime = 0.6f;

	//まきあげの ゲージ(下から の たかさで かんがえる)
	constexpr float kBarH = 440.0f;
	constexpr float kBarBottom = 600.0f;
	constexpr float kBarLeft = 1110.0f;
	constexpr float kBarRight = 1160.0f;

	//むらに もどったときに たつ ところ(いけの みぎ)
	const Cell kReturnCell = { 7, 11 };

	/// <summary>
	/// みどりの わくの たかさ(あばれる さかなほど せまい)
	/// </summary>
	float ZoneHeight(const FishData& d)
	{
		return 175.0f - 60.0f * d.difficulty;
	}

	std::wstring Stars(int rarity)
	{
		std::wstring s;
		for (int i = 0; i < 5; i++) s += i < rarity ? L"★" : L"☆";
		return s;
	}
}

FishingScene::FishingScene(SceneManager& manager) :
	Scene(manager)
{
}

void FishingScene::Init()
{
	Sound::GetInstance().PlayBGM(BGM::Village);
	SaveManager::GetInstance().Current().fish.resize(FishTable::All().size());
}

bool FishingScene::IsHolding() const
{
	const Input& input = Input::GetInstance();
	for (int p = 0; p < Game::kPlayerNum; p++)
	{
		if (input.IsPressed(p, Button::Decide)) return true;
	}
	return false;
}

Vec2 FishingScene::RodTip() const
{
	//まきあげ中は さおが しなる
	if (state_ == State::Reel)
	{
		const float pull = IsHolding() ? 1.0f : 0.4f;
		return { 410.0f + std::sin(time_ * 23.0f) * 3.0f * pull, 440.0f + 25.0f * pull };
	}
	if (state_ == State::Bite) return { 405.0f, 430.0f + std::sin(time_ * 30.0f) * 4.0f };
	return { 395.0f, 410.0f };
}

void FishingScene::Update()
{
	const float dt = Game::kFrameTime;
	time_ += dt;
	stateTime_ += dt;
	if (state_ != State::Bite) bobberDip_ = std::max(0.0f, bobberDip_ - 2.5f * dt);
	if (isLeaving_) return;

	switch (state_)
	{
	case State::Ready: UpdateReady(); break;
	case State::Cast: UpdateCast(); break;
	case State::Wait: UpdateWait(); break;
	case State::Bite: UpdateBite(); break;
	case State::Reel: UpdateReel(); break;
	case State::Caught:
	case State::Miss: UpdateResult(); break;
	}
}

void FishingScene::UpdateReady()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	if (input.IsAnyTriggered(Button::Cancel))
	{
		isLeaving_ = true;
		sound.PlaySE(SE::Cancel);
		sceneManager_.ChangeSceneWithFade(std::make_shared<CampScene>(sceneManager_, kReturnCell));
		return;
	}
	if (input.IsAnyTriggered(Button::Decide))
	{
		castFrom_ = RodTip() + Vec2(0.0f, 60.0f);
		bobber_ = { RandF(620.0f, 960.0f), RandF(400.0f, 520.0f) };
		state_ = State::Cast;
		stateTime_ = 0.0f;
		sound.PlaySE(SE::Whoosh);
	}
}

void FishingScene::UpdateCast()
{
	if (stateTime_ < kCastTime) return;

	//ちゃくすい
	Sound::GetInstance().PlaySE(SE::Splash);
	Effects::GetInstance().Burst(bobber_, 10, { 200, 230, 255 }, 60.0f, 160.0f, 5.0f, 0.5f, ParticleShape::Circle, 400.0f, false);
	Effects::GetInstance().Ring(bobber_, { 220, 240, 255 }, 6.0f, 50.0f, 0.6f);

	const SaveSlot& s = SaveManager::GetInstance().Current();
	fish_ = FishTable::Pick(s.isCleared);
	size_ = FishTable::RollSize(fish_);
	waitTimer_ = RandF(2.0f, 6.0f);
	nibbleTimer_ = RandF(0.6f, 1.4f);
	state_ = State::Wait;
	stateTime_ = 0.0f;
}

void FishingScene::UpdateWait()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const float dt = Game::kFrameTime;

	if (input.IsAnyTriggered(Button::Cancel))
	{
		state_ = State::Ready;
		stateTime_ = 0.0f;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (input.IsAnyTriggered(Button::Decide))
	{
		Miss(L"はやすぎた… さかなが にげてしまった");
		return;
	}

	//つんつん(まだ あたりではない)
	nibbleTimer_ -= dt;
	if (nibbleTimer_ <= 0.0f && waitTimer_ > 0.5f)
	{
		nibbleTimer_ = RandF(0.5f, 1.6f);
		bobberDip_ = RandF(0.2f, 0.4f);
		sound.PlaySE(SE::Tick, 0.7f);
		Effects::GetInstance().Ring(bobber_, { 220, 240, 255 }, 4.0f, 26.0f, 0.4f);
	}

	waitTimer_ -= dt;
	if (waitTimer_ > 0.0f) return;

	//あたり！ めずらしい さかなほど みじかい
	const FishData& d = FishTable::All()[fish_];
	biteTimer_ = 1.5f - 0.08f * d.rarity;
	bobberDip_ = 1.0f;
	state_ = State::Bite;
	stateTime_ = 0.0f;
	sound.PlaySE(SE::Signal);
	input.Vibrate(500, 200);
	Effects::GetInstance().Burst(bobber_, 8, { 200, 230, 255 }, 40.0f, 120.0f, 4.0f, 0.4f, ParticleShape::Circle, 300.0f, false);
}

void FishingScene::UpdateBite()
{
	if (Input::GetInstance().IsAnyTriggered(Button::Decide))
	{
		StartReel();
		return;
	}
	biteTimer_ -= Game::kFrameTime;
	if (biteTimer_ <= 0.0f) Miss(L"にげられた… うきが しずんだら すぐに {A}！");
}

void FishingScene::StartReel()
{
	zoneY_ = 0.0f;
	zoneVel_ = 0.0f;
	fishY_ = kBarH * 0.25f;
	fishTarget_ = fishY_;
	fishMoveTimer_ = 0.5f;
	progress_ = 0.4f;
	reelTick_ = 0.0f;
	bobberDip_ = 0.0f;
	state_ = State::Reel;
	stateTime_ = 0.0f;
	Sound::GetInstance().PlaySE(SE::Decide);
	Input::GetInstance().Vibrate(400, 150);
}

void FishingScene::UpdateReel()
{
	const float dt = Game::kFrameTime;
	const FishData& d = FishTable::All()[fish_];
	const float zoneH = ZoneHeight(d);

	//みどりの わく: おすと うえへ、はなすと おちる
	zoneVel_ += (IsHolding() ? 1000.0f : -750.0f) * dt;
	zoneVel_ = std::clamp(zoneVel_, -360.0f, 360.0f);
	zoneY_ += zoneVel_ * dt;
	if (zoneY_ < 0.0f)
	{
		zoneY_ = 0.0f;
		if (zoneVel_ < 0.0f) zoneVel_ = -zoneVel_ * 0.25f;
	}
	if (zoneY_ > kBarH - zoneH)
	{
		zoneY_ = kBarH - zoneH;
		if (zoneVel_ > 0.0f) zoneVel_ = 0.0f;
	}

	//さかなは きまぐれに うごく
	fishMoveTimer_ -= dt;
	if (fishMoveTimer_ <= 0.0f)
	{
		fishMoveTimer_ = RandF(0.6f, 1.6f) * (1.4f - d.difficulty * 0.8f);
		const bool isDash = RandF(0.0f, 1.0f) < d.difficulty * 0.5f;
		const float range = isDash ? kBarH : kBarH * 0.35f;
		fishTarget_ = std::clamp(fishY_ + RandF(-range, range), 12.0f, kBarH - 12.0f);
	}
	const float speed = 50.0f + 230.0f * d.difficulty;
	fishY_ += std::clamp(fishTarget_ - fishY_, -speed * dt, speed * dt);

	const bool isInside = fishY_ >= zoneY_ && fishY_ <= zoneY_ + zoneH;
	if (isInside) progress_ += (0.42f - 0.14f * d.difficulty) * dt;
	else progress_ -= (0.09f + 0.09f * d.difficulty) * dt;

	//リールの おと
	reelTick_ -= dt;
	if (isInside && reelTick_ <= 0.0f)
	{
		reelTick_ = 0.22f;
		Sound::GetInstance().PlaySE(SE::Tick, 1.3f + progress_ * 0.5f);
	}

	//さかなが あばれる しぶき
	if (RandI(0, 9) == 0)
	{
		const Vec2 p = bobber_ + (kRodBase - bobber_) * (progress_ * 0.45f);
		Effects::GetInstance().Burst(p, 2, { 210, 235, 255 }, 40.0f, 110.0f, 4.0f, 0.4f, ParticleShape::Circle, 350.0f, false);
	}

	if (progress_ >= 1.0f) Land();
	else if (progress_ <= 0.0f) Miss(L"いとが きれた… にげられた");
}

void FishingScene::Land()
{
	SaveSlot& s = SaveManager::GetInstance().Current();
	if (s.fish.size() < FishTable::All().size()) s.fish.resize(FishTable::All().size());
	FishRecord& rec = s.fish[fish_];
	isNewFish_ = rec.count == 0;
	isRecord_ = !isNewFish_ && size_ > rec.best;
	rec.count++;
	rec.best = std::max(rec.best, size_);

	//もちかえって たきびで りょうりに できる
	isStockFull_ = static_cast<int>(rec.stock.size()) >= FishTable::kMaxStock;
	if (!isStockFull_) rec.stock.push_back(size_);
	gotRewards_ = KeyItem::GiveFishRewards(s);
	SaveManager::GetInstance().Save();
	todayCount_++;

	const FishData& d = FishTable::All()[fish_];
	Sound& sound = Sound::GetInstance();
	sound.PlaySE(SE::Splash);
	sound.PlaySE(isNewFish_ || d.rarity >= 4 ? SE::Fanfare : SE::Point);
	Effects::GetInstance().Burst({ 640.0f, 300.0f }, 16, { 200, 230, 255 }, 120.0f, 300.0f, 6.0f, 0.7f, ParticleShape::Circle, 500.0f, false);
	if (isNewFish_ || isRecord_ || !gotRewards_.empty()) Effects::GetInstance().ConfettiBurst({ 640.0f, 200.0f }, 40);
	Input::GetInstance().Vibrate(600, 250);

	state_ = State::Caught;
	stateTime_ = 0.0f;
}

void FishingScene::Miss(const std::wstring& message)
{
	message_ = message;
	state_ = State::Miss;
	stateTime_ = 0.0f;
	Sound::GetInstance().PlaySE(SE::Buzzer);
}

void FishingScene::UpdateResult()
{
	Input& input = Input::GetInstance();
	if (stateTime_ < 0.5f) return;
	const bool isPressed = input.IsAnyTriggered(Button::Decide) || input.IsAnyTriggered(Button::Cancel);
	if (isPressed || (state_ == State::Miss && stateTime_ > 2.0f))
	{
		state_ = State::Ready;
		stateTime_ = 0.0f;
		if (isPressed) Sound::GetInstance().PlaySE(SE::Decide);
	}
}

//==============================//
// 表示
//==============================//

void FishingScene::Draw()
{
	DrawBackground();
	DrawBobber();
	DrawHeroAndLine();
	Effects::GetInstance().Draw();
	DrawHud();
	if (state_ == State::Reel) DrawReelGauge();
	if (state_ == State::Caught) DrawCaught();
}

void FishingScene::DrawBackground()
{
	//そらと とおくの もり
	DrawUtil::Gradient(0, 0, Game::kScreenWidth, static_cast<int>(kWaterTop) + 10, { 140, 200, 245 }, { 215, 235, 250 });
	DrawOvalAA(260.0f, 250.0f, 420.0f, 90.0f, 40, GetColor(120, 170, 110), TRUE);
	DrawOvalAA(980.0f, 255.0f, 480.0f, 110.0f, 40, GetColor(105, 160, 100), TRUE);
	for (int k = 0; k < 18; k++)
	{
		const float x = k * 76.0f + 20.0f;
		const float y = 222.0f + static_cast<float>((k * 37) % 5) * 4.0f;
		DrawCircleAA(x, y, 34.0f + static_cast<float>((k * 13) % 3) * 6.0f, 20, GetColor(60, 120, 70), TRUE);
		DrawCircleAA(x - 10.0f, y - 10.0f, 14.0f, 14, GetColor(85, 150, 85), TRUE);
	}

	//みず
	DrawUtil::Gradient(0, static_cast<int>(kWaterTop), Game::kScreenWidth, Game::kScreenHeight, { 95, 165, 215 }, { 35, 90, 155 });
	DrawLineAA(0.0f, kWaterTop, static_cast<float>(Game::kScreenWidth), kWaterTop, GetColor(210, 235, 250), 3.0f);
	for (int k = 0; k < 14; k++)
	{
		const float y = kWaterTop + 30.0f + k * 32.0f;
		const float x = std::fmod(k * 197.0f + time_ * (12.0f + k * 1.5f), 1400.0f) - 60.0f;
		const float w = 40.0f + k * 4.0f;
		DrawLineAA(x, y, x + w, y, GetColor(150, 200, 240), 2.0f);
	}

	//はすの は
	const Vec2 pads[] = { { 560.0f, 330.0f }, { 1010.0f, 360.0f }, { 880.0f, 610.0f }, { 1180.0f, 470.0f } };
	for (int k = 0; k < 4; k++)
	{
		const Vec2 p = pads[k] + Vec2(0.0f, std::sin(time_ * 1.2f + k) * 2.0f);
		DrawOvalAA(p.x, p.y, 34.0f, 13.0f, 20, GetColor(70, 150, 80), TRUE);
		DrawTriangleAA(p.x, p.y, p.x + 30.0f, p.y - 7.0f, p.x + 30.0f, p.y + 3.0f, GetColor(85, 155, 205), TRUE);
		if (k % 2 == 0) DrawCircleAA(p.x - 10.0f, p.y - 6.0f, 6.0f, 12, GetColor(255, 180, 210), TRUE);
	}

	//きしと あし
	DrawOvalAA(150.0f, 720.0f, 360.0f, 150.0f, 48, GetColor(110, 85, 55), TRUE);
	DrawOvalAA(140.0f, 726.0f, 345.0f, 140.0f, 48, GetColor(100, 170, 85), TRUE);
	for (int k = 0; k < 9; k++)
	{
		const float x = 1170.0f + k * 12.0f;
		const float sway = std::sin(time_ * 1.5f + k) * 4.0f;
		DrawLineAA(x, 720.0f, x + sway, 560.0f + (k % 3) * 20.0f, GetColor(90, 140, 60), 4.0f);
		if (k % 3 == 0) DrawOvalAA(x + sway, 560.0f + (k % 3) * 20.0f, 4.0f, 14.0f, 10, GetColor(120, 80, 45), TRUE);
	}
}

void FishingScene::DrawHeroAndLine()
{
	UnitDraw::Hero(kHeroFeet, kHeroSize, state_ == State::Reel ? time_ * 2.0f : time_, 1.0f, false, false);

	//さお
	const Vec2 tip = RodTip();
	DrawLineAA(kRodBase.x, kRodBase.y, tip.x, tip.y, GetColor(120, 80, 40), 5.0f);
	DrawLineAA(kRodBase.x, kRodBase.y, kRodBase.x + (tip.x - kRodBase.x) * 0.25f, kRodBase.y + (tip.y - kRodBase.y) * 0.25f, GetColor(60, 40, 30), 7.0f);

	//いと(たるみ)
	Vec2 end = tip + Vec2(std::sin(time_ * 2.0f) * 6.0f, 60.0f);
	float sag = 0.0f;
	switch (state_)
	{
	case State::Cast:
	{
		const float t = std::min(1.0f, stateTime_ / kCastTime);
		end = castFrom_ + (bobber_ - castFrom_) * t - Vec2(0.0f, std::sin(t * kPi) * 180.0f);
		sag = 10.0f;
		break;
	}
	case State::Wait: end = bobber_; sag = 28.0f; break;
	case State::Bite: end = bobber_ + Vec2(0.0f, 8.0f); sag = 4.0f; break;
	case State::Reel: end = bobber_ + (kRodBase - bobber_) * (progress_ * 0.45f); break;
	case State::Caught: return;
	default: break;
	}
	Vec2 prev = tip;
	for (int i = 1; i <= 16; i++)
	{
		const float t = i / 16.0f;
		const Vec2 p = tip + (end - tip) * t + Vec2(0.0f, std::sin(t * kPi) * sag);
		DrawLineAA(prev.x, prev.y, p.x, p.y, GetColor(240, 240, 240), 1.5f);
		prev = p;
	}

	//ぶらさがって いる うき
	if (state_ == State::Ready || state_ == State::Miss || state_ == State::Cast)
	{
		DrawCircleAA(end.x, end.y, 7.0f, 14, GetColor(240, 240, 240), TRUE);
		DrawCircleAA(end.x, end.y - 2.0f, 6.0f, 14, GetColor(230, 60, 50), TRUE);
	}
}

void FishingScene::DrawBobber()
{
	if (state_ != State::Wait && state_ != State::Bite && state_ != State::Reel) return;

	Vec2 p = bobber_;
	if (state_ == State::Reel) p = bobber_ + (kRodBase - bobber_) * (progress_ * 0.45f) + Vec2(std::sin(time_ * 31.0f) * 4.0f, 0.0f);

	//さかなの かげ(ちかづいてくる)
	if (state_ == State::Wait && waitTimer_ < 1.8f)
	{
		const float t = 1.0f - waitTimer_ / 1.8f;
		const Vec2 sp = p + Vec2(-140.0f * (1.0f - t), 40.0f * (1.0f - t) + 12.0f);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(110 * t));
		DrawOvalAA(sp.x, sp.y, 34.0f, 11.0f, 20, GetColor(20, 40, 70), TRUE);
		DrawTriangleAA(sp.x - 30.0f, sp.y, sp.x - 48.0f, sp.y - 10.0f, sp.x - 48.0f, sp.y + 10.0f, GetColor(20, 40, 70), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//なみもん
	for (int k = 0; k < 2; k++)
	{
		const float t = std::fmod(time_ * 0.8f + k * 0.5f, 1.0f);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(160 * (1.0f - t)));
		DrawOvalAA(p.x, p.y + 4.0f, 10.0f + t * 34.0f, 3.0f + t * 10.0f, 24, GetColor(220, 240, 255), FALSE, 2.0f);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//うき(しずむほど みえなく なる)
	const float sink = (state_ == State::Bite ? 1.0f : bobberDip_) * 12.0f;
	const float bob = std::sin(time_ * 2.5f) * 1.5f;
	const float y = p.y + sink + bob;
	if (sink < 11.0f) DrawCircleAA(p.x, y, 8.0f, 16, GetColor(245, 245, 245), TRUE);
	DrawCircleAA(p.x, y - 6.0f, 7.0f, 16, GetColor(230, 60, 50), TRUE);
	DrawLineAA(p.x, y - 13.0f, p.x, y - 20.0f, GetColor(40, 40, 40), 2.0f);

	//あたりの「！」
	if (state_ == State::Bite)
	{
		const float pop = EaseOutBack(std::min(1.0f, stateTime_ * 6.0f));
		Font::DrawCenterScaled(p.x, p.y - 64.0f, pop, L"！", GetColor(255, 90, 60), FontSize::Large, GetColor(255, 255, 255));
	}
}

void FishingScene::DrawReelGauge()
{
	const FishData& d = FishTable::All()[fish_];
	const float zoneH = ZoneHeight(d);
	const unsigned int edge = GetColor(20, 15, 30);

	DrawUtil::Panel(kBarLeft - 26.0f, kBarBottom - kBarH - 40.0f, kBarRight + 56.0f, kBarBottom + 20.0f, { 40, 34, 60 }, 230, { 200, 180, 120 });
	Font::DrawCenter((kBarLeft + kBarRight) * 0.5f + 15.0f, kBarBottom - kBarH - 22.0f, L"まきあげ", GetColor(255, 230, 140), FontSize::Small, edge);

	//ゲージの なか(みず)
	DrawBoxAA(kBarLeft, kBarBottom - kBarH, kBarRight, kBarBottom, GetColor(40, 90, 150), TRUE);

	//みどりの わく
	const bool isInside = fishY_ >= zoneY_ && fishY_ <= zoneY_ + zoneH;
	const unsigned int zoneCol = isInside ? GetColor(120, 230, 110) : GetColor(90, 180, 90);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
	DrawBoxAA(kBarLeft + 2.0f, kBarBottom - zoneY_ - zoneH, kBarRight - 2.0f, kBarBottom - zoneY_, zoneCol, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawBoxAA(kBarLeft + 2.0f, kBarBottom - zoneY_ - zoneH, kBarRight - 2.0f, kBarBottom - zoneY_, GetColor(230, 255, 220), FALSE, 2.0f);

	//さかな
	const float wiggle = std::sin(time_ * 18.0f) * 2.0f;
	FishDraw::Fish({ (kBarLeft + kBarRight) * 0.5f + wiggle, kBarBottom - fishY_ }, 42.0f, d, time_ * 2.0f, std::sin(time_ * 3.0f) >= 0.0f ? 1.0f : -1.0f, true);
	DrawBoxAA(kBarLeft, kBarBottom - kBarH, kBarRight, kBarBottom, GetColor(200, 180, 120), FALSE, 3.0f);

	//すすみぐあい
	const float px1 = kBarRight + 14.0f;
	const float px2 = kBarRight + 30.0f;
	DrawBoxAA(px1, kBarBottom - kBarH, px2, kBarBottom, GetColor(30, 25, 40), TRUE);
	const Game::Rgb col = DrawUtil::Lerp({ 240, 90, 60 }, { 120, 230, 100 }, Clamp01(progress_));
	DrawBoxAA(px1, kBarBottom - kBarH * Clamp01(progress_), px2, kBarBottom, Game::ToColor(col), TRUE);
	DrawBoxAA(px1, kBarBottom - kBarH, px2, kBarBottom, GetColor(200, 180, 120), FALSE, 2.0f);
}

void FishingScene::DrawCaught()
{
	const FishData& d = FishTable::All()[fish_];
	const unsigned int edge = GetColor(20, 15, 30);
	const float pop = EaseOutBack(std::min(1.0f, stateTime_ * 3.0f));

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawUtil::Window(640.0f - 320.0f * pop, 90.0f, 640.0f + 320.0f * pop, 620.0f);
	if (pop < 0.3f) return;
	Font::DrawCenterScaled(640.0f, 140.0f, pop, L"つれた！", GetColor(255, 220, 90), FontSize::Large, GetColor(60, 30, 10));

	//さかな(★が おおいほど おおきく)
	const float len = 200.0f + d.rarity * 18.0f;
	SetDrawBlendMode(DX_BLENDMODE_ADD, 50);
	DrawCircleAA(640.0f, 280.0f, 120.0f, 40, Game::ToColor(d.color), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	FishDraw::Fish({ 640.0f, 280.0f + std::sin(time_ * 3.0f) * 4.0f }, len, d, time_, 1.0f, false);

	Font::DrawCenterFit(640.0f, 390.0f, d.name, GetColor(255, 240, 160), FontSize::Medium, edge, 560.0f);
	Font::DrawCenter(640.0f, 435.0f, Stars(d.rarity).c_str(), GetColor(255, 210, 80), FontSize::Small, edge);

	const FishRecord& rec = SaveManager::GetInstance().Current().fish[fish_];
	const std::wstring sizeText = L"おおきさ " + std::to_wstring(size_) + L"cm　（さいこう " + std::to_wstring(rec.best) + L"cm・" + std::to_wstring(rec.count) + L"ひき め）";
	Font::DrawCenterFit(640.0f, 475.0f, sizeText, GetColor(230, 225, 245), FontSize::Small, edge, 560.0f);

	//もちもの(たきびで りょうり)
	const std::wstring stock = isStockFull_ ? L"もちきれない ので いけに かえした（" + std::to_wstring(FishTable::kMaxStock) + L"ひき まで）" :
		L"もちかえった（もちもの " + std::to_wstring(rec.stock.size()) + L"ひき）　たきびで「" + d.dish + L"」に できる";
	Font::DrawCenterFit(640.0f, 507.0f, stock, isStockFull_ ? GetColor(180, 170, 200) : GetColor(200, 240, 210), FontSize::Small, edge, 600.0f);

	const float bounce = std::fabs(std::sin(time_ * 5.0f)) * 6.0f;
	std::wstring news;
	if (!gotRewards_.empty()) news = std::wstring(L"だいじな もの「") + KeyItem::Get(gotRewards_.back()).name + L"」を てにいれた！（" + KeyItem::EffectText(gotRewards_.back()) + L"）";
	else if (isNewFish_) news = L"NEW！ つりずかんに とうろく";
	else if (isRecord_) news = L"きろく こうしん！";
	if (!news.empty()) Font::DrawCenterFit(640.0f, 545.0f - bounce, news, gotRewards_.empty() ? (isNewFish_ ? GetColor(255, 120, 90) : GetColor(255, 200, 80)) : GetColor(255, 220, 120), FontSize::Small, edge, 600.0f);

	if (stateTime_ > 0.5f) ButtonGuide::DrawCenter(640.0f, 592.0f, L"{A}：つづける", GetColor(170, 160, 200), FontSize::Small, edge);
}

void FishingScene::DrawHud()
{
	const unsigned int edge = GetColor(20, 15, 30);
	const SaveSlot& s = SaveManager::GetInstance().Current();
	int kinds = 0;
	int total = 0;
	for (size_t i = 0; i < FishTable::All().size(); i++)
	{
		if (!FishTable::IsAvailable(static_cast<int>(i), s.isCleared)) continue;
		total++;
		if (i < s.fish.size() && s.fish[i].count > 0) kinds++;
	}
	DrawUtil::Panel(20.0f, 16.0f, 360.0f, 108.0f, { 30, 26, 46 }, 210, { 200, 180, 120 });
	Font::DrawFit(36.0f, 24.0f, L"むらの いけ", GetColor(255, 230, 140), FontSize::Small, edge, 310.0f);
	Font::DrawFit(36.0f, 62.0f, L"つりずかん " + std::to_wstring(kinds) + L"/" + std::to_wstring(total) + L"　きょう " + std::to_wstring(todayCount_) + L"ひき",
		GetColor(230, 225, 245), FontSize::Small, edge, 310.0f);

	//したの あんない
	std::wstring guide;
	switch (state_)
	{
	case State::Ready: guide = L"{A}：なげる　{B}：むらへ もどる"; break;
	case State::Cast: break;
	case State::Wait: guide = L"うきが しずんだら {A}！　{B}：ひきあげる"; break;
	case State::Bite: guide = L"いまだ！ {A}"; break;
	case State::Reel: guide = L"{A} おしっぱなしで わくが うえへ。さかなを みどりの わくに いれよう"; break;
	case State::Miss: guide = message_; break;
	case State::Caught: break;
	}
	if (guide.empty()) return;
	const float w = ButtonGuide::Width(guide, FontSize::Small) + 48.0f;
	DrawUtil::Panel(640.0f - w * 0.5f, 650.0f, 640.0f + w * 0.5f, 700.0f, { 30, 26, 46 }, 210, state_ == State::Bite ? Game::Rgb{ 255, 120, 90 } : Game::Rgb{ 200, 180, 120 });
	ButtonGuide::DrawCenter(640.0f, 675.0f, guide, state_ == State::Bite ? GetColor(255, 200, 120) : GetColor(240, 235, 250), FontSize::Small, edge);
}
