#include "CampScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "DungeonScene.h"
#include "ZukanScene.h"
#include "FishZukanScene.h"
#include "FishingScene.h"
#include "FishData.h"
#include "SmithScene.h"
#include "SettingsScene.h"
#include "SaveData.h"
#include "HeroGrowth.h"
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
#include "AuraRenderer.h"
#include "Sprite.h"
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
	constexpr int kNoticeLines = 10;
	constexpr int kNoticeListRows = 8;		//いちらんで いちどに 見える 吹き出し
	constexpr float kNoticeBubbleH = 54.0f;
	constexpr float kNoticeLineH = 38.0f;
	constexpr float kNoticeWidth = 740.0f;

	//------------------------------//
	// ステージの くわしい じょうほう
	//------------------------------//

	/// <summary>
	/// ステージの なかの エリア(キラプルンの すみかは むずかしさ)
	/// </summary>
	struct AreaView
	{
		std::wstring name;
		int first;			//はじまりの階(すみかは むげんの どの階の まものか)
		int last;
		Game::Rgb color;
		int bossId;			//-1は ぬし なし
		std::vector<int> monsters;
	};

	const wchar_t* StageDesc(int stage)
	{
		switch (stage)
		{
		case Stage::kNormal:	return L"さいしょの ダンジョン。4かいごとに けしきが かわり、B12Fの おくで まおうが まちうけている。ここで なかまを ふやして つよく なろう。";
		case Stage::kInfinite:	return L"クリア後に ひらく エンドコンテンツ。20かいごとに エリアと ぬしが かわる。5かいごとに たきびの きゅうけいじょ、10かいごとに つよい まもの。";
		case Stage::kTraining:	return L"キラプルンが むれで すむ きんいろの どうくつ。しゅぎょうの きっぷを つかって はいる。そざい・おまもりは でないが、けいけんちが たくさん はいる。";
		case Stage::kAbyss:		return L"むげんの めいきゅうの さらに おく。くるった オーラを まとう まものたちが すむ。なかまには ならないが、しんえんの そざいを おとす。";
		default:				return L"";
		}
	}

	std::vector<AreaView> BuildAreas(int stage)
	{
		std::vector<AreaView> areas;
		if (stage == Stage::kNormal)
		{
			areas.push_back({ L"つちの どうくつ", 1, 4, { 150, 120, 90 }, -1, {} });
			areas.push_back({ L"みずの どうくつ", 5, 8, { 90, 130, 175 }, -1, {} });
			areas.push_back({ L"まおうの すみか", 9, Stage::kNormalLastFloor, { 150, 70, 100 }, -1, {} });
		}
		else if (stage == Stage::kInfinite)
		{
			const Game::Rgb colors[5] = { { 100, 90, 160 }, { 120, 170, 215 }, { 190, 80, 40 }, { 110, 60, 150 }, { 230, 210, 140 } };
			for (int i = 0; i < 5; i++) areas.push_back({ Stage::InfiniteArea(1 + i * 20), 1 + i * 20, 20 + i * 20, colors[i], -1, {} });
		}
		else if (stage == Stage::kTraining)
		{
			for (int i = 0; i < Stage::kTrainingRankNum; i++)
			{
				const Stage::TrainingRank& rk = Stage::Rank(i);
				areas.push_back({ rk.name, rk.baseFloor + 2, rk.baseFloor + Stage::kTrainingLastFloor * 2, { 210, 170, 70 }, -1, { Stage::kKiraPurun } });
			}
		}
		else if (stage == Stage::kAbyss)
		{
			const Game::Rgb colors[5] = { { 90, 70, 170 }, { 170, 160, 140 }, { 90, 150, 140 }, { 190, 130, 200 }, { 150, 40, 80 } };
			for (int i = 0; i * Stage::kAbyssLayerFloors + Stage::kAbyssFirstFloor <= Stage::kAbyssLastFloor; i++)
			{
				const int first = Stage::kAbyssFirstFloor + i * Stage::kAbyssLayerFloors;
				areas.push_back({ Stage::AbyssArea(first), first, first + Stage::kAbyssLayerFloors - 1, colors[i % 5], -1, {} });
			}
		}

		//その エリアの ぬしと まもの(むげんの 階で しらべる。しんえんは もとに なる むげんの 階)
		const bool isNormal = stage == Stage::kNormal;
		const bool isAbyss = stage == Stage::kAbyss;
		const auto& all = MonsterTable::All();
		for (AreaView& a : areas)
		{
			if (isAbyss) a.bossId = Stage::AbyssBossFor(a.last);
			else if (stage != Stage::kTraining) a.bossId = MonsterTable::BossFor(a.last, !isNormal);
			const int first = isAbyss ? Stage::AbyssSourceFloor(a.first) : a.first;
			const int last = isAbyss ? Stage::AbyssSourceFloor(a.last) : a.last;
			for (const MonsterData& m : all)
			{
				if (m.type == MonsterType::Boss || (m.id == Stage::kKiraPurun && stage != Stage::kNormal && stage != Stage::kInfinite)) continue;
				const int lo = isNormal ? m.minFloor : m.infMinFloor;
				const int hi = isNormal ? m.maxFloor : m.infMaxFloor;
				if (lo > 0 && lo <= last && hi >= first) a.monsters.push_back(m.id);
			}
		}
		return areas;
	}

	/// <summary>
	/// その階が ある エリア
	/// </summary>
	int AreaOf(int stage, int floor)
	{
		if (stage == Stage::kNormal) return std::clamp((floor - 1) / 4, 0, 2);
		if (stage == Stage::kInfinite) return std::clamp((floor - 1) / 20, 0, 4);
		if (stage == Stage::kAbyss) return std::clamp((floor - Stage::kAbyssFirstFloor) / Stage::kAbyssLayerFloors, 0, (Stage::kAbyssLastFloor - Stage::kAbyssFirstFloor) / Stage::kAbyssLayerFloors);
		return 0;
	}

	/// <summary>
	/// むげん・しんえんの はじまりの階と さいごの階
	/// </summary>
	int FirstFloorOf(int stage) { return stage == Stage::kAbyss ? Stage::kAbyssFirstFloor : 1; }
	int LastFloorOf(int stage) { return stage == Stage::kAbyss ? Stage::kAbyssLastFloor : Stage::kInfiniteLastFloor; }

	const Cell kTrees[] =
	{
		{ 8, 1 }, { 9, 2 }, { 16, 1 }, { 17, 1 }, { 1, 8 }, { 9, 12 }, { 22, 11 }, { 23, 12 }, { 21, 13 }, { 8, 13 }, { 24, 9 },
	};
}

CampScene::CampScene(SceneManager& manager, const Cell& start) :
	Scene(manager),
	startCell_(start)
{
}

void CampScene::Init()
{
	BuildMap();
	heroCell_ = IsBlocked(startCell_) ? Cell{ 13, 5 } : startCell_;
	if (heroCell_.x < kPond.x + kPond.w + 1 && heroCell_.y >= kPond.y) heroFacing_ = -1.0f;	//いけを みる
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

	//いけの まわり(うえ・した・みぎ)
	for (int x = kPond.x; x < kPond.x + kPond.w; x++)
	{
		spots_[kPond.y - 1][x] = Spot::Pond;
		spots_[kPond.y + kPond.h][x] = Spot::Pond;
	}
	for (int y = kPond.y; y < kPond.y + kPond.h; y++) spots_[y][kPond.x + kPond.w] = Spot::Pond;
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
		const std::vector<PauseItem> items = PauseItems();
		const int num = static_cast<int>(items.size());
		if (input.IsAnyTriggered(Button::Up)) { cursor_ = (cursor_ + num - 1) % num; Sound::GetInstance().PlaySE(SE::Cursor); }
		if (input.IsAnyTriggered(Button::Down)) { cursor_ = (cursor_ + 1) % num; Sound::GetInstance().PlaySE(SE::Cursor); }
		cursor_ = std::clamp(cursor_, 0, num - 1);
		const PauseItem item = items[cursor_];
		if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Menu) || (input.IsAnyTriggered(Button::Decide) && item == PauseItem::Close))
		{
			mode_ = Mode::Walk;
			Sound::GetInstance().PlaySE(SE::Cancel);
		}
		else if (!input.IsAnyTriggered(Button::Decide))
		{
		}
		else if (item == PauseItem::KeyItems)
		{
			mode_ = Mode::KeyItems;
			Sound::GetInstance().PlaySE(SE::Decide);
		}
		else if (item == PauseItem::Respec)
		{
			OpenRespec();
		}
		else if (item == PauseItem::Settings)
		{
			isLeaving_ = true;
			Sound::GetInstance().PlaySE(SE::Decide);
			SaveManager::GetInstance().Save();
			sceneManager_.ChangeSceneWithFade(std::make_shared<SettingsScene>(sceneManager_, SettingsScene::Return::Camp));
		}
		else if (item == PauseItem::Title)
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
	case Mode::RankSelect: UpdateRankSelect(); break;
	case Mode::Respec: UpdateRespec(); break;
	case Mode::PartySelect: UpdatePartySelect(); break;
	case Mode::LibraryMenu: UpdateLibraryMenu(); break;
	case Mode::FireMenu: UpdateFireMenu(); break;
	case Mode::Cook: UpdateCook(); break;
	case Mode::KeyItems:
		if (keyItemMenu_.Update()) mode_ = Mode::Pause;
		break;
	}
}

bool CampScene::HasUnreadNotice() const
{
	return UpdateLog::VersionCode(UpdateLog::Latest()) > SaveManager::GetInstance().GetSettings().readNotices;
}

int CampScene::NoticeCount() const
{
	return static_cast<int>(UpdateLog::All().size());
}

const UpdateEntry& CampScene::NoticeEntry(int index) const
{
	//0が いちばん あたらしい
	const auto& all = UpdateLog::All();
	return all[all.size() - 1 - index];
}

void CampScene::OpenNotice()
{
	isNoticeNew_ = HasUnreadNotice();
	Settings& st = SaveManager::GetInstance().GetSettings();
	noticeReadBefore_ = st.readNotices;
	st.readNotices = UpdateLog::VersionCode(UpdateLog::Latest());
	SaveManager::GetInstance().SaveSettings();

	//まずは バージョンの いちらん(吹き出し)
	noticeCursor_ = 0;
	noticeListTop_ = 0;
	noticeDetail_ = -1;
	noticeScroll_ = 0;
	mode_ = Mode::Notice;
	Sound::GetInstance().PlaySE(SE::Decide);
}

void CampScene::OpenNoticeDetail(int index)
{
	//えらんだ バージョンの ないようを 折りかえして ならべる
	noticeDetail_ = index;
	noticeScroll_ = 0;
	noticeRows_.clear();
	for (const std::wstring& line : NoticeEntry(index).lines)
	{
		const auto wrapped = Font::Wrap(L"・" + line, FontSize::Small, kNoticeWidth);
		for (size_t k = 0; k < wrapped.size(); k++) noticeRows_.push_back({ (k == 0 ? L"" : L"　") + wrapped[k], false });
	}
}

int CampScene::NoticeRepeatDir()
{
	//おしっぱなし・スティックを たおしたままでも つづけて うごく
	Input& input = Input::GetInstance();
	int dir = 0;
	for (int p = 0; p < Game::kPlayerNum; p++)
	{
		if (input.IsPressed(p, Button::Up)) dir = -1;
		if (input.IsPressed(p, Button::Down)) dir = 1;
	}
	if (input.IsAnyTriggered(Button::Up) || input.IsAnyTriggered(Button::Down))
	{
		noticeHold_ = 0.35f;
		return dir;
	}
	if (dir == 0) return 0;
	noticeHold_ -= Game::kFrameTime;
	if (noticeHold_ > 0.0f) return 0;
	noticeHold_ = 0.08f;
	return dir;
}

void CampScene::UpdateNotice()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = NoticeCount();
	const int dir = NoticeRepeatDir();

	//いちらん: 吹き出しを えらんで {A} で くわしく
	if (noticeDetail_ < 0)
	{
		const int next = std::clamp(noticeCursor_ + dir, 0, std::max(0, num - 1));
		if (next != noticeCursor_)
		{
			noticeCursor_ = next;
			sound.PlaySE(SE::Cursor);
		}
		noticeListTop_ = std::clamp(noticeListTop_, std::max(0, noticeCursor_ - kNoticeListRows + 1), noticeCursor_);
		if (input.IsAnyTriggered(Button::Cancel))
		{
			mode_ = Mode::Walk;
			sound.PlaySE(SE::Cancel);
		}
		else if (input.IsAnyTriggered(Button::Decide) && num > 0)
		{
			OpenNoticeDetail(noticeCursor_);
			sound.PlaySE(SE::Decide);
		}
		return;
	}

	//くわしく: 上下で スクロール、左右で となりの バージョン
	const int maxScroll = std::max(0, static_cast<int>(noticeRows_.size()) - kNoticeLines);
	const int next = std::clamp(noticeScroll_ + dir, 0, maxScroll);
	if (next != noticeScroll_)
	{
		noticeScroll_ = next;
		sound.PlaySE(SE::Cursor);
	}
	const int side = input.IsAnyTriggered(Button::Left) ? -1 : (input.IsAnyTriggered(Button::Right) ? 1 : 0);
	if (side != 0 && noticeDetail_ + side >= 0 && noticeDetail_ + side < num)
	{
		noticeCursor_ = noticeDetail_ + side;
		noticeListTop_ = std::clamp(noticeListTop_, std::max(0, noticeCursor_ - kNoticeListRows + 1), noticeCursor_);
		OpenNoticeDetail(noticeCursor_);
		sound.PlaySE(SE::Cursor);
	}
	if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Decide))
	{
		noticeDetail_ = -1;
		sound.PlaySE(SE::Cancel);
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
			mode_ = Mode::LibraryMenu;
			libraryCursor_ = 0;
			Sound::GetInstance().PlaySE(SE::Decide);
			return;
		case Spot::Pond:
			isLeaving_ = true;
			Sound::GetInstance().PlaySE(SE::Decide);
			sceneManager_.ChangeSceneWithFade(std::make_shared<FishingScene>(sceneManager_));
			return;
		case Spot::Smith:
			isLeaving_ = true;
			Sound::GetInstance().PlaySE(SE::Decide);
			sceneManager_.ChangeSceneWithFade(std::make_shared<SmithScene>(sceneManager_));
			return;
		case Spot::Fire:
			mode_ = Mode::FireMenu;
			fireCursor_ = 0;
			Sound::GetInstance().PlaySE(SE::Decide);
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

	//キラプルンの すみか(むげん B20F の ぬしを たおすと かいほう)
	if (s.IsTrainingOpen())
	{
		const std::wstring mark = s.IsTrainingNew() ? L"　NEW！" : L"";
		choices_.push_back({ std::wstring(Stage::Name(Stage::kTraining)) + L"（しゅぎょう・きっぷ " + std::to_wstring(s.tickets) + L"まい）" + mark, Stage::kTraining, false });
	}
	else if (s.isCleared)
	{
		choices_.push_back({ L"？？？（むげん B20Fの ぬしを たおすと かいほう）", Stage::kTraining, true });
	}

	//しんえん(むげん B100F を とうはした えいゆうの あかしで かいほう)
	if (s.hasRelic)
	{
		choices_.push_back({ std::wstring(Stage::Name(Stage::kAbyss)) + L"（B" + std::to_wstring(Stage::kAbyssFirstFloor) + L"〜B" + std::to_wstring(Stage::kAbyssLastFloor) + L"F）", Stage::kAbyss, false });
	}
	else if (s.isCleared)
	{
		choices_.push_back({ L"？？？（むげん B100Fの ぬしを たおすと かいほう）", Stage::kAbyss, true });
	}
	choices_.push_back({ L"やめる", -2, false });
	cursor_ = 0;
	stageCursor_ = 0;
	ResetAreaCursor();
	mode_ = Mode::StageSelect;

	//行き先を見たので「！」を消す
	if (s.IsNew(NewsBit::kInfinite) || s.IsTrainingNew())
	{
		SaveManager::GetInstance().Current().seenNews |= NewsBit::kInfinite | (s.IsTrainingOpen() ? NewsBit::kTraining : 0);
		SaveManager::GetInstance().Save();
	}
}

void CampScene::DrawNewMarks()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();

	//建物の左上
	if (s.IsNew(NewsBit::kInfinite) || s.IsTrainingNew()) DrawUtil::NewMark(kOriginX + kCave.x * kTile + 18.0f, 26.0f, time_);
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
			cursor_ = stageCursor_;
			sound.PlaySE(SE::Cancel);
		}
		else if (input.IsAnyTriggered(Button::Decide))
		{
			StartDungeon(pendingStage_, false);
		}
		return;
	}

	const int num = static_cast<int>(choices_.size());
	const int prev = cursor_;
	if (input.IsAnyTriggered(Button::Up)) { cursor_ = (cursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { cursor_ = (cursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	stageCursor_ = cursor_;
	if (cursor_ != prev) ResetAreaCursor();

	//左右で エリアを えらんで くわしく 見る
	const int stage = ChoiceStage(cursor_);
	const int areaNum = (stage >= 0 && !choices_[cursor_].isLocked) ? static_cast<int>(BuildAreas(stage).size()) : 0;
	if (areaNum > 1)
	{
		if (input.IsAnyTriggered(Button::Left)) { areaCursor_ = (areaCursor_ + areaNum - 1) % areaNum; sound.PlaySE(SE::Cursor); }
		if (input.IsAnyTriggered(Button::Right)) { areaCursor_ = (areaCursor_ + 1) % areaNum; sound.PlaySE(SE::Cursor); }
	}

	if (input.IsAnyTriggered(Button::Cancel)) { mode_ = Mode::Walk; sound.PlaySE(SE::Cancel); return; }
	if (!input.IsAnyTriggered(Button::Decide)) return;

	const StageChoice& c = choices_[cursor_];
	if (c.isLocked) { sound.PlaySE(SE::Buzzer); return; }
	if (c.stage == -2) { mode_ = Mode::Walk; sound.PlaySE(SE::Cancel); return; }
	if (c.stage == -1) { StartDungeon(SaveManager::GetInstance().Current().run.stage, true); return; }

	//キラプルンの すみかは むずかしさを えらぶ
	if (c.stage == Stage::kTraining)
	{
		rankCursor_ = std::clamp(areaCursor_, 0, Stage::kTrainingRankNum - 1);
		mode_ = Mode::RankSelect;
		sound.PlaySE(SE::Decide);
		return;
	}

	//むげんの めいきゅう・しんえんは たどりついた休憩所の次の階から はじめられる
	if (c.stage == Stage::kInfinite || c.stage == Stage::kAbyss)
	{
		const SaveSlot& s = SaveManager::GetInstance().Current();
		const bool isAbyss = c.stage == Stage::kAbyss;
		const int first = FirstFloorOf(c.stage);
		const int lastFloor = LastFloorOf(c.stage);
		const int deepest = isAbyss ? s.deepestAbyss : s.deepestInfinite;
		floorChoices_.clear();
		floorStage_ = c.stage;
		for (int f = first; f <= std::min(deepest, lastFloor); f += Stage::kRestInterval) floorChoices_.push_back(f);
		if (floorChoices_.empty()) floorChoices_.push_back(first);

		//前回の つづき(さいしょの こうもく)
		lastFloorChoice_ = 0;
		const int last = std::min(isAbyss ? s.lastAbyss : s.lastInfinite, lastFloor);
		if (last > first)
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
		BeginNewRun(c.stage, floorChoices_[0]);
		return;
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
	BeginNewRun(floorStage_, floorChoices_[floorCursor_]);
}

void CampScene::UpdateRankSelect()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	if (input.IsAnyTriggered(Button::Up)) { rankCursor_ = (rankCursor_ + Stage::kTrainingRankNum - 1) % Stage::kTrainingRankNum; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { rankCursor_ = (rankCursor_ + 1) % Stage::kTrainingRankNum; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel)) { mode_ = Mode::StageSelect; sound.PlaySE(SE::Cancel); return; }
	if (!input.IsAnyTriggered(Button::Decide)) return;

	//きっぷが たりなければ はいれない
	if (SaveManager::GetInstance().Current().tickets < Stage::Rank(rankCursor_).ticketCost)
	{
		sound.PlaySE(SE::Buzzer);
		return;
	}
	mode_ = Mode::StageSelect;
	BeginNewRun(Stage::kTraining, 1);
}

void CampScene::DrawRankSelect()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawUtil::Window(340.0f, 150.0f, 940.0f, 300.0f + Stage::kTrainingRankNum * 84.0f);
	Font::DrawCenter(640.0f, 190.0f, L"むずかしさを えらぶ", GetColor(255, 230, 140), FontSize::Medium, GetColor(20, 15, 30));
	Font::DrawCenterFit(640.0f, 232.0f, L"もっている きっぷ：" + std::to_wstring(s.tickets) + L"まい", GetColor(255, 220, 130), FontSize::Small, GetColor(20, 15, 30), 560.0f);

	for (int i = 0; i < Stage::kTrainingRankNum; i++)
	{
		const Stage::TrainingRank& rk = Stage::Rank(i);
		const float y = 290.0f + i * 84.0f;
		const bool isSel = i == rankCursor_;
		const bool canPay = s.tickets >= rk.ticketCost;
		if (isSel) DrawUtil::Cursor(370.0f, y, time_);
		const unsigned int col = !canPay ? GetColor(120, 110, 140) : (isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230));
		const std::wstring head = std::wstring(rk.name) + L"　きっぷ " + std::to_wstring(rk.ticketCost) + L"まい";
		Font::DrawFit(410.0f, y - 14.0f, head, col, FontSize::Small, GetColor(20, 15, 30), 500.0f);
		const std::wstring sub = L"レベル" + std::to_wstring(rk.levelCap) + L"まで あがりやすい（まもの：むげん B" +
			std::to_wstring(rk.baseFloor + 2) + L"〜" + std::to_wstring(rk.baseFloor + Stage::kTrainingLastFloor * 2) + L"F くらい）";
		Font::DrawFit(430.0f, y + 18.0f, sub, canPay ? GetColor(180, 200, 230) : GetColor(110, 105, 130), FontSize::Small, GetColor(20, 15, 30), 490.0f);
	}
	Font::DrawCenter(640.0f, 280.0f + Stage::kTrainingRankNum * 84.0f, L"きっぷは むげんの めいきゅうの まものが おとす", GetColor(170, 160, 190), FontSize::Small, GetColor(20, 15, 30));
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
			L"B" + std::to_wstring(f) + L"F から（" + (floorStage_ == Stage::kAbyss ? Stage::AbyssArea(f) : Stage::InfiniteArea(f)) + L"）";
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
	const int startFloor = (!isContinue && (stage == Stage::kInfinite || stage == Stage::kAbyss)) ? pendingFloor_ : 1;
	const std::vector<PartyMember> party = isContinue ? std::vector<PartyMember>() : partySel_;
	sceneManager_.ChangeSceneWithFade(std::make_shared<DungeonScene>(sceneManager_, stage, isContinue, startFloor, party, rankCursor_));
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
		case Spot::Library: text = L"{A}：しょこ"; break;
		case Spot::Pond: text = L"{A}：つりを する"; break;
		case Spot::Smith: text = L"{A}：かじや"; break;
		case Spot::Fire: text = L"{A}：たきび（りょうり・せってい）"; break;
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
	if (mode_ == Mode::RankSelect) DrawRankSelect();

	if (mode_ == Mode::Pause)
	{
		const std::vector<PauseItem> items = PauseItems();
		const float h = 60.0f + items.size() * 70.0f;
		DrawUtil::Window(420.0f, 360.0f - h * 0.5f, 860.0f, 360.0f + h * 0.5f);
		for (size_t i = 0; i < items.size(); i++)
		{
			const float y = 360.0f - h * 0.5f + 65.0f + i * 70.0f;
			const wchar_t* text = PauseItemName(items[i]);
			const bool isSel = static_cast<int>(i) == cursor_;
			if (isSel) DrawUtil::CursorBeside(640.0f, y, text, FontSize::Medium, time_);
			Font::DrawCenter(640.0f, y, text, isSel ? GetColor(255, 240, 150) : GetColor(190, 180, 210), FontSize::Medium, GetColor(20, 15, 30));
		}
	}
	if (mode_ == Mode::Respec) DrawRespec();

	if (mode_ == Mode::Notice) DrawNotice();
	if (mode_ == Mode::KeyItems) keyItemMenu_.Draw();
	if (mode_ == Mode::PartySelect) DrawPartySelect();
	if (mode_ == Mode::LibraryMenu) DrawLibraryMenu();
	if (mode_ == Mode::FireMenu) DrawFireMenu();
	if (mode_ == Mode::Cook) DrawCook();
	if (mode_ == Mode::Unlock) DrawUnlock();
}

//==============================//
// たきびの りょうり
//==============================//

namespace
{
	constexpr int kCookRows = 6;	//いちどに 見える さかな
	constexpr float kCookRowH = 64.0f;

	/// <summary>
	/// もっている なかで いちばん おおきい さかな(-1は もっていない)
	/// </summary>
	int BiggestStock(const FishRecord& rec)
	{
		int best = -1;
		for (size_t i = 0; i < rec.stock.size(); i++)
		{
			if (best < 0 || rec.stock[i] > rec.stock[best]) best = static_cast<int>(i);
		}
		return best;
	}
}

std::wstring CampScene::PendingMealText() const
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	if (s.mealFish < 0) return L"";
	return std::wstring(FishTable::All()[s.mealFish].dish) + L"（" + FishTable::MealText(s.mealFish, s.mealSize) + L"）";
}

void CampScene::UpdateFireMenu()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	constexpr int kItemNum = 3;
	if (input.IsAnyTriggered(Button::Up)) { fireCursor_ = (fireCursor_ + kItemNum - 1) % kItemNum; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { fireCursor_ = (fireCursor_ + 1) % kItemNum; sound.PlaySE(SE::Cursor); }

	if (input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && fireCursor_ == 2))
	{
		mode_ = Mode::Walk;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	if (fireCursor_ == 0)
	{
		OpenCook();
		return;
	}
	isLeaving_ = true;
	sound.PlaySE(SE::Decide);
	sceneManager_.ChangeSceneWithFade(std::make_shared<SettingsScene>(sceneManager_, SettingsScene::Return::Camp));
}

void CampScene::DrawFireMenu()
{
	const unsigned int edge = GetColor(20, 15, 30);
	DrawUtil::Window(400.0f, 190.0f, 880.0f, 520.0f);
	Font::DrawCenter(640.0f, 230.0f, L"たきび", GetColor(255, 230, 140), FontSize::Medium, edge);

	const wchar_t* items[3] = { L"りょうりする", L"せってい", L"やめる" };
	for (int i = 0; i < 3; i++)
	{
		const float y = 300.0f + i * 56.0f;
		if (i == fireCursor_) DrawUtil::CursorBeside(640.0f, y, items[i], FontSize::Small, time_);
		Font::DrawCenter(640.0f, y, items[i], i == fireCursor_ ? GetColor(255, 240, 150) : GetColor(190, 180, 210), FontSize::Small, edge);
	}

	const std::wstring meal = PendingMealText();
	Font::DrawCenterFit(640.0f, 450.0f, meal.empty() ? std::wstring(L"つった さかなを りょうりすると つぎの ぼうけんで たべられる") : L"つぎの ぼうけんで たべる：" + meal,
		meal.empty() ? GetColor(170, 160, 190) : GetColor(255, 210, 150), FontSize::Small, edge, 450.0f);
	ButtonGuide::DrawCenter(640.0f, 490.0f, L"{A}：けってい　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}

void CampScene::OpenCook()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	cookList_.clear();
	for (size_t i = 0; i < s.fish.size() && i < FishTable::All().size(); i++)
	{
		if (!s.fish[i].stock.empty()) cookList_.push_back(static_cast<int>(i));
	}
	if (cookList_.empty())
	{
		Sound::GetInstance().PlaySE(SE::Buzzer);
		mode_ = Mode::Walk;
		Effects::GetInstance().Popup(heroPos_ + Vec2(0.0f, -80.0f), L"さかなを もっていない（いけで つろう）", { 255, 220, 200 }, FontSize::Small);
		return;
	}
	cookCursor_ = 0;
	cookTop_ = 0;
	mode_ = Mode::Cook;
	Sound::GetInstance().PlaySE(SE::Decide);
}

void CampScene::UpdateCook()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = static_cast<int>(cookList_.size());
	if (input.IsAnyTriggered(Button::Up) && num > 1) { cookCursor_ = (cookCursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down) && num > 1) { cookCursor_ = (cookCursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	cookTop_ = std::clamp(cookTop_, std::max(0, cookCursor_ - kCookRows + 1), cookCursor_);

	if (input.IsAnyTriggered(Button::Cancel))
	{
		mode_ = Mode::FireMenu;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	SaveSlot& s = SaveManager::GetInstance().Current();
	const int fish = cookList_[cookCursor_];
	FishRecord& rec = s.fish[fish];
	const int pick = BiggestStock(rec);
	if (pick < 0) return;

	//まえに つくった りょうりは さかなに もどす(もてる ぶんだけ)
	if (s.mealFish >= 0 && s.mealFish < static_cast<int>(s.fish.size()))
	{
		std::vector<int>& back = s.fish[s.mealFish].stock;
		if (static_cast<int>(back.size()) < FishTable::kMaxStock) back.push_back(s.mealSize);
	}
	s.mealFish = fish;
	s.mealSize = rec.stock[pick];
	rec.stock.erase(rec.stock.begin() + pick);
	SaveManager::GetInstance().Save();

	sound.PlaySE(SE::Fanfare);
	const Vec2 fp = CellToPos(kFire);
	Effects::GetInstance().Burst(fp, 18, { 255, 190, 90 }, 60.0f, 180.0f, 5.0f, 0.8f, ParticleShape::Star, -60.0f, true);
	Effects::GetInstance().Popup(fp + Vec2(0.0f, -70.0f), std::wstring(FishTable::All()[fish].dish) + L"が できた！", { 255, 220, 140 }, FontSize::Small);
	mode_ = Mode::Walk;
}

void CampScene::DrawCook()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const unsigned int edge = GetColor(20, 15, 30);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawUtil::Window(250.0f, 70.0f, 1030.0f, 650.0f);
	Font::DrawCenter(640.0f, 110.0f, L"なにを りょうりする？", GetColor(255, 230, 140), FontSize::Medium, edge);
	Font::DrawCenterFit(640.0f, 152.0f, L"つぎの あたらしい ぼうけんの あいだ きく（1かいに 1しな・おおきい さかなほど すこし つよい）",
		GetColor(190, 180, 210), FontSize::Small, edge, 740.0f);

	const int num = static_cast<int>(cookList_.size());
	const int end = std::min(num, cookTop_ + kCookRows);
	for (int i = cookTop_; i < end; i++)
	{
		const int fish = cookList_[i];
		const FishData& d = FishTable::All()[fish];
		const FishRecord& rec = s.fish[fish];
		const int pick = BiggestStock(rec);
		const int size = pick >= 0 ? rec.stock[pick] : d.minSize;
		const float y = 200.0f + (i - cookTop_) * kCookRowH;
		const bool isSel = i == cookCursor_;
		if (isSel)
		{
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
			DrawBoxAA(290.0f, y, 990.0f, y + kCookRowH - 6.0f, GetColor(255, 220, 120), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			DrawUtil::Cursor(272.0f, y + kCookRowH * 0.5f - 3.0f, time_);
		}
		FishDraw::Fish({ 340.0f, y + kCookRowH * 0.5f - 3.0f }, 64.0f, d, isSel ? time_ : 0.0f, 1.0f, false);
		Font::DrawFit(392.0f, y + 2.0f, std::wstring(d.dish) + L"　" + FishTable::MealText(fish, size), isSel ? GetColor(255, 240, 150) : GetColor(235, 230, 250), FontSize::Small, edge, 580.0f);
		Font::DrawFit(392.0f, y + 30.0f, std::wstring(d.name) + L" " + std::to_wstring(size) + L"cm を つかう　（もちもの " + std::to_wstring(rec.stock.size()) + L"ひき）",
			GetColor(170, 165, 195), FontSize::Small, edge, 580.0f);
	}

	const float bob = std::sin(time_ * 6.0f) * 3.0f;
	if (cookTop_ > 0) DrawTriangleAA(1000.0f, 196.0f + bob, 990.0f, 212.0f + bob, 1010.0f, 212.0f + bob, GetColor(255, 230, 120), TRUE);
	if (end < num) DrawTriangleAA(990.0f, 574.0f - bob, 1010.0f, 574.0f - bob, 1000.0f, 590.0f - bob, GetColor(255, 230, 120), TRUE);

	const std::wstring meal = PendingMealText();
	if (!meal.empty()) Font::DrawCenterFit(640.0f, 592.0f, L"いまの りょうり：" + meal + L"（つくりなおすと さかなに もどる）", GetColor(255, 210, 150), FontSize::Small, edge, 740.0f);
	ButtonGuide::DrawCenter(640.0f, 624.0f, L"{UD}：えらぶ　{A}：りょうりする　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}

void CampScene::UpdateLibraryMenu()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	constexpr int kItemNum = 3;
	if (input.IsAnyTriggered(Button::Up)) { libraryCursor_ = (libraryCursor_ + kItemNum - 1) % kItemNum; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { libraryCursor_ = (libraryCursor_ + 1) % kItemNum; sound.PlaySE(SE::Cursor); }

	if (input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && libraryCursor_ == 2))
	{
		mode_ = Mode::Walk;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	isLeaving_ = true;
	sound.PlaySE(SE::Decide);
	if (libraryCursor_ == 0) sceneManager_.ChangeSceneWithFade(std::make_shared<ZukanScene>(sceneManager_));
	else sceneManager_.ChangeSceneWithFade(std::make_shared<FishZukanScene>(sceneManager_));
}

void CampScene::DrawLibraryMenu()
{
	const unsigned int edge = GetColor(20, 15, 30);
	const SaveSlot& s = SaveManager::GetInstance().Current();
	DrawUtil::Window(440.0f, 210.0f, 840.0f, 500.0f);
	Font::DrawCenter(640.0f, 250.0f, L"しょこ", GetColor(255, 230, 140), FontSize::Medium, edge);

	const wchar_t* items[3] = { L"モンスターずかん", L"つりずかん", L"やめる" };
	for (int i = 0; i < 3; i++)
	{
		const float y = 320.0f + i * 56.0f;
		if (i == libraryCursor_) DrawUtil::CursorBeside(640.0f, y, items[i], FontSize::Small, time_);
		Font::DrawCenter(640.0f, y, items[i], i == libraryCursor_ ? GetColor(255, 240, 150) : GetColor(190, 180, 210), FontSize::Small, edge);
	}
	//まだ見ていない色違いのページ
	if (s.IsNew(NewsBit::kShinyZukan)) DrawUtil::NewMark(640.0f + Font::Width(items[0], FontSize::Small) * 0.5f + 20.0f, 320.0f, time_, 13.0f);
	ButtonGuide::DrawCenter(640.0f, 470.0f, L"{A}：けってい　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}

void CampScene::DrawNoticeBoard()
{
	//けいじばん
	const Vec2 c = CellToPos(kBoard);
	constexpr int kBoardTile = 83;	//かんばん(Kenney Tiny Town)
	Sprite::DrawRect(Sprite::Tile(SpriteSheet::Town, kBoardTile), c.x - kTile * 0.5f, c.y - kTile * 0.5f, kTile, kTile);

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
	const unsigned int edge = GetColor(20, 15, 30);
	const float bob = std::sin(time_ * 6.0f) * 3.0f;
	const int num = NoticeCount();

	//吹き出し1つ(アップデートは金、パッチは青。左に しっぽ)
	auto bubble = [&](float left, float top, float right, float bottom, bool isPatch, bool isSel)
	{
		const unsigned int fill = isPatch ? GetColor(40, 62, 105) : GetColor(110, 78, 25);
		const unsigned int frame = isSel ? GetColor(255, 235, 140) : (isPatch ? GetColor(110, 160, 230) : GetColor(220, 170, 70));
		const float midY = (top + bottom) * 0.5f;
		DrawTriangleAA(left + 2.0f, midY - 9.0f, left + 2.0f, midY + 9.0f, left - 14.0f, midY + 4.0f, frame, TRUE);
		DrawRoundRectAA(left, top, right, bottom, 14.0f, 14.0f, 8, fill, TRUE);
		DrawRoundRectAA(left, top, right, bottom, 14.0f, 14.0f, 8, frame, FALSE, isSel ? 3.0f : 2.0f);
		DrawTriangleAA(left + 4.0f, midY - 7.0f, left + 4.0f, midY + 7.0f, left - 9.0f, midY + 3.0f, fill, TRUE);
	};
	auto title = [](const UpdateEntry& e)
	{
		return (UpdateLog::IsPatch(e.version) ? L"パッチ　ver" : L"アップデート　ver") + e.version;
	};

	if (num == 0)
	{
		Font::DrawCenter(640.0f, 112.0f, L"おしらせ", GetColor(255, 230, 140), FontSize::Medium, edge);
		Font::DrawCenter(640.0f, 340.0f, L"おしらせは まだ ありません", GetColor(235, 230, 250), FontSize::Small, edge);
		ButtonGuide::DrawCenter(640.0f, 610.0f, L"{B}：とじる", GetColor(160, 150, 180), FontSize::Small, edge);
		return;
	}

	//いちらん
	if (noticeDetail_ < 0)
	{
		Font::DrawCenter(640.0f, 112.0f, L"おしらせ", GetColor(255, 230, 140), FontSize::Medium, edge);
		if (isNoticeNew_) Font::DrawCenter(772.0f, 112.0f, L"NEW!", GetColor(255, 110, 90), FontSize::Small, edge);

		const int end = std::min(num, noticeListTop_ + kNoticeListRows);
		for (int i = noticeListTop_; i < end; i++)
		{
			const UpdateEntry& e = NoticeEntry(i);
			const bool isPatch = UpdateLog::IsPatch(e.version);
			const bool isSel = i == noticeCursor_;
			const float top = 150.0f + (i - noticeListTop_) * kNoticeBubbleH;
			const float left = isSel ? 282.0f : 290.0f;
			bubble(left, top, 1030.0f, top + kNoticeBubbleH - 10.0f, isPatch, isSel);
			if (isSel) DrawUtil::Cursor(248.0f, top + 22.0f, time_);

			const std::wstring head = title(e);
			Font::DrawFit(left + 18.0f, top + 7.0f, head, isPatch ? GetColor(170, 215, 255) : GetColor(255, 220, 120), FontSize::Small, edge, 300.0f);

			//1ぎょうめを ちょっとだけ(けんすうも)
			const std::wstring peek = (e.lines.empty() ? L"" : e.lines[0]) + L"（" + std::to_wstring(e.lines.size()) + L"けん）";
			Font::DrawFit(left + 330.0f, top + 7.0f, peek, GetColor(215, 210, 230), FontSize::Small, edge, 1030.0f - left - 350.0f);

			//まだ よんでいない バージョン
			if (UpdateLog::VersionCode(e.version) > noticeReadBefore_)
			{
				Font::DrawCenter(1030.0f, top + 2.0f, L"NEW!", GetColor(255, 110, 90), FontSize::Small, edge);
			}
		}

		if (noticeListTop_ > 0) DrawTriangleAA(1055.0f, 150.0f + bob, 1045.0f, 166.0f + bob, 1065.0f, 166.0f + bob, GetColor(255, 230, 120), TRUE);
		if (end < num) DrawTriangleAA(1045.0f, 570.0f - bob, 1065.0f, 570.0f - bob, 1055.0f, 586.0f - bob, GetColor(255, 230, 120), TRUE);
		ButtonGuide::DrawCenter(640.0f, 610.0f, L"{UD}：えらぶ　{A}：くわしく みる　{B}：とじる", GetColor(160, 150, 180), FontSize::Small, edge);
		return;
	}

	//くわしく
	const UpdateEntry& e = NoticeEntry(noticeDetail_);
	const bool isPatch = UpdateLog::IsPatch(e.version);
	bubble(300.0f, 92.0f, 980.0f, 140.0f, isPatch, true);
	Font::DrawCenter(640.0f, 116.0f, title(e).c_str(), isPatch ? GetColor(170, 215, 255) : GetColor(255, 220, 120), FontSize::Medium, edge);
	Font::DrawCenter(1010.0f, 116.0f, (std::to_wstring(noticeDetail_ + 1) + L"/" + std::to_wstring(num)).c_str(), GetColor(170, 160, 190), FontSize::Small, edge);

	//となりの バージョンへの やじるし(左が あたらしい)
	if (noticeDetail_ > 0) DrawTriangleAA(250.0f + bob, 116.0f, 266.0f + bob, 104.0f, 266.0f + bob, 128.0f, GetColor(255, 230, 120), TRUE);
	if (noticeDetail_ + 1 < num) DrawTriangleAA(1030.0f - bob, 116.0f, 1014.0f - bob, 104.0f, 1014.0f - bob, 128.0f, GetColor(255, 230, 120), TRUE);

	const int font = Font::Get(FontSize::Small);
	const int end = std::min(static_cast<int>(noticeRows_.size()), noticeScroll_ + kNoticeLines);
	for (int i = noticeScroll_; i < end; i++)
	{
		const float y = 170.0f + (i - noticeScroll_) * kNoticeLineH;
		DrawStringToHandle(262, static_cast<int>(y + 2.0f), noticeRows_[i].text.c_str(), GetColor(235, 230, 250), font, edge);
	}
	if (noticeScroll_ > 0) DrawTriangleAA(1050.0f, 170.0f + bob, 1040.0f, 186.0f + bob, 1060.0f, 186.0f + bob, GetColor(255, 230, 120), TRUE);
	if (end < static_cast<int>(noticeRows_.size())) DrawTriangleAA(1040.0f, 560.0f - bob, 1060.0f, 560.0f - bob, 1050.0f, 576.0f - bob, GetColor(255, 230, 120), TRUE);

	ButtonGuide::DrawCenter(640.0f, 610.0f, L"{UD}：スクロール　{LR}：ほかの バージョン　{A} / {B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}

namespace
{
	//------------------------------//
	// むらの ドット絵(Kenney Tiny Town の ばんごう)
	//------------------------------//
	constexpr int kGrassTiles[3] = { 0, 1, 2 };		//くさ・くさむら・はな
	constexpr int kPathTile = 43;					//いしだたみ
	constexpr int kTreeTiles[3] = { 16, 5, 16 };	//まるい木・しげみ(とがった木は くさに とけこむので つかわない)
	constexpr int kLibrarianTile = 100;				//しょこばん(Tiny Dungeon)
	constexpr int kSmithTile = 86;					//かじや(Tiny Dungeon)
	constexpr int kAnvilTile = 74;
	constexpr int kHammerTile = 117;

	/// <summary>
	/// いえの タイル(やね・やねの した・やねの まんなか・かべ・まど・とびら)。はしは 左・まんなか・右
	/// </summary>
	struct HouseStyle
	{
		int roof[3];
		int roofLow[3];
		int gable;
		int wall[3];
		int window;
		int door;
	};
	constexpr HouseStyle kStoneHouse = { { 48, 49, 50 }, { 60, 61, 62 }, 63, { 76, 77, 79 }, 88, 90 };
	constexpr HouseStyle kWoodHouse = { { 52, 53, 54 }, { 64, 65, 66 }, 67, { 72, 73, 75 }, 84, 86 };

	void DrawTownTile(int index, float x, float y)
	{
		Sprite::DrawRect(Sprite::Tile(SpriteSheet::Town, index), x, y, kTile, kTile);
	}

	/// <summary>
	/// いえを タイルで くみたてる(上 2だんが やね、のこりが かべ。いちばん下の doorCol に とびら)
	/// </summary>
	void DrawHouse(const HouseStyle& st, float x0, float y0, int w, int h, int doorCol)
	{
		const int roofRows = h >= 3 ? 2 : 1;
		for (int r = 0; r < h; r++)
		{
			for (int c = 0; c < w; c++)
			{
				const int edge = c == 0 ? 0 : (c == w - 1 ? 2 : 1);
				int tile = st.wall[edge];
				if (r == 0) tile = st.roof[edge];
				else if (r < roofRows) tile = c == w / 2 ? st.gable : st.roofLow[edge];
				else if (r == h - 1 && c == doorCol) tile = st.door;
				else if (r == h - 2 && edge == 1 && c % 2 == 1) tile = st.window;
				DrawTownTile(tile, x0 + c * kTile, y0 + r * kTile);
			}
		}
	}

	void DrawPerson(int tile, const Vec2& feet, float scale, float bob, bool isFlip)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
		DrawOvalAA(feet.x, feet.y, 15.0f, 4.0f, 12, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		Sprite::DrawFeet(Sprite::Tile(SpriteSheet::Dungeon, tile), feet - Vec2(0.0f, bob), scale, scale, isFlip);
	}
}

void CampScene::DrawGround()
{
	for (int y = 0; y < kRows; y++)
	{
		for (int x = 0; x < kCols; x++)
		{
			const float sx = kOriginX + x * kTile;
			const float sy = y * kTile;
			int tile = kGrassTiles[0];
			if (path_[y][x]) tile = kPathTile;
			else if ((x * 7 + y * 3) % 5 == 0) tile = kGrassTiles[1];
			else if ((x * 13 + y * 7) % 11 == 0) tile = kGrassTiles[2];
			DrawTownTile(tile, sx, sy);
		}
	}

	//外周の木
	auto tree = [](int x, int y)
	{
		DrawTownTile(kTreeTiles[(x * 3 + y * 5) % 3], kOriginX + x * kTile, y * kTile);
	};
	for (int x = 0; x < kCols; x++)
	{
		if (x < kCave.x || x >= kCave.x + kCave.w) tree(x, 0);
		tree(x, kRows - 1);
	}
	for (int y = 1; y < kRows - 1; y++)
	{
		tree(0, y);
		tree(kCols - 1, y);
	}
	for (const Cell& t : kTrees) tree(t.x, t.y);

	//池
	const float px = kOriginX + (kPond.x + kPond.w * 0.5f) * kTile;
	const float py = (kPond.y + kPond.h * 0.5f) * kTile;
	DrawOvalAA(px, py + 4.0f, kPond.w * kTile * 0.5f + 4.0f, kPond.h * kTile * 0.45f + 4.0f, 40, GetColor(62, 39, 49), TRUE);
	DrawOvalAA(px, py, kPond.w * kTile * 0.5f, kPond.h * kTile * 0.45f, 40, GetColor(76, 150, 220), TRUE);
	DrawOvalAA(px, py, kPond.w * kTile * 0.5f, kPond.h * kTile * 0.45f, 40, GetColor(170, 220, 250), FALSE, 3.0f);
	for (int k = 0; k < 3; k++)
	{
		const float wx = px - 50.0f + k * 45.0f + std::sin(time_ + k) * 6.0f;
		DrawBoxAA(wx, py - 11.0f + k * 12.0f, wx + 18.0f, py - 8.0f + k * 12.0f, GetColor(190, 230, 255), TRUE);
	}
}

void CampScene::DrawBuildings()
{
	//ダンジョンの入口(いしの もん)
	{
		const float x1 = kOriginX + kCave.x * kTile;
		const float x2 = x1 + kCave.w * kTile;
		const float y2 = (kCave.y + kCave.h) * kTile;
		DrawBoxAA(x1 + kTile, kTile, x2 - kTile, y2, GetColor(20, 14, 20), TRUE);
		const int rows[3][4] = { { 96, 97, 97, 98 }, { 108, 113, 114, 110 }, { 120, -1, -1, 122 } };
		for (int r = 0; r < 3; r++)
		{
			for (int c = 0; c < 4; c++)
			{
				if (rows[r][c] >= 0) DrawTownTile(rows[r][c], x1 + c * kTile, r * kTile);
			}
		}
		const float cx = (x1 + x2) * 0.5f;
		const float glow = 0.5f + 0.5f * std::sin(time_ * 2.0f);
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(40 + 40 * glow));
		DrawOvalAA(cx, y2 - 30.0f, 40.0f, 50.0f, 24, GetColor(120, 80, 200), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		Font::DrawCenter(cx, y2 + 14.0f, L"ダンジョン", GetColor(255, 240, 200), FontSize::Small, GetColor(30, 20, 20));
	}

	DrawArchive();
	DrawSmithy();

	//テント(ちいさな いえ)
	DrawHouse(kWoodHouse, kOriginX + kTent.x * kTile, kTent.y * kTile, kTent.w, kTent.h, kTent.w / 2);

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
	const float y1 = kLibrary.y * kTile;
	const float y2 = (kLibrary.y + kLibrary.h) * kTile;
	const float cx = x1 + kLibrary.w * kTile * 0.5f;

	//いしの いえ(とびらは しょこの まえの マス)
	DrawHouse(kStoneHouse, x1, y1, kLibrary.w, kLibrary.h, 4 - kLibrary.x);
	Font::DrawCenter(cx, y1 + kTile * 0.55f, L"しょこ", GetColor(255, 255, 255), FontSize::Small, GetColor(40, 30, 50));

	//しょこばん(ゆっくり ゆれる)
	const float bob = std::fabs(std::sin(time_ * 1.5f)) * 2.0f;
	DrawPerson(kLibrarianTile, { x1 + kTile * 4.0f, y2 - 2.0f }, 2.6f, bob, true);
}

void CampScene::DrawSmithy()
{
	const float x1 = kOriginX + kSmith.x * kTile;
	const float x2 = x1 + kSmith.w * kTile;
	const float y1 = kSmith.y * kTile;
	const float y2 = (kSmith.y + kSmith.h) * kTile;
	const float cx = (x1 + x2) * 0.5f;

	//きの いえ(とびらは かじやの まえの マス)
	DrawHouse(kWoodHouse, x1, y1, kSmith.w, kSmith.h, 21 - kSmith.x);
	Font::DrawCenter(cx, y1 + kTile * 0.55f, L"かじや", GetColor(255, 255, 255), FontSize::Small, GetColor(60, 25, 15));

	//えんとつの けむり
	const float chx = x2 - 40.0f;
	for (int k = 0; k < 3; k++)
	{
		const float t = std::fmod(time_ * 0.6f + k * 0.33f, 1.0f);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(120 * (1.0f - t)));
		DrawCircleAA(chx + t * 20.0f, y1 - 4.0f - t * 60.0f, 8.0f + t * 10.0f, 12, GetColor(200, 200, 200), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//かなとこ と かじや(つちを ふりおろす)
	const Vec2 anvil = { x1 + 76.0f, y2 - 2.0f };
	Sprite::DrawFeet(Sprite::Tile(SpriteSheet::Dungeon, kAnvilTile), anvil, 2.0f, 2.0f);
	const Vec2 smith = { x1 + 40.0f, y2 - 2.0f };
	DrawPerson(kSmithTile, smith, 2.6f, 0.0f, false);

	const float period = 0.9f;
	const float p = std::fmod(time_, period) / period;
	const float down = 1.2f;
	const float up = -0.9f;
	float angle = down;
	if (p < 0.55f) angle = down + (up - down) * std::sin(p / 0.55f * 1.5708f);
	else if (p < 0.7f) angle = up + (down - up) * ((p - 0.55f) / 0.15f);
	Sprite::DrawFeet(Sprite::Tile(SpriteSheet::Dungeon, kHammerTile), smith + Vec2(12.0f, -18.0f), 1.8f, 1.8f, false, angle);

	//うったときの ひばな
	if (p >= 0.7f && p < 0.85f)
	{
		SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
		for (int k = 0; k < 3; k++)
		{
			DrawUtil::Star(anvil.x - 6.0f + k * 8.0f, anvil.y - 30.0f - k * 4.0f - (p - 0.7f) * 120.0f, 4.0f, time_ * 8.0f + k, GetColor(255, 200, 90));
		}
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
	if (s.deepestAbyss > 0) info += L"　しんえん B" + std::to_wstring(s.deepestAbyss) + L"F";
	const int title = Title::Best(s);
	if (title >= 0) info += std::wstring(L"　「") + Title::Name(title) + L"」";
	Font::DrawFit(50.0f, 672.0f, info, GetColor(240, 235, 255), FontSize::Small, GetColor(10, 10, 20), 985.0f);
	ButtonGuide::Draw(1236.0f - ButtonGuide::Width(L"{M}：メニュー", FontSize::Small), 684.0f, L"{M}：メニュー", GetColor(170, 160, 190), FontSize::Small, GetColor(10, 10, 20));
}

void CampScene::DrawStageSelect()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const unsigned int edge = GetColor(20, 15, 30);
	DrawUtil::Window(24.0f, 24.0f, 1256.0f, 696.0f);
	Font::DrawCenter(224.0f, 66.0f, L"どこへ いく？", GetColor(255, 230, 140), FontSize::Medium, edge);

	//ひだり: いきさきの いちらん
	for (size_t i = 0; i < choices_.size(); i++)
	{
		const StageChoice& c = choices_[i];
		const float top = 108.0f + i * 66.0f;
		const bool isSel = static_cast<int>(i) == stageCursor_;
		const int stage = ChoiceStage(static_cast<int>(i));
		const std::vector<AreaView> areas = (stage >= 0 && !c.isLocked) ? BuildAreas(stage) : std::vector<AreaView>();
		const Game::Rgb tint = areas.empty() ? Game::Rgb{ 90, 85, 110 } : areas[0].color;
		DrawUtil::Panel(48.0f, top, 404.0f, top + 56.0f, isSel ? Game::Rgb{ 70, 60, 105 } : Game::Rgb{ 32, 28, 48 }, 235,
			isSel ? Game::Rgb{ 255, 220, 120 } : Game::Rgb{ 85, 75, 110 });
		DrawBoxAA(54.0f, top + 8.0f, 62.0f, top + 48.0f, Game::ToColor(tint), TRUE);
		if (isSel && mode_ == Mode::StageSelect) DrawUtil::Cursor(36.0f, top + 28.0f, time_);
		const unsigned int col = c.isLocked ? GetColor(120, 110, 140) : (isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230));
		Font::DrawFit(74.0f, top + 13.0f, c.text, col, FontSize::Small, edge, 318.0f);
	}

	//みぎ: くわしい じょうほう
	DrawStageDetail(424.0f, 48.0f, 1236.0f, 640.0f);
	ButtonGuide::DrawCenter(640.0f, 668.0f, L"{UD}：いきさき　{LR}：エリアを みる　{A}：けってい　{B}：もどる", GetColor(170, 160, 190), FontSize::Small, edge);

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

int CampScene::ChoiceStage(int index) const
{
	if (index < 0 || index >= static_cast<int>(choices_.size())) return -1;
	const int stage = choices_[index].stage;
	if (stage == -1) return SaveManager::GetInstance().Current().run.stage;
	return stage < 0 ? -1 : stage;
}

void CampScene::ResetAreaCursor()
{
	//いま いる階 / いちばん ふかい階の エリアから 見せる
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const int stage = ChoiceStage(stageCursor_);
	areaCursor_ = 0;
	if (stageCursor_ < 0 || stageCursor_ >= static_cast<int>(choices_.size())) return;
	if (choices_[stageCursor_].stage == -1)
	{
		areaCursor_ = stage == Stage::kTraining ? std::clamp(s.run.rank, 0, Stage::kTrainingRankNum - 1) : AreaOf(stage, s.run.floor);
	}
	else if (stage == Stage::kNormal)
	{
		areaCursor_ = AreaOf(stage, std::max(1, s.deepestNormal));
	}
	else if (stage == Stage::kInfinite)
	{
		areaCursor_ = AreaOf(stage, std::max(1, s.deepestInfinite));
	}
	else if (stage == Stage::kAbyss)
	{
		areaCursor_ = AreaOf(stage, std::max(Stage::kAbyssFirstFloor, s.deepestAbyss));
	}
	else if (stage == Stage::kTraining)
	{
		areaCursor_ = rankCursor_;
	}
}

void CampScene::DrawStageDetail(float left, float top, float right, float bottom)
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const unsigned int edge = GetColor(20, 15, 30);
	const float bob = std::sin(time_ * 6.0f) * 3.0f;
	const int index = stageCursor_;
	if (index < 0 || index >= static_cast<int>(choices_.size())) return;
	const StageChoice& choice = choices_[index];
	const int stage = ChoiceStage(index);
	auto isSeen = [&s](int id) { return id >= 0 && id < static_cast<int>(s.zukan.size()) && s.zukan[id].isSeen; };
	auto isDefeated = [&s](int id) { return id >= 0 && id < static_cast<int>(s.zukan.size()) && s.zukan[id].isDefeated; };

	//まものの え(まだ 見ていない ものは まっくろな かげ)
	auto drawMonster = [&](int id, const Vec2& feet, float size)
	{
		MonsterData m = MonsterTable::Get(id);
		const bool seen = isSeen(id);
		if (!seen)
		{
			m.color = { 25, 22, 35 };
			SetDrawBright(40, 36, 55);
		}
		//しんえんは しんえん版の すがた(オーラ)で みせる
		if (seen && stage == Stage::kAbyss)
		{
			AuraRenderer::Draw(feet, { 160, 110, 230 }, time_ + id * 0.3f, [&](const Vec2& f) { UnitDraw::Monster(f, size, m, time_, 1.0f, false); });
			return;
		}
		UnitDraw::Monster(feet, size, m, seen ? time_ : 0.0f, 1.0f, false);
		SetDrawBright(255, 255, 255);
	};

	//むらに もどる
	if (stage < 0)
	{
		DrawUtil::Panel(left, top, right, bottom, { 30, 26, 46 }, 230, { 90, 80, 110 });
		Font::DrawCenter((left + right) * 0.5f, (top + bottom) * 0.5f - 20.0f, L"むらに もどる", GetColor(210, 200, 230), FontSize::Medium, edge);
		Font::DrawCenter((left + right) * 0.5f, (top + bottom) * 0.5f + 24.0f, L"じゅんびを ととのえてから でかけよう", GetColor(160, 150, 180), FontSize::Small, edge);
		return;
	}

	//まだ ひらいていない
	if (choice.isLocked)
	{
		DrawUtil::Panel(left, top, right, bottom, { 30, 26, 46 }, 230, { 90, 80, 110 });
		Font::DrawCenter((left + right) * 0.5f, top + 120.0f, L"？？？", GetColor(150, 140, 170), FontSize::Huge, edge);
		Font::DrawCenterFit((left + right) * 0.5f, top + 260.0f, L"まだ いけない ばしょ", GetColor(200, 190, 220), FontSize::Medium, edge, right - left - 40.0f);
		Font::DrawCenterFit((left + right) * 0.5f, top + 320.0f, choice.text, GetColor(255, 210, 150), FontSize::Small, edge, right - left - 40.0f);
		return;
	}

	const std::vector<AreaView> areas = BuildAreas(stage);
	const int areaNum = static_cast<int>(areas.size());
	const int areaIdx = std::clamp(areaCursor_, 0, std::max(0, areaNum - 1));
	const Game::Rgb tint = areas.empty() ? Game::Rgb{ 120, 110, 150 } : areas[0].color;

	//------------------------------//
	// ステージの なまえと せつめい
	//------------------------------//
	DrawUtil::Panel(left, top, right, top + 160.0f, { 36, 30, 56 }, 235, tint);
	Font::DrawFit(left + 24.0f, top + 12.0f, Stage::Name(stage), GetColor(255, 235, 160), FontSize::Medium, edge, 420.0f);
	std::wstring sub;
	if (stage == Stage::kNormal) sub = L"B1〜B" + std::to_wstring(Stage::kNormalLastFloor) + L"F・ぬし 1たい";
	else if (stage == Stage::kInfinite) sub = L"B1〜B" + std::to_wstring(Stage::kInfiniteLastFloor) + L"F・ぬし 5たい";
	else if (stage == Stage::kAbyss) sub = L"B" + std::to_wstring(Stage::kAbyssFirstFloor) + L"〜B" + std::to_wstring(Stage::kAbyssLastFloor) + L"F・ぬし " + std::to_wstring(static_cast<int>(areas.size())) + L"たい";
	else sub = L"ぜん" + std::to_wstring(Stage::kTrainingLastFloor) + L"かい・きっぷで ちょうせん";
	if (choice.stage == -1) sub = L"ぼうけんの つづき：B" + std::to_wstring(s.run.floor) + L"F から";
	Font::DrawFit(right - 330.0f, top + 20.0f, sub, GetColor(255, 210, 130), FontSize::Small, edge, 310.0f);
	const auto desc = Font::Wrap(StageDesc(stage), FontSize::Small, right - left - 48.0f);
	for (size_t i = 0; i < desc.size() && i < 3; i++)
	{
		Font::DrawFit(left + 24.0f, top + 62.0f + i * 31.0f, desc[i], GetColor(225, 220, 240), FontSize::Small, edge, right - left - 48.0f);
	}

	//------------------------------//
	// エリアの タブ
	//------------------------------//
	const float tabTop = top + 172.0f;
	const float tabL = left + 26.0f;
	const float tabW = (right - left - 52.0f) / std::max(1, areaNum);
	for (int i = 0; i < areaNum; i++)
	{
		const AreaView& a = areas[i];
		const bool isSel = i == areaIdx;
		const float x1 = tabL + i * tabW + 3.0f;
		const float x2 = tabL + (i + 1) * tabW - 3.0f;
		const Game::Rgb fill = isSel ? a.color : Game::Rgb{ static_cast<int>(a.color.r * 0.35f), static_cast<int>(a.color.g * 0.35f), static_cast<int>(a.color.b * 0.35f) };
		DrawUtil::Panel(x1, tabTop + (isSel ? 0.0f : 6.0f), x2, tabTop + 44.0f, fill, 235, isSel ? Game::Rgb{ 255, 240, 170 } : Game::Rgb{ 90, 80, 110 });
		const std::wstring label = stage == Stage::kTraining ? a.name : L"B" + std::to_wstring(a.first) + L"〜" + std::to_wstring(a.last);
		Font::DrawCenterFit((x1 + x2) * 0.5f, tabTop + (isSel ? 22.0f : 25.0f), label, isSel ? GetColor(255, 255, 255) : GetColor(170, 160, 190), FontSize::Small, edge, x2 - x1 - 10.0f);
	}
	if (areaNum > 1)
	{
		DrawTriangleAA(left + 6.0f + bob, tabTop + 22.0f, left + 20.0f + bob, tabTop + 10.0f, left + 20.0f + bob, tabTop + 34.0f, GetColor(255, 230, 120), TRUE);
		DrawTriangleAA(right - 6.0f - bob, tabTop + 22.0f, right - 20.0f - bob, tabTop + 10.0f, right - 20.0f - bob, tabTop + 34.0f, GetColor(255, 230, 120), TRUE);
	}
	if (areas.empty()) return;
	const AreaView& area = areas[areaIdx];

	//------------------------------//
	// エリアの なかみ(ぬし・でてくる まもの)
	//------------------------------//
	const float boxTop = tabTop + 52.0f;
	const float boxBottom = boxTop + 236.0f;
	DrawUtil::Panel(left, boxTop, right, boxBottom, { 28, 24, 42 }, 235, area.color);

	//ぬしの カード(すみかは しゅやくの キラプルン)
	const float cardL = left + 14.0f;
	const float cardR = left + 230.0f;
	DrawUtil::Panel(cardL, boxTop + 12.0f, cardR, boxBottom - 12.0f, { 46, 36, 62 }, 240, { 150, 120, 170 });
	const float cardX = (cardL + cardR) * 0.5f;
	const int starId = stage == Stage::kTraining ? Stage::kKiraPurun : area.bossId;
	Font::DrawCenter(cardX, boxTop + 34.0f, stage == Stage::kTraining ? L"しゅやく" : L"エリアの ぬし", GetColor(255, 200, 140), FontSize::Small, edge);
	if (starId >= 0)
	{
		//あしもとの ひかり
		SetDrawBlendMode(DX_BLENDMODE_ADD, 70);
		DrawOvalAA(cardX, boxTop + 156.0f, 60.0f, 14.0f, 24, Game::ToColor(area.color), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		drawMonster(starId, { cardX, boxTop + 156.0f }, stage == Stage::kTraining ? 34.0f : 44.0f);

		const MonsterData& m = MonsterTable::Get(starId);
		const bool isAbyss = stage == Stage::kAbyss;
		const std::wstring name = isSeen(starId) ? (isAbyss ? L"くるった " : L"") + m.name : L"？？？";
		Font::DrawCenterFit(cardX, boxTop + 178.0f, name, GetColor(255, 255, 255), FontSize::Small, edge, cardR - cardL - 16.0f);
		std::wstring state;
		unsigned int stateCol = GetColor(170, 160, 190);
		if (stage == Stage::kTraining) { state = L"けいけんち " + std::to_wstring(Stage::Rank(areaIdx).kiraExp); stateCol = GetColor(255, 220, 120); }
		else if (isAbyss) //しんえんの ぬしは すすんだ ふかさで みる
		{
			const bool isBeaten = s.deepestAbyss > area.last || (area.last >= Stage::kAbyssLastFloor && s.isAbyssCleared);
			state = isBeaten ? L"たおした！" : L"B" + std::to_wstring(area.last) + L"F で まつ";
			if (isBeaten) stateCol = GetColor(140, 230, 150);
		}
		else if (isDefeated(starId)) { state = L"たおした！"; stateCol = GetColor(140, 230, 150); }
		else if (isSeen(starId)) state = L"まだ たおしていない";
		else state = L"B" + std::to_wstring(area.last) + L"F で まつ";
		Font::DrawCenterFit(cardX, boxTop + 206.0f, state, stateCol, FontSize::Small, edge, cardR - cardL - 16.0f);
	}
	else
	{
		Font::DrawCenter(cardX, boxTop + 120.0f, L"ぬしは いない", GetColor(150, 140, 170), FontSize::Small, edge);
		Font::DrawCenter(cardX, boxTop + 152.0f, L"さきへ すすもう", GetColor(130, 120, 150), FontSize::Small, edge);
	}

	//エリアの なまえと ルール
	const float infoL = cardR + 20.0f;
	const float infoW = right - infoL - 16.0f;
	const std::wstring areaTitle = stage == Stage::kTraining ? std::wstring(area.name) + L"（きっぷ " + std::to_wstring(Stage::Rank(areaIdx).ticketCost) + L"まい）" : area.name;
	Font::DrawFit(infoL, boxTop + 14.0f, areaTitle, Game::ToColor(area.color), FontSize::Medium, edge, infoW);
	std::wstring areaSub;
	if (stage == Stage::kTraining)
	{
		const Stage::TrainingRank& rk = Stage::Rank(areaIdx);
		areaSub = L"レベル" + std::to_wstring(rk.levelCap) + L"まで あがりやすい・まものは むげん B" + std::to_wstring(area.first) + L"〜" + std::to_wstring(area.last) + L"F なみ";
	}
	else
	{
		areaSub = L"B" + std::to_wstring(area.first) + L"〜B" + std::to_wstring(area.last) + L"F";
		if (area.bossId >= 0) areaSub += L"・B" + std::to_wstring(area.last) + L"F で ぬしと たたかう";
		if (stage == Stage::kInfinite) areaSub += L"・つよさ ×" + std::to_wstring(100 + area.last * 4 / 10).insert(1, L".");
		if (stage == Stage::kAbyss)
		{
			const float rate = Stage::kAbyssBaseRate * (1.0f + Stage::kAbyssFloorRate * (area.last - Stage::kAbyssFirstFloor + 1));
			areaSub += L"・つよさ ×" + std::to_wstring(static_cast<int>(rate * 100.0f + 0.5f)).insert(1, L".");
		}
	}
	Font::DrawFit(infoL, boxTop + 54.0f, areaSub, GetColor(200, 195, 220), FontSize::Small, edge, infoW);

	//でてくる まもの
	int seenNum = 0;
	for (int id : area.monsters) if (isSeen(id)) seenNum++;
	const std::wstring head = L"でてくる まもの　みつけた " + std::to_wstring(seenNum) + L" / " + std::to_wstring(area.monsters.size());
	Font::DrawFit(infoL, boxTop + 88.0f, head, GetColor(255, 220, 140), FontSize::Small, edge, infoW);
	constexpr int kIconCols = 10;
	constexpr int kIconMax = kIconCols * 2;
	const float cellW = infoW / kIconCols;
	for (int i = 0; i < static_cast<int>(area.monsters.size()) && i < kIconMax; i++)
	{
		const float cx = infoL + (i % kIconCols + 0.5f) * cellW;
		const float feet = boxTop + 168.0f + (i / kIconCols) * 58.0f;
		drawMonster(area.monsters[i], { cx, feet }, 17.0f);
		if (isDefeated(area.monsters[i])) DrawCircleAA(cx + cellW * 0.35f, feet - 6.0f, 4.0f, 10, GetColor(200, 200, 220), TRUE);
	}
	if (static_cast<int>(area.monsters.size()) > kIconMax)
	{
		Font::DrawFit(right - 70.0f, boxBottom - 34.0f, L"+" + std::to_wstring(area.monsters.size() - kIconMax), GetColor(200, 190, 220), FontSize::Small, edge, 60.0f);
	}

	//------------------------------//
	// きろくと ルール
	//------------------------------//
	const float noteTop = boxBottom + 10.0f;
	DrawUtil::Panel(left, noteTop, right, bottom, { 32, 28, 48 }, 230, { 90, 80, 110 });
	std::vector<std::wstring> notes;
	if (choice.stage == -1)
	{
		notes.push_back(L"つづき：B" + std::to_wstring(s.run.floor) + L"F　ゆうしゃ レベル " + std::to_wstring(s.run.heroLevel) + L"　なかま " + std::to_wstring(s.run.allies.size()) + L"たい");
	}
	if (stage == Stage::kNormal)
	{
		notes.push_back(L"きろく：さいしん B" + std::to_wstring(s.deepestNormal) + L"F　クリア " + std::to_wstring(s.clearCount) + L"かい");
		notes.push_back(L"B1F の ワープゲートで、いちど たどりついた B5F・B10F へ とべる");
	}
	else if (stage == Stage::kInfinite)
	{
		int bossDown = 0;
		for (const AreaView& a : areas) if (isDefeated(a.bossId)) bossDown++;
		notes.push_back(L"きろく：さいしん B" + std::to_wstring(s.deepestInfinite) + L"F　ぬし " + std::to_wstring(bossDown) + L" / " + std::to_wstring(areas.size()) + L" たおした");
		notes.push_back(L"5かいごとに たきび（ぜんかいふく・おまもりの つけかえ・むらへ かえれる）");
		notes.push_back(L"10かいごとに つよい まもの。しゅぎょうの きっぷを おとす ことも");
	}
	else if (stage == Stage::kAbyss)
	{
		int titleNum = 0;
		for (int t = 0; t < Title::Count(); t++) if (Title::IsOwned(s, t)) titleNum++;
		notes.push_back(L"きろく：さいしん " + (s.deepestAbyss > 0 ? L"B" + std::to_wstring(s.deepestAbyss) + L"F" : std::wstring(L"ー")) + L"　しょうごう " + std::to_wstring(titleNum) + L" / " + std::to_wstring(Title::Count()) + (s.isAbyssCleared ? L"　★とうは" : L""));
		notes.push_back(L"まものは みんな しんえん版。なかまに ならず、しんえんの そざいを おとす");
		notes.push_back(L"オーラの いろ＝ついた とくぎ　あか:2かい こうげき　みどり:どく　きいろ:まひ　だいだい:もやす　むらさき:すいとる");
	}
	else
	{
		notes.push_back(L"もっている きっぷ：" + std::to_wstring(s.tickets) + L"まい（むげんの めいきゅうで てにはいる）");
		notes.push_back(L"キラプルンは にげまわる。かべぎわに おいつめて たおそう");
		notes.push_back(L"レベルが こえると けいけんちは 1/10。むずかしさを あげよう");
	}
	for (size_t i = 0; i < notes.size() && i < 4; i++)
	{
		Font::DrawFit(left + 24.0f, noteTop + 10.0f + i * 28.0f, notes[i], i == 0 ? GetColor(255, 230, 160) : GetColor(215, 210, 230), FontSize::Small, edge, right - left - 48.0f);
	}
}

//==============================//
// メニュー
//==============================//

std::vector<CampScene::PauseItem> CampScene::PauseItems() const
{
	//クリア後は せいちょうの ふりなおし
	std::vector<PauseItem> items = { PauseItem::Settings, PauseItem::KeyItems };
	if (SaveManager::GetInstance().Current().isCleared) items.push_back(PauseItem::Respec);
	items.push_back(PauseItem::Title);
	items.push_back(PauseItem::Close);
	return items;
}

const wchar_t* CampScene::PauseItemName(PauseItem item)
{
	switch (item)
	{
	case PauseItem::Settings:	return L"せってい";
	case PauseItem::KeyItems:	return L"だいじな もの";
	case PauseItem::Respec:		return L"せいちょうの ふりなおし";
	case PauseItem::Title:		return L"タイトルへ もどる";
	default:					return L"とじる";
	}
}

//==============================//
// せいちょうの ふりなおし(クリア後)
//==============================//

namespace
{
	//こうげき・ぼうぎょ・HP・かいしん の あとに「ぜんぶ もどす」「けってい」「やめる」
	constexpr int kRespecStatRows = HeroGrowth::kNum;
	constexpr int kRespecReset = kRespecStatRows;
	constexpr int kRespecDecide = kRespecStatRows + 1;
	constexpr int kRespecCancel = kRespecStatRows + 2;
	constexpr int kRespecRows = kRespecStatRows + 3;
	constexpr int kRespecBigStep = 10;	//LB・RB で うごく かず
}

int CampScene::RespecLeft() const
{
	const int used = respecPts_[0] + respecPts_[1] + respecPts_[2] + respecPts_[3];
	return HeroGrowth::TotalPoints(SaveManager::GetInstance().Current().hero.level) - used;
}

void CampScene::OpenRespec()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();

	//ぼうけんの とちゅうは のうりょくが ぼうけんの きろくに あるので できない
	if (s.run.isActive)
	{
		Sound::GetInstance().PlaySE(SE::Buzzer);
		respecMsg_ = L"ぼうけんの とちゅうは ふりなおせません";
		respecMsgTime_ = 2.0f;
		mode_ = Mode::Respec;
		respecCursor_ = kRespecCancel;
		respecPts_ = HeroGrowth::PointsOf(s.hero);
		return;
	}
	respecPts_ = HeroGrowth::PointsOf(s.hero);
	respecCursor_ = 0;
	respecMsg_.clear();
	respecMsgTime_ = 0.0f;
	mode_ = Mode::Respec;
	Sound::GetInstance().PlaySE(SE::Decide);
}

void CampScene::UpdateRespec()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	SaveSlot& s = SaveManager::GetInstance().Current();
	if (respecMsgTime_ > 0.0f) respecMsgTime_ -= Game::kFrameTime;

	if (input.IsAnyTriggered(Button::Up)) { respecCursor_ = (respecCursor_ + kRespecRows - 1) % kRespecRows; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { respecCursor_ = (respecCursor_ + 1) % kRespecRows; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel)) { mode_ = Mode::Pause; sound.PlaySE(SE::Cancel); return; }

	const bool isLocked = s.run.isActive;

	//ポイントを ふる(←→で 1、LB・RBで 10)
	if (respecCursor_ < kRespecStatRows && !isLocked)
	{
		int step = 0;
		if (input.IsAnyTriggered(Button::Right)) step = 1;
		if (input.IsAnyTriggered(Button::Left)) step = -1;
		if (input.IsAnyTriggered(Button::PageR)) step = kRespecBigStep;
		if (input.IsAnyTriggered(Button::PageL)) step = -kRespecBigStep;
		if (step != 0)
		{
			int& p = respecPts_[respecCursor_];
			const int next = std::clamp(p + step, 0, p + std::max(0, RespecLeft()));
			if (next != p) { p = next; sound.PlaySE(SE::Tick); }
			else sound.PlaySE(SE::Buzzer);
		}
	}

	if (!input.IsAnyTriggered(Button::Decide)) return;
	if (respecCursor_ == kRespecCancel) { mode_ = Mode::Pause; sound.PlaySE(SE::Cancel); return; }
	if (isLocked) { sound.PlaySE(SE::Buzzer); return; }
	if (respecCursor_ == kRespecReset)
	{
		respecPts_ = {};
		sound.PlaySE(SE::Cancel);
		respecMsg_ = L"ポイントを ぜんぶ もどした";
		respecMsgTime_ = 1.5f;
		return;
	}
	if (respecCursor_ == kRespecDecide)
	{
		if (RespecLeft() > 0)
		{
			sound.PlaySE(SE::Buzzer);
			respecMsg_ = L"のこりの ポイントを ぜんぶ ふってね（のこり " + std::to_wstring(RespecLeft()) + L"）";
			respecMsgTime_ = 2.0f;
			return;
		}
		HeroGrowth::Apply(s.hero, respecPts_);
		SaveManager::GetInstance().Save();
		sound.PlaySE(SE::Fanfare);
		respecMsg_ = L"せいちょうを ふりなおした！";
		respecMsgTime_ = 2.0f;
		respecCursor_ = kRespecCancel;
	}
}

void CampScene::DrawRespec()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const unsigned int edge = GetColor(20, 15, 30);
	const int level = s.hero.level;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawUtil::Window(290.0f, 90.0f, 990.0f, 640.0f);
	Font::DrawCenter(640.0f, 130.0f, L"せいちょうの ふりなおし", GetColor(255, 230, 140), FontSize::Medium, edge);
	const std::wstring head = L"レベル " + std::to_wstring(level) + L"　ふれる ポイント " + std::to_wstring(HeroGrowth::TotalPoints(level)) + L"　のこり " + std::to_wstring(RespecLeft());
	Font::DrawCenterFit(640.0f, 175.0f, head, RespecLeft() > 0 ? GetColor(255, 200, 120) : GetColor(170, 230, 170), FontSize::Small, edge, 640.0f);

	//ふりなおした あとの のうりょく
	HeroProgress preview = s.hero;
	if (preview.maxHp <= 0) preview = { level, s.hero.exp, HeroGrowth::kBaseHp, HeroGrowth::kBaseAtk, HeroGrowth::kBaseDef, 0 };
	HeroGrowth::Apply(preview, respecPts_);
	const int values[HeroGrowth::kNum] = { preview.atk, preview.def, preview.maxHp, preview.crit };
	const wchar_t* labels[HeroGrowth::kNum] = { L"こうげき", L"ぼうぎょ", L"さいだいHP", L"かいしん" };
	const wchar_t* units[HeroGrowth::kNum] = { L"", L"", L"", L"%" };
	for (int i = 0; i < HeroGrowth::kNum; i++)
	{
		const float y = 225.0f + i * 58.0f;
		const bool isSel = respecCursor_ == i;
		if (isSel) DrawUtil::Cursor(330.0f, y, time_);
		const unsigned int col = isSel ? GetColor(255, 240, 150) : GetColor(220, 210, 235);
		Font::DrawFit(370.0f, y - 14.0f, labels[i], col, FontSize::Small, edge, 160.0f);
		Font::DrawFit(540.0f, y - 14.0f, std::wstring(L"◀ ") + std::to_wstring(respecPts_[i]) + L" ▶", col, FontSize::Small, edge, 140.0f);
		Font::DrawFit(690.0f, y - 14.0f, std::wstring(L"（1ポイントで ") + HeroGrowth::kChoices[i].name + L"）", GetColor(170, 160, 190), FontSize::Small, edge, 170.0f);
		Font::DrawFit(870.0f, y - 14.0f, std::to_wstring(values[i]) + units[i], GetColor(255, 255, 255), FontSize::Small, edge, 100.0f);
	}

	const wchar_t* actions[3] = { L"ぜんぶ もどす", L"けってい", L"やめる" };
	for (int i = 0; i < 3; i++)
	{
		const float y = 225.0f + (kRespecStatRows + i) * 58.0f + 10.0f;
		const bool isSel = respecCursor_ == kRespecStatRows + i;
		if (isSel) DrawUtil::Cursor(330.0f, y, time_);
		Font::DrawFit(370.0f, y - 14.0f, actions[i], isSel ? GetColor(255, 240, 150) : GetColor(220, 210, 235), FontSize::Small, edge, 400.0f);
	}

	if (respecMsgTime_ > 0.0f && !respecMsg_.empty())
	{
		Font::DrawCenterFit(640.0f, 575.0f, respecMsg_, GetColor(255, 220, 160), FontSize::Small, edge, 640.0f);
	}
	ButtonGuide::DrawCenter(640.0f, 612.0f, L"{UD}：えらぶ　{LR}：ふる（{L}{R}で 10）　{A}：けってい　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}
