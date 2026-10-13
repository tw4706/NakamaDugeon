#include "KeyItemMenu.h"
#include "SaveData.h"
#include "MonsterData.h"
#include "Input.h"
#include "Sound.h"
#include "DrawUtil.h"
#include "ButtonGuide.h"
#include "Font.h"
#include<DxLib.h>
#include<cmath>

bool KeyItemMenu::Update()
{
	time_ += Game::kFrameTime;
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = KeyItem::Count();
	if (input.IsAnyTriggered(Button::Up)) { cursor_ = (cursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { cursor_ = (cursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Decide))
	{
		sound.PlaySE(SE::Cancel);
		return true;
	}
	return false;
}

void KeyItemMenu::Draw() const
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const unsigned int edge = GetColor(20, 15, 30);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(10, 8, 20), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const int num = KeyItem::Count();
	const std::wstring count = L"だいじな もの　" + std::to_wstring(KeyItem::OwnedCount(s)) + L" / " + std::to_wstring(num);
	Font::DrawCenter(640.0f, 50.0f, count.c_str(), GetColor(255, 230, 140), FontSize::Medium, edge);

	//いちらん
	DrawUtil::Window(60.0f, 95.0f, 560.0f, 600.0f);
	for (int i = 0; i < num; i++)
	{
		const float y = 140.0f + i * 72.0f;
		const bool isSel = i == cursor_;
		const bool isOwned = KeyItem::IsOwned(s, i);
		if (isSel)
		{
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
			DrawBoxAA(76.0f, y - 30.0f, 544.0f, y + 30.0f, GetColor(255, 220, 120), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}

		//かざりの たま(もっていれば ひかる)
		const float gx = 112.0f;
		if (isOwned)
		{
			SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(90 + 50 * std::sin(time_ * 3.0f + i)));
			DrawCircleAA(gx, y, 24.0f, 20, GetColor(255, 190, 80), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			DrawUtil::Star(gx, y, 16.0f, time_ * 0.8f + i, GetColor(255, 220, 90));
		}
		else
		{
			DrawCircleAA(gx, y, 16.0f, 20, GetColor(60, 55, 80), FALSE, 2.0f);
		}
		Font::DrawFit(150.0f, y - 13.0f, isOwned ? KeyItem::Get(i).name : L"？？？", isOwned ? (isSel ? GetColor(255, 240, 150) : GetColor(240, 235, 255)) : GetColor(120, 110, 140), FontSize::Small, edge, 380.0f);
	}

	//くわしく
	DrawUtil::Window(580.0f, 95.0f, 1220.0f, 600.0f);
	const KeyItem::Def& d = KeyItem::Get(cursor_);
	if (KeyItem::IsOwned(s, cursor_))
	{
		Font::DrawCenterFit(900.0f, 145.0f, d.name, GetColor(255, 230, 140), FontSize::Medium, edge, 600.0f);
		const auto lines = Font::Wrap(d.text, FontSize::Small, 580.0f);
		for (size_t k = 0; k < lines.size() && k < 2; k++)
		{
			Font::DrawFit(610.0f, 200.0f + k * 32.0f, lines[k], GetColor(220, 215, 240), FontSize::Small, edge, 580.0f);
		}
		Font::DrawFit(610.0f, 290.0f, L"もっているだけで ずっと はつどう", GetColor(255, 220, 140), FontSize::Small, edge, 580.0f);
		float y = 330.0f;
		for (const std::wstring& line : Font::Wrap(KeyItem::EffectText(cursor_), FontSize::Small, 560.0f))
		{
			if (y > 520.0f) break;
			Font::DrawFit(630.0f, y, line, GetColor(200, 240, 210), FontSize::Small, edge, 560.0f);
			y += 32.0f;
		}
		const std::wstring from = L"てにいれた ばしょ：" + MonsterTable::Get(d.bossId).name + L"を たおした";
		Font::DrawFit(610.0f, 540.0f, from, GetColor(170, 160, 200), FontSize::Small, edge, 580.0f);
	}
	else
	{
		Font::DrawCenter(900.0f, 300.0f, L"まだ てにいれて いない", GetColor(150, 140, 170), FontSize::Small, edge);
		Font::DrawCenter(900.0f, 345.0f, L"どこかの ぬしが もっている らしい…", GetColor(130, 120, 160), FontSize::Small, edge);
	}

	ButtonGuide::DrawCenter(640.0f, 650.0f, L"{UD}：えらぶ　{B}：とじる", GetColor(170, 160, 200), FontSize::Small, edge);
}
