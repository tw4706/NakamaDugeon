#pragma once
#include "Scene.h"
#include "MathUtil.h"
#include<string>
#include<vector>

/// <summary>
/// むらの いけで つり
/// </summary>
class FishingScene : public Scene
{
public:
	FishingScene(SceneManager& manager);

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	enum class State
	{
		Ready,		//なげる まえ
		Cast,		//うきが とんでいる
		Wait,		//あたりを まつ
		Bite,		//あたり！(すぐに {A})
		Reel,		//まきあげ
		Caught,		//つれた
		Miss,		//にげられた・はやすぎた
	};

	void UpdateReady();
	void UpdateCast();
	void UpdateWait();
	void UpdateBite();
	void UpdateReel();
	void UpdateResult();
	void StartReel();
	void Land();
	void Miss(const std::wstring& message);

	/// <summary>
	/// だれかが {A} を おしているか(まきあげ)
	/// </summary>
	bool IsHolding() const;

	void DrawBackground();
	void DrawHeroAndLine();
	void DrawBobber();
	void DrawReelGauge();
	void DrawCaught();
	void DrawHud();

	Vec2 RodTip() const;

	State state_ = State::Ready;
	float time_ = 0.0f;
	float stateTime_ = 0.0f;
	bool isLeaving_ = false;

	//うき
	Vec2 castFrom_;
	Vec2 bobber_;
	float bobberDip_ = 0.0f;	//しずみぐあい(0〜1)
	float waitTimer_ = 0.0f;
	float nibbleTimer_ = 0.0f;
	float biteTimer_ = 0.0f;

	//かかった さかな
	int fish_ = 0;
	int size_ = 0;

	//まきあげ(下から の たかさ)
	float zoneY_ = 0.0f;
	float zoneVel_ = 0.0f;
	float fishY_ = 0.0f;
	float fishTarget_ = 0.0f;
	float fishMoveTimer_ = 0.0f;
	float progress_ = 0.0f;
	float reelTick_ = 0.0f;

	//けっか
	bool isNewFish_ = false;
	bool isRecord_ = false;
	bool isStockFull_ = false;		//もちきれなかった
	std::vector<int> gotRewards_;	//つりずかんの ほうびで もらった だいじな もの
	std::wstring message_;
	int todayCount_ = 0;
};
