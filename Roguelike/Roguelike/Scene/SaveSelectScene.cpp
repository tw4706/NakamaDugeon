#include "SaveSelectScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "CampScene.h"
#include "SaveData.h"
#include "MonsterData.h"
#include "Input.h"
#include "Sound.h"
#include "DrawUtil.h"
#include "Font.h"
#include "ButtonGuide.h"
#include<DxLib.h>
#include<ctime>
#include<string>

namespace
{
	std::wstring PlayTime(int seconds)
	{
		const int h = seconds / 3600;
		const int m = seconds / 60 % 60;
		wchar_t buf[32];
		swprintf_s(buf, L"%d:%02d", h, m);
		return buf;
	}

	std::wstring DateText(long long t)
	{
		if (t <= 0) return L"";
		std::tm tm = {};
		const std::time_t tt = static_cast<std::time_t>(t);
		localtime_s(&tm, &tt);
		wchar_t buf[48];
		swprintf_s(buf, L"%d/%02d/%02d %02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
		return buf;
	}
}

SaveSelectScene::SaveSelectScene(SceneManager& manager) :
	Scene(manager)
{
}

void SaveSelectScene::Init()
{
	Sound::GetInstance().PlayBGM(BGM::Title);
}

void SaveSelectScene::Update()
{
	time_ += Game::kFrameTime;
	if (isDecided_) return;

	Input& input = Input::GetInstance();
	constexpr int kNum = SaveManager::kSlotNum;
	if (input.IsAnyTriggered(Button::Up)) { cursor_ = (cursor_ + kNum - 1) % kNum; Sound::GetInstance().PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { cursor_ = (cursor_ + 1) % kNum; Sound::GetInstance().PlaySE(SE::Cursor); }

	if (input.IsAnyTriggered(Button::Cancel))
	{
		isDecided_ = true;
		Sound::GetInstance().PlaySE(SE::Cancel);
		sceneManager_.ChangeSceneWithFade(std::make_shared<TitleScene>(sceneManager_));
		return;
	}
	if (input.IsAnyTriggered(Button::Decide))
	{
		isDecided_ = true;
		Sound::GetInstance().PlaySE(SE::Decide);
		SaveManager::GetInstance().Select(cursor_);
		sceneManager_.ChangeSceneWithFade(std::make_shared<CampScene>(sceneManager_));
	}
}

void SaveSelectScene::Draw()
{
	DrawUtil::Gradient(0, 0, Game::kScreenWidth, Game::kScreenHeight, { 40, 30, 60 }, { 15, 10, 25 });
	Font::DrawCenter(640.0f, 70.0f, L"ぼうけんの きろくを えらんでください", GetColor(255, 230, 140), FontSize::Medium, GetColor(30, 20, 10));

	const SaveManager& sm = SaveManager::GetInstance();
	const int monsterNum = static_cast<int>(MonsterTable::All().size());
	const int fontS = Font::Get(FontSize::Small);

	for (int i = 0; i < SaveManager::kSlotNum; i++)
	{
		const SaveSlot& s = sm.Slot(i);
		const float y = 140.0f + i * 170.0f;
		const bool isSel = i == cursor_;
		DrawUtil::Panel(220.0f, y, 1060.0f, y + 150.0f, isSel ? Game::Rgb{ 50, 42, 80 } : Game::Rgb{ 28, 24, 44 }, 235,
			isSel ? Game::Rgb{ 255, 220, 120 } : Game::Rgb{ 110, 100, 140 });
		if (isSel) DrawUtil::Cursor(180.0f, y + 75.0f, time_);

		const std::wstring title = L"きろく " + std::to_wstring(i + 1);
		DrawStringToHandle(250, static_cast<int>(y + 14.0f), title.c_str(), GetColor(255, 230, 140), Font::Get(FontSize::Medium), GetColor(20, 15, 30));

		if (!s.exists)
		{
			Font::DrawCenter(640.0f, y + 90.0f, L"― あたらしく はじめる ―", GetColor(200, 190, 220), FontSize::Small, GetColor(20, 15, 30));
			continue;
		}

		//図鑑の登録数
		int seen = 0;
		for (const auto& z : s.zukan) if (z.isSeen) seen++;

		if (s.isCleared)
		{
			DrawUtil::Star(560.0f, y + 34.0f, 16.0f, time_ * 0.5f, GetColor(255, 220, 60));
			DrawStringToHandle(585, static_cast<int>(y + 20.0f), L"クリアずみ", GetColor(255, 220, 60), fontS, GetColor(20, 15, 30));
		}

		const std::wstring line1 = L"ゆうしゃ Lv" + std::to_wstring(s.run.isActive ? s.run.heroLevel : s.hero.level) + L"　とうたつ：B" + std::to_wstring(s.deepestNormal) + L"F" +
			(s.isCleared ? L"　むげん：B" + std::to_wstring(s.deepestInfinite) + L"F" : L"") +
			L"　ずかん：" + std::to_wstring(seen) + L"/" + std::to_wstring(monsterNum);
		Font::DrawFit(250.0f, y + 66.0f, line1, GetColor(230, 230, 250), FontSize::Small, GetColor(20, 15, 30), 780.0f);

		const std::wstring line2 = L"プレイじかん " + PlayTime(s.playSeconds) + L"　さいご：" + DateText(s.lastPlayed);
		Font::DrawFit(250.0f, y + 104.0f, line2, GetColor(180, 170, 200), FontSize::Small, GetColor(20, 15, 30), 780.0f);

		if (s.run.isActive)
		{
			const std::wstring run = L"ぼうけんちゅう B" + std::to_wstring(s.run.floor) + L"F";
			Font::DrawFit(780.0f, y + 20.0f, run, GetColor(140, 220, 255), FontSize::Small, GetColor(20, 15, 30), 260.0f);
		}
	}

	ButtonGuide::DrawCenter(640.0f, 680.0f, L"{A}：けってい　{B}：もどる　（きろくを けすときは せってい から）",
		GetColor(170, 160, 190), FontSize::Small, GetColor(10, 10, 20));
}
