#include "FishZukanScene.h"
#include "SceneManager.h"
#include "CampScene.h"
#include "SaveData.h"
#include "FishData.h"
#include "Input.h"
#include "Sound.h"
#include "DrawUtil.h"
#include "Font.h"
#include "ButtonGuide.h"
#include<DxLib.h>
#include<cmath>
#include<string>

namespace
{
	constexpr int kCols = 5;
	constexpr float kCellW = 132.0f;
	constexpr float kCellH = 150.0f;
	constexpr float kGridX = 40.0f;
	constexpr float kGridY = 130.0f;

	//しょこの まえ
	const Cell kReturnCell = { 4, 7 };

	std::wstring Stars(int rarity)
	{
		std::wstring s;
		for (int i = 0; i < 5; i++) s += i < rarity ? L"★" : L"☆";
		return s;
	}
}

FishZukanScene::FishZukanScene(SceneManager& manager) :
	Scene(manager)
{
}

void FishZukanScene::Init()
{
	SaveManager::GetInstance().Current().fish.resize(FishTable::All().size());
}

void FishZukanScene::Update()
{
	time_ += Game::kFrameTime;
	if (isLeaving_) return;

	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = static_cast<int>(FishTable::All().size());

	int next = cursor_;
	if (input.IsAnyTriggered(Button::Left)) next--;
	if (input.IsAnyTriggered(Button::Right)) next++;
	if (input.IsAnyTriggered(Button::Up)) next -= kCols;
	if (input.IsAnyTriggered(Button::Down)) next += kCols;
	if (next != cursor_ && next >= 0 && next < num)
	{
		cursor_ = next;
		sound.PlaySE(SE::Cursor);
	}

	if (input.IsAnyTriggered(Button::Cancel))
	{
		isLeaving_ = true;
		sound.PlaySE(SE::Cancel);
		sceneManager_.ChangeSceneWithFade(std::make_shared<CampScene>(sceneManager_, kReturnCell));
	}
}

void FishZukanScene::Draw()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const auto& all = FishTable::All();
	const unsigned int edge = GetColor(20, 15, 30);
	auto record = [&](int i) { return i < static_cast<int>(s.fish.size()) ? s.fish[i] : FishRecord(); };

	DrawUtil::Gradient(0, 0, Game::kScreenWidth, Game::kScreenHeight, { 40, 80, 110 }, { 15, 30, 50 });

	//とうろくした かず(クリア前は ひみつの さかなを かぞえない)
	int caught = 0;
	int total = 0;
	int count = 0;
	for (int i = 0; i < static_cast<int>(all.size()); i++)
	{
		const FishRecord rec = record(i);
		count += rec.count;
		if (rec.count > 0) caught++;
		if (FishTable::IsAvailable(i, s.isCleared) || rec.count > 0) total++;
	}
	Font::DrawCenterFit(370.0f, 50.0f, L"つりずかん", GetColor(180, 230, 255), FontSize::Medium, edge, 640.0f);
	const std::wstring head = L"つった しゅるい " + std::to_wstring(caught) + L"/" + std::to_wstring(total) + L"　つった かず " + std::to_wstring(count) + L"ひき";
	Font::DrawCenterFit(370.0f, 95.0f, head, GetColor(230, 225, 245), FontSize::Small, edge, 640.0f);

	//いちらん
	for (int i = 0; i < static_cast<int>(all.size()); i++)
	{
		const float x = kGridX + (i % kCols) * kCellW;
		const float y = kGridY + (i / kCols) * kCellH;
		const bool isSel = i == cursor_;
		const FishRecord rec = record(i);
		const bool isCaught = rec.count > 0;
		DrawUtil::Panel(x, y, x + kCellW - 10.0f, y + kCellH - 10.0f, isSel ? Game::Rgb{ 60, 100, 140 } : Game::Rgb{ 22, 36, 56 }, 230,
			isSel ? Game::Rgb{ 255, 220, 120 } : Game::Rgb{ 80, 110, 140 });

		const float cx = x + (kCellW - 10.0f) * 0.5f;
		Font::DrawFit(x + 8.0f, y + 4.0f, L"No." + std::to_wstring(i + 1), GetColor(170, 190, 210), FontSize::Small, edge, kCellW - 30.0f);

		//クリア前の ひみつの さかなは「？」
		if (!isCaught && !FishTable::IsAvailable(i, s.isCleared))
		{
			Font::DrawCenter(cx, y + 76.0f, L"？", GetColor(110, 120, 140), FontSize::Medium, edge);
			continue;
		}
		FishDraw::Fish({ cx, y + 76.0f }, 84.0f, all[i], isCaught ? time_ + i : 0.0f, 1.0f, !isCaught);
		if (isCaught) Font::DrawCenter(cx, y + 116.0f, Stars(all[i].rarity).c_str(), GetColor(255, 210, 80), FontSize::Small, edge);
	}

	//くわしい せつめい
	const FishData& d = all[cursor_];
	const FishRecord rec = record(cursor_);
	const bool isCaught = rec.count > 0;
	const bool isSecret = !isCaught && !FishTable::IsAvailable(cursor_, s.isCleared);
	DrawUtil::Window(720.0f, 30.0f, 1250.0f, 690.0f);

	if (isSecret)
	{
		Font::DrawCenter(985.0f, 300.0f, (L"No." + std::to_wstring(cursor_ + 1)).c_str(), GetColor(150, 160, 180), FontSize::Medium, edge);
		Font::DrawCenter(985.0f, 360.0f, L"ゆうしゃが もっと つよく なると…？", GetColor(180, 190, 210), FontSize::Small, edge);
		ButtonGuide::DrawCenter(985.0f, 650.0f, L"{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
		return;
	}

	//みずの まる と さかな
	SetDrawBlendMode(DX_BLENDMODE_ADD, 60);
	DrawCircleAA(985.0f, 190.0f, 120.0f, 40, isCaught ? Game::ToColor(d.color) : GetColor(40, 50, 70), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	for (int k = 0; k < 3; k++)
	{
		const float bx = 900.0f + k * 70.0f;
		const float by = 260.0f - std::fmod(time_ * 30.0f + k * 40.0f, 160.0f);
		DrawCircleAA(bx, by, 5.0f, 10, GetColor(170, 220, 250), FALSE, 1.5f);
	}
	FishDraw::Fish({ 985.0f, 190.0f }, 260.0f, d, isCaught ? time_ : 0.0f, 1.0f, !isCaught);

	const std::wstring name = L"No." + std::to_wstring(cursor_ + 1) + L"　" + (isCaught ? std::wstring(d.name) : std::wstring(L"？？？"));
	Font::DrawCenterFit(985.0f, 330.0f, name, GetColor(255, 240, 160), FontSize::Small, edge, 490.0f);

	if (isCaught)
	{
		Font::DrawFit(750.0f, 370.0f, std::wstring(L"めずらしさ　") + Stars(d.rarity), GetColor(255, 210, 80), FontSize::Small, edge, 480.0f);
		Font::DrawFit(750.0f, 405.0f, L"おおきさ　" + std::to_wstring(d.minSize) + L"〜" + std::to_wstring(d.maxSize) + L"cm",
			GetColor(220, 225, 255), FontSize::Small, edge, 480.0f);
		Font::DrawFit(750.0f, 440.0f, L"さいこう　" + std::to_wstring(rec.best) + L"cm　　つった かず　" + std::to_wstring(rec.count) + L"ひき",
			GetColor(220, 225, 255), FontSize::Small, edge, 480.0f);

		const auto lines = Font::Wrap(d.desc, FontSize::Small, 470.0f);
		for (size_t k = 0; k < lines.size() && k < 4; k++)
		{
			Font::DrawFit(750.0f, 482.0f + k * 30.0f, lines[k], GetColor(240, 235, 220), FontSize::Small, edge, 470.0f);
		}
	}
	else
	{
		Font::DrawFit(750.0f, 370.0f, std::wstring(L"めずらしさ　") + Stars(d.rarity), GetColor(150, 140, 170), FontSize::Small, edge, 480.0f);
		Font::DrawFit(750.0f, 420.0f, L"まだ つって いない（むらの いけで つれる）", GetColor(150, 160, 180), FontSize::Small, edge, 480.0f);
	}

	ButtonGuide::DrawCenter(985.0f, 650.0f, L"{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}
