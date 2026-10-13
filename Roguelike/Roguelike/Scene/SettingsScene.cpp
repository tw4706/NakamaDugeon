#include "SettingsScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "CampScene.h"
#include "SaveData.h"
#include "Input.h"
#include "Sound.h"
#include "DrawUtil.h"
#include "ButtonGuide.h"
#include "Font.h"
#include<DxLib.h>
#include<algorithm>
#include<string>

SettingsScene::SettingsScene(SceneManager& manager, Return returnTo) :
	Scene(manager),
	returnTo_(returnTo)
{
}

void SettingsScene::Init()
{
}

void SettingsScene::Leave()
{
	if (isLeaving_) return;
	isLeaving_ = true;
	SaveManager::GetInstance().SaveSettings();
	if (returnTo_ == Return::Camp && SaveManager::GetInstance().CurrentIndex() >= 0)
	{
		sceneManager_.ChangeSceneWithFade(std::make_shared<CampScene>(sceneManager_));
	}
	else
	{
		sceneManager_.ChangeSceneWithFade(std::make_shared<TitleScene>(sceneManager_));
	}
}

void SettingsScene::Update()
{
	time_ += Game::kFrameTime;
	if (isLeaving_) return;

	Input& input = Input::GetInstance();
	SaveManager& sm = SaveManager::GetInstance();
	Sound& sound = Sound::GetInstance();

	const bool up = input.IsAnyTriggered(Button::Up);
	const bool down = input.IsAnyTriggered(Button::Down);
	const bool left = input.IsAnyTriggered(Button::Left);
	const bool right = input.IsAnyTriggered(Button::Right);
	const bool decide = input.IsAnyTriggered(Button::Decide);
	const bool cancel = input.IsAnyTriggered(Button::Cancel);

	switch (mode_)
	{
	case Mode::Main:
	{
		const SettingsMenu::Result result = menu_.Update();
		if (result == SettingsMenu::Result::Close) Leave();
		if (result == SettingsMenu::Result::Delete) { mode_ = Mode::DeleteSelect; deleteSlot_ = 0; }
		break;
	}
	case Mode::DeleteSelect:
		if (up) { deleteSlot_ = (deleteSlot_ + SaveManager::kSlotNum - 1) % SaveManager::kSlotNum; sound.PlaySE(SE::Cursor); }
		if (down) { deleteSlot_ = (deleteSlot_ + 1) % SaveManager::kSlotNum; sound.PlaySE(SE::Cursor); }
		if (cancel) { mode_ = Mode::Main; sound.PlaySE(SE::Cancel); }
		if (decide)
		{
			if (!sm.Slot(deleteSlot_).exists) { sound.PlaySE(SE::Buzzer); break; }
			mode_ = Mode::DeleteConfirm1;
			confirmCursor_ = 1;
			sound.PlaySE(SE::Decide);
		}
		break;

	case Mode::DeleteConfirm1:
	case Mode::DeleteConfirm2:
		if (left || right || up || down) { confirmCursor_ = 1 - confirmCursor_; sound.PlaySE(SE::Cursor); }
		if (cancel || (decide && confirmCursor_ == 1)) { mode_ = Mode::DeleteSelect; sound.PlaySE(SE::Cancel); break; }
		if (decide && confirmCursor_ == 0)
		{
			if (mode_ == Mode::DeleteConfirm1)
			{
				mode_ = Mode::DeleteConfirm2;
				confirmCursor_ = 1;
				sound.PlaySE(SE::Decide);
			}
			else
			{
				//遊んでいる記録を消したらタイトルへ戻す
				const bool isCurrent = sm.CurrentIndex() == deleteSlot_;
				sm.Delete(deleteSlot_);
				sound.PlaySE(SE::Explosion);
				menu_.Notice(L"きろく " + std::to_wstring(deleteSlot_ + 1) + L" を けしました");
				mode_ = Mode::Main;
				if (isCurrent) returnTo_ = Return::Title;
			}
		}
		break;
	}
}

void SettingsScene::Draw()
{
	DrawUtil::Gradient(0, 0, Game::kScreenWidth, Game::kScreenHeight, { 35, 30, 55 }, { 15, 12, 25 });
	menu_.Draw();

	//記録の削除
	if (mode_ == Mode::DeleteSelect || mode_ == Mode::DeleteConfirm1 || mode_ == Mode::DeleteConfirm2)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
		DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		const SaveManager& sm = SaveManager::GetInstance();
		DrawUtil::Window(340.0f, 160.0f, 940.0f, 560.0f);
		Font::DrawCenter(640.0f, 200.0f, L"どの きろくを けしますか？", GetColor(255, 140, 140), FontSize::Medium, GetColor(20, 15, 30));
		for (int i = 0; i < SaveManager::kSlotNum; i++)
		{
			const float y = 270.0f + i * 70.0f;
			const bool isSel = i == deleteSlot_;
			const std::wstring text = L"きろく " + std::to_wstring(i + 1) + (sm.Slot(i).exists ? L"" : L"（からっぽ）");
			if (isSel && mode_ == Mode::DeleteSelect) DrawUtil::CursorBeside(640.0f, y, text.c_str(), FontSize::Medium, time_);
			Font::DrawCenter(640.0f, y, text.c_str(), isSel ? GetColor(255, 240, 150) : GetColor(190, 180, 210), FontSize::Medium, GetColor(20, 15, 30));
		}
		ButtonGuide::DrawCenter(640.0f, 530.0f, L"{A}：えらぶ　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, GetColor(20, 15, 30));

		if (mode_ != Mode::DeleteSelect)
		{
			DrawUtil::Window(380.0f, 300.0f, 900.0f, 500.0f);
			const wchar_t* q = mode_ == Mode::DeleteConfirm1 ? L"ほんとうに けしますか？" : L"もとに もどせません。けしますか？";
			Font::DrawCenter(640.0f, 360.0f, q, GetColor(255, 200, 200), FontSize::Medium, GetColor(20, 15, 30));
			const wchar_t* choices[2] = { L"けす", L"やめる" };
			for (int i = 0; i < 2; i++)
			{
				const float bx = 540.0f + i * 200.0f;
				const bool isSel = i == confirmCursor_;
				DrawUtil::Panel(bx - 70.0f, 410.0f, bx + 70.0f, 462.0f, isSel ? Game::Rgb{ 190, 60, 70 } : Game::Rgb{ 50, 45, 70 }, 240,
					isSel ? Game::Rgb{ 255, 230, 120 } : Game::Rgb{ 120, 110, 150 });
				Font::DrawCenter(bx, 436.0f, choices[i], GetColor(255, 255, 255), FontSize::Medium, GetColor(20, 15, 30));
			}
		}
	}
}
