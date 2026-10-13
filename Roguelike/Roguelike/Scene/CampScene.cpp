#include "CampScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "DungeonScene.h"
#include "ZukanScene.h"
#include "SmithScene.h"
#include "SettingsScene.h"
#include "SaveData.h"
#include "MonsterData.h"
#include "Stage.h"
#include "Unit.h"
#include "Input.h"
#include "Sound.h"
#include "Effects.h"
#include "DrawUtil.h"
#include "Font.h"
#include "ButtonGuide.h"
#include "UpdateLog.h"
#include<DxLib.h>
#include<cmath>
#include<algorithm>

using namespace MathUtil;

namespace
{
	constexpr float kTile = 48.0f;
	constexpr float kOriginX = 16.0f;
	constexpr float kWalkSpeed = 250.0f;
	constexpr float kHeroHalfW = 14.0f;		//当たり判定の半分の幅
	constexpr float kHeroHalfH = 10.0f;
	constexpr float kWanderSpeed = 90.0f;
	constexpr int kMaxWanderer = 8;

	struct RectCell
	{
		int x;
		int y;
		int w;
		int h;
	};

	//建物の場所
	constexpr RectCell kCave = { 11, 0, 4, 3 };
	constexpr RectCell kLibrary = { 2, 3, 5, 4 };
	constexpr RectCell kSmith = { 19, 3, 5, 4 };
	constexpr RectCell kTent = { 16, 9, 3, 3 };
	constexpr RectCell kPond = { 3, 10, 4, 3 };
	constexpr Cell kFire = { 13, 10 };
	constexpr Cell kBoard = { 22, 1 };		//お知らせのけいじばん

	//お知らせの表示
	constexpr int kNoticeLines = 11;
	constexpr float kNoticeLineH = 38.0f;
	constexpr float kNoticeWidth = 740.0f;

	const Cell kTrees[] =
	{
		{ 8, 1 }, { 9, 2 }, { 16, 1 }, { 17, 1 }, { 1, 8 }, { 9, 12 }, { 22, 11 }, { 23, 12 }, { 21, 13 }, { 8, 13 }, { 24, 9 },
	};
}

CampScene::CampScene(SceneManager& manager) :
	Scene(manager)
{
}

void CampScene::Init()
{
	BuildMap();
	heroCell_ = { 13, 5 };
	heroPos_ = CellToPos(heroCell_);
	Sound::GetInstance().PlayBGM(BGM::Village);

	//仲間にしたことのあるモンスターが歩き回る
	const SaveSlot& s = SaveManager::GetInstance().Current();
	std::vector<std::pair<int, bool>> list;
	for (size_t i = 0; i < s.zukan.size(); i++)
	{
		if (s.zukan[i].isRecruited) list.push_back({ static_cast<int>(i), false });
		if (i < s.zukanShiny.size() && s.zukanShiny[i].isRecruited) list.push_back({ static_cast<int>(i), true });
	}
	std::shuffle(list.begin(), list.end(), Rng());
	wanderers_.clear();
	for (const auto& [id, isShiny] : list)
	{
		if (static_cast<int>(wanderers_.size()) >= kMaxWanderer) break;
		Wanderer w;
		w.monsterId = id;
		w.isShiny = isShiny;
		for (int tries = 0; tries < 50; tries++)
		{
			w.cell = { RandI(7, 23), RandI(8, 13) };
			bool isUsed = IsBlocked(w.cell) || SpotAt(w.cell) != Spot::None;
			for (const auto& o : wanderers_) if (o.cell == w.cell) isUsed = true;
			if (!isUsed) break;
		}
		w.drawPos = CellToPos(w.cell);
		w.timer = RandF(0.3f, 1.5f);
		w.facing = RandI(0, 1) == 0 ? 1.0f : -1.0f;
		wanderers_.push_back(w);
	}

	//はじめてクリアして戻ったときは解放の案内
	if (s.IsNew(NewsBit::kClearBanner))
	{
		mode_ = Mode::Unlock;
		Sound::GetInstance().PlaySE(SE::Fanfare);
	}
}

void CampScene::UpdateUnlock()
{
	Input& input = Input::GetInstance();
	if (time_ < 0.8f) return;
	if (!input.IsAnyTriggered(Button::Decide) && !input.IsAnyTriggered(Button::Cancel)) return;

	SaveManager& sm = SaveManager::GetInstance();
	sm.Current().seenNews |= NewsBit::kClearBanner;
	sm.Save();
	mode_ = Mode::Walk;
	Sound::GetInstance().PlaySE(SE::Decide);
}

void CampScene::BuildMap()
{
	for (int y = 0; y < kRows; y++)
	{
		for (int x = 0; x < kCols; x++)
		{
			blocked_[y][x] = (x == 0 || y == 0 || x == kCols - 1 || y == kRows - 1);
			spots_[y][x] = Spot::None;
			path_[y][x] = false;
		}
	}

	auto block = [this](const RectCell& r)
	{
		for (int y = r.y; y < r.y + r.h; y++)
		{
			for (int x = r.x; x < r.x + r.w; x++) blocked_[y][x] = true;
		}
	};
	block(kCave);
	block(kLibrary);
	block(kSmith);
	block(kTent);
	block(kPond);
	blocked_[kFire.y][kFire.x] = true;
	blocked_[kBoard.y][kBoard.x] = true;
	for (const Cell& t : kTrees) blocked_[t.y][t.x] = true;

	//道
	for (int y = 3; y < kRows - 1; y++) { path_[y][12] = true; path_[y][13] = true; }
	for (int x = 3; x < 23; x++) path_[7][x] = true;

	//入口などの場所
	spots_[3][12] = Spot::Dungeon;
	spots_[3][13] = Spot::Dungeon;
	spots_[7][4] = Spot::Library;
	spots_[7][21] = Spot::Smith;
	spots_[kFire.y][kFire.x - 1] = Spot::Fire;
	spots_[kFire.y][kFire.x + 1] = Spot::Fire;
	spots_[kFire.y - 1][kFire.x] = Spot::Fire;
	spots_[kFire.y + 1][kFire.x] = Spot::Fire;
	for (int dx = -1; dx <= 1; dx++) spots_[kBoard.y + 1][kBoard.x + dx] = Spot::Notice;
}

bool CampScene::IsBlocked(const Cell& c) const
{
	if (c.x < 0 || c.y < 0 || c.x >= kCols || c.y >= kRows) return true;
	return blocked_[c.y][c.x];
}

CampScene::Spot CampScene::SpotAt(const Cell& c) const
{
	if (c.x < 0 || c.y < 0 || c.x >= kCols || c.y >= kRows) return Spot::None;
	return spots_[c.y][c.x];
}

Vec2 CampScene::CellToPos(const Cell& c) const
{
	return { kOriginX + (c.x + 0.5f) * kTile, (c.y + 0.5f) * kTile };
}

Cell CampScene::PosToCell(const Vec2& p) const
{
	return { static_cast<int>(std::floor((p.x - kOriginX) / kTile)), static_cast<int>(std::floor(p.y / kTile)) };
}

bool CampScene::IsHitWall(const Vec2& p) const
{
	//足元の四角で判定
	const float cy = p.y + 8.0f;
	return IsBlocked(PosToCell({ p.x - kHeroHalfW, cy - kHeroHalfH })) || IsBlocked(PosToCell({ p.x + kHeroHalfW, cy - kHeroHalfH })) ||
		IsBlocked(PosToCell({ p.x - kHeroHalfW, cy + kHeroHalfH })) || IsBlocked(PosToCell({ p.x + kHeroHalfW, cy + kHeroHalfH }));
}

void CampScene::Update()
{
	const float dt = Game::kFrameTime;
	time_ += dt;

	//たき火の火の粉
	if (RandI(0, 3) == 0)
	{
		const Vec2 fp = CellToPos(kFire);
		Particle p;
		p.pos = fp + Vec2(RandF(-10.0f, 10.0f), -10.0f);
		p.vel = { RandF(-15.0f, 15.0f), RandF(-90.0f, -40.0f) };
		p.life = p.maxLife = RandF(0.5f, 1.1f);
		p.size = RandF(2.0f, 5.0f);
		p.color = { 255, 170, 60 };
		p.isAdd = true;
		Effects::GetInstance().Add(p);
	}

	//歩き回るモンスター
	for (auto& w : wanderers_)
	{
		const Vec2 diff = CellToPos(w.cell) - w.drawPos;
		const float dist = diff.Length();
		if (dist <= kWanderSpeed * dt) w.drawPos = CellToPos(w.cell);
		else w.drawPos += diff * (kWanderSpeed * dt / dist);
		w.timer -= dt;
		if (w.timer > 0.0f) continue;
		w.timer = RandF(0.6f, 1.8f);
		static const Cell kDirs[4] = { {1,0},{-1,0},{0,1},{0,-1} };
		const Cell to = w.cell + kDirs[RandI(0, 3)];
		bool isFree = !IsBlocked(to) && to != heroCell_ && to.y >= 4;
		for (const auto& o : wanderers_) if (&o != &w && o.cell == to) isFree = false;
		if (isFree)
		{
			if (to.x != w.cell.x) w.facing = to.x > w.cell.x ? 1.0f : -1.0f;
			w.cell = to;
		}
	}

	if (isLeaving_) return;
	switch (mode_)
	{
	case Mode::Walk: UpdateWalk(); break;
	case Mode::StageSelect:
	case Mode::Confirm: UpdateStageSelect(); break;
	case Mode::Pause:
	{
		Input& input = Input::GetInstance();
		if (input.IsAnyTriggered(Button::Up)) { cursor_ = (cursor_ + 3) % 4; Sound::GetInstance().PlaySE(SE::Cursor); }
		if (input.IsAnyTriggered(Button::Down)) { cursor_ = (cursor_ + 1) % 4; Sound::GetInstance().PlaySE(SE::Cursor); }
		if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Menu) || (input.IsAnyTriggered(Button::Decide) && cursor_ == 3))
		{
			mode_ = Mode::Walk;
			Sound::GetInstance().PlaySE(SE::Cancel);
		}
		else if (input.IsAnyTriggered(Button::Decide) && cursor_ == 1)
		{
			mode_ = Mode::KeyItems;
			Sound::GetInstance().PlaySE(SE::Decide);
		}
		else if (input.IsAnyTriggered(Button::Decide) && cursor_ == 0)
		{
			isLeaving_ = true;
			Sound::GetInstance().PlaySE(SE::Decide);
			SaveManager::GetInstance().Save();
			sceneManager_.ChangeSceneWithFade(std::make_shared<SettingsScene>(sceneManager_, SettingsScene::Return::Camp));
		}
		else if (input.IsAnyTriggered(Button::Decide))
		{
			isLeaving_ = true;
			Sound::GetInstance().PlaySE(SE::Decide);
			SaveManager::GetInstance().Save();
			sceneManager_.ChangeSceneWithFade(std::make_shared<TitleScene>(sceneManager_));
		}
		break;
	}
	case Mode::Notice: UpdateNotice(); break;
	case Mode::Unlock: UpdateUnlock(); break;
	case Mode::FloorSelect: UpdateFloorSelect(); break;
	case Mode::PartySelect: UpdatePartySelect(); break;
	case Mode::KeyItems:
		if (keyItemMenu_.Update()) mode_ = Mode::Pause;
		break;
	}
}

bool CampScene::HasUnreadNotice() const
{
	return UpdateLog::VersionCode(UpdateLog::Latest()) > SaveManager::GetInstance().GetSettings().readNotices;
}

void CampScene::OpenNotice()
{
	//新しい順に並べる
	noticeRows_.clear();
	const auto& all = UpdateLog::All();
	for (auto it = all.rbegin(); it != all.rend(); ++it)
	{
		const bool isPatch = UpdateLog::IsPatch(it->version);
		noticeRows_.push_back({ (isPatch ? L"パッチ　ver" : L"アップデート　ver") + it->version, true, isPatch });
		for (const std::wstring& line : it->lines)
		{
			const auto wrapped = Font::Wrap(L"・" + line, FontSize::Small, kNoticeWidth);
			for (size_t k = 0; k < wrapped.size(); k++) noticeRows_.push_back({ (k == 0 ? L"" : L"　") + wrapped[k], false });
		}
	}
	if (noticeRows_.empty()) noticeRows_.push_back({ L"おしらせは まだ ありません", false });

	isNoticeNew_ = HasUnreadNotice();
	Settings& st = SaveManager::GetInstance().GetSettings();
	st.readNotices = UpdateLog::VersionCode(UpdateLog::Latest());
	SaveManager::GetInstance().SaveSettings();

	noticeScroll_ = 0;
	mode_ = Mode::Notice;
	Sound::GetInstance().PlaySE(SE::Decide);
}

void CampScene::UpdateNotice()
{
	Input& input = Input::GetInstance();
	const int maxScroll = std::max(0, static_cast<int>(noticeRows_.size()) - kNoticeLines);
	//おしっぱなし・スティックを たおしたままでも つづけて うごく
	int dir = 0;
	for (int p = 0; p < Game::kPlayerNum; p++)
	{
		if (input.IsPressed(p, Button::Up)) dir = -1;
		if (input.IsPressed(p, Button::Down)) dir = 1;
	}
	bool isStep = false;
	if (input.IsAnyTriggered(Button::Up) || input.IsAnyTriggered(Button::Down))
	{
		isStep = true;
		noticeHold_ = 0.35f;
	}
	else if (dir != 0)
	{
		noticeHold_ -= Game::kFrameTime;
		if (noticeHold_ <= 0.0f)
		{
			isStep = true;
			noticeHold_ = 0.08f;
		}
	}
	const int next = std::clamp(noticeScroll_ + dir, 0, maxScroll);
	if (isStep && next != noticeScroll_)
	{
		noticeScroll_ = next;
		Sound::GetInstance().PlaySE(SE::Cursor);
	}
	if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Decide))
	{
		mode_ = Mode::Walk;
		Sound::GetInstance().PlaySE(SE::Cancel);
	}
}

void CampScene::UpdateWalk()
{
	Input& input = Input::GetInstance();

	if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Menu))
	{
		mode_ = Mode::Pause;
		cursor_ = 3;
		Sound::GetInstance().PlaySE(SE::Cursor);
		return;
	}

	if (input.IsAnyTriggered(Button::Decide))
	{
		switch (SpotAt(heroCell_))
		{
		case Spot::Dungeon:
			OpenStageSelect();
			Sound::GetInstance().PlaySE(SE::Decide);
			return;
		case Spot::Library:
			isLeaving_ = true;
			Sound::GetInstance().PlaySE(SE::Decide);
			sceneManager_.ChangeSceneWithFade(std::make_shared<ZukanScene>(sceneManager_));
			return;
		case Spot::Smith:
			isLeaving_ = true;
			Sound::GetInstance().PlaySE(SE::Decide);
			sceneManager_.ChangeSceneWithFade(std::make_shared<SmithScene>(sceneManager_));
			return;
		case Spot::Fire:
			isLeaving_ = true;
			Sound::GetInstance().PlaySE(SE::Decide);
			sceneManager_.ChangeSceneWithFade(std::make_shared<SettingsScene>(sceneManager_, SettingsScene::Return::Camp));
			return;
		case Spot::Notice:
			OpenNotice();
			return;
		default:
			break;
		}
	}

	Vec2 move = input.GetMove(0) + input.GetMove(1);
	if (move.Length() > 1.0f) move = move.Normalized();
	if (move.Length() < 0.2f) return;
	if (std::fabs(move.x) > 0.2f) heroFacing_ = move.x > 0.0f ? 1.0f : -1.0f;

	const Vec2 delta = move * (kWalkSpeed * SaveManager::GetInstance().MoveRate() * Game::kFrameTime);
	const Vec2 center = CellToPos(heroCell_);

	//横に動く。角に引っかかったらマスの中心へ寄せる
	const Vec2 nextX = heroPos_ + Vec2(delta.x, 0.0f);
	if (!IsHitWall(nextX)) heroPos_ = nextX;
	else if (std::fabs(move.y) < 0.2f && !IsBlocked(heroCell_ + Cell{ delta.x > 0.0f ? 1 : -1, 0 }))
	{
		const float d = std::clamp(center.y - heroPos_.y, -std::fabs(delta.x), std::fabs(delta.x));
		if (!IsHitWall(heroPos_ + Vec2(0.0f, d))) heroPos_.y += d;
	}

	//縦に動く
	const Vec2 nextY = heroPos_ + Vec2(0.0f, delta.y);
	if (!IsHitWall(nextY)) heroPos_ = nextY;
	else if (std::fabs(move.x) < 0.2f && !IsBlocked(heroCell_ + Cell{ 0, delta.y > 0.0f ? 1 : -1 }))
	{
		const float d = std::clamp(center.x - heroPos_.x, -std::fabs(delta.y), std::fabs(delta.y));
		if (!IsHitWall(heroPos_ + Vec2(d, 0.0f))) heroPos_.x += d;
	}

	const Cell prev = heroCell_;
	heroCell_ = PosToCell(heroPos_ + Vec2(0.0f, 8.0f));
	if (heroCell_ != prev && SpotAt(heroCell_) != Spot::None && SpotAt(heroCell_) != SpotAt(prev)) Sound::GetInstance().PlaySE(SE::Tick);
}

void CampScene::OpenStageSelect()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	choices_.clear();
	if (s.run.isActive)
	{
		choices_.push_back({ L"つづきから：" + std::wstring(Stage::Name(s.run.stage)) + L" B" + std::to_wstring(s.run.floor) + L"F", -1, false });
	}
	choices_.push_back({ std::wstring(Stage::Name(Stage::kNormal)) + L"（B1〜B12F）", Stage::kNormal, false });
	if (s.isCleared)
	{
		const std::wstring mark = s.IsNew(NewsBit::kInfinite) ? L"　NEW！" : L"";
		choices_.push_back({ std::wstring(Stage::Name(Stage::kInfinite)) + L"（B1〜B" + std::to_wstring(Stage::kInfiniteLastFloor) + L"F）" + mark, Stage::kInfinite, false });
	}
	else
	{
		choices_.push_back({ L"？？？（クリアで かいほう）", Stage::kInfinite, true });
	}
	choices_.push_back({ L"やめる", -2, false });
	cursor_ = 0;
	mode_ = Mode::StageSelect;

	//行き先を見たので「！」を消す
	if (s.IsNew(NewsBit::kInfinite))
	{
		SaveManager::GetInstance().Current().seenNews |= NewsBit::kInfinite;
		SaveManager::GetInstance().Save();
	}
}

void CampScene::DrawNewMarks()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();

	//建物の左上
	if (s.IsNew(NewsBit::kInfinite)) DrawUtil::NewMark(kOriginX + kCave.x * kTile + 18.0f, 26.0f, time_);
	if (s.IsNew(NewsBit::kShinyZukan)) DrawUtil::NewMark(kOriginX + kLibrary.x * kTile + 14.0f, kLibrary.y * kTile + 4.0f, time_);
	if (s.IsNew(NewsBit::kCharm)) DrawUtil::NewMark(kOriginX + kSmith.x * kTile + 14.0f, kSmith.y * kTile + 4.0f, time_);
}

void CampScene::DrawUnlock()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const float pop = EaseOutBack(std::min(1.0f, time_ * 2.5f));
	if (pop < 0.05f) return;
	DrawUtil::Window(640.0f - 380.0f * pop, 150.0f, 640.0f + 380.0f * pop, 560.0f);
	const float titleFit = std::min(1.0f, 700.0f / Font::Width(L"エンドコンテンツ かいほう！", FontSize::Large));
	Font::DrawCenterScaled(640.0f, 205.0f, pop * titleFit, L"エンドコンテンツ かいほう！", GetColor(255, 220, 90), FontSize::Large, GetColor(60, 30, 10));
	Font::DrawCenter(640.0f, 268.0f, L"あたらしく あそべるように なったもの", GetColor(220, 215, 240), FontSize::Small, GetColor(20, 15, 30));

	const wchar_t* lines[] =
	{
		L"むげんの めいきゅう（ダンジョンの いりぐち）",
		L"おまもり の れんせい・そうび（かじや）",
		L"いろちがいの ずかん（しょこ）",
		L"いろちがいの モンスターが でるように なった",
	};
	for (int i = 0; i < 4; i++)
	{
		const float y = 318.0f + i * 44.0f;
		DrawUtil::Star(318.0f, y + 12.0f, 10.0f, time_ * 2.0f + i, GetColor(255, 220, 80));
		Font::DrawFit(340.0f, y, lines[i], GetColor(255, 255, 255), FontSize::Small, GetColor(20, 15, 30), 640.0f);
	}
	if (time_ > 0.8f) Font::DrawCenter(640.0f, 520.0f, L"「！」の ついた たてものを たずねてみよう", GetColor(255, 230, 150), FontSize::Small, GetColor(20, 15, 30));
}

void CampScene::UpdateStageSelect()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();

	if (mode_ == Mode::Confirm)
	{
		if (input.IsAnyTriggered(Button::Left) || input.IsAnyTriggered(Button::Right)) { cursor_ = 1 - cursor_; sound.PlaySE(SE::Cursor); }
		if (input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && cursor_ == 1))
		{
			mode_ = Mode::StageSelect;
			cursor_ = 0;
			sound.PlaySE(SE::Cancel);
		}
		else if (input.IsAnyTriggered(Button::Decide))
		{
			StartDungeon(pendingStage_, false);
		}
		return;
	}

	const int num = static_cast<int>(choices_.size());
	if (input.IsAnyTriggered(Button::Up)) { cursor_ = (cursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { cursor_ = (cursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel)) { mode_ = Mode::Walk; sound.PlaySE(SE::Cancel); return; }
	if (!input.IsAnyTriggered(Button::Decide)) return;

	const StageChoice& c = choices_[cursor_];
	if (c.isLocked) { sound.PlaySE(SE::Buzzer); return; }
	if (c.stage == -2) { mode_ = Mode::Walk; sound.PlaySE(SE::Cancel); return; }
	if (c.stage == -1) { StartDungeon(SaveManager::GetInstance().Current().run.stage, true); return; }

	//むげんの めいきゅうは たどりついた休憩所の次の階から はじめられる
	if (c.stage == Stage::kInfinite)
	{
		const SaveSlot& s = SaveManager::GetInstance().Current();
		const int deepest = s.deepestInfinite;
		floorChoices_.clear();
		for (int f = 1; f <= std::min(deepest, Stage::kInfiniteLastFloor); f += Stage::kRestInterval) floorChoices_.push_back(f);

		//前回の つづき(さいしょの こうもく)
		lastFloorChoice_ = 0;
		const int last = std::min(s.lastInfinite, Stage::kInfiniteLastFloor);
		if (last > 1)
		{
			lastFloorChoice_ = last;
			floorChoices_.insert(floorChoices_.begin(), last);
		}
		if (floorChoices_.size() > 1)
		{
			floorCursor_ = lastFloorChoice_ > 0 ? 0 : static_cast<int>(floorChoices_.size()) - 1;
			mode_ = Mode::FloorSelect;
			sound.PlaySE(SE::Decide);
			return;
		}
	}
	BeginNewRun(c.stage, 1);
}

void CampScene::BeginNewRun(int stage, int startFloor)
{
	pendingStage_ = stage;
	pendingFloor_ = startFloor;

	//冒険の途中データがあれば確認
	if (SaveManager::GetInstance().Current().run.isActive)
	{
		mode_ = Mode::Confirm;
		cursor_ = 1;
		Sound::GetInstance().PlaySE(SE::Cursor);
		return;
	}
	StartDungeon(stage, false);
}

void CampScene::UpdateFloorSelect()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = static_cast<int>(floorChoices_.size());
	if (input.IsAnyTriggered(Button::Up)) { floorCursor_ = (floorCursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { floorCursor_ = (floorCursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel)) { mode_ = Mode::StageSelect; sound.PlaySE(SE::Cancel); return; }
	if (!input.IsAnyTriggered(Button::Decide)) return;
	mode_ = Mode::StageSelect;
	BeginNewRun(Stage::kInfinite, floorChoices_[floorCursor_]);
}

void CampScene::DrawFloorSelect()
{
	constexpr int kVisible = 7;
	const int num = static_cast<int>(floorChoices_.size());
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawUtil::Window(380.0f, 150.0f, 900.0f, 230.0f + kVisible * 54.0f);
	Font::DrawCenter(640.0f, 190.0f, L"どこから はじめる？", GetColor(255, 230, 140), FontSize::Medium, GetColor(20, 15, 30));

	const int first = std::clamp(floorCursor_ - kVisible / 2, 0, std::max(0, num - kVisible));
	for (int i = first; i < std::min(num, first + kVisible); i++)
	{
		const int f = floorChoices_[i];
		const float y = 250.0f + (i - first) * 54.0f;
		const bool isSel = i == floorCursor_;
		if (isSel) DrawUtil::Cursor(410.0f, y, time_);
		const bool isLast = i == 0 && lastFloorChoice_ > 0;
		const std::wstring text = isLast ? L"ぜんかいの つづき（B" + std::to_wstring(f) + L"F から）" :
			L"B" + std::to_wstring(f) + L"F から（" + Stage::InfiniteArea(f) + L"）";
		if (isLast) DrawLineAA(450.0f, y + 26.0f, 860.0f, y + 26.0f, GetColor(120, 100, 150), 1.0f);
		Font::DrawFit(450.0f, y - 14.0f, text, isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230), FontSize::Small, GetColor(20, 15, 30), 420.0f);
	}

	//スクロールのしるし
	const float bob = std::sin(time_ * 6.0f) * 3.0f;
	if (first > 0) DrawTriangleAA(860.0f, 228.0f + bob, 850.0f, 244.0f + bob, 870.0f, 244.0f + bob, GetColor(255, 230, 120), TRUE);
	if (first + kVisible < num) DrawTriangleAA(850.0f, 590.0f - bob, 870.0f, 590.0f - bob, 860.0f, 606.0f - bob, GetColor(255, 230, 120), TRUE);
}

void CampScene::StartDungeon(int stage, bool isContinue)
{
	//あたらしい ぼうけんは つれていく なかまを えらんでから
	pendingStage_ = stage;
	if (!isContinue)
	{
		OpenPartySelect();
		if (!partyCands_.empty()) return;
	}
	LaunchDungeon(stage, isContinue);
}

void CampScene::LaunchDungeon(int stage, bool isContinue)
{
	isLeaving_ = true;
	Sound::GetInstance().PlaySE(SE::Whoosh);
	const int startFloor = (!isContinue && stage == Stage::kInfinite) ? pendingFloor_ : 1;
	const std::vector<PartyMember> party = isContinue ? std::vector<PartyMember>() : partySel_;
	sceneManager_.ChangeSceneWithFade(std::make_shared<DungeonScene>(sceneManager_, stage, isContinue, startFloor, party));
}

//==============================//
// なかまを えらぶ
//==============================//

namespace
{
	constexpr int kPartyCols = 8;
	constexpr int kPartyRows = 4;	//いちどに 見える だん
	constexpr float kPartyCellW = 148.0f;
	constexpr float kPartyCellH = 104.0f;
	constexpr float kPartyGridX = 48.0f;
	constexpr float kPartyGridY = 150.0f;

	bool IsSameMember(const PartyMember& a, const PartyMember& b)
	{
		return a.monsterId == b.monsterId && a.isShiny == b.isShiny;
	}
}

int CampScene::MaxParty() const
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	return 2 + s.GearBonus(nullptr).Get(SkillKind::AllySlot);
}

void CampScene::OpenPartySelect()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const auto& all = MonsterTable::All();

	//なかまに した ことの ある モンスター(色違いは べつに)
	partyCands_.clear();
	for (int id = 0; id < static_cast<int>(all.size()); id++)
	{
		if (all[id].recruitRate <= 0) continue;
		if (id < static_cast<int>(s.zukan.size()) && s.zukan[id].isRecruited) partyCands_.push_back({ id, false });
		if (id < static_cast<int>(s.zukanShiny.size()) && s.zukanShiny[id].isRecruited) partyCands_.push_back({ id, true });
	}

	//まえと おなじ なかまを えらんでおく
	partySel_.clear();
	for (const PartyMember& p : s.lastParty)
	{
		if (static_cast<int>(partySel_.size()) >= MaxParty()) break;
		for (const PartyMember& c : partyCands_)
		{
			if (IsSameMember(c, p)) { partySel_.push_back(p); break; }
		}
	}
	partyCursor_ = -1;
	partyMsgTime_ = 0.0f;
	if (!partyCands_.empty())
	{
		mode_ = Mode::PartySelect;
		Sound::GetInstance().PlaySE(SE::Decide);
	}
}

void CampScene::UpdatePartySelect()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	partyMsgTime_ = std::max(0.0f, partyMsgTime_ - Game::kFrameTime);
	const int num = static_cast<int>(partyCands_.size());

	if (input.IsAnyTriggered(Button::Cancel))
	{
		mode_ = Mode::StageSelect;
		sound.PlaySE(SE::Cancel);
		return;
	}

	//うごかす(いちばん うえの だんから うえで「しゅっぱつ」)
	int next = partyCursor_;
	if (partyCursor_ < 0)
	{
		if (input.IsAnyTriggered(Button::Down)) next = 0;
	}
	else
	{
		if (input.IsAnyTriggered(Button::Left) && partyCursor_ % kPartyCols > 0) next--;
		if (input.IsAnyTriggered(Button::Right) && partyCursor_ % kPartyCols < kPartyCols - 1 && partyCursor_ + 1 < num) next++;
		if (input.IsAnyTriggered(Button::Up)) next = partyCursor_ < kPartyCols ? -1 : partyCursor_ - kPartyCols;
		if (input.IsAnyTriggered(Button::Down) && partyCursor_ + kPartyCols < num) next = partyCursor_ + kPartyCols;
	}
	if (next != partyCursor_)
	{
		partyCursor_ = next;
		sound.PlaySE(SE::Cursor);
	}

	if (!input.IsAnyTriggered(Button::Decide)) return;

	//しゅっぱつ
	if (partyCursor_ < 0)
	{
		SaveSlot& s = SaveManager::GetInstance().Current();
		s.lastParty = partySel_;
		SaveManager::GetInstance().Save();
		LaunchDungeon(pendingStage_, false);
		return;
	}

	//えらぶ・はずす
	const PartyMember& c = partyCands_[partyCursor_];
	for (size_t i = 0; i < partySel_.size(); i++)
	{
		if (IsSameMember(partySel_[i], c))
		{
			partySel_.erase(partySel_.begin() + i);
			sound.PlaySE(SE::Cancel);
			return;
		}
	}
	if (static_cast<int>(partySel_.size()) >= MaxParty())
	{
		sound.PlaySE(SE::Buzzer);
		partyMsg_ = L"つれていけるのは " + std::to_wstring(MaxParty()) + L"たい まで";
		partyMsgTime_ = 2.0f;
		return;
	}
	partySel_.push_back(c);
	sound.PlaySE(SE::Ready);
}

void CampScene::DrawPartySelect()
{
	const unsigned int edge = GetColor(20, 15, 30);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(12, 10, 24), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	Font::DrawCenter(640.0f, 42.0f, L"つれていく なかまを えらぶ", GetColor(255, 230, 140), FontSize::Medium, edge);

	//しゅっぱつ ボタン
	const bool isDepart = partyCursor_ < 0;
	const float bob = isDepart ? std::sin(time_ * 6.0f) * 2.0f : 0.0f;
	DrawUtil::Panel(490.0f, 78.0f + bob, 790.0f, 126.0f + bob, isDepart ? Game::Rgb{ 120, 70, 40 } : Game::Rgb{ 40, 34, 60 }, 240,
		isDepart ? Game::Rgb{ 255, 220, 120 } : Game::Rgb{ 110, 100, 140 });
	const std::wstring depart = partySel_.empty() ? L"ひとりで しゅっぱつ！" : L"しゅっぱつ！";
	Font::DrawCenter(640.0f, 102.0f + bob, depart.c_str(), isDepart ? GetColor(255, 245, 190) : GetColor(200, 190, 220), FontSize::Small, edge);

	//いちらん(カーソルの だんが 見えるように ずらす)
	const int num = static_cast<int>(partyCands_.size());
	const int rowNum = (num + kPartyCols - 1) / kPartyCols;
	const int curRow = partyCursor_ < 0 ? 0 : partyCursor_ / kPartyCols;
	const int firstRow = std::clamp(curRow - 1, 0, std::max(0, rowNum - kPartyRows));
	for (int i = firstRow * kPartyCols; i < std::min(num, (firstRow + kPartyRows) * kPartyCols); i++)
	{
		const PartyMember& c = partyCands_[i];
		const float x = kPartyGridX + (i % kPartyCols) * kPartyCellW;
		const float y = kPartyGridY + (i / kPartyCols - firstRow) * kPartyCellH;
		const bool isSel = i == partyCursor_;
		int order = -1;
		for (size_t k = 0; k < partySel_.size(); k++) if (IsSameMember(partySel_[k], c)) order = static_cast<int>(k);

		DrawUtil::Panel(x, y, x + kPartyCellW - 10.0f, y + kPartyCellH - 10.0f, order >= 0 ? Game::Rgb{ 50, 80, 70 } : (isSel ? Game::Rgb{ 80, 70, 120 } : Game::Rgb{ 30, 26, 46 }), 235,
			isSel ? Game::Rgb{ 255, 220, 120 } : (order >= 0 ? Game::Rgb{ 130, 220, 160 } : Game::Rgb{ 90, 80, 110 }));
		const MonsterData m = c.isShiny ? MonsterTable::ShinyVariant(c.monsterId) : MonsterTable::Get(c.monsterId);
		const float cx = x + (kPartyCellW - 10.0f) * 0.5f;
		UnitDraw::Monster({ cx, y + 62.0f }, m.type == MonsterType::Boss ? 26.0f : 22.0f, m, time_, 1.0f, false);
		const float nameScale = std::min(1.0f, (kPartyCellW - 20.0f) / std::max(1, Font::Width(m.name, FontSize::Small)));
		Font::DrawCenterScaled(cx, y + 80.0f, nameScale, m.name.c_str(), c.isShiny ? GetColor(255, 230, 120) : GetColor(230, 225, 245), FontSize::Small, edge);

		//えらんだ じゅんばん
		if (order >= 0)
		{
			DrawCircleAA(x + 18.0f, y + 18.0f, 13.0f, 16, GetColor(110, 210, 140), TRUE);
			Font::DrawCenter(x + 18.0f, y + 18.0f, std::to_wstring(order + 1).c_str(), GetColor(20, 40, 30), FontSize::Small, GetColor(200, 255, 220));
		}
		if (c.isShiny) DrawUtil::Star(x + kPartyCellW - 28.0f, y + 18.0f, 9.0f, time_, GetColor(255, 220, 80));
	}

	//スクロールの しるし
	const float arrowBob = std::sin(time_ * 6.0f) * 3.0f;
	if (firstRow > 0) DrawTriangleAA(640.0f, 136.0f - arrowBob, 628.0f, 148.0f - arrowBob, 652.0f, 148.0f - arrowBob, GetColor(255, 230, 120), TRUE);
	if (firstRow + kPartyRows < rowNum) DrawTriangleAA(628.0f, 568.0f + arrowBob, 652.0f, 568.0f + arrowBob, 640.0f, 580.0f + arrowBob, GetColor(255, 230, 120), TRUE);

	//したの じょうほう
	DrawUtil::Window(40.0f, 588.0f, 1240.0f, 676.0f);
	std::wstring names;
	for (const PartyMember& p : partySel_)
	{
		const MonsterData m = p.isShiny ? MonsterTable::ShinyVariant(p.monsterId) : MonsterTable::Get(p.monsterId);
		names += (names.empty() ? L"" : L"・") + m.name;
	}
	const std::wstring party = L"つれていく（" + std::to_wstring(partySel_.size()) + L"/" + std::to_wstring(MaxParty()) + L"）：" + (names.empty() ? std::wstring(L"なし") : names);
	Font::DrawFit(64.0f, 600.0f, party, GetColor(200, 240, 210), FontSize::Small, edge, 1150.0f);
	if (partyMsgTime_ > 0.0f)
	{
		Font::DrawFit(64.0f, 636.0f, partyMsg_, GetColor(255, 170, 150), FontSize::Small, edge, 1150.0f);
	}
	else if (partyCursor_ >= 0)
	{
		const PartyMember& c = partyCands_[partyCursor_];
		const MonsterData m = c.isShiny ? MonsterTable::ShinyVariant(c.monsterId) : MonsterTable::Get(c.monsterId);
		const std::wstring info = m.name + L"　とくぎ：" + MonsterTable::SkillName(m.skill) + L"　（ゆうしゃと おなじ レベルで くわわる）";
		Font::DrawFit(64.0f, 636.0f, info, GetColor(220, 215, 240), FontSize::Small, edge, 1150.0f);
	}
	else
	{
		Font::DrawFit(64.0f, 636.0f, L"なかまに した ことの ある モンスターを つれていける", GetColor(180, 170, 200), FontSize::Small, edge, 1150.0f);
	}
	ButtonGuide::DrawCenter(640.0f, 700.0f, L"{A}：えらぶ・はずす／しゅっぱつ　{B}：もどる", GetColor(170, 160, 200), FontSize::Small, edge);
}

//==============================//
// 表示
//==============================//

void CampScene::Draw()
{
	DrawGround();
	DrawBuildings();

	//キャラ(奥から)
	struct Drawn { float y; int index; };
	std::vector<Drawn> order;
	order.push_back({ heroPos_.y, -1 });
	for (size_t i = 0; i < wanderers_.size(); i++) order.push_back({ wanderers_[i].drawPos.y, static_cast<int>(i) });
	std::sort(order.begin(), order.end(), [](const Drawn& a, const Drawn& b) { return a.y < b.y; });
	for (const Drawn& d : order)
	{
		if (d.index < 0)
		{
			UnitDraw::Hero(heroPos_ + Vec2(0.0f, kTile * 0.42f), 36.0f, time_, heroFacing_, false);
			continue;
		}
		const Wanderer& w = wanderers_[d.index];
		const MonsterData m = w.isShiny ? MonsterTable::ShinyVariant(w.monsterId) : MonsterTable::Get(w.monsterId);
		const float size = m.type == MonsterType::Boss ? 40.0f : 30.0f;
		UnitDraw::Monster(w.drawPos + Vec2(0.0f, kTile * 0.42f), size, m, time_, w.facing, false);
		if (w.isShiny && static_cast<int>(time_ * 3.0f + d.index) % 4 == 0)
		{
			SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
			DrawUtil::Star(w.drawPos.x + 14.0f, w.drawPos.y - 18.0f, 6.0f, time_ * 4.0f, GetColor(255, 255, 200));
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}
	}

	Effects::GetInstance().Draw();
	DrawNoticeBoard();
	DrawNewMarks();

	//立っている場所の案内
	const Spot spot = SpotAt(heroCell_);
	if (spot != Spot::None && mode_ == Mode::Walk)
	{
		const wchar_t* text = L"";
		switch (spot)
		{
		case Spot::Dungeon: text = L"{A}：ダンジョンへ"; break;
		case Spot::Library: text = L"{A}：モンスターずかん"; break;
		case Spot::Smith: text = L"{A}：かじや"; break;
		case Spot::Fire: text = L"{A}：せってい"; break;
		case Spot::Notice: text = L"{A}：おしらせ"; break;
		default: break;
		}
		//おしらせは吹き出しに案内を出す
		const float bob = std::sin(time_ * 5.0f) * 3.0f;
		if (spot != Spot::Notice) ButtonGuide::DrawCenter(heroPos_.x, heroPos_.y - 70.0f + bob, text, GetColor(255, 240, 160), FontSize::Small, GetColor(20, 15, 30));
	}

	DrawHud();
	if (mode_ == Mode::StageSelect || mode_ == Mode::Confirm) DrawStageSelect();
	if (mode_ == Mode::FloorSelect) DrawFloorSelect();

	if (mode_ == Mode::Pause)
	{
		DrawUtil::Window(420.0f, 195.0f, 860.0f, 515.0f);
		const wchar_t* items[4] = { L"せってい", L"だいじな もの", L"タイトルへ もどる", L"とじる" };
		for (int i = 0; i < 4; i++)
		{
			const float y = 250.0f + i * 70.0f;
			if (i == cursor_) DrawUtil::CursorBeside(640.0f, y, items[i], FontSize::Medium, time_);
			Font::DrawCenter(640.0f, y, items[i], i == cursor_ ? GetColor(255, 240, 150) : GetColor(190, 180, 210), FontSize::Medium, GetColor(20, 15, 30));
		}
	}

	if (mode_ == Mode::Notice) DrawNotice();
	if (mode_ == Mode::KeyItems) keyItemMenu_.Draw();
	if (mode_ == Mode::PartySelect) DrawPartySelect();
	if (mode_ == Mode::Unlock) DrawUnlock();
}

void CampScene::DrawNoticeBoard()
{
	//けいじばん
	const Vec2 c = CellToPos(kBoard);
	const unsigned int wood = GetColor(140, 95, 55);
	const unsigned int dark = GetColor(90, 60, 35);
	DrawBoxAA(c.x - 18.0f, c.y - 2.0f, c.x - 12.0f, c.y + 24.0f, dark, TRUE);
	DrawBoxAA(c.x + 12.0f, c.y - 2.0f, c.x + 18.0f, c.y + 24.0f, dark, TRUE);
	DrawRoundRectAA(c.x - 26.0f, c.y - 22.0f, c.x + 26.0f, c.y + 8.0f, 4.0f, 4.0f, 6, wood, TRUE);
	DrawRoundRectAA(c.x - 26.0f, c.y - 22.0f, c.x + 26.0f, c.y + 8.0f, 4.0f, 4.0f, 6, dark, FALSE, 2.0f);
	DrawBoxAA(c.x - 16.0f, c.y - 17.0f, c.x - 1.0f, c.y + 3.0f, GetColor(245, 240, 225), TRUE);
	DrawBoxAA(c.x + 3.0f, c.y - 15.0f, c.x + 18.0f, c.y + 1.0f, GetColor(235, 225, 200), TRUE);

	//吹き出し(未読があるときは目立たせる)
	const bool isUnread = HasUnreadNotice();
	const std::wstring latest = UpdateLog::Latest();
	const bool isHere = SpotAt(heroCell_) == Spot::Notice && mode_ == Mode::Walk;
	std::wstring text = isUnread ? L"NEW! ver" + latest : (latest.empty() ? L"おしらせ" : L"ver" + latest);
	if (isHere) text = L"{A}：おしらせ";
	const float w = ButtonGuide::Width(text, FontSize::Small) + 32.0f;
	const float bob = isUnread ? std::sin(time_ * 5.0f) * 3.0f : 0.0f;
	const float right = c.x - 36.0f;
	const float top = 12.0f + bob;
	const float bottom = 52.0f + bob;
	const unsigned int fill = isUnread ? GetColor(255, 250, 225) : GetColor(235, 235, 240);
	const unsigned int frame = isUnread ? GetColor(230, 90, 60) : GetColor(150, 150, 170);
	DrawTriangleAA(right - 4.0f, top + 14.0f, right - 4.0f, bottom - 8.0f, right + 14.0f, bottom + 2.0f, frame, TRUE);
	DrawRoundRectAA(right - w, top, right, bottom, 14.0f, 14.0f, 8, fill, TRUE);
	DrawRoundRectAA(right - w, top, right, bottom, 14.0f, 14.0f, 8, frame, FALSE, 3.0f);
	DrawTriangleAA(right - 6.0f, top + 16.0f, right - 6.0f, bottom - 10.0f, right + 8.0f, bottom - 2.0f, fill, TRUE);
	ButtonGuide::DrawCenter(right - w * 0.5f, (top + bottom) * 0.5f, text, isUnread ? GetColor(220, 60, 50) : GetColor(90, 90, 110), FontSize::Small, fill);

	//未読のきらめき
	if (isUnread && static_cast<int>(time_ * 4.0f) % 3 == 0)
	{
		SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
		DrawUtil::Star(right - w + 6.0f, top + 4.0f, 7.0f, time_ * 4.0f, GetColor(255, 220, 120));
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

void CampScene::DrawNotice()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawUtil::Window(200.0f, 70.0f, 1080.0f, 640.0f);
	Font::DrawCenter(640.0f, 112.0f, L"おしらせ", GetColor(255, 230, 140), FontSize::Medium, GetColor(20, 15, 30));
	if (isNoticeNew_) Font::DrawCenter(772.0f, 112.0f, L"NEW!", GetColor(255, 110, 90), FontSize::Small, GetColor(20, 15, 30));

	const int font = Font::Get(FontSize::Small);
	const int end = std::min(static_cast<int>(noticeRows_.size()), noticeScroll_ + kNoticeLines);
	for (int i = noticeScroll_; i < end; i++)
	{
		const NoticeRow& row = noticeRows_[i];
		const float y = 160.0f + (i - noticeScroll_) * kNoticeLineH;
		if (row.isHeader)
		{
			//アップデートは金、パッチは青
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
			DrawRoundRectAA(240.0f, y - 2.0f, 1040.0f, y + 32.0f, 8.0f, 8.0f, 6, row.isPatch ? GetColor(50, 80, 130) : GetColor(140, 100, 30), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			DrawStringToHandle(256, static_cast<int>(y + 2.0f), row.text.c_str(), row.isPatch ? GetColor(170, 215, 255) : GetColor(255, 220, 120), font, GetColor(20, 15, 30));
		}
		else
		{
			DrawStringToHandle(262, static_cast<int>(y + 2.0f), row.text.c_str(), GetColor(235, 230, 250), font, GetColor(20, 15, 30));
		}
	}

	//スクロールのしるし
	const float bob = std::sin(time_ * 6.0f) * 3.0f;
	if (noticeScroll_ > 0) DrawTriangleAA(1050.0f, 160.0f + bob, 1040.0f, 176.0f + bob, 1060.0f, 176.0f + bob, GetColor(255, 230, 120), TRUE);
	if (end < static_cast<int>(noticeRows_.size())) DrawTriangleAA(1040.0f, 560.0f - bob, 1060.0f, 560.0f - bob, 1050.0f, 576.0f - bob, GetColor(255, 230, 120), TRUE);

	ButtonGuide::DrawCenter(640.0f, 610.0f, L"{UD}：スクロール　{A} / {B}：とじる", GetColor(160, 150, 180), FontSize::Small, GetColor(20, 15, 30));
}

void CampScene::DrawGround()
{
	for (int y = 0; y < kRows; y++)
	{
		for (int x = 0; x < kCols; x++)
		{
			const float sx = kOriginX + x * kTile;
			const float sy = y * kTile;
			unsigned int col = ((x * 7 + y * 3) % 5 == 0) ? GetColor(98, 168, 88) : GetColor(108, 180, 96);
			if (path_[y][x]) col = ((x + y) % 2 == 0) ? GetColor(196, 168, 120) : GetColor(186, 158, 112);
			DrawBoxAA(sx, sy, sx + kTile, sy + kTile, col, TRUE);

			//草の模様
			if (!path_[y][x] && (x * 13 + y * 7) % 4 == 0)
			{
				DrawLineAA(sx + 14.0f, sy + 30.0f, sx + 18.0f, sy + 22.0f, GetColor(70, 140, 70), 2.0f);
				DrawLineAA(sx + 20.0f, sy + 30.0f, sx + 21.0f, sy + 21.0f, GetColor(70, 140, 70), 2.0f);
			}
		}
	}

	//外周の木
	auto tree = [](float cx, float cy)
	{
		DrawOvalAA(cx, cy + 16.0f, 16.0f, 6.0f, 12, GetColor(50, 90, 50), TRUE);
		DrawBoxAA(cx - 5.0f, cy, cx + 5.0f, cy + 16.0f, GetColor(110, 75, 45), TRUE);
		DrawCircleAA(cx, cy - 6.0f, 20.0f, 20, GetColor(50, 120, 60), TRUE);
		DrawCircleAA(cx - 6.0f, cy - 12.0f, 9.0f, 12, GetColor(80, 160, 80), TRUE);
	};
	for (int x = 0; x < kCols; x++)
	{
		if (x < kCave.x || x >= kCave.x + kCave.w) tree(kOriginX + (x + 0.5f) * kTile, 0.5f * kTile);
		tree(kOriginX + (x + 0.5f) * kTile, (kRows - 0.5f) * kTile);
	}
	for (int y = 1; y < kRows - 1; y++)
	{
		tree(kOriginX + 0.5f * kTile, (y + 0.5f) * kTile);
		tree(kOriginX + (kCols - 0.5f) * kTile, (y + 0.5f) * kTile);
	}
	for (const Cell& t : kTrees) tree(kOriginX + (t.x + 0.5f) * kTile, (t.y + 0.5f) * kTile);

	//池
	const float px = kOriginX + (kPond.x + kPond.w * 0.5f) * kTile;
	const float py = (kPond.y + kPond.h * 0.5f) * kTile;
	DrawOvalAA(px, py, kPond.w * kTile * 0.5f, kPond.h * kTile * 0.45f, 40, GetColor(70, 140, 210), TRUE);
	DrawOvalAA(px, py, kPond.w * kTile * 0.5f, kPond.h * kTile * 0.45f, 40, GetColor(200, 230, 255), FALSE, 3.0f);
	for (int k = 0; k < 3; k++)
	{
		const float wx = px - 50.0f + k * 45.0f + std::sin(time_ + k) * 6.0f;
		DrawLineAA(wx, py - 10.0f + k * 12.0f, wx + 18.0f, py - 10.0f + k * 12.0f, GetColor(170, 210, 250), 2.0f);
	}
}

void CampScene::DrawBuildings()
{
	const int fontS = Font::Get(FontSize::Small);
	(void)fontS;

	//ダンジョンの入口(ほら穴)
	{
		const float x1 = kOriginX + kCave.x * kTile;
		const float x2 = x1 + kCave.w * kTile;
		const float y2 = (kCave.y + kCave.h) * kTile;
		DrawBoxAA(x1, 0.0f, x2, y2, GetColor(110, 100, 95), TRUE);
		for (int k = 0; k < 6; k++)
		{
			DrawCircleAA(x1 + 15.0f + k * 32.0f, 10.0f + (k % 2) * 14.0f, 22.0f, 16, GetColor(130, 120, 112), TRUE);
		}
		const float cx = (x1 + x2) * 0.5f;
		DrawOvalAA(cx, y2 - 10.0f, 70.0f, 80.0f, 32, GetColor(20, 14, 20), TRUE);
		DrawBoxAA(cx - 70.0f, y2 - 10.0f, cx + 70.0f, y2, GetColor(20, 14, 20), TRUE);
		const float glow = 0.5f + 0.5f * std::sin(time_ * 2.0f);
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(40 + 40 * glow));
		DrawOvalAA(cx, y2 - 30.0f, 40.0f, 50.0f, 24, GetColor(120, 80, 200), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		Font::DrawCenter(cx, y2 + 14.0f, L"ダンジョン", GetColor(255, 240, 200), FontSize::Small, GetColor(30, 20, 20));
	}

	DrawArchive();
	DrawSmithy();

	//テント
	{
		const float x1 = kOriginX + kTent.x * kTile;
		const float x2 = x1 + kTent.w * kTile;
		const float y2 = (kTent.y + kTent.h) * kTile - 6.0f;
		DrawTriangleAA(x1, y2, x2, y2, (x1 + x2) * 0.5f, kTent.y * kTile + 6.0f, GetColor(200, 90, 70), TRUE);
		DrawTriangleAA((x1 + x2) * 0.5f - 20.0f, y2, (x1 + x2) * 0.5f + 20.0f, y2, (x1 + x2) * 0.5f, y2 - 50.0f, GetColor(70, 30, 30), TRUE);
	}

	//たき火
	{
		const Vec2 fp = CellToPos(kFire);
		DrawLineAA(fp.x - 16.0f, fp.y + 14.0f, fp.x + 16.0f, fp.y + 6.0f, GetColor(110, 70, 40), 6.0f);
		DrawLineAA(fp.x - 16.0f, fp.y + 6.0f, fp.x + 16.0f, fp.y + 14.0f, GetColor(110, 70, 40), 6.0f);
		const float flick = std::sin(time_ * 14.0f) * 3.0f;
		SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
		DrawCircleAA(fp.x, fp.y, 46.0f + flick, 24, GetColor(255, 120, 40), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		DrawOvalAA(fp.x, fp.y - 4.0f, 12.0f, 20.0f + flick, 16, GetColor(255, 160, 50), TRUE);
		DrawOvalAA(fp.x, fp.y, 7.0f, 12.0f, 12, GetColor(255, 230, 120), TRUE);
		Font::DrawCenter(fp.x, fp.y - 46.0f, L"たきび", GetColor(255, 240, 200), FontSize::Small, GetColor(30, 20, 20));
	}
}

void CampScene::DrawArchive()
{
	const float x1 = kOriginX + kLibrary.x * kTile;
	const float x2 = x1 + kLibrary.w * kTile;
	const float y1 = kLibrary.y * kTile;
	const float y2 = (kLibrary.y + kLibrary.h) * kTile;
	const float cx = (x1 + x2) * 0.5f;
	const unsigned int stone = GetColor(205, 198, 182);
	const unsigned int stoneDark = GetColor(150, 142, 128);

	//石の壁
	DrawBoxAA(x1 + 8.0f, y1 + 66.0f, x2 - 8.0f, y2 - 10.0f, stone, TRUE);
	for (int row = 0; row < 8; row++)
	{
		const float ly = y1 + 82.0f + row * 18.0f;
		if (ly > y2 - 14.0f) break;
		DrawLineAA(x1 + 8.0f, ly, x2 - 8.0f, ly, stoneDark, 1.0f);
	}

	//段
	DrawBoxAA(x1 - 4.0f, y2 - 12.0f, x2 + 4.0f, y2 - 4.0f, stoneDark, TRUE);
	DrawBoxAA(x1 - 10.0f, y2 - 4.0f, x2 + 10.0f, y2 + 2.0f, GetColor(130, 122, 110), TRUE);

	//本棚の窓
	const unsigned int books[] = { GetColor(170, 60, 50), GetColor(60, 100, 160), GetColor(70, 130, 80), GetColor(190, 150, 60), GetColor(120, 70, 140) };
	auto shelf = [&](float wx1)
	{
		const float wx2 = wx1 + 34.0f;
		DrawBoxAA(wx1 - 3.0f, y1 + 96.0f, wx2 + 3.0f, y1 + 156.0f, GetColor(90, 60, 40), TRUE);
		for (int s = 0; s < 2; s++)
		{
			const float by = y1 + 100.0f + s * 28.0f;
			DrawBoxAA(wx1, by, wx2, by + 24.0f, GetColor(60, 40, 30), TRUE);
			float bx = wx1 + 2.0f;
			for (int b = 0; bx < wx2 - 4.0f; b++)
			{
				const float bw = 4.0f + static_cast<float>((b * 7 + s * 3) % 3);
				const float bh = 16.0f + static_cast<float>((b * 5 + s) % 3) * 3.0f;
				DrawBoxAA(bx, by + 24.0f - bh, bx + bw, by + 24.0f, books[(b + s * 2) % 5], TRUE);
				bx += bw + 1.0f;
			}
		}
	};
	shelf(x1 + 30.0f);
	shelf(x2 - 64.0f);

	//柱
	const float columns[] = { x1 + 20.0f, cx - 34.0f, cx + 34.0f, x2 - 20.0f };
	for (float c : columns)
	{
		DrawBoxAA(c - 7.0f, y1 + 72.0f, c + 7.0f, y2 - 12.0f, GetColor(240, 236, 226), TRUE);
		DrawLineAA(c - 2.0f, y1 + 76.0f, c - 2.0f, y2 - 16.0f, GetColor(200, 195, 185), 1.0f);
		DrawLineAA(c + 3.0f, y1 + 76.0f, c + 3.0f, y2 - 16.0f, GetColor(200, 195, 185), 1.0f);
		DrawBoxAA(c - 10.0f, y1 + 68.0f, c + 10.0f, y1 + 74.0f, GetColor(225, 220, 208), TRUE);
		DrawBoxAA(c - 10.0f, y2 - 16.0f, c + 10.0f, y2 - 12.0f, GetColor(225, 220, 208), TRUE);
	}

	//三角屋根
	DrawBoxAA(x1 - 8.0f, y1 + 58.0f, x2 + 8.0f, y1 + 70.0f, GetColor(185, 178, 162), TRUE);
	DrawTriangleAA(x1 - 8.0f, y1 + 58.0f, x2 + 8.0f, y1 + 58.0f, cx, y1 + 4.0f, GetColor(185, 178, 162), TRUE);
	DrawTriangleAA(x1 + 14.0f, y1 + 54.0f, x2 - 14.0f, y1 + 54.0f, cx, y1 + 14.0f, GetColor(215, 208, 192), TRUE);

	//アーチの扉
	const float dx = kOriginX + 4.5f * kTile;
	DrawCircleAA(dx, y2 - 50.0f, 18.0f, 20, GetColor(80, 50, 35), TRUE);
	DrawBoxAA(dx - 18.0f, y2 - 50.0f, dx + 18.0f, y2 - 12.0f, GetColor(80, 50, 35), TRUE);
	DrawLineAA(dx, y2 - 66.0f, dx, y2 - 12.0f, GetColor(55, 35, 25), 2.0f);
	DrawCircleAA(dx - 5.0f, y2 - 32.0f, 2.5f, 8, GetColor(240, 210, 120), TRUE);
	DrawCircleAA(dx + 5.0f, y2 - 32.0f, 2.5f, 8, GetColor(240, 210, 120), TRUE);

	Font::DrawCenter(cx, y1 + 42.0f, L"しょこ", GetColor(255, 255, 255), FontSize::Small, GetColor(40, 30, 50));

	//書庫の人(本を読む)
	const float kx = x2 - 46.0f;
	const float fy = y2 - 6.0f;
	const float bob = std::sin(time_ * 1.5f) * 1.0f;
	DrawOvalAA(kx, fy, 16.0f, 4.0f, 12, GetColor(60, 50, 40), TRUE);
	DrawQuadrangleAA(kx - 9.0f, fy - 36.0f + bob, kx + 9.0f, fy - 36.0f + bob, kx + 15.0f, fy, kx - 15.0f, fy, GetColor(70, 60, 120), TRUE);
	DrawLineAA(kx, fy - 34.0f + bob, kx, fy, GetColor(200, 170, 80), 2.0f);
	const float hy = fy - 45.0f + bob;
	DrawCircleAA(kx, hy, 9.0f, 16, GetColor(250, 215, 180), TRUE);
	DrawTriangleAA(kx - 6.0f, hy + 3.0f, kx + 6.0f, hy + 3.0f, kx, hy + 15.0f, GetColor(235, 235, 240), TRUE);
	DrawCircleAA(kx, hy - 6.0f, 8.0f, 16, GetColor(220, 220, 225), TRUE);
	DrawBoxAA(kx - 9.0f, hy - 6.0f, kx + 9.0f, hy - 2.0f, GetColor(250, 215, 180), TRUE);
	DrawCircleAA(kx - 4.0f, hy, 3.0f, 10, GetColor(60, 50, 40), FALSE, 1.2f);
	DrawCircleAA(kx + 4.0f, hy, 3.0f, 10, GetColor(60, 50, 40), FALSE, 1.2f);
	DrawLineAA(kx - 1.0f, hy, kx + 1.0f, hy, GetColor(60, 50, 40), 1.2f);

	//開いた本(ときどき めくる)
	const float by = fy - 24.0f + bob;
	DrawQuadrangleAA(kx - 14.0f, by - 4.0f, kx, by, kx, by + 8.0f, kx - 14.0f, by + 4.0f, GetColor(150, 60, 50), TRUE);
	DrawQuadrangleAA(kx, by, kx + 14.0f, by - 4.0f, kx + 14.0f, by + 4.0f, kx, by + 8.0f, GetColor(150, 60, 50), TRUE);
	DrawQuadrangleAA(kx - 12.0f, by - 5.0f, kx, by - 1.0f, kx, by + 6.0f, kx - 12.0f, by + 2.0f, GetColor(250, 245, 225), TRUE);
	DrawQuadrangleAA(kx, by - 1.0f, kx + 12.0f, by - 5.0f, kx + 12.0f, by + 2.0f, kx, by + 6.0f, GetColor(250, 245, 225), TRUE);
	const float flip = std::fmod(time_, 4.0f);
	if (flip < 0.5f)
	{
		const float px = kx + 12.0f - flip / 0.5f * 24.0f;
		DrawLineAA(kx, by - 1.0f, px, by - 8.0f, GetColor(255, 255, 240), 2.0f);
	}
}

void CampScene::DrawSmithy()
{
	const float x1 = kOriginX + kSmith.x * kTile;
	const float x2 = x1 + kSmith.w * kTile;
	const float y1 = kSmith.y * kTile;
	const float y2 = (kSmith.y + kSmith.h) * kTile;
	const float cx = (x1 + x2) * 0.5f;

	//えんとつ
	const float chx = x2 - 40.0f;
	DrawBoxAA(chx - 13.0f, y1 - 6.0f, chx + 13.0f, y1 + 50.0f, GetColor(150, 75, 55), TRUE);
	for (int row = 0; row < 7; row++)
	{
		const float ly = y1 - 6.0f + row * 8.0f;
		DrawLineAA(chx - 13.0f, ly, chx + 13.0f, ly, GetColor(110, 50, 40), 1.0f);
		const float off = (row % 2) ? 0.0f : 6.0f;
		DrawLineAA(chx - 6.0f + off, ly, chx - 6.0f + off, ly + 8.0f, GetColor(110, 50, 40), 1.0f);
	}
	DrawBoxAA(chx - 16.0f, y1 - 10.0f, chx + 16.0f, y1 - 4.0f, GetColor(120, 60, 45), TRUE);

	//木の壁
	DrawBoxAA(x1 + 6.0f, y1 + 70.0f, x2 - 6.0f, y2, GetColor(125, 82, 50), TRUE);
	for (float ly = y1 + 82.0f; ly < y2; ly += 13.0f)
	{
		DrawLineAA(x1 + 6.0f, ly, x2 - 6.0f, ly, GetColor(95, 60, 36), 1.5f);
	}
	DrawBoxAA(x1 + 4.0f, y1 + 70.0f, x1 + 12.0f, y2, GetColor(75, 48, 30), TRUE);
	DrawBoxAA(x2 - 12.0f, y1 + 70.0f, x2 - 4.0f, y2, GetColor(75, 48, 30), TRUE);

	//かわら屋根
	DrawQuadrangleAA(x1 + 18.0f, y1 + 22.0f, x2 - 18.0f, y1 + 22.0f, x2 + 14.0f, y1 + 76.0f, x1 - 14.0f, y1 + 76.0f, GetColor(70, 76, 92), TRUE);
	for (int k = 0; k <= 12; k++)
	{
		const float t = k / 12.0f;
		const float tx = x1 + 18.0f + (x2 - x1 - 36.0f) * t;
		const float bx = x1 - 14.0f + (x2 - x1 + 28.0f) * t;
		DrawLineAA(tx, y1 + 22.0f, bx, y1 + 76.0f, GetColor(50, 54, 68), 1.5f);
	}
	DrawBoxAA(x1 - 14.0f, y1 + 72.0f, x2 + 14.0f, y1 + 78.0f, GetColor(45, 48, 60), TRUE);
	DrawBoxAA(x1 + 12.0f, y1 + 16.0f, x2 - 12.0f, y1 + 24.0f, GetColor(45, 48, 60), TRUE);

	//看板
	DrawBoxAA(cx - 44.0f, y1 + 32.0f, cx + 44.0f, y1 + 60.0f, GetColor(200, 160, 100), TRUE);
	DrawBoxAA(cx - 44.0f, y1 + 32.0f, cx + 44.0f, y1 + 60.0f, GetColor(90, 55, 30), FALSE, 2.0f);
	Font::DrawCenter(cx, y1 + 46.0f, L"かじや", GetColor(60, 30, 15), FontSize::Small, GetColor(240, 220, 180));

	//炉(えんとつの下)
	const float fx1 = chx - 30.0f;
	const float fx2 = chx + 26.0f;
	const float fTop = y2 - 74.0f;
	DrawBoxAA(fx1 - 6.0f, fTop - 6.0f, fx2 + 6.0f, y2, GetColor(110, 105, 100), TRUE);
	DrawCircleAA((fx1 + fx2) * 0.5f, fTop + 18.0f, (fx2 - fx1) * 0.5f, 20, GetColor(35, 22, 18), TRUE);
	DrawBoxAA(fx1, fTop + 18.0f, fx2, y2 - 8.0f, GetColor(35, 22, 18), TRUE);
	const float flick = std::sin(time_ * 13.0f) * 2.5f + std::sin(time_ * 7.0f) * 1.5f;
	const float fcx = (fx1 + fx2) * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_ADD, 140);
	DrawCircleAA(fcx, y2 - 22.0f, 34.0f + flick, 24, GetColor(255, 110, 30), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawOvalAA(fcx, y2 - 22.0f, 18.0f, 16.0f + flick, 16, GetColor(255, 140, 40), TRUE);
	DrawOvalAA(fcx, y2 - 18.0f, 10.0f, 9.0f, 12, GetColor(255, 230, 120), TRUE);

	//のれんの入口
	const float dx = kOriginX + 21.5f * kTile;
	DrawBoxAA(dx - 20.0f, y2 - 58.0f, dx + 20.0f, y2, GetColor(40, 28, 20), TRUE);
	for (int k = 0; k < 3; k++)
	{
		const float nx = dx - 22.0f + k * 15.0f;
		const float sway = std::sin(time_ * 1.8f + k) * 1.5f;
		DrawQuadrangleAA(nx, y2 - 60.0f, nx + 14.0f, y2 - 60.0f, nx + 14.0f + sway, y2 - 34.0f, nx + sway, y2 - 34.0f, GetColor(40, 60, 120), TRUE);
	}
	DrawCircleAA(dx, y2 - 48.0f, 6.0f, 12, GetColor(240, 240, 240), FALSE, 2.0f);

	//金床
	const float ax = x1 + 76.0f;
	const float fy = y2 - 6.0f;
	const float anvilTop = fy - 20.0f;
	DrawBoxAA(ax - 8.0f, fy - 6.0f, ax + 10.0f, fy, GetColor(50, 50, 58), TRUE);
	DrawBoxAA(ax - 4.0f, anvilTop + 6.0f, ax + 6.0f, fy - 6.0f, GetColor(65, 65, 75), TRUE);
	DrawBoxAA(ax - 12.0f, anvilTop, ax + 14.0f, anvilTop + 6.0f, GetColor(80, 80, 92), TRUE);
	DrawTriangleAA(ax + 14.0f, anvilTop, ax + 14.0f, anvilTop + 6.0f, ax + 24.0f, anvilTop + 1.0f, GetColor(80, 80, 92), TRUE);
	//焼けた刀
	DrawLineAA(ax - 18.0f, anvilTop - 1.0f, ax + 18.0f, anvilTop - 2.0f, GetColor(255, 120, 50), 3.0f);
	DrawLineAA(ax - 24.0f, anvilTop - 1.0f, ax - 18.0f, anvilTop - 1.0f, GetColor(60, 40, 30), 3.0f);

	//刀鍛冶
	const float sx = x1 + 44.0f;
	const float period = 0.9f;
	const float p = std::fmod(time_, period) / period;
	const float strikeAngle = std::atan2(anvilTop - (fy - 36.0f), ax - (sx + 4.0f));
	const float upAngle = -2.2f;
	float angle = strikeAngle;
	if (p < 0.55f) angle = strikeAngle + (upAngle - strikeAngle) * std::sin(p / 0.55f * 1.5708f);
	else if (p < 0.7f) angle = upAngle + (strikeAngle - upAngle) * ((p - 0.55f) / 0.15f);
	DrawOvalAA(sx, fy, 16.0f, 4.0f, 12, GetColor(60, 50, 40), TRUE);
	DrawQuadrangleAA(sx - 9.0f, fy - 22.0f, sx + 9.0f, fy - 22.0f, sx + 13.0f, fy, sx - 13.0f, fy, GetColor(40, 45, 70), TRUE);
	DrawLineAA(sx, fy - 20.0f, sx, fy, GetColor(25, 28, 45), 1.5f);
	DrawBoxAA(sx - 9.0f, fy - 40.0f, sx + 9.0f, fy - 20.0f, GetColor(245, 245, 240), TRUE);
	DrawLineAA(sx - 6.0f, fy - 40.0f, sx + 2.0f, fy - 28.0f, GetColor(190, 190, 185), 1.5f);
	DrawLineAA(sx + 6.0f, fy - 40.0f, sx - 2.0f, fy - 28.0f, GetColor(190, 190, 185), 1.5f);
	DrawBoxAA(sx - 9.0f, fy - 24.0f, sx + 9.0f, fy - 21.0f, GetColor(40, 45, 70), TRUE);
	const float hy = fy - 48.0f;
	DrawCircleAA(sx, hy, 8.5f, 16, GetColor(240, 200, 165), TRUE);
	DrawCircleAA(sx + 4.0f, hy - 1.0f, 1.3f, 6, GetColor(40, 30, 30), TRUE);
	//えぼし
	DrawQuadrangleAA(sx - 8.0f, hy - 5.0f, sx + 7.0f, hy - 5.0f, sx + 2.0f, hy - 22.0f, sx - 6.0f, hy - 20.0f, GetColor(25, 25, 30), TRUE);
	DrawBoxAA(sx - 9.0f, hy - 7.0f, sx + 9.0f, hy - 4.0f, GetColor(25, 25, 30), TRUE);
	//うでと つち
	const float shx = sx + 4.0f;
	const float shy = fy - 36.0f;
	const float ca = std::cos(angle);
	const float sa = std::sin(angle);
	const float hx = shx + ca * 30.0f;
	const float hyy = shy + sa * 30.0f;
	DrawLineAA(shx, shy, shx + ca * 14.0f, shy + sa * 14.0f, GetColor(245, 245, 240), 5.0f);
	DrawLineAA(shx + ca * 12.0f, shy + sa * 12.0f, hx, hyy, GetColor(130, 90, 55), 3.0f);
	DrawLineAA(hx - sa * 7.0f, hyy + ca * 7.0f, hx + sa * 7.0f, hyy - ca * 7.0f, GetColor(70, 70, 80), 7.0f);

	//火花
	if (p >= 0.7f)
	{
		const float t = (p - 0.7f) / 0.3f;
		const int strike = static_cast<int>(time_ / period);
		const float hitX = ax + 2.0f;
		const float hitY = anvilTop - 2.0f;
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255 * (1.0f - t)));
		if (t < 0.3f) DrawCircleAA(hitX, hitY, 10.0f * (1.0f - t / 0.3f) + 4.0f, 12, GetColor(255, 220, 140), TRUE);
		for (int k = 0; k < 8; k++)
		{
			const unsigned int h = static_cast<unsigned int>(strike * 73856093 ^ (k + 1) * 19349663);
			const float r1 = static_cast<float>(h % 1000) / 1000.0f;
			const float r2 = static_cast<float>((h / 1000) % 1000) / 1000.0f;
			const float dir = -1.5708f + (r1 - 0.5f) * 2.6f;
			const float speed = 30.0f + r2 * 40.0f;
			const float spx = hitX + std::cos(dir) * speed * t;
			const float spy = hitY + std::sin(dir) * speed * t + 50.0f * t * t;
			DrawCircleAA(spx, spy, 2.0f, 6, GetColor(255, 200 - static_cast<int>(r2 * 80), 60), TRUE);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//けむり
	for (int k = 0; k < 3; k++)
	{
		const float t = std::fmod(time_ * 0.6f + k * 0.33f, 1.0f);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(120 * (1.0f - t)));
		DrawCircleAA(chx + t * 20.0f, y1 - 14.0f - t * 60.0f, 8.0f + t * 10.0f, 12, GetColor(200, 200, 200), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

void CampScene::DrawHud()
{
	const SaveManager& sm = SaveManager::GetInstance();
	const SaveSlot& s = sm.GetInstance().Slot(sm.CurrentIndex());

	int seen = 0;
	for (const auto& z : s.zukan) if (z.isSeen) seen++;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
	DrawRoundRectAA(30.0f, 660.0f, 1250.0f, 708.0f, 12.0f, 12.0f, 8, GetColor(15, 12, 25), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	std::wstring info = L"きろく" + std::to_wstring(sm.CurrentIndex() + 1) + L"　ゆうしゃ Lv" + std::to_wstring(s.run.isActive ? s.run.heroLevel : s.hero.level) + L"　ずかん " + std::to_wstring(seen) + L"/" + std::to_wstring(s.zukan.size()) +
		L"　とうたつ " + (s.deepestNormal > 0 ? L"B" + std::to_wstring(s.deepestNormal) + L"F" : std::wstring(L"ー"));
	if (s.isCleared) info += L"　★クリア　むげん " + (s.deepestInfinite > 0 ? L"B" + std::to_wstring(s.deepestInfinite) + L"F" : std::wstring(L"ー"));
	Font::DrawFit(50.0f, 672.0f, info, GetColor(240, 235, 255), FontSize::Small, GetColor(10, 10, 20), 985.0f);
	ButtonGuide::Draw(1236.0f - ButtonGuide::Width(L"{M}：メニュー", FontSize::Small), 684.0f, L"{M}：メニュー", GetColor(170, 160, 190), FontSize::Small, GetColor(10, 10, 20));
}

void CampScene::DrawStageSelect()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const float h = 90.0f + choices_.size() * 62.0f;
	DrawUtil::Window(330.0f, 170.0f, 950.0f, 170.0f + h);
	Font::DrawCenter(640.0f, 210.0f, L"どこへ いく？", GetColor(255, 230, 140), FontSize::Medium, GetColor(20, 15, 30));
	for (size_t i = 0; i < choices_.size(); i++)
	{
		const float y = 270.0f + i * 62.0f;
		const bool isSel = static_cast<int>(i) == cursor_ && mode_ == Mode::StageSelect;
		if (isSel) DrawUtil::Cursor(360.0f, y, time_);
		const unsigned int col = choices_[i].isLocked ? GetColor(120, 110, 140) : (isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230));
		Font::DrawFit(400.0f, y - 14.0f, choices_[i].text, col, FontSize::Small, GetColor(20, 15, 30), 530.0f);
	}

	if (mode_ == Mode::Confirm)
	{
		DrawUtil::Window(380.0f, 300.0f, 900.0f, 500.0f);
		Font::DrawCenter(640.0f, 350.0f, L"いまの ぼうけんの きろくは なくなります", GetColor(255, 200, 200), FontSize::Small, GetColor(20, 15, 30));
		Font::DrawCenter(640.0f, 385.0f, L"あたらしく はじめますか？", GetColor(255, 255, 255), FontSize::Small, GetColor(20, 15, 30));
		const wchar_t* c[2] = { L"はい", L"いいえ" };
		for (int i = 0; i < 2; i++)
		{
			const float bx = 540.0f + i * 200.0f;
			const bool isSel = i == cursor_;
			DrawUtil::Panel(bx - 70.0f, 420.0f, bx + 70.0f, 472.0f, isSel ? Game::Rgb{ 190, 60, 70 } : Game::Rgb{ 50, 45, 70 }, 240,
				isSel ? Game::Rgb{ 255, 230, 120 } : Game::Rgb{ 120, 110, 150 });
			Font::DrawCenter(bx, 446.0f, c[i], GetColor(255, 255, 255), FontSize::Medium, GetColor(20, 15, 30));
		}
	}
}
