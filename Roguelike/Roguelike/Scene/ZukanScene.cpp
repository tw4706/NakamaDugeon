#include "ZukanScene.h"
#include "SceneManager.h"
#include "CampScene.h"
#include "SaveData.h"
#include "MonsterData.h"
#include "Unit.h"
#include "Input.h"
#include "Sound.h"
#include "DrawUtil.h"
#include "Font.h"
#include "ButtonGuide.h"
#include<DxLib.h>
#include<cmath>
#include<string>
#include<algorithm>

namespace
{
	constexpr int kCols = 5;
	constexpr int kPageSize = 20;	//1ページの数(5×4)
	constexpr float kCellW = 132.0f;
	constexpr float kCellH = 120.0f;
	constexpr float kGridX = 40.0f;
	constexpr float kGridY = 130.0f;
}

ZukanScene::ZukanScene(SceneManager& manager) :
	Scene(manager)
{
}

void ZukanScene::Init()
{
}

void ZukanScene::Update()
{
	time_ += Game::kFrameTime;
	if (isLeaving_) return;

	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = static_cast<int>(MonsterTable::All().size());

	//左右はページをまたいで、上下はページの中で動く
	int next = cursor_;
	if (input.IsAnyTriggered(Button::Left)) next--;
	if (input.IsAnyTriggered(Button::Right)) next++;
	const int slot = cursor_ % kPageSize;
	if (input.IsAnyTriggered(Button::Up) && slot >= kCols) next -= kCols;
	if (input.IsAnyTriggered(Button::Down) && slot < kPageSize - kCols) next += kCols;

	//LB・RB で ページを まとめて めくる(おなじ いちへ)
	const int pageNum = (num + kPageSize - 1) / kPageSize;
	const int pageDir = (input.IsAnyTriggered(Button::PageR) ? 1 : 0) - (input.IsAnyTriggered(Button::PageL) ? 1 : 0);
	if (pageDir != 0 && pageNum > 1)
	{
		const int page = (cursor_ / kPageSize + pageNum + pageDir) % pageNum;
		next = std::min(num - 1, page * kPageSize + slot);
	}
	if (next != cursor_ && next >= 0 && next < num)
	{
		cursor_ = next;
		sound.PlaySE(SE::Cursor);
	}

	//色違いのページはクリア後
	if (input.IsAnyTriggered(Button::Decide))
	{
		if (SaveManager::GetInstance().Current().isCleared)
		{
			isShinyPage_ = !isShinyPage_;
			sound.PlaySE(SE::Decide);

			//色違いのページを見たので「！」を消す
			SaveSlot& s = SaveManager::GetInstance().Current();
			if (isShinyPage_ && s.IsNew(NewsBit::kShinyZukan))
			{
				s.seenNews |= NewsBit::kShinyZukan;
				SaveManager::GetInstance().Save();
			}
		}
		else
		{
			sound.PlaySE(SE::Buzzer);
		}
	}

	if (input.IsAnyTriggered(Button::Cancel))
	{
		isLeaving_ = true;
		sound.PlaySE(SE::Cancel);
		sceneManager_.ChangeSceneWithFade(std::make_shared<CampScene>(sceneManager_));
	}
}

void ZukanScene::Draw()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const auto& book = isShinyPage_ ? s.zukanShiny : s.zukan;
	const auto& all = MonsterTable::All();
	const int fontS = Font::Get(FontSize::Small);
	const unsigned int edge = GetColor(20, 15, 30);

	if (isShinyPage_) DrawUtil::Gradient(0, 0, Game::kScreenWidth, Game::kScreenHeight, { 70, 50, 90 }, { 25, 15, 40 });
	else DrawUtil::Gradient(0, 0, Game::kScreenWidth, Game::kScreenHeight, { 70, 55, 40 }, { 30, 22, 18 });

	//登録数(いろちがいの ぬしは いない・仲間にならないものは数えない)
	int seen = 0;
	int recruited = 0;
	int seenTotal = 0;
	int recruitTotal = 0;
	for (size_t i = 0; i < all.size(); i++)
	{
		if (isShinyPage_ && all[i].type == MonsterType::Boss) continue;
		const ZukanEntry z = i < book.size() ? book[i] : ZukanEntry();
		seenTotal++;
		if (z.isSeen) seen++;
		if (all[i].recruitRate > 0)
		{
			recruitTotal++;
			if (z.isRecruited) recruited++;
		}
	}
	const std::wstring title = isShinyPage_ ? L"モンスターずかん（いろちがい）" : L"モンスターずかん";
	Font::DrawCenterFit(370.0f, 50.0f, title, GetColor(255, 230, 140), FontSize::Medium, edge, 640.0f);
	const std::wstring count = L"みつけた " + std::to_wstring(seen) + L"/" + std::to_wstring(seenTotal) +
		L"　なかまにした " + std::to_wstring(recruited) + L"/" + std::to_wstring(recruitTotal);
	Font::DrawCenterFit(370.0f, 95.0f, count, GetColor(230, 225, 245), FontSize::Small, edge, 640.0f);

	//一覧(1ページ20体)
	const int page = cursor_ / kPageSize;
	const int pageNum = (static_cast<int>(all.size()) + kPageSize - 1) / kPageSize;
	const int pageEnd = std::min(static_cast<int>(all.size()), (page + 1) * kPageSize);
	for (int i = page * kPageSize; i < pageEnd; i++)
	{
		const int slot = i - page * kPageSize;
		const float x = kGridX + (slot % kCols) * kCellW;
		const float y = kGridY + (slot / kCols) * kCellH;
		const bool isSel = i == cursor_;
		const ZukanEntry z = i < static_cast<int>(book.size()) ? book[i] : ZukanEntry();
		DrawUtil::Panel(x, y, x + kCellW - 10.0f, y + kCellH - 10.0f, isSel ? Game::Rgb{ 80, 70, 120 } : Game::Rgb{ 30, 26, 46 }, 230,
			isSel ? Game::Rgb{ 255, 220, 120 } : Game::Rgb{ 90, 80, 110 });

		const float cx = x + (kCellW - 10.0f) * 0.5f;

		//ぬしには いろちがいが いない
		if (isShinyPage_ && all[i].type == MonsterType::Boss)
		{
			Font::DrawFit(x + 8.0f, y + 4.0f, L"No." + std::to_wstring(i + 1), GetColor(110, 100, 130), FontSize::Small, edge, kCellW - 50.0f);
			Font::DrawCenter(cx, y + 64.0f, L"ー", GetColor(110, 100, 130), FontSize::Medium, edge);
			continue;
		}

		MonsterData m = isShinyPage_ ? MonsterTable::ShinyVariant(i) : all[i];
		if (!z.isSeen) m.color = { 25, 22, 35 };
		const float size = m.type == MonsterType::Boss ? 34.0f : 28.0f;
		//まだ みていない ものは まっくろな かげに
		if (!z.isSeen) SetDrawBright(40, 36, 55);
		UnitDraw::Monster({ cx, y + 82.0f }, size, m, z.isSeen ? time_ : 0.0f, 1.0f, false);
		SetDrawBright(255, 255, 255);

		const std::wstring no = L"No." + std::to_wstring(i + 1);
		Font::DrawFit(x + 8.0f, y + 4.0f, no, GetColor(170, 160, 190), FontSize::Small, edge, kCellW - 50.0f);
		if (z.isRecruited) DrawUtil::Star(x + kCellW - 28.0f, y + kCellH - 30.0f, 10.0f, 0.0f, GetColor(255, 200, 80));
		else if (z.isDefeated) DrawCircleAA(x + kCellW - 28.0f, y + kCellH - 30.0f, 6.0f, 12, GetColor(200, 200, 220), TRUE);
	}

	//ページ
	if (pageNum > 1)
	{
		const std::wstring pageText = std::to_wstring(page + 1) + L" / " + std::to_wstring(pageNum);
		Font::DrawCenter(370.0f, 640.0f, pageText.c_str(), GetColor(230, 225, 245), FontSize::Small, edge);
		const float bob = std::sin(time_ * 6.0f) * 3.0f;
		const unsigned int arrow = GetColor(255, 230, 120);
		if (page > 0) DrawTriangleAA(300.0f - bob, 640.0f, 316.0f - bob, 630.0f, 316.0f - bob, 650.0f, arrow, TRUE);
		if (page < pageNum - 1) DrawTriangleAA(440.0f + bob, 640.0f, 424.0f + bob, 630.0f, 424.0f + bob, 650.0f, arrow, TRUE);
		ButtonGuide::DrawCenter(250.0f, 640.0f, L"{L}", GetColor(230, 225, 245), FontSize::Small, edge);
		ButtonGuide::DrawCenter(490.0f, 640.0f, L"{R}", GetColor(230, 225, 245), FontSize::Small, edge);
	}

	//詳しい説明
	const int id = cursor_;
	const ZukanEntry z = id < static_cast<int>(book.size()) ? book[id] : ZukanEntry();
	const MonsterData m = isShinyPage_ ? MonsterTable::ShinyVariant(id) : all[id];
	DrawUtil::Window(720.0f, 30.0f, 1250.0f, 690.0f);

	if (isShinyPage_ && all[id].type == MonsterType::Boss)
	{
		Font::DrawCenter(985.0f, 300.0f, (L"No." + std::to_wstring(id + 1)).c_str(), GetColor(150, 140, 170), FontSize::Medium, edge);
		Font::DrawCenter(985.0f, 360.0f, L"ぬしには いろちがいが いない", GetColor(180, 170, 200), FontSize::Small, edge);
		ButtonGuide::DrawCenter(985.0f, 650.0f, L"{A}：いろちがい　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
		return;
	}

	SetDrawBlendMode(DX_BLENDMODE_ADD, 60);
	DrawCircleAA(985.0f, 190.0f, 110.0f, 40, z.isSeen ? Game::ToColor(m.color) : GetColor(40, 40, 60), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	MonsterData drawData = m;
	if (!z.isSeen) drawData.color = { 25, 22, 35 };
	if (!z.isSeen) SetDrawBright(40, 36, 55);
	UnitDraw::Monster({ 985.0f, 270.0f }, m.type == MonsterType::Boss ? 90.0f : 72.0f, drawData, time_, 1.0f, false);
	SetDrawBright(255, 255, 255);

	const std::wstring name = L"No." + std::to_wstring(id + 1) + L"　" + (z.isSeen ? (isShinyPage_ ? L"いろちがいの " : L"") + m.name : std::wstring(L"？？？"));
	Font::DrawCenterFit(985.0f, 330.0f, name, GetColor(255, 240, 160), FontSize::Small, edge, 490.0f);

	if (z.isSeen)
	{
		const std::wstring stat = L"HP " + std::to_wstring(m.hp) + L"　こうげき " + std::to_wstring(m.atk) + L"　ぼうぎょ " + std::to_wstring(m.def);
		Font::DrawFit(750.0f, 370.0f, stat, GetColor(220, 225, 255), FontSize::Small, edge, 480.0f);
		Font::DrawFit(750.0f, 405.0f, std::wstring(L"とくぎ：") + (m.skill == Skill::None ? L"なし" : MonsterTable::SkillName(m.skill)), GetColor(220, 225, 255), FontSize::Small, edge, 480.0f);
		//でるところ(はじまりの どうくつ と むげんの めいきゅう)
		std::wstring where = L"でるところ：";
		if (m.type == MonsterType::Boss)
		{
			if (m.minFloor > 0) where += L"B" + std::to_wstring(m.minFloor) + L"F の おく";
			else where += L"むげん B" + std::to_wstring(m.infMinFloor) + L"F の ぬし";
		}
		else
		{
			if (m.minFloor > 0) where += L"B" + std::to_wstring(m.minFloor) + L"〜B" + std::to_wstring(m.maxFloor) + L"F";
			if (m.minFloor > 0 && m.infMinFloor > 0) where += L"／";
			if (m.infMinFloor > 0) where += L"むげん B" + std::to_wstring(m.infMinFloor) + L"〜B" + std::to_wstring(m.infMaxFloor) + L"F";
		}
		Font::DrawFit(750.0f, 440.0f, where, GetColor(220, 225, 255), FontSize::Small, edge, 480.0f);

		//説明文(窓の幅で、なるべく言葉の切れ目で折り返す)
		const auto lines = Font::Wrap(m.desc, FontSize::Small, 470.0f);
		for (size_t k = 0; k < lines.size() && k < 3; k++)
		{
			Font::DrawFit(750.0f, 482.0f + k * 30.0f, lines[k], GetColor(240, 235, 220), FontSize::Small, edge, 470.0f);
		}
	}
	else
	{
		DrawStringToHandle(750, 400, L"まだ みつけて いない", GetColor(150, 140, 170), fontS, edge);
	}

	const std::wstring flags = std::wstring(z.isSeen ? L"● みた　" : L"○ みた　") + (z.isDefeated ? L"● たおした　" : L"○ たおした　") + (z.isRecruited ? L"★ なかま" : L"☆ なかま");
	Font::DrawFit(750.0f, 580.0f, flags, GetColor(255, 220, 140), FontSize::Small, edge, 480.0f);

	const wchar_t* help = s.isCleared ? L"{A}：いろちがい　{B}：もどる" : L"{B}：もどる";
	ButtonGuide::DrawCenter(985.0f, 650.0f, help, GetColor(160, 150, 180), FontSize::Small, edge);

	//まだ見ていない色違いのページ
	if (s.IsNew(NewsBit::kShinyZukan))
	{
		DrawUtil::NewMark(985.0f - ButtonGuide::Width(help, FontSize::Small) * 0.5f - 24.0f, 650.0f, time_, 13.0f);
	}
}
