#include "DungeonScene.h"
#include "SceneManager.h"
#include "CampScene.h"
#include "SaveData.h"
#include "Stage.h"
#include "Charm.h"
#include "EquipData.h"
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
	constexpr float kTile = 48.0f;
	constexpr int kBaseAlly = 2;
	constexpr int kAllyWarpDistance = 7;	//これより離れて見えなくなったら追いつく
	constexpr int kAllyChaseRange = 5;		//主人公からこれより遠い敵は追わない
	constexpr int kAllyPathLimit = 120;		//仲間が道を探す距離
	constexpr int kWarpFloors[] = { 5, 10 };	//B1Fから飛べる階
	constexpr int kVisionRadius = 6;
	constexpr int kSpawnInterval = 35;
	constexpr int kRegenInterval = 4;
	constexpr float kStepTime = 0.13f;		//1マス進む時間
	constexpr float kDiagonalWait = 0.05f;	//ななめ入力を待つ時間
	constexpr int kMessageNum = 4;
	constexpr float kMessageWidth = 880.0f;
	constexpr int kBaseCrit = 6;

	//主人公の初期値
	constexpr int kHeroHp = 32;
	constexpr int kHeroAtk = 7;
	constexpr int kHeroDef = 2;

	//敵の強さの倍率
	constexpr float kEnemyHpRate = 0.85f;
	constexpr float kEnemyAtkRate = 0.8f;

	//素材・お守りが落ちる確率(%)
	constexpr int kMaterialRate = 30;
	constexpr int kCharmRateNormal = 3;
	constexpr int kCharmRateInfinite = 5;

	//色違いの出現率(千分率)
	constexpr int kShinyPermil = 1000;	//テスト用に100%(もとは 10)

	const Cell kDirs[8] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };

	//------------------------------//
	// 性格
	//------------------------------//
	struct Personality
	{
		const wchar_t* name;
		const wchar_t* desc;
		SkillValue plus;
		SkillValue minus;
	};

	const Personality kPersonalities[] =
	{
		{ L"ちからもち", L"こうげき+2 ぼうぎょ-1", { SkillKind::Atk, 2 }, { SkillKind::Def, -1 } },
		{ L"しんちょう", L"ぼうぎょ+2 こうげき-1", { SkillKind::Def, 2 }, { SkillKind::Atk, -1 } },
		{ L"タフ", L"さいだいHP+8 こうげき-1", { SkillKind::Hp, 8 }, { SkillKind::Atk, -1 } },
		{ L"すばしこい", L"かいひ+5% さいだいHP-4", { SkillKind::Evade, 5 }, { SkillKind::Hp, -4 } },
		{ L"ひとなつっこい", L"スカウト+5% こうげき-1", { SkillKind::Recruit, 5 }, { SkillKind::Atk, -1 } },
		{ L"がんばりや", L"けいけんち+10% さいだいHP-3", { SkillKind::Exp, 10 }, { SkillKind::Hp, -3 } },
	};
	constexpr int kPersonalityNum = static_cast<int>(std::size(kPersonalities));

	//------------------------------//
	// レベルアップの2択
	//------------------------------//
	struct GrowthChoice
	{
		const wchar_t* name;
		int hp;
		int atk;
		int def;
		int crit;
	};

	const GrowthChoice kGrowth[] =
	{
		{ L"こうげき +1", 0, 1, 0, 0 },
		{ L"ぼうぎょ +1", 0, 0, 1, 0 },
		{ L"さいだいHP +4", 4, 0, 0, 0 },
		{ L"かいしん +2%", 0, 0, 0, 2 },
	};
	constexpr int kGrowthNum = static_cast<int>(std::size(kGrowth));

	//------------------------------//
	// 見た目
	//------------------------------//
	struct Theme
	{
		Game::Rgb floorA;
		Game::Rgb floorB;
		Game::Rgb wall;
		Game::Rgb wallTop;
	};

	const Theme& ThemeOf(int floor, bool isInfinite)
	{
		static const Theme kThemes[3] =
		{
			{ { 120, 100, 80 }, { 112, 92, 74 }, { 70, 55, 50 }, { 100, 82, 70 } },
			{ { 90, 120, 150 }, { 84, 112, 142 }, { 50, 65, 95 }, { 80, 100, 135 } },
			{ { 120, 70, 90 }, { 112, 64, 84 }, { 70, 30, 45 }, { 110, 50, 65 } },
		};

		//むげんの めいきゅうは20階ごとにエリアが変わる
		static const Theme kInfiniteThemes[5] =
		{
			{ { 70, 80, 110 }, { 64, 74, 104 }, { 35, 30, 60 }, { 70, 60, 110 } },		//まよいの いりぐち
			{ { 150, 180, 205 }, { 140, 170, 196 }, { 70, 100, 140 }, { 120, 160, 200 } },	//こおりの どうくつ
			{ { 110, 60, 45 }, { 100, 52, 40 }, { 60, 22, 18 }, { 150, 60, 30 } },		//ようがんの ちか
			{ { 60, 45, 80 }, { 54, 40, 72 }, { 25, 15, 40 }, { 75, 45, 110 } },		//やみの しんでん
			{ { 190, 190, 215 }, { 180, 180, 206 }, { 120, 130, 170 }, { 230, 215, 150 } },	//てんくうの とう
		};
		if (isInfinite) return kInfiniteThemes[std::clamp((floor - 1) / 20, 0, 4)];
		return kThemes[std::min(2, (floor - 1) / 4)];
	}
}

DungeonScene::DungeonScene(SceneManager& manager, int stage, bool isContinue, int startFloor, const std::vector<PartyMember>& party) :
	Scene(manager),
	stage_(stage),
	isContinue_(isContinue),
	startFloor_(std::max(1, startFloor)),
	party_(party)
{
}

SaveSlot& DungeonScene::Save()
{
	return SaveManager::GetInstance().Current();
}

//==============================//
// 冒険の開始・保存
//==============================//

void DungeonScene::Init()
{
	units_.clear();
	Unit hero;
	hero.name = L"ゆうしゃ";
	hero.faction = Faction::Player;
	units_.push_back(hero);

	if (isContinue_ && Save().run.isActive) LoadRun();
	else NewRun();

	StartFloor(true);
	if (!isContinue_)
	{
		if (stage_ == Stage::kInfinite)
		{
			AddMessage(std::wstring(Stage::Name(stage_)) + L" B" + std::to_wstring(floor_) + L"F（" + Stage::InfiniteArea(floor_) + L"）に ふみこんだ！", GetColor(255, 240, 160));
		}
		else
		{
			AddMessage(std::wstring(Stage::Name(stage_)) + L"に ふみこんだ！", GetColor(255, 240, 160));
		}
		AddMessage(L"こんかいの せいかく：" + std::wstring(kPersonalities[personality_].name) + L"（" + kPersonalities[personality_].desc + L"）", GetColor(200, 220, 255));
		if (AllyCount() > 0) AddMessage(L"なかま " + std::to_wstring(AllyCount()) + L"たいが いっしょに きた！", GetColor(150, 230, 255));
	}
	else
	{
		AddMessage(L"ぼうけんの つづきから はじめた", GetColor(255, 240, 160));
	}
}

void DungeonScene::NewRun()
{
	SaveSlot& s = Save();
	s.run = RunData();
	s.run.isActive = true;
	s.run.stage = stage_;
	s.run.floor = 1;
	s.Fit(static_cast<int>(MonsterTable::All().size()));

	floor_ = startFloor_;
	turn_ = 0;
	personality_ = RandI(0, kPersonalityNum - 1);
	isGutsUsed_ = false;

	//主人公の育ちは前の冒険から引き継ぐ
	const HeroProgress& h = s.hero;
	const bool isGrown = h.maxHp > 0;
	baseMaxHp_ = isGrown ? h.maxHp : kHeroHp;
	baseAtk_ = isGrown ? h.atk : kHeroAtk;
	baseDef_ = isGrown ? h.def : kHeroDef;
	baseCrit_ = isGrown ? h.crit : 0;

	Unit& hero = units_[0];
	hero.level = isGrown ? std::max(1, h.level) : 1;
	hero.exp = isGrown ? h.exp : 0;
	RecomputeBonus();
	hero.hp = hero.maxHp;

	//えらんだ なかまを つれていく(わくの かずまで)
	for (const PartyMember& p : party_)
	{
		if (AllyCount() >= MaxAlly()) break;
		units_.push_back(MakeAllyById(p.monsterId, p.isShiny));
	}
}

Unit DungeonScene::MakeAllyById(int monsterId, bool isShiny)
{
	Unit a;
	a.monsterId = monsterId;
	a.isShiny = isShiny;
	const MonsterData m = a.Data();
	a.name = m.name;
	a.faction = Faction::Ally;
	a.maxHp = m.hp;
	a.atk = m.atk;
	a.def = m.def;
	a.skill = m.skill;
	a.level = 1;
	while (a.level < units_[0].level)
	{
		a.level++;
		GrowAlly(a);
	}
	a.hp = a.maxHp;
	ApplyAllyHpBonus(a);
	a.hp = a.maxHp;
	return a;
}

void DungeonScene::LoadRun()
{
	const RunData& r = Save().run;
	stage_ = r.stage;
	floor_ = r.floor;
	turn_ = r.turn;
	tactics_ = static_cast<Tactics>(r.tactics);
	baseMaxHp_ = r.heroMaxHp;
	baseAtk_ = r.heroAtk;
	baseDef_ = r.heroDef;
	baseCrit_ = r.heroCrit;
	personality_ = std::clamp(r.personality, 0, kPersonalityNum - 1);
	isGutsUsed_ = r.isGutsUsed;

	Unit& hero = units_[0];
	hero.level = r.heroLevel;
	hero.exp = r.heroExp;
	RecomputeBonus();
	hero.hp = std::clamp(r.heroHp, 1, hero.maxHp);

	for (const RunAlly& a : r.allies)
	{
		const MonsterData m = a.isShiny ? MonsterTable::ShinyVariant(a.monsterId) : MonsterTable::Get(a.monsterId);
		Unit u;
		u.monsterId = a.monsterId;
		u.isShiny = a.isShiny;
		u.name = m.name;
		u.faction = Faction::Ally;
		u.level = a.level;
		u.exp = a.exp;
		u.hp = a.hp;
		u.maxHp = a.maxHp;
		u.atk = a.atk;
		u.def = a.def;
		u.skill = m.skill;
		ApplyAllyHpBonus(u);
		units_.push_back(u);
	}
}

void DungeonScene::SaveRun()
{
	SaveSlot& s = Save();
	RunData& r = s.run;
	const Unit& hero = units_[0];
	r.isActive = true;
	r.stage = stage_;
	r.floor = floor_;
	r.turn = turn_;
	r.tactics = static_cast<int>(tactics_);
	r.heroLevel = hero.level;
	r.heroExp = hero.exp;
	r.heroHp = hero.hp;
	r.heroMaxHp = baseMaxHp_;
	r.heroAtk = baseAtk_;
	r.heroDef = baseDef_;
	r.heroCrit = baseCrit_;
	r.personality = personality_;
	r.isGutsUsed = isGutsUsed_;

	r.allies.clear();
	for (const Unit& u : units_)
	{
		if (u.faction != Faction::Ally || !u.isAlive) continue;
		//だいじな もので ふえた ぶんは のぞいて きろく
		const int baseMaxHp = u.maxHp - u.bonusHp;
		r.allies.push_back({ u.monsterId, u.isShiny, u.level, u.exp, std::min(u.hp, baseMaxHp), baseMaxHp, u.atk, u.def });
	}

	if (stage_ == Stage::kNormal) s.deepestNormal = std::max(s.deepestNormal, floor_);
	else s.deepestInfinite = std::max(s.deepestInfinite, floor_);
	SaveManager::GetInstance().Save();
}

void DungeonScene::BringBack(bool isDead)
{
	//拾ったものを拠点へ
	SaveSlot& s = Save();
	RunData& r = s.run;
	resultMaterials_ = 0;
	for (size_t i = 0; i < r.materials.size() && i < s.materials.size(); i++)
	{
		s.materials[i] += r.materials[i];
		resultMaterials_ += r.materials[i];
	}
	for (size_t i = 0; i < r.shinyMaterials.size() && i < s.shinyMaterials.size(); i++)
	{
		s.shinyMaterials[i] += r.shinyMaterials[i];
		resultMaterials_ += r.shinyMaterials[i];
	}

	//ちからつきても ひろった おまもりは もちかえる
	(void)isDead;
	resultCharms_ = static_cast<int>(r.foundCharms.size());
	const int firstNew = static_cast<int>(s.charms.size());
	for (const Charm& c : r.foundCharms) s.charms.push_back(c);

	//冒険中に付けかえたお守りはそのまま装備
	if (r.runCharm >= 0 && r.runCharm < resultCharms_) s.equippedCharm = firstNew + r.runCharm;
	EndRun();
}

void DungeonScene::EndRun()
{
	SaveSlot& s = Save();
	if (stage_ == Stage::kNormal) s.deepestNormal = std::max(s.deepestNormal, floor_);
	else s.deepestInfinite = std::max(s.deepestInfinite, floor_);

	//つぎに むげんの めいきゅうへ いくとき つづきから はじめられるように
	if (stage_ == Stage::kInfinite) s.lastInfinite = state_ == State::Clear ? 0 : floor_;

	//えらび残したレベルアップの強化は自動で決める
	while (pendingChoices_ > 0)
	{
		const GrowthChoice& g = kGrowth[RandI(0, kGrowthNum - 1)];
		baseMaxHp_ += g.hp;
		baseAtk_ += g.atk;
		baseDef_ += g.def;
		baseCrit_ += g.crit;
		pendingChoices_--;
	}

	//主人公の育ちを拠点に持ち帰る
	const Unit& hero = units_[0];
	s.hero = { hero.level, hero.exp, baseMaxHp_, baseAtk_, baseDef_, baseCrit_ };
	const int monsterNum = static_cast<int>(MonsterTable::All().size());
	s.run = RunData();
	s.Fit(monsterNum);
	SaveManager::GetInstance().Save();
}

const Charm* DungeonScene::RunCharm()
{
	const RunData& r = Save().run;
	if (r.runCharm >= 0 && r.runCharm < static_cast<int>(r.foundCharms.size())) return &r.foundCharms[r.runCharm];
	return nullptr;
}

void DungeonScene::RecomputeBonus()
{
	bonus_ = Save().GearBonus(RunCharm());
	const Personality& p = kPersonalities[personality_];
	bonus_.Add(p.plus);
	bonus_.Add(p.minus);
	ApplyHeroStats();
}

void DungeonScene::ApplyHeroStats()
{
	Unit& hero = units_[0];
	hero.maxHp = std::max(1, baseMaxHp_ + bonus_.Get(SkillKind::Hp));
	hero.atk = std::max(1, baseAtk_ + bonus_.Get(SkillKind::Atk));
	hero.def = std::max(0, baseDef_ + bonus_.Get(SkillKind::Def));
	hero.hp = std::min(hero.hp, hero.maxHp);
}

//==============================//
// 進行
//==============================//

int DungeonScene::ThemeFloor() const
{
	return floor_;
}

bool DungeonScene::IsBossAlive() const
{
	for (const Unit& u : units_)
	{
		if (u.isAlive && u.faction == Faction::Enemy && u.IsBoss()) return true;
	}
	return false;
}

bool DungeonScene::Chance(int percent) const
{
	return RandI(0, 99) < percent;
}

void DungeonScene::StartFloor(bool isFirst)
{
	map_.Generate();

	std::vector<Unit> party;
	for (const Unit& u : units_)
	{
		if (u.IsParty() && u.isAlive) party.push_back(u);
	}
	units_ = party;

	//階段で全員回復
	if (!isFirst && bonus_.Get(SkillKind::FloorHeal) > 0)
	{
		for (Unit& u : units_) u.hp = u.maxHp;
		AddMessage(L"いやしの ちからで みんな かいふくした", GetColor(150, 255, 170));
	}

	units_[0].cell = map_.StartCell();
	for (size_t i = 1; i < units_.size(); i++)
	{
		units_[i].cell = map_.StartCell();
		for (const Cell& d : kDirs)
		{
			const Cell c = map_.StartCell() + d;
			if (map_.Get(c) == Tile::Floor && !IsBlocked(c)) { units_[i].cell = c; break; }
		}
	}
	for (Unit& u : units_)
	{
		u.drawPos = CellToPos(u.cell);
		u.poisonTurns = u.burnTurns = u.paralyzeTurns = 0;
		u.isStunned = false;
	}

	//B1Fにはたどりついた階へのワープゲート
	gates_.clear();
	if (stage_ == Stage::kNormal && floor_ == 1)
	{
		for (int target : kWarpFloors) PlaceGate(target);
	}

	const int enemyNum = std::min(10, 4 + ThemeFloor() / 2);
	for (int i = 0; i < enemyNum; i++) SpawnEnemy(false);

	//ボスは階段の上で待つ
	const bool isInfinite = stage_ == Stage::kInfinite;
	const int bossId = MonsterTable::BossFor(floor_, isInfinite);
	if (bossId >= 0)
	{
		Unit boss = MakeEnemy(bossId, false, false);
		boss.cell = map_.StairsCell();
		boss.drawPos = CellToPos(boss.cell);
		units_.push_back(boss);
		Sound::GetInstance().PlayBGM(BGM::Boss);
		AddMessage(L"おくから おそろしい けはいが する…", GetColor(255, 120, 120));
		if (isInfinite) AddMessage(L"この かいの ぬし " + boss.name + L"を たおさないと さきへ すすめない", GetColor(255, 170, 150));
	}
	else
	{
		Sound::GetInstance().PlayBGM(BGM::Dungeon);
	}

	//無限ダンジョンの強敵(ぬしの階は出ない)
	if (isInfinite && floor_ % Stage::kEliteInterval == 0 && bossId < 0)
	{
		Unit elite = MakeEnemy(MonsterTable::PickForFloor(floor_, true), false, true);
		elite.cell = map_.RandomFloor([this](const Cell& c) { return IsBlocked(c) || Distance(c, units_[0].cell) < 8; });
		elite.drawPos = CellToPos(elite.cell);
		units_.push_back(elite);
		AddMessage(L"この かいには つよい まものの けはいが する…", GetColor(255, 160, 120));
	}

	map_.UpdateVision(units_[0].cell, kVisionRadius + bonus_.Get(SkillKind::Vision));
	camera_ = units_[0].drawPos;
	projectiles_.clear();
	commandTarget_ = -1;
	state_ = State::Input;
	UpdateZukanSeen();

	//降りた時に自動セーブ
	SaveRun();
}

Unit DungeonScene::MakeEnemy(int monsterId, bool isShiny, bool isElite) const
{
	Unit e;
	e.monsterId = monsterId;
	e.isShiny = isShiny;
	e.isElite = isElite;
	const MonsterData m = e.Data();
	e.name = (isShiny ? L"いろちがいの " : (isElite ? L"つよい " : L"")) + m.name;
	e.faction = Faction::Enemy;

	//無限ダンジョンは深いほど少しずつ強い(B100で1.4倍)
	float scale = 1.0f;
	if (stage_ == Stage::kInfinite) scale += 0.004f * floor_;
	if (isElite) scale *= 1.6f;
	e.hp = e.maxHp = std::max(1, static_cast<int>(m.hp * scale * kEnemyHpRate * (isElite ? 1.5f : 1.0f)));
	e.atk = std::max(1, static_cast<int>(m.atk * scale * kEnemyAtkRate));
	e.def = static_cast<int>(m.def * scale);
	e.skill = m.skill;
	return e;
}

void DungeonScene::SpawnEnemy(bool isOutOfSight)
{
	const int id = MonsterTable::PickForFloor(floor_, stage_ == Stage::kInfinite);

	//色違いはクリア後だけ
	bool isShiny = false;
	if (Save().isCleared)
	{
		const int permil = kShinyPermil * (100 + bonus_.Get(SkillKind::Shiny)) / 100;
		isShiny = RandI(0, 999) < permil;
	}

	Unit e = MakeEnemy(id, isShiny, false);
	auto blocked = [this, isOutOfSight](const Cell& c)
	{
		if (IsBlocked(c)) return true;
		if (map_.Rooms().front().Contains(c) && !isOutOfSight) return true;
		if (isOutOfSight && map_.IsVisible(c)) return true;
		return Distance(c, units_[0].cell) < 6;
	};
	e.cell = map_.RandomFloor(blocked);
	if (blocked(e.cell)) return;
	e.drawPos = CellToPos(e.cell);
	units_.push_back(e);
}

void DungeonScene::Update()
{
	const float dt = Game::kFrameTime;
	time_ += dt;
	stateTime_ += dt;
	waitTimer_ = std::max(0.0f, waitTimer_ - dt);

	for (Unit& u : units_)
	{
		//一定の速さでマスへ向かう
		const Vec2 diff = CellToPos(u.cell) - u.drawPos;
		const float dist = diff.Length();
		const float step = kTile / kStepTime * SaveManager::GetInstance().MoveRate() * dt;
		if (dist <= step || dist > kTile * 3.0f) u.drawPos = CellToPos(u.cell);
		else u.drawPos += diff * (step / dist);
		u.hitFlash = std::max(0.0f, u.hitFlash - dt * 4.0f);
		u.attackAnim = std::max(0.0f, u.attackAnim - dt * 6.0f);
	}
	camera_ += (units_[0].drawPos - camera_) * std::min(1.0f, dt * 12.0f);

	for (auto& p : projectiles_) p.t += dt / 0.18f;
	std::erase_if(projectiles_, [](const Projectile& p) { return p.t >= 1.0f; });
	for (auto& m : messages_) m.time += dt;

	//色違いのキラキラ
	for (const Unit& u : units_)
	{
		if (!u.isAlive || !u.isShiny || !map_.IsVisible(u.cell) || RandI(0, 7) != 0) continue;
		Particle p;
		p.pos = u.drawPos + Vec2(RandF(-20.0f, 20.0f), RandF(-40.0f, 0.0f));
		p.vel = { 0.0f, -30.0f };
		p.life = p.maxLife = 0.6f;
		p.size = RandF(4.0f, 8.0f);
		p.spin = 4.0f;
		p.color = { 255, 255, 200 };
		p.shape = ParticleShape::Star;
		p.isAdd = true;
		Effects::GetInstance().Add(p);
	}

	Input& input = Input::GetInstance();
	switch (state_)
	{
	case State::Input:		UpdateInput(); break;
	case State::Recruit:	UpdateRecruit(); break;
	case State::LevelChoice:	UpdateLevelChoice(); break;
	case State::Menu:		UpdateMenu(); break;
	case State::Status:
		if (input.IsAnyTriggered(Button::Decide) || input.IsAnyTriggered(Button::Cancel))
		{
			state_ = State::Menu;
			Sound::GetInstance().PlaySE(SE::Cancel);
		}
		break;
	case State::Settings:
		if (settingsMenu_.Update() == SettingsMenu::Result::Close) state_ = State::Menu;
		break;
	case State::LeaveConfirm:	UpdateLeaveConfirm(); break;
	case State::KeyItems:
		if (keyItemMenu_.Update()) state_ = State::Menu;
		break;
	case State::Rest:		UpdateRest(); break;
	case State::CharmSwap:	UpdateCharmSwap(); break;
	case State::Descend:
		if (stateTime_ >= 0.6f)
		{
			//ワープゲートで飛ぶ
			if (warpTarget_ > 0)
			{
				floor_ = warpTarget_ - 1;
				warpTarget_ = 0;
				GoNextFloor();
				AddMessage(L"ワープゲートで とんだ！", GetColor(150, 220, 255));
			}
			//無限ダンジョンは区切りの階で休憩
			else if (stage_ == Stage::kInfinite && floor_ % Stage::kRestInterval == 0)
			{
				state_ = State::Rest;
				menuCursor_ = 0;
				stateTime_ = 0.0f;
				Sound::GetInstance().PlayBGM(BGM::Village);

				//休憩所で みんな全回復
				for (Unit& u : units_)
				{
					if (!u.IsParty() || !u.isAlive) continue;
					u.hp = u.maxHp;
					u.poisonTurns = 0;
					u.burnTurns = 0;
					u.paralyzeTurns = 0;
				}
				AddMessage(L"たきびで やすんで みんな かいふくした", GetColor(150, 255, 170));
			}
			else
			{
				GoNextFloor();
			}
		}
		break;
	case State::GameOver:
	case State::Clear:
	case State::Return:
		if (stateTime_ > 2.0f && input.IsAnyTriggered(Button::Decide))
		{
			Sound::GetInstance().PlaySE(SE::Decide);
			sceneManager_.ChangeSceneWithFade(std::make_shared<CampScene>(sceneManager_));
			stateTime_ = -100.0f;
		}
		break;
	}
}

void DungeonScene::GoNextFloor()
{
	floor_++;
	StartFloor(false);
	const std::wstring prefix = stage_ == Stage::kInfinite ? L"むげん " : L"";
	AddMessage(prefix + L"B" + std::to_wstring(floor_) + L"F に おりた", GetColor(200, 220, 255));
	stateTime_ = 0.0f;
}

void DungeonScene::UpdateInput()
{
	if (waitTimer_ > 0.0f) return;

	//レベルアップの選択が残っていれば先に
	if (pendingChoices_ > 0)
	{
		state_ = State::LevelChoice;
		stateTime_ = 0.0f;
		choiceCursor_ = 0;
		choiceOptions_[0] = RandI(0, kGrowthNum - 1);
		do { choiceOptions_[1] = RandI(0, kGrowthNum - 1); } while (choiceOptions_[1] == choiceOptions_[0]);
		return;
	}

	Input& input = Input::GetInstance();
	Unit& hero = units_[0];

	//しびれ・ひるみ中は動けない
	if (hero.paralyzeTurns > 0 || hero.isStunned)
	{
		AddMessage(hero.isStunned ? L"ゆうしゃは ひるんで うごけない！" : L"からだが しびれて うごけない！", GetColor(255, 230, 120));
		if (hero.paralyzeTurns > 0) hero.paralyzeTurns--;
		hero.isStunned = false;
		EndPlayerTurn();
		waitTimer_ = 0.35f;
		return;
	}

	if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Menu))
	{
		state_ = State::Menu;
		menuCursor_ = 0;
		Sound::GetInstance().PlaySE(SE::Cursor);
		return;
	}

	if (input.IsAnyTriggered(Button::Decide)) isAttackQueued_ = true;

	//前のマスへの移動が終わるまで待つ
	const bool isReady = (CellToPos(hero.cell) - hero.drawPos).Length() < kTile * 0.2f;
	const bool isTurnHeld = input.IsPressed(0, Button::Turn) || input.IsPressed(1, Button::Turn);

	Vec2 move = input.GetMove(0) + input.GetMove(1);
	const int dx = (move.x > 0.38f) - (move.x < -0.38f);
	const int dy = (move.y > 0.38f) - (move.y < -0.38f);
	if (dx == 0 && dy == 0) moveHoldTime_ = 0.0f;
	else moveHoldTime_ += Game::kFrameTime;

	//向きを変える
	if ((dx != 0 || dy != 0) && (isTurnHeld || moveHoldTime_ >= kDiagonalWait))
	{
		hero.dir = { dx, dy };
		if (dx != 0) hero.facing = static_cast<float>(dx);
	}

	if (!isReady) return;

	if (isAttackQueued_)
	{
		isAttackQueued_ = false;
		const Cell front = hero.cell + hero.dir;
		const bool isCorner = hero.dir.x != 0 && hero.dir.y != 0 &&
			(!map_.IsWalkable({ hero.cell.x + hero.dir.x, hero.cell.y }) || !map_.IsWalkable({ hero.cell.x, hero.cell.y + hero.dir.y }));
		const int target = isCorner ? -1 : UnitAt(front);
		if (target >= 0 && units_[target].faction == Faction::Enemy)
		{
			Attack(0, target);
			EndPlayerTurn();
			return;
		}

		if (map_.Get(hero.cell) == Tile::Stairs)
		{
			//ボスの階は倒すまで降りられない
			if (IsBossAlive())
			{
				Sound::GetInstance().PlaySE(SE::Buzzer);
				AddMessage(L"ふしぎな ちからで かいだんが ふさがれている…", GetColor(255, 150, 150));
				return;
			}
			Sound::GetInstance().PlaySE(SE::Whoosh);
			state_ = State::Descend;
			stateTime_ = 0.0f;
			return;
		}

		//ワープゲート
		const int gate = GateAt(hero.cell);
		if (gate >= 0)
		{
			const int target = gates_[gate].floor;
			if (!IsGateOpen(target))
			{
				Sound::GetInstance().PlaySE(SE::Buzzer);
				AddMessage(L"まだ B" + std::to_wstring(target) + L"F に たどりついて いない…", GetColor(255, 170, 150));
				return;
			}
			Sound::GetInstance().PlaySE(SE::Shiny);
			Effects::GetInstance().Ring(hero.drawPos, { 140, 210, 255 }, 10.0f, 120.0f, 0.5f);
			Effects::GetInstance().Burst(hero.drawPos, 24, { 170, 230, 255 }, 80.0f, 260.0f, 9.0f, 0.6f, ParticleShape::Star, -120.0f);
			warpTarget_ = target;
			state_ = State::Descend;
			stateTime_ = 0.0f;
			return;
		}

		//からぶり
		hero.attackAnim = 1.0f;
		hero.attackDir = Vec2(static_cast<float>(hero.dir.x), static_cast<float>(hero.dir.y)).Normalized();
		Sound::GetInstance().PlaySE(SE::Whoosh);
		EndPlayerTurn();
		waitTimer_ = std::max(waitTimer_, 0.12f);
		return;
	}

	if (isTurnHeld || moveHoldTime_ < kDiagonalWait) return;
	if (dx == 0 && dy == 0) return;

	const Cell to = hero.cell + Cell{ dx, dy };
	if (dx != 0 && dy != 0 && (!map_.IsWalkable({ hero.cell.x + dx, hero.cell.y }) || !map_.IsWalkable({ hero.cell.x, hero.cell.y + dy }))) return;

	//敵がいたら向くだけ
	const int other = UnitAt(to);
	if (other >= 0 && units_[other].faction == Faction::Enemy)
	{
		return;
	}
	else if (other >= 0)
	{
		units_[other].cell = hero.cell;
		hero.cell = to;
		EndPlayerTurn();
	}
	else if (map_.IsWalkable(to))
	{
		hero.cell = to;
		if (map_.Get(to) == Tile::Stairs) AddMessage(L"かいだんが ある (" + ButtonGuide::Name(Button::Decide) + L"で おりる)", GetColor(200, 220, 255));
		const int gate = GateAt(to);
		if (gate >= 0)
		{
			const int target = gates_[gate].floor;
			if (IsGateOpen(target)) AddMessage(L"B" + std::to_wstring(target) + L"F への ワープゲート (" + ButtonGuide::Name(Button::Decide) + L"で とぶ)", GetColor(150, 220, 255));
			else AddMessage(L"B" + std::to_wstring(target) + L"F への ワープゲート (まだ つかえない)", GetColor(170, 170, 200));
		}
		EndPlayerTurn();
	}
}

void DungeonScene::EndPlayerTurn()
{
	turn_++;

	//仲間は主人公に近い順に動く(通路で道をふさがないように)
	std::vector<int> allies;
	for (size_t i = 1; i < units_.size(); i++)
	{
		if (units_[i].isAlive && units_[i].faction == Faction::Ally) allies.push_back(static_cast<int>(i));
	}
	std::sort(allies.begin(), allies.end(), [this](int a, int b) { return Distance(units_[a].cell, units_[0].cell) < Distance(units_[b].cell, units_[0].cell); });
	for (int i : allies)
	{
		if (!units_[0].isAlive) break;
		if (units_[i].isAlive && !SkipByStatus(i)) ActAlly(i);
	}
	for (size_t i = 1; i < units_.size(); i++)
	{
		if (!units_[0].isAlive) break;
		if (units_[i].isAlive && units_[i].faction == Faction::Enemy && !SkipByStatus(static_cast<int>(i))) ActEnemy(static_cast<int>(i));
	}

	if (units_[0].isAlive) TickStatus();

	//自然回復(見える敵がいなければ 毎ターン 最大HPの1%、戦闘中は4ターンごとに1)
	bool isCalm = true;
	for (const Unit& u : units_)
	{
		if (u.isAlive && u.faction == Faction::Enemy && map_.IsVisible(u.cell)) { isCalm = false; break; }
	}
	const bool isTick = turn_ % kRegenInterval == 0;
	for (size_t i = 0; i < units_.size(); i++)
	{
		Unit& u = units_[i];
		if (!u.IsParty() || !u.isAlive) continue;
		int amount = 0;
		if (isCalm) amount += std::max(1, u.maxHp / 100);
		else if (isTick) amount += 1;

		//しぜんかいふく は 1ポイントごとに 最大HPの0.5%(戦闘中も)
		const int regen = i == 0 ? bonus_.Get(SkillKind::Regen) : 0;
		if (isTick && regen > 0) amount += std::max(regen, u.maxHp * regen / 200);
		u.hp = std::min(u.maxHp, u.hp + amount);
	}

	if (turn_ % kSpawnInterval == 0) SpawnEnemy(true);

	CleanupDead();
	map_.UpdateVision(units_[0].cell, kVisionRadius + bonus_.Get(SkillKind::Vision));
	UpdateZukanSeen();

	if (state_ == State::GameOver || state_ == State::Clear) return;
	if (hasPendingRecruit_)
	{
		state_ = State::Recruit;
		recruitCursor_ = 1;		//うっかり決めないよう「いいえ」から
		isRecruitConfirm_ = false;
		stateTime_ = 0.0f;
	}
}

bool DungeonScene::SkipByStatus(int index)
{
	Unit& u = units_[index];
	if (u.isStunned)
	{
		u.isStunned = false;
		return true;
	}
	if (u.paralyzeTurns > 0)
	{
		u.paralyzeTurns--;
		return true;
	}
	return false;
}

void DungeonScene::TickStatus()
{
	Effects& fx = Effects::GetInstance();
	const size_t num = units_.size();
	for (size_t i = 0; i < num; i++)
	{
		Unit& u = units_[i];
		if (!u.isAlive) continue;

		if (u.poisonTurns > 0)
		{
			u.poisonTurns--;
			u.hp -= u.poisonDamage;
			u.hitFlash = 0.6f;
			fx.Popup(u.drawPos + Vec2(0.0f, -50.0f), std::to_wstring(u.poisonDamage), { 200, 120, 255 }, FontSize::Small);
			fx.Burst(u.drawPos + Vec2(0.0f, -20.0f), 5, { 180, 90, 230 }, 30.0f, 90.0f, 6.0f, 0.5f, ParticleShape::Circle, -60.0f, false);
		}
		if (u.burnTurns > 0)
		{
			u.burnTurns--;
			u.hp -= u.burnDamage;
			u.hitFlash = 0.6f;
			fx.Popup(u.drawPos + Vec2(0.0f, -50.0f), std::to_wstring(u.burnDamage), { 255, 150, 60 }, FontSize::Small);
			fx.Burst(u.drawPos + Vec2(0.0f, -20.0f), 8, { 255, 140, 40 }, 40.0f, 120.0f, 7.0f, 0.4f, ParticleShape::Circle, -150.0f);

			//となりの同じ陣営に燃え移る
			for (size_t k = 0; k < num; k++)
			{
				Unit& o = units_[k];
				if (k == i || !o.isAlive || o.IsParty() != u.IsParty() || o.burnTurns > 0) continue;
				if (Distance(o.cell, u.cell) == 1 && Chance(25))
				{
					o.burnTurns = 2;
					o.burnDamage = std::max(1, u.burnDamage / 2);
				}
			}
		}

		if (u.hp <= 0)
		{
			//状態異常で倒れた時は主人公が倒した扱い
			Kill(static_cast<int>(i), u.IsParty() ? static_cast<int>(i) : 0);
			if (state_ == State::GameOver) return;
		}
	}
}

void DungeonScene::CleanupDead()
{
	for (size_t i = units_.size() - 1; i >= 1; i--)
	{
		if (!units_[i].isAlive)
		{
			units_.erase(units_.begin() + i);
			if (commandTarget_ == static_cast<int>(i)) commandTarget_ = -1;
			else if (commandTarget_ > static_cast<int>(i)) commandTarget_--;
		}
	}
}

void DungeonScene::UpdateZukanSeen()
{
	SaveSlot& s = Save();
	Effects& fx = Effects::GetInstance();
	for (Unit& u : units_)
	{
		if (!u.isAlive || u.faction != Faction::Enemy || !map_.IsVisible(u.cell)) continue;
		auto& book = u.isShiny ? s.zukanShiny : s.zukan;
		if (u.monsterId < static_cast<int>(book.size())) book[u.monsterId].isSeen = true;

		if ((u.isShiny || u.isElite) && !u.isNoticed)
		{
			u.isNoticed = true;
			if (u.isShiny)
			{
				//色違いの登場演出
				const Vec2 center = u.drawPos + Vec2(0.0f, -20.0f);
				Sound::GetInstance().PlaySE(SE::ShinyJackpot);
				fx.Flash({ 255, 250, 210 }, 90);
				fx.Burst(center, 36, { 255, 255, 200 }, 120.0f, 380.0f, 11.0f, 0.9f, ParticleShape::Star);
				fx.Burst(center, 20, { 255, 210, 120 }, 60.0f, 200.0f, 7.0f, 1.2f, ParticleShape::Circle, -80.0f);
				fx.Ring(center, { 255, 235, 140 }, 10.0f, 130.0f, 0.5f);
				fx.Ring(center, { 255, 255, 255 }, 10.0f, 80.0f, 0.35f);
				fx.Popup(u.drawPos + Vec2(0.0f, -80.0f), L"いろちがい！", { 255, 230, 120 }, FontSize::Medium);
				AddMessage(L"いろちがいの " + MonsterTable::Get(u.monsterId).name + L"が あらわれた！", GetColor(255, 240, 120));
				const bool isFirst = u.monsterId < static_cast<int>(s.zukanShiny.size()) && !s.zukanShiny[u.monsterId].isDefeated;
				if (isFirst) AddMessage(L"はじめての いろちがい！ たおすと かならず そざいを おとす", GetColor(255, 220, 160));
			}
			else
			{
				Sound::GetInstance().PlaySE(SE::Signal);
				fx.Ring(u.drawPos, { 255, 90, 90 }, 20.0f, 120.0f, 0.5f);
				AddMessage(u.name + L"が いる！", GetColor(255, 140, 120));
			}
		}
	}
}

//==============================//
// 各画面の入力
//==============================//

void DungeonScene::UpdateRecruit()
{
	Input& input = Input::GetInstance();
	if (stateTime_ < 0.4f) return;

	const bool isMove = input.IsAnyTriggered(Button::Left) || input.IsAnyTriggered(Button::Right) ||
		input.IsAnyTriggered(Button::Up) || input.IsAnyTriggered(Button::Down);
	const bool isDecide = input.IsAnyTriggered(Button::Decide);
	const bool isCancel = input.IsAnyTriggered(Button::Cancel);

	//「ほんとうに なかまに しない？」の確認
	if (isRecruitConfirm_)
	{
		if (isMove)
		{
			recruitConfirmCursor_ = 1 - recruitConfirmCursor_;
			Sound::GetInstance().PlaySE(SE::Cursor);
		}
		if (isCancel || (isDecide && recruitConfirmCursor_ == 1))
		{
			isRecruitConfirm_ = false;
			Sound::GetInstance().PlaySE(SE::Cancel);
			return;
		}
		if (!isDecide) return;
		AddMessage(pendingUnit_.name + L"は さびしそうに さっていった", GetColor(190, 190, 210));
		Sound::GetInstance().PlaySE(SE::Cancel);
		isRecruitConfirm_ = false;
	}
	else
	{
		if (isMove)
		{
			recruitCursor_ = 1 - recruitCursor_;
			Sound::GetInstance().PlaySE(SE::Cursor);
		}
		if (isDecide && recruitCursor_ == 0)
		{
			AcceptRecruit();
		}
		else if (isCancel || isDecide)
		{
			//ことわる前に確認する(最初は「もどる」側)
			isRecruitConfirm_ = true;
			recruitConfirmCursor_ = 1;
			Sound::GetInstance().PlaySE(SE::Cursor);
			return;
		}
		else
		{
			return;
		}
	}
	hasPendingRecruit_ = false;
	state_ = State::Input;
	waitTimer_ = 0.2f;
}

void DungeonScene::GrowAlly(Unit& u) const
{
	//もとが強いモンスターほどよく育つ
	const MonsterData& m = MonsterTable::Get(u.monsterId);
	const int hpUp = std::max(3, m.hp / 6);
	u.maxHp += hpUp;
	u.hp += hpUp;
	u.atk += std::max(1, m.atk / 6);
	if (u.level % 2 == 0) u.def += std::max(1, m.def / 5);
}

void DungeonScene::ApplyAllyHpBonus(Unit& u)
{
	if (u.faction != Faction::Ally || u.monsterId < 0) return;
	const int baseMaxHp = u.maxHp - u.bonusHp;
	const int bonus = baseMaxHp * KeyItem::AllyHpRate(Save()) / 100;
	const int diff = bonus - u.bonusHp;
	u.maxHp += diff;
	if (diff > 0) u.hp += diff;
	u.hp = std::min(u.hp, u.maxHp);
	u.bonusHp = bonus;
}

Unit DungeonScene::MakeAllyFrom(const Unit& enemy) const
{
	Unit a = enemy;
	a.faction = Faction::Ally;
	a.isAlive = true;
	a.isElite = false;
	a.poisonTurns = a.burnTurns = a.paralyzeTurns = 0;
	a.isStunned = false;
	a.name = a.Data().name;

	//敵の倍率をもどしてから、主人公と同じレベルまで育てる
	a.maxHp = static_cast<int>(std::round(a.maxHp / kEnemyHpRate));
	a.atk = static_cast<int>(std::round(a.atk / kEnemyAtkRate));
	a.level = 1;
	a.exp = 0;
	while (a.level < units_[0].level)
	{
		a.level++;
		GrowAlly(a);
	}
	a.hp = a.maxHp;
	return a;
}

void DungeonScene::AcceptRecruit()
{
	Unit a = MakeAllyFrom(pendingUnit_);
	ApplyAllyHpBonus(a);
	a.hp = a.maxHp;

	if (IsBlocked(a.cell) || !map_.IsWalkable(a.cell))
	{
		for (const Cell& d : kDirs)
		{
			const Cell c = units_[0].cell + d;
			if (map_.IsWalkable(c) && !IsBlocked(c)) { a.cell = c; break; }
		}
	}
	a.drawPos = CellToPos(a.cell);
	units_.push_back(a);

	SaveSlot& s = Save();
	auto& book = a.isShiny ? s.zukanShiny : s.zukan;
	if (a.monsterId < static_cast<int>(book.size())) book[a.monsterId].isRecruited = true;

	Effects& fx = Effects::GetInstance();
	fx.Burst(a.drawPos, 24, { 255, 230, 120 }, 120.0f, 380.0f, 12.0f, 0.7f, ParticleShape::Star);
	fx.Ring(a.drawPos, { 255, 200, 255 }, 10.0f, 110.0f, 0.5f);
	fx.Popup(a.drawPos + Vec2(0.0f, -60.0f), L"なかまになった！", { 255, 220, 120 }, FontSize::Medium);
	Sound::GetInstance().PlaySE(SE::Fanfare);
	AddMessage(a.name + L"が なかまに なった！", GetColor(255, 220, 120));
}

void DungeonScene::UpdateLevelChoice()
{
	Input& input = Input::GetInstance();
	if (stateTime_ < 0.3f) return;
	if (input.IsAnyTriggered(Button::Left) || input.IsAnyTriggered(Button::Right) ||
		input.IsAnyTriggered(Button::Up) || input.IsAnyTriggered(Button::Down))
	{
		choiceCursor_ = 1 - choiceCursor_;
		Sound::GetInstance().PlaySE(SE::Cursor);
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	const GrowthChoice& g = kGrowth[choiceOptions_[choiceCursor_]];
	baseMaxHp_ += g.hp;
	baseAtk_ += g.atk;
	baseDef_ += g.def;
	baseCrit_ += g.crit;
	units_[0].hp += g.hp;
	ApplyHeroStats();

	Sound::GetInstance().PlaySE(SE::Point);
	Effects::GetInstance().Popup(units_[0].drawPos + Vec2(0.0f, -70.0f), g.name, { 255, 230, 120 }, FontSize::Small);
	AddMessage(std::wstring(L"ゆうしゃは ") + g.name + L" を えらんだ", GetColor(255, 230, 120));
	pendingChoices_--;
	state_ = State::Input;
	waitTimer_ = 0.15f;
}

void DungeonScene::UpdateMenu()
{
	Input& input = Input::GetInstance();
	constexpr int kItemNum = 6;
	if (input.IsAnyTriggered(Button::Up)) { menuCursor_ = (menuCursor_ + kItemNum - 1) % kItemNum; Sound::GetInstance().PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { menuCursor_ = (menuCursor_ + 1) % kItemNum; Sound::GetInstance().PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Menu))
	{
		state_ = State::Input;
		Sound::GetInstance().PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	if (menuCursor_ == 0)
	{
		tactics_ = tactics_ == Tactics::Attack ? Tactics::Careful : Tactics::Attack;
		Sound::GetInstance().PlaySE(SE::Decide);
	}
	else if (menuCursor_ == 1)
	{
		state_ = State::Status;
		Sound::GetInstance().PlaySE(SE::Decide);
	}
	else if (menuCursor_ == 2)
	{
		state_ = State::KeyItems;
		Sound::GetInstance().PlaySE(SE::Decide);
	}
	else if (menuCursor_ == 3)
	{
		state_ = State::Settings;
		Sound::GetInstance().PlaySE(SE::Decide);
	}
	else if (menuCursor_ == 4)
	{
		state_ = State::LeaveConfirm;
		leaveCursor_ = 1;
		Sound::GetInstance().PlaySE(SE::Decide);
	}
	else
	{
		state_ = State::Input;
		Sound::GetInstance().PlaySE(SE::Cancel);
	}
}

void DungeonScene::UpdateLeaveConfirm()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	if (input.IsAnyTriggered(Button::Left) || input.IsAnyTriggered(Button::Right) ||
		input.IsAnyTriggered(Button::Up) || input.IsAnyTriggered(Button::Down))
	{
		leaveCursor_ = 1 - leaveCursor_;
		sound.PlaySE(SE::Cursor);
	}
	if (input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && leaveCursor_ == 1))
	{
		state_ = State::Menu;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	//ひろった ものを ぜんぶ もって 村へ
	sound.PlaySE(SE::Whoosh);
	sound.PlayBGM(BGM::Victory);
	BringBack();
	state_ = State::Return;
	stateTime_ = 0.0f;
}

void DungeonScene::UpdateRest()
{
	Input& input = Input::GetInstance();
	if (stateTime_ < 0.4f) return;
	constexpr int kItemNum = 3;

	if (input.IsAnyTriggered(Button::Up)) { menuCursor_ = (menuCursor_ + kItemNum - 1) % kItemNum; Sound::GetInstance().PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { menuCursor_ = (menuCursor_ + 1) % kItemNum; Sound::GetInstance().PlaySE(SE::Cursor); }
	if (!input.IsAnyTriggered(Button::Decide)) return;

	if (menuCursor_ == 0)
	{
		Sound::GetInstance().PlaySE(SE::Whoosh);
		GoNextFloor();
	}
	else if (menuCursor_ == 1)
	{
		if (Save().run.foundCharms.empty())
		{
			Sound::GetInstance().PlaySE(SE::Buzzer);
			return;
		}
		state_ = State::CharmSwap;
		swapCursor_ = 0;
		Sound::GetInstance().PlaySE(SE::Decide);
	}
	else
	{
		//持ち帰って拠点へ
		Sound::GetInstance().PlaySE(SE::Fanfare);
		Sound::GetInstance().PlayBGM(BGM::Victory);
		BringBack();
		state_ = State::Return;
		stateTime_ = 0.0f;
	}
}

void DungeonScene::UpdateCharmSwap()
{
	Input& input = Input::GetInstance();
	RunData& r = Save().run;

	//0番は拠点で付けたお守り
	const int num = static_cast<int>(r.foundCharms.size()) + 1;
	if (input.IsAnyTriggered(Button::Up)) { swapCursor_ = (swapCursor_ + num - 1) % num; Sound::GetInstance().PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { swapCursor_ = (swapCursor_ + 1) % num; Sound::GetInstance().PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel))
	{
		state_ = State::Rest;
		Sound::GetInstance().PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	r.runCharm = swapCursor_ - 1;
	RecomputeBonus();
	Sound::GetInstance().PlaySE(SE::Point);
	Effects::GetInstance().Flash({ 255, 240, 200 }, 100);
	state_ = State::Rest;
}

//==============================//
// 行動
//==============================//

void DungeonScene::ActAlly(int index)
{
	Unit& self = units_[index];
	const Unit& hero = units_[0];

	//見えないほど離れたら主人公のそばへ追いつく
	if (Distance(self.cell, hero.cell) > kAllyWarpDistance && !map_.IsVisible(self.cell) && WarpNearHero(index))
	{
		AddMessage(self.name + L"が おいついてきた", GetColor(170, 220, 255));
		return;
	}

	if (TryHeal(index)) return;

	const bool isCareful = tactics_ == Tactics::Careful;
	if (isCareful && self.hp < self.maxHp * 0.35f && Distance(self.cell, hero.cell) > 1)
	{
		StepToward(index, hero.cell);
		return;
	}

	const int range = isCareful ? 2 : 6;
	int target = NearestEnemyOf(index, range);

	//号令の相手を優先
	if (commandTarget_ > 0 && commandTarget_ < static_cast<int>(units_.size()) && units_[commandTarget_].isAlive &&
		units_[commandTarget_].faction == Faction::Enemy && Distance(self.cell, units_[commandTarget_].cell) <= 6)
	{
		target = commandTarget_;
	}

	if (target >= 0 && isCareful && Distance(units_[target].cell, hero.cell) > 2 && Distance(units_[target].cell, self.cell) > 1) target = -1;

	//主人公から離れすぎる敵は追わない
	if (target >= 0 && !IsAdjacent(self.cell, units_[target].cell) && Distance(units_[target].cell, hero.cell) > kAllyChaseRange) target = -1;

	if (target >= 0)
	{
		if (IsAdjacent(self.cell, units_[target].cell))
		{
			MeleeAttack(index, target);
			return;
		}
		if (TryRanged(index, target)) return;
		StepToward(index, units_[target].cell);
		return;
	}

	//遅れているときは2歩進んで追いつく
	if (Distance(self.cell, hero.cell) > 2) StepToward(index, hero.cell);
	if (Distance(self.cell, hero.cell) > 3) StepToward(index, hero.cell);
}

void DungeonScene::PlaceGate(int targetFloor)
{
	//スタートの部屋の中、すこし離れた床に置く(横を優先)
	const Cell start = map_.StartCell();
	for (int r = 2; r <= 4; r++)
	{
		std::vector<Cell> cands = { { -r, 0 }, { r, 0 }, { 0, -r }, { 0, r } };
		for (int dy = -r; dy <= r; dy++)
		{
			for (int dx = -r; dx <= r; dx++)
			{
				if (std::max(std::abs(dx), std::abs(dy)) == r && dx != 0 && dy != 0) cands.push_back({ dx, dy });
			}
		}
		for (const Cell& d : cands)
		{
			const Cell c = start + d;
			if (map_.Get(c) != Tile::Floor || IsBlocked(c) || !map_.Rooms().front().Contains(c)) continue;
			bool isNear = false;
			for (const Gate& g : gates_) if (Distance(g.cell, c) < 2) isNear = true;
			if (isNear) continue;
			gates_.push_back({ c, targetFloor });
			return;
		}
	}
}

int DungeonScene::GateAt(const Cell& c) const
{
	for (size_t i = 0; i < gates_.size(); i++)
	{
		if (gates_[i].cell == c) return static_cast<int>(i);
	}
	return -1;
}

bool DungeonScene::IsGateOpen(int targetFloor)
{
	return Save().deepestNormal >= targetFloor;
}

void DungeonScene::DrawGates()
{
	const Vec2 off(Game::kScreenWidth * 0.5f - camera_.x, Game::kScreenHeight * 0.5f - camera_.y);
	for (const Gate& g : gates_)
	{
		if (!map_.IsExplored(g.cell)) continue;
		const Vec2 p = CellToPos(g.cell) + off;
		const bool isOpen = IsGateOpen(g.floor);
		const unsigned int main = isOpen ? GetColor(120, 210, 255) : GetColor(110, 110, 130);
		const unsigned int light = isOpen ? GetColor(220, 245, 255) : GetColor(160, 160, 175);

		//床の魔法陣
		SetDrawBlendMode(DX_BLENDMODE_ADD, isOpen ? static_cast<int>(120 + 50 * std::sin(time_ * 4.0f)) : 50);
		DrawOvalAA(p.x, p.y + 6.0f, 22.0f, 11.0f, 24, main, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		DrawOvalAA(p.x, p.y + 6.0f, 21.0f, 10.0f, 24, light, FALSE, 2.0f);

		//まわる光の粒
		const float speed = isOpen ? 2.5f : 0.6f;
		for (int k = 0; k < 6; k++)
		{
			const float a = time_ * speed + kPi * 2.0f * k / 6.0f;
			const float rise = isOpen ? std::fmod(time_ * 20.0f + k * 7.0f, 24.0f) : 0.0f;
			DrawCircleAA(p.x + std::cos(a) * 18.0f, p.y + 6.0f + std::sin(a) * 9.0f - rise, 2.5f, 8, light, TRUE);
		}

		const std::wstring label = L"B" + std::to_wstring(g.floor) + L"F";
		Font::DrawCenter(p.x, p.y - 22.0f, label.c_str(), isOpen ? GetColor(200, 240, 255) : GetColor(140, 140, 160), FontSize::Small, GetColor(10, 10, 25));
	}
}

bool DungeonScene::WarpNearHero(int index)
{
	const Cell heroCell = units_[0].cell;
	for (int r = 1; r <= 2; r++)
	{
		for (int dy = -r; dy <= r; dy++)
		{
			for (int dx = -r; dx <= r; dx++)
			{
				const Cell c = heroCell + Cell{ dx, dy };
				if (Distance(c, heroCell) != r || !map_.IsWalkable(c) || IsBlocked(c)) continue;
				Unit& u = units_[index];
				u.cell = c;
				u.drawPos = CellToPos(c);
				Effects::GetInstance().Smoke(u.drawPos, 8, { 220, 220, 240 });
				return true;
			}
		}
	}
	return false;
}

void DungeonScene::ActEnemy(int index)
{
	if (TrySummon(index)) return;
	Unit& self = units_[index];
	if (TryHeal(index)) return;

	int target = -1;
	int best = 999;
	for (size_t i = 0; i < units_.size(); i++)
	{
		const Unit& u = units_[i];
		if (!u.isAlive || !u.IsParty()) continue;
		const int d = Distance(self.cell, u.cell);
		if (d <= 7 && d < best && map_.HasLineOfSight(self.cell, u.cell))
		{
			best = d;
			target = static_cast<int>(i);
		}
	}

	if (target < 0)
	{
		if (RandI(0, 2) == 0)
		{
			const Cell to = self.cell + kDirs[RandI(0, 3)];
			if (map_.IsWalkable(to) && !IsBlocked(to)) MoveUnit(index, to);
		}
		return;
	}

	if (IsAdjacent(self.cell, units_[target].cell))
	{
		MeleeAttack(index, target);
		return;
	}
	if (RandI(0, 1) == 0 && TryRanged(index, target)) return;
	StepToward(index, units_[target].cell);
}

void DungeonScene::MeleeAttack(int index, int target)
{
	Attack(index, target);
	const Unit& self = units_[index];

	//ボス・強敵は弱ると2回攻撃
	const bool isRage = self.faction == Faction::Enemy && (self.IsBoss() || self.isElite) && self.hp < self.maxHp / 2;
	if ((self.skill == Skill::Double || isRage) && units_[target].isAlive) Attack(index, target);

	//まわりの相手もまとめて攻撃
	if (units_[index].skill != Skill::Wide || !units_[index].isAlive) return;
	const bool isEnemy = units_[index].faction == Faction::Enemy;
	for (size_t i = 0; i < units_.size(); i++)
	{
		const Unit& o = units_[i];
		if (static_cast<int>(i) == target || !o.isAlive || o.IsParty() != isEnemy) continue;
		if (IsAdjacent(units_[index].cell, o.cell)) Attack(index, static_cast<int>(i), 0.7f, true);
	}
	Effects::GetInstance().Ring(units_[index].drawPos, { 255, 230, 160 }, 20.0f, 90.0f, 0.3f);
}

bool DungeonScene::TrySummon(int index)
{
	const Unit& self = units_[index];
	if (self.skill != Skill::Summon || self.faction != Faction::Enemy) return false;
	if (turn_ % 6 != 0 || Distance(self.cell, units_[0].cell) > 8) return false;

	int enemyNum = 0;
	for (const Unit& u : units_) if (u.isAlive && u.faction == Faction::Enemy) enemyNum++;
	if (enemyNum >= 12) return false;

	//まわりのあいている床に手下を2体
	const Cell center = self.cell;
	const std::wstring name = self.name;
	const Vec2 pos = self.drawPos;
	int summoned = 0;
	for (const Cell& d : kDirs)
	{
		if (summoned >= 2) break;
		const Cell c = center + d;
		if (!map_.IsWalkable(c) || IsBlocked(c)) continue;
		Unit e = MakeEnemy(MonsterTable::PickForFloor(floor_, stage_ == Stage::kInfinite), false, false);
		e.cell = c;
		e.drawPos = CellToPos(c);
		units_.push_back(e);
		Effects::GetInstance().Smoke(e.drawPos, 10, { 200, 180, 230 });
		summoned++;
	}
	if (summoned == 0) return false;

	Sound::GetInstance().PlaySE(SE::Signal);
	Effects::GetInstance().Ring(pos, { 220, 160, 255 }, 20.0f, 140.0f, 0.5f);
	AddMessage(name + L"は てしたを よんだ！", GetColor(220, 170, 255));
	return true;
}

bool DungeonScene::TryHeal(int index)
{
	Unit& self = units_[index];
	if (self.skill != Skill::Heal) return false;

	int target = -1;
	float lowest = 0.5f;
	for (size_t i = 0; i < units_.size(); i++)
	{
		const Unit& u = units_[i];
		if (!u.isAlive || u.IsParty() != self.IsParty()) continue;
		const float rate = static_cast<float>(u.hp) / u.maxHp;
		if (rate < lowest && Distance(self.cell, u.cell) <= 4 && map_.HasLineOfSight(self.cell, u.cell))
		{
			lowest = rate;
			target = static_cast<int>(i);
		}
	}
	if (target < 0 || RandI(0, 3) == 0) return false;

	Unit& t = units_[target];
	const int amount = std::max(4, static_cast<int>(t.maxHp * 0.35f));
	t.hp = std::min(t.maxHp, t.hp + amount);
	t.poisonTurns = 0;
	Effects& fx = Effects::GetInstance();
	fx.Burst(t.drawPos + Vec2(0.0f, -20.0f), 14, { 140, 255, 160 }, 60.0f, 200.0f, 8.0f, 0.6f, ParticleShape::Star, -100.0f);
	fx.Popup(t.drawPos + Vec2(0.0f, -50.0f), L"+" + std::to_wstring(amount), { 140, 255, 160 }, FontSize::Small);
	Sound::GetInstance().PlaySE(SE::Ready);
	if (self.IsParty()) AddMessage(self.name + L"は " + t.name + L"を かいふくした", GetColor(150, 255, 170));
	return true;
}

bool DungeonScene::TryRanged(int index, int target)
{
	Unit& self = units_[index];
	if (self.skill != Skill::Ranged) return false;
	const Unit& t = units_[target];
	if (Distance(self.cell, t.cell) > 4 || !map_.HasLineOfSight(self.cell, t.cell)) return false;

	projectiles_.push_back({ self.drawPos + Vec2(0.0f, -24.0f), t.drawPos + Vec2(0.0f, -20.0f), 0.0f, self.Data().color });
	Sound::GetInstance().PlaySE(SE::Dash);
	const int dmg = std::max(1, static_cast<int>(self.atk * 0.8f * RandF(0.85f, 1.15f) - t.def * 0.5f));
	Damage(index, target, dmg, false);
	return true;
}

void DungeonScene::Attack(int attacker, int target, float rate, bool isExtra)
{
	Unit& a = units_[attacker];
	Unit& t = units_[target];
	if (!t.isAlive) return;

	a.attackAnim = 1.0f;
	a.attackDir = (CellToPos(t.cell) - CellToPos(a.cell)).Normalized();
	if (t.cell.x != a.cell.x) a.facing = t.cell.x > a.cell.x ? 1.0f : -1.0f;

	//主人公は回避できる
	if (target == 0 && Chance(bonus_.Get(SkillKind::Evade)))
	{
		Effects::GetInstance().Popup(t.drawPos + Vec2(0.0f, -50.0f), L"ミス", { 200, 220, 255 }, FontSize::Small);
		Sound::GetInstance().PlaySE(SE::Whoosh);
		return;
	}

	const int critRate = kBaseCrit + (attacker == 0 ? baseCrit_ + bonus_.Get(SkillKind::Crit) : 0);
	const bool isCritical = Chance(critRate);
	float raw = a.atk * RandF(0.85f, 1.15f) - t.def * 0.5f;
	if (isCritical) raw = a.atk * 1.8f;
	raw *= rate;

	//号令の相手には仲間のダメージが上がる
	if (a.faction == Faction::Ally && target == commandTarget_) raw *= 1.0f + 0.15f * bonus_.Get(SkillKind::Command);

	//だいじな もので なかまの こうげきが あがる
	if (a.faction == Faction::Ally && attacker != 0) raw *= 1.0f + KeyItem::AllyAtkRate(Save()) / 100.0f;

	//ゆうしゃの こうげきが あがる スキル
	if (attacker == 0)
	{
		const Unit& hero = units_[0];
		if (t.paralyzeTurns > 0 || t.isStunned) raw *= 1.0f + bonus_.Get(SkillKind::Freeze) / 100.0f;
		if (t.poisonTurns > 0) raw *= 1.0f + bonus_.Get(SkillKind::VenomEdge) / 100.0f;
		if (hero.hp >= hero.maxHp) raw *= 1.0f + bonus_.Get(SkillKind::Peak) / 100.0f;
		if (hero.hp * 100 <= hero.maxHp * 30) raw *= 1.0f + bonus_.Get(SkillKind::Desperate) / 100.0f;
		const int bond = bonus_.Get(SkillKind::Bond);
		if (bond > 0)
		{
			int nearCount = 0;
			for (const Unit& u : units_)
			{
				if (u.isAlive && u.faction == Faction::Ally && u.monsterId >= 0 && Distance(u.cell, hero.cell) <= 2) nearCount++;
			}
			raw *= 1.0f + bond * nearCount / 100.0f;
		}
	}

	//いあつ: となりの てきの こうげきが よわまる
	if (target == 0 && a.faction == Faction::Enemy && Distance(a.cell, t.cell) <= 1)
	{
		raw *= std::max(0.2f, 1.0f - bonus_.Get(SkillKind::Intimidate) / 100.0f);
	}

	//主人公への被ダメ半減
	if (target == 0 && Chance(bonus_.Get(SkillKind::HalfDamage)))
	{
		raw *= 0.5f;
		Effects::GetInstance().Popup(t.drawPos + Vec2(0.0f, -80.0f), L"はんげん！", { 160, 220, 255 }, FontSize::Small);
	}

	const int dmg = std::max(1, static_cast<int>(std::round(raw)));
	Damage(attacker, target, dmg, isCritical);
	if (!units_[target].isAlive && attacker != 0) return;

	//モンスターの特技による状態異常
	Unit& tt = units_[target];
	const int resist = target == 0 ? bonus_.Get(SkillKind::Resist) : 0;
	if (tt.isAlive && a.skill == Skill::Poison && Chance(70 - resist))
	{
		tt.poisonTurns = 4;
		tt.poisonDamage = std::max(1, a.atk / 4);
		if (tt.IsParty()) AddMessage(tt.name + L"は どくに なった！", GetColor(210, 150, 255));
	}
	if (tt.isAlive && a.skill == Skill::Freeze && Chance(30 - resist / 2))
	{
		tt.paralyzeTurns = 2;
		if (tt.IsParty()) AddMessage(tt.name + L"は しびれて うごけなくなった！", GetColor(255, 230, 120));
	}
	if (tt.isAlive && a.skill == Skill::Burn && Chance(40 - resist / 2))
	{
		tt.burnTurns = 3;
		tt.burnDamage = std::max(1, a.atk / 5);
		if (tt.IsParty()) AddMessage(tt.name + L"は もえあがった！", GetColor(255, 160, 80));
	}

	//はんげき(となりの てきに すぐ やりかえす)
	if (target == 0 && !isExtra && a.faction == Faction::Enemy && a.isAlive && units_[0].isAlive &&
		Distance(a.cell, units_[0].cell) <= 1 && Chance(bonus_.Get(SkillKind::Counter)))
	{
		Effects::GetInstance().Popup(units_[0].drawPos + Vec2(0.0f, -80.0f), L"はんげき！", { 255, 200, 120 }, FontSize::Small);
		Attack(0, attacker, 1.0f, true);
	}

	if (attacker == 0) HeroOnHit(target, dmg);

	//2回攻撃・範囲攻撃は追加攻撃では起こさない
	if (attacker != 0 || isExtra) return;

	if (units_[target].isAlive && Chance(bonus_.Get(SkillKind::Double)))
	{
		Effects::GetInstance().Popup(units_[0].drawPos + Vec2(0.0f, -80.0f), L"れんぞく！", { 255, 220, 140 }, FontSize::Small);
		Attack(0, target, 1.0f, true);
	}

	//範囲攻撃
	const int wide = bonus_.Get(SkillKind::Wide);
	if (wide > 0)
	{
		const Cell heroCell = units_[0].cell;
		const Cell targetCell = units_[target].cell;
		for (size_t i = 1; i < units_.size(); i++)
		{
			const Unit& o = units_[i];
			if (static_cast<int>(i) == target || !o.isAlive || o.faction != Faction::Enemy) continue;
			bool isHit = false;
			if (wide == 1) isHit = Distance(o.cell, heroCell) == 1 && Distance(o.cell, targetCell) == 1;
			else if (wide == 2) isHit = Distance(o.cell, heroCell) == 1;
			else isHit = Distance(o.cell, heroCell) <= 2 && map_.HasLineOfSight(heroCell, o.cell);
			if (isHit) Attack(0, static_cast<int>(i), 0.6f, true);
		}
		Effects::GetInstance().Ring(units_[0].drawPos, { 255, 230, 160 }, 20.0f, wide >= 3 ? 110.0f : 70.0f, 0.25f);
	}
}

void DungeonScene::HeroOnHit(int target, int damage)
{
	Unit& hero = units_[0];

	//吸収
	const int drain = bonus_.Get(SkillKind::Drain);
	if (drain > 0) hero.hp = std::min(hero.maxHp, hero.hp + std::max(1, damage * drain / 100));

	Unit& t = units_[target];
	if (!t.isAlive) return;

	//号令
	if (bonus_.Get(SkillKind::Command) > 0) commandTarget_ = target;

	const int poison = bonus_.Get(SkillKind::Poison);
	if (poison > 0)
	{
		t.poisonTurns = 3 + poison;
		t.poisonDamage = std::max(t.poisonDamage, 1 + poison * 2 + floor_ / 3);
	}
	const int paralyze = bonus_.Get(SkillKind::Paralyze);
	if (paralyze > 0 && Chance(12 * paralyze) && !t.IsBoss())
	{
		t.paralyzeTurns = 2;
		Effects::GetInstance().Popup(t.drawPos + Vec2(0.0f, -70.0f), L"まひ！", { 255, 230, 100 }, FontSize::Small);
	}
	const int burn = bonus_.Get(SkillKind::Burn) + bonus_.Get(SkillKind::Blast);
	if (burn > 0)
	{
		t.burnTurns = 3;
		t.burnDamage = std::max(t.burnDamage, std::max(1, hero.atk * burn / 5));
	}
	if (Chance(bonus_.Get(SkillKind::Stun)) && !t.IsBoss())
	{
		t.isStunned = true;
		Effects::GetInstance().Popup(t.drawPos + Vec2(0.0f, -70.0f), L"ひるんだ！", { 255, 255, 255 }, FontSize::Small);
	}
}

void DungeonScene::Damage(int attacker, int target, int amount, bool isCritical)
{
	Unit& a = units_[attacker];
	Unit& t = units_[target];
	t.hp -= amount;
	t.hitFlash = 1.0f;

	Effects& fx = Effects::GetInstance();
	const Vec2 pos = t.drawPos + Vec2(0.0f, -24.0f);
	const Game::Rgb numColor = t.IsParty() ? Game::Rgb{ 255, 110, 100 } : Game::Rgb{ 255, 255, 255 };
	fx.Popup(pos + Vec2(0.0f, -20.0f), std::to_wstring(amount), numColor, isCritical ? FontSize::Large : FontSize::Medium);
	fx.Burst(pos, isCritical ? 16 : 7, { 255, 230, 150 }, 100.0f, 320.0f, 4.0f, 0.3f, ParticleShape::Spark);
	if (isCritical)
	{
		fx.Shake(8.0f, 0.2f);
		Sound::GetInstance().PlaySE(SE::Bang);
		AddMessage(L"かいしんの いちげき！", GetColor(255, 230, 100));
	}
	else
	{
		Sound::GetInstance().PlaySE(SE::Hit, RandF(0.9f, 1.1f));
		if (target == 0) fx.Shake(4.0f, 0.15f);
	}
	waitTimer_ = std::max(waitTimer_, 0.18f);

	if (a.skill == Skill::Drain && a.isAlive) a.hp = std::min(a.maxHp, a.hp + amount / 2);

	if (t.hp <= 0) Kill(target, attacker);
}

void DungeonScene::Kill(int target, int killer)
{
	Unit& t = units_[target];
	if (!t.isAlive) return;

	//ふくつ
	if (target == 0 && bonus_.Get(SkillKind::Guts) > 0 && !isGutsUsed_)
	{
		isGutsUsed_ = true;
		t.hp = 1;
		Effects::GetInstance().Flash({ 255, 240, 160 }, 160);
		Sound::GetInstance().PlaySE(SE::Point);
		AddMessage(L"ふくつの ちからで もちこたえた！", GetColor(255, 230, 120));
		return;
	}

	t.hp = 0;
	t.isAlive = false;

	Effects& fx = Effects::GetInstance();
	const Vec2 pos = t.drawPos + Vec2(0.0f, -20.0f);
	fx.Smoke(pos, 12, { 200, 200, 210 });
	fx.Burst(pos, 14, { 255, 240, 160 }, 120.0f, 360.0f, 9.0f, 0.5f, ParticleShape::Star);

	if (target == 0)
	{
		Sound::GetInstance().StopBGM();
		Sound::GetInstance().PlaySE(SE::Buzzer);
		AddMessage(L"ゆうしゃは ちからつきた…", GetColor(255, 120, 120));

		//死んだら素材だけ持ち帰り、お守りは消える
		BringBack(true);
		state_ = State::GameOver;
		stateTime_ = 0.0f;
		return;
	}

	if (t.faction == Faction::Ally)
	{
		Sound::GetInstance().PlaySE(SE::Fall);
		AddMessage(t.name + L"は たおれてしまった…", GetColor(255, 150, 150));
		return;
	}

	Sound::GetInstance().PlaySE(SE::Pop);
	AddMessage(t.name + L"を たおした！", GetColor(255, 255, 255));

	//いのちの けいやく: たおすたびに かいふく
	const int pact = bonus_.Get(SkillKind::LifePact);
	if (pact > 0 && killer >= 0 && killer < static_cast<int>(units_.size()) && units_[killer].IsParty() && units_[0].isAlive)
	{
		Unit& hero = units_[0];
		const int heal = std::max(1, hero.maxHp * pact / 100);
		hero.hp = std::min(hero.maxHp, hero.hp + heal);
		fx.Popup(hero.drawPos + Vec2(0.0f, -60.0f), L"+" + std::to_wstring(heal), { 140, 255, 160 }, FontSize::Small);
	}

	//ばくえん: もえている てきを たおすと ばくはつして もえうつる
	if (bonus_.Get(SkillKind::Blast) > 0 && t.burnTurns > 0 && blastDepth_ < 4 && units_[0].isAlive)
	{
		blastDepth_++;
		const Cell center = t.cell;
		const Unit& hero = units_[0];
		const int burnLv = bonus_.Get(SkillKind::Burn) + bonus_.Get(SkillKind::Blast);
		fx.Ring(t.drawPos, { 255, 140, 60 }, 20.0f, 110.0f, 0.35f);
		fx.Burst(t.drawPos, 24, { 255, 150, 60 }, 140.0f, 420.0f, 9.0f, 0.5f, ParticleShape::Spark);
		Sound::GetInstance().PlaySE(SE::Explosion);
		for (size_t i = 1; i < units_.size(); i++)
		{
			Unit& o = units_[i];
			if (static_cast<int>(i) == target || !o.isAlive || o.faction != Faction::Enemy || Distance(o.cell, center) > 1) continue;
			o.burnTurns = 3;
			o.burnDamage = std::max(o.burnDamage, std::max(1, hero.atk * burnLv / 5));
			Damage(0, static_cast<int>(i), std::max(1, static_cast<int>(hero.atk * 0.8f)), false);
		}
		blastDepth_--;
	}

	SaveSlot& s = Save();
	auto& book = t.isShiny ? s.zukanShiny : s.zukan;
	const bool isFirstShiny = t.isShiny && t.monsterId < static_cast<int>(book.size()) && !book[t.monsterId].isDefeated;
	if (t.monsterId < static_cast<int>(book.size())) book[t.monsterId].isDefeated = true;

	DropLoot(t, isFirstShiny);

	//ぬしを はじめて たおすと だいじな もの(もっているだけで 効く)
	const int keyItem = t.IsBoss() ? KeyItem::ForBoss(t.monsterId) : -1;
	const bool isNewKeyItem = keyItem >= 0 && !KeyItem::IsOwned(s, keyItem);
	if (isNewKeyItem)
	{
		KeyItem::Give(s, keyItem);
		gotKeyItem_ = keyItem;
		RecomputeBonus();
		for (Unit& u : units_) ApplyAllyHpBonus(u);
		AddMessage(std::wstring(L"だいじな もの「") + KeyItem::Get(keyItem).name + L"」を てにいれた！", GetColor(255, 220, 120));
		AddMessage(L"（" + KeyItem::EffectText(keyItem) + L"）", GetColor(255, 240, 200));
	}

	//さいごのボスを倒したらクリア(はじまりの どうくつ B12F・むげんの めいきゅう B100F)
	const bool isFinalBoss = t.IsBoss() && (stage_ == Stage::kNormal || floor_ >= Stage::kInfiniteLastFloor);
	if (isFinalBoss)
	{
		if (stage_ == Stage::kNormal)
		{
			isFirstClear_ = !s.isCleared;
			s.isCleared = true;
			s.clearCount++;
		}
		else
		{
			//とくべつな もの(ずっと効く)
			isFirstClear_ = isNewKeyItem;
		}
		Sound::GetInstance().StopBGM();
		Sound::GetInstance().PlaySE(SE::Crash);
		Sound::GetInstance().PlaySE(SE::Fanfare);
		Sound::GetInstance().PlayBGM(BGM::Victory);
		fx.Flash({ 255, 255, 230 }, 200);
		fx.ConfettiBurst(units_[0].drawPos + Vec2(0.0f, 100.0f), 120);
		state_ = State::Clear;
		BringBack();
		stateTime_ = 0.0f;
		return;
	}

	//とちゅうの ぬしを倒したら階段がひらく
	if (t.IsBoss())
	{
		Sound::GetInstance().StopBGM();
		Sound::GetInstance().PlaySE(SE::Crash);
		Sound::GetInstance().PlaySE(SE::Fanfare);
		Sound::GetInstance().PlayBGM(BGM::Dungeon);
		fx.Flash({ 255, 255, 230 }, 160);
		fx.ConfettiBurst(t.drawPos + Vec2(0.0f, 60.0f), 80);
		AddMessage(L"かいだんの ふういんが とけた！", GetColor(255, 230, 140));
	}

	//経験値
	const MonsterData m = t.Data();
	const int exp = m.exp * (t.isElite ? 3 : 1) * (100 + bonus_.Get(SkillKind::Exp)) / 100;
	for (size_t i = 0; i < units_.size(); i++)
	{
		if (units_[i].isAlive && units_[i].IsParty()) GainExp(static_cast<int>(i), exp);
	}

	//起き上がって仲間になりたそうにする
	//仲間にならないモンスター(確率0)はスカウトでも変わらない
	const int rate = m.recruitRate > 0 ? m.recruitRate + bonus_.Get(SkillKind::Recruit) : 0;
	if (!t.isElite && units_[killer].IsParty() && !hasPendingRecruit_ && Chance(rate))
	{
		if (AllyCount() < MaxAlly())
		{
			hasPendingRecruit_ = true;
			pendingUnit_ = t;
			Sound::GetInstance().PlaySE(SE::Ready);
		}
		else
		{
			AddMessage(m.name + L"が なかまに なりたそうだが、いっぱいだ", GetColor(200, 200, 220));
		}
	}
}

void DungeonScene::DropLoot(const Unit& dead, bool isFirstShiny)
{
	SaveSlot& s = Save();
	RunData& r = s.run;
	Effects& fx = Effects::GetInstance();
	const EquipSeries* series = EquipTable::Find(dead.monsterId);
	const Vec2 pos = dead.drawPos + Vec2(0.0f, -60.0f);

	//素材
	if (series && dead.monsterId < static_cast<int>(r.materials.size()))
	{
		const bool isGuaranteed = dead.IsBoss() || dead.isElite;
		if (isGuaranteed || Chance(kMaterialRate + bonus_.Get(SkillKind::Treasure)))
		{
			const int count = dead.IsBoss() ? 3 : (dead.isElite ? 2 : 1);
			r.materials[dead.monsterId] += count;
			fx.Popup(pos, L"+" + series->material, { 200, 255, 200 }, FontSize::Small);
		}
		//色違いの素材: はじめて倒したときは確定、あとは普通の素材と同じ確率
		if (dead.isShiny && (isFirstShiny || Chance(kMaterialRate + bonus_.Get(SkillKind::Treasure))))
		{
			r.shinyMaterials[dead.monsterId] += 1;
			AddMessage(EquipTable::MaterialName(*series, true) + L"を てにいれた！", GetColor(255, 240, 140));
			fx.Burst(pos, 16, { 255, 240, 160 }, 80.0f, 240.0f, 8.0f, 0.7f, ParticleShape::Star);
		}
	}

	//お守り(クリア後だけ)
	if (!s.isCleared) return;
	const bool isCharmGuaranteed = dead.IsBoss() || dead.isElite || dead.isShiny;
	const int charmRate = stage_ == Stage::kInfinite ? std::min(15, kCharmRateInfinite + floor_ / 10) : kCharmRateNormal;
	if (!isCharmGuaranteed && !Chance(charmRate)) return;

	const int power = stage_ == Stage::kInfinite ? floor_ / 5 + (dead.isElite ? 3 : 0) + (dead.IsBoss() ? 4 : 0) : 2 + (dead.IsBoss() ? 3 : 0);
	const Charm c = CharmMaker::Make(power, bonus_.Get(SkillKind::Luck));
	r.foundCharms.push_back(c);
	Sound::GetInstance().PlaySE(SE::BigCoin);
	fx.Burst(dead.drawPos, 18, CharmMaker::RarityColor(c.rarity), 100.0f, 300.0f, 9.0f, 0.7f, ParticleShape::Star);
	AddMessage(c.Name() + L"を ひろった！", Game::ToColor(CharmMaker::RarityColor(c.rarity)));
}

void DungeonScene::GainExp(int index, int exp)
{
	Unit& u = units_[index];
	u.exp += exp;
	while (u.exp >= u.NextExp())
	{
		u.exp -= u.NextExp();
		u.level++;

		if (index == 0)
		{
			//主人公は少しだけ育ち、あとは2択で選ぶ
			baseMaxHp_ += 4;
			baseAtk_ += 1;
			if (u.level % 2 == 0) baseDef_ += 1;
			u.hp += 4;
			ApplyHeroStats();
			pendingChoices_++;
		}
		else
		{
			GrowAlly(u);
			ApplyAllyHpBonus(u);
		}

		Effects& fx = Effects::GetInstance();
		fx.Popup(u.drawPos + Vec2(0.0f, -70.0f), L"LEVEL UP!", { 255, 230, 80 }, FontSize::Medium);
		fx.Burst(u.drawPos + Vec2(0.0f, -20.0f), 16, { 255, 230, 120 }, 80.0f, 260.0f, 9.0f, 0.7f, ParticleShape::Star, -150.0f);
		Sound::GetInstance().PlaySE(SE::BigCoin);
		AddMessage(u.name + L"は レベル" + std::to_wstring(u.level) + L"に あがった！", GetColor(255, 230, 100));
	}
}

void DungeonScene::MoveUnit(int index, const Cell& to)
{
	Unit& u = units_[index];
	if (to.x != u.cell.x) u.facing = to.x > u.cell.x ? 1.0f : -1.0f;
	u.cell = to;
}

void DungeonScene::StepToward(int index, const Cell& to)
{
	//主人公のそばで止まっている仲間とは入れかわれる
	const bool isAlly = units_[index].faction == Faction::Ally;
	auto isPassable = [this, isAlly](int other)
	{
		return isAlly && other > 0 && units_[other].faction == Faction::Ally && Distance(units_[other].cell, units_[0].cell) <= 2;
	};
	auto blocked = [this, &isPassable](const Cell& c)
	{
		const int other = UnitAt(c);
		return other >= 0 && !isPassable(other);
	};

	const Cell from = units_[index].cell;
	const int limit = isAlly ? kAllyPathLimit : 30;
	Cell next = map_.NextStep(from, to, [this](const Cell& c) { return IsBlocked(c); }, limit);
	if (next == from && isAlly) next = map_.NextStep(from, to, blocked, limit);
	if (next == from) return;

	const int other = UnitAt(next);
	if (other < 0)
	{
		MoveUnit(index, next);
	}
	else if (isPassable(other))
	{
		units_[other].cell = from;
		MoveUnit(index, next);
	}
}

//==============================//
// 判定
//==============================//

int DungeonScene::UnitAt(const Cell& c) const
{
	for (size_t i = 0; i < units_.size(); i++)
	{
		if (units_[i].isAlive && units_[i].cell == c) return static_cast<int>(i);
	}
	return -1;
}

bool DungeonScene::IsBlocked(const Cell& c) const
{
	return UnitAt(c) >= 0;
}

bool DungeonScene::IsAdjacent(const Cell& a, const Cell& b) const
{
	if (Distance(a, b) != 1) return false;
	const int dx = b.x - a.x;
	const int dy = b.y - a.y;
	if (dx != 0 && dy != 0) return map_.IsWalkable({ a.x + dx, a.y }) && map_.IsWalkable({ a.x, a.y + dy });
	return true;
}

int DungeonScene::Distance(const Cell& a, const Cell& b) const
{
	return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

int DungeonScene::NearestEnemyOf(int index, int range) const
{
	const Unit& self = units_[index];
	int target = -1;
	int best = range + 1;
	for (size_t i = 0; i < units_.size(); i++)
	{
		const Unit& u = units_[i];
		if (!u.isAlive || u.faction != Faction::Enemy || !map_.IsVisible(u.cell)) continue;
		const int d = Distance(self.cell, u.cell);
		if (d < best && map_.HasLineOfSight(self.cell, u.cell))
		{
			best = d;
			target = static_cast<int>(i);
		}
	}
	return target;
}

int DungeonScene::AllyCount() const
{
	int count = 0;
	for (const Unit& u : units_)
	{
		if (u.isAlive && u.faction == Faction::Ally) count++;
	}
	return count;
}

int DungeonScene::MaxAlly() const
{
	return kBaseAlly + bonus_.Get(SkillKind::AllySlot);
}

Vec2 DungeonScene::CellToPos(const Cell& c) const
{
	return { (c.x + 0.5f) * kTile, (c.y + 0.5f) * kTile };
}

//==============================//
// 表示
//==============================//

void DungeonScene::AddMessage(const std::wstring& text, unsigned int color)
{
	if (text.empty()) return;
	messages_.push_back({ text, color, 0.0f });
	while (messages_.size() > kMessageNum) messages_.pop_front();
}

void DungeonScene::Draw()
{
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(12, 10, 18), TRUE);

	if (state_ == State::Rest || state_ == State::CharmSwap)
	{
		DrawRest();
		if (state_ == State::CharmSwap) DrawCharmSwap();
		return;
	}

	DrawMap();
	if (state_ != State::Return && state_ != State::Clear && state_ != State::GameOver) DrawGates();
	DrawUnits();
	DrawHud();
	DrawMinimap();
	DrawMessages();

	if (state_ == State::Recruit) DrawRecruit();
	if (state_ == State::LevelChoice) DrawLevelChoice();
	if (state_ == State::Menu) DrawMenu();
	if (state_ == State::Status) DrawStatus();
	if (state_ == State::LeaveConfirm) { DrawMenu(); DrawLeaveConfirm(); }
	if (state_ == State::KeyItems) keyItemMenu_.Draw();
	if (state_ == State::Settings)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
		DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(10, 8, 20), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		settingsMenu_.Draw();
	}

	if (state_ == State::Descend)
	{
		const int alpha = static_cast<int>(255 * Clamp01(stateTime_ / 0.5f));
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
		DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		const std::wstring text = L"B" + std::to_wstring(warpTarget_ > 0 ? warpTarget_ : floor_ + 1) + L"F";
		Font::DrawCenterAlpha(640.0f, 360.0f, 1.0f, alpha, text.c_str(), GetColor(255, 255, 255), FontSize::Huge, GetColor(40, 40, 60));
	}
	if (state_ == State::GameOver || state_ == State::Clear || state_ == State::Return) DrawResult();
}

void DungeonScene::DrawMap()
{
	const Theme& th = ThemeOf(ThemeFloor(), stage_ == Stage::kInfinite);
	const float ox = Game::kScreenWidth * 0.5f - camera_.x;
	const float oy = Game::kScreenHeight * 0.5f - camera_.y;
	const int x0 = std::max(0, static_cast<int>(-ox / kTile) - 1);
	const int y0 = std::max(0, static_cast<int>(-oy / kTile) - 1);
	const int x1 = std::min(DungeonMap::kWidth - 1, x0 + static_cast<int>(Game::kScreenWidth / kTile) + 2);
	const int y1 = std::min(DungeonMap::kHeight - 1, y0 + static_cast<int>(Game::kScreenHeight / kTile) + 2);

	for (int y = y0; y <= y1; y++)
	{
		for (int x = x0; x <= x1; x++)
		{
			const Cell c = { x, y };
			if (!map_.IsExplored(c)) continue;
			const float sx = ox + x * kTile;
			const float sy = oy + y * kTile;
			const Tile tile = map_.Get(c);

			if (tile == Tile::Wall)
			{
				bool isEdge = false;
				for (const Cell& d : kDirs)
				{
					if (map_.IsWalkable(c + d)) { isEdge = true; break; }
				}
				if (!isEdge) continue;
				DrawBoxAA(sx, sy, sx + kTile, sy + kTile, Game::ToColor(th.wall), TRUE);
				DrawBoxAA(sx, sy, sx + kTile, sy + kTile * 0.35f, Game::ToColor(th.wallTop), TRUE);
				DrawLineAA(sx, sy + kTile * 0.35f, sx + kTile, sy + kTile * 0.35f, Game::ToColor(DrawUtil::Scale(th.wall, 0.7f)), 2.0f);
			}
			else
			{
				const Game::Rgb fc = (x + y) % 2 == 0 ? th.floorA : th.floorB;
				DrawBoxAA(sx, sy, sx + kTile, sy + kTile, Game::ToColor(fc), TRUE);
				DrawBoxAA(sx + 1.0f, sy + 1.0f, sx + kTile - 1.0f, sy + kTile - 1.0f, Game::ToColor(DrawUtil::Scale(fc, 0.9f)), FALSE);

				if (tile == Tile::Stairs)
				{
					DrawBoxAA(sx + 6.0f, sy + 6.0f, sx + kTile - 6.0f, sy + kTile - 6.0f, GetColor(30, 25, 30), TRUE);
					for (int k = 0; k < 4; k++)
					{
						const float yy = sy + 10.0f + k * 8.0f;
						DrawBoxAA(sx + 8.0f + k * 4.0f, yy, sx + kTile - 8.0f, yy + 5.0f, GetColor(170 - k * 30, 160 - k * 30, 150 - k * 30), TRUE);
					}
					if (map_.IsVisible(c))
					{
						const float a = 0.5f + 0.5f * std::sin(time_ * 4.0f);
						SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(60 * a));
						DrawBoxAA(sx, sy, sx + kTile, sy + kTile, GetColor(255, 255, 200), TRUE);
						SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
					}
				}
			}

			if (!map_.IsVisible(c))
			{
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
				DrawBoxAA(sx, sy, sx + kTile, sy + kTile, GetColor(8, 6, 14), TRUE);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			}
		}
	}
}

void DungeonScene::DrawUnits()
{
	const Vec2 off(Game::kScreenWidth * 0.5f - camera_.x, Game::kScreenHeight * 0.5f - camera_.y);

	std::vector<int> order;
	for (size_t i = 0; i < units_.size(); i++)
	{
		const Unit& u = units_[i];
		if (!u.isAlive && i != 0) continue;
		if (u.faction == Faction::Enemy && !map_.IsVisible(u.cell)) continue;
		order.push_back(static_cast<int>(i));
	}
	std::sort(order.begin(), order.end(), [this](int a, int b) { return units_[a].drawPos.y < units_[b].drawPos.y; });

	for (int i : order)
	{
		const Unit& u = units_[i];
		const Vec2 lunge = u.attackDir * (std::sin(u.attackAnim * kPi) * 14.0f);
		//歩くときの小さなはね
		const float rest = std::min(1.0f, (CellToPos(u.cell) - u.drawPos).Length() / kTile);
		const Vec2 hop(0.0f, -std::sin(rest * kPi) * 6.0f);
		const Vec2 ground = u.drawPos + off + Vec2(0.0f, kTile * 0.42f);
		const Vec2 feet = ground + lunge + hop;
		const bool isHurt = u.hitFlash > 0.5f || !u.isAlive;
		float size = u.IsBoss() ? 64.0f : 34.0f;
		if (u.isElite) size = 46.0f;

		//主人公の向き
		if (i == 0 && u.isAlive)
		{
			const Vec2 d = Vec2(static_cast<float>(u.dir.x), static_cast<float>(u.dir.y)).Normalized();
			const Vec2 side(-d.y, d.x);
			const Vec2 tip = ground + Vec2(d.x * 46.0f, d.y * 24.0f);
			const Vec2 back = ground + Vec2(d.x * 32.0f, d.y * 14.0f);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(170 + 60 * std::sin(time_ * 6.0f)));
			DrawTriangleAA(tip.x, tip.y, back.x + side.x * 9.0f, back.y + side.y * 6.0f, back.x - side.x * 9.0f, back.y - side.y * 6.0f, GetColor(255, 230, 90), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}

		if (u.faction == Faction::Ally)
		{
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
			DrawOvalAA(feet.x, feet.y, 22.0f, 7.0f, 16, GetColor(120, 200, 255), FALSE, 3.0f);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}

		//強敵の赤いオーラ
		if (u.isElite)
		{
			SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(70 + 40 * std::sin(time_ * 5.0f)));
			DrawCircleAA(feet.x, feet.y - size * 0.6f, size * 0.95f, 24, GetColor(255, 60, 40), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}

		if (u.monsterId < 0) UnitDraw::Hero(feet, size, time_, u.facing, isHurt);
		else UnitDraw::Monster(feet, size, u.Data(), time_, u.facing, isHurt);

		if (u.hitFlash > 0.0f)
		{
			SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(160 * u.hitFlash));
			DrawCircleAA(feet.x, feet.y - size * 0.6f, size * 0.7f, 24, GetColor(255, 255, 255), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}

		//状態異常のしるし
		float iconX = feet.x - 16.0f;
		const float iconY = feet.y - size * 1.25f - 10.0f;
		if (u.poisonTurns > 0) { DrawCircleAA(iconX, iconY, 6.0f, 12, GetColor(180, 90, 230), TRUE); iconX += 14.0f; }
		if (u.burnTurns > 0) { DrawOvalAA(iconX, iconY, 5.0f, 8.0f, 12, GetColor(255, 130, 40), TRUE); iconX += 14.0f; }
		if (u.paralyzeTurns > 0 || u.isStunned) { DrawUtil::Star(iconX, iconY, 7.0f, time_ * 5.0f, GetColor(255, 230, 80)); }

		if (u.hp < u.maxHp && i != 0)
		{
			const float w = u.IsBoss() ? 90.0f : 40.0f;
			const float by = feet.y - size * (u.IsBoss() ? 1.9f : 1.45f);
			const float rate = Clamp01(static_cast<float>(u.hp) / u.maxHp);
			DrawBoxAA(feet.x - w * 0.5f, by, feet.x + w * 0.5f, by + 6.0f, GetColor(20, 20, 30), TRUE);
			const unsigned int col = u.IsParty() ? GetColor(100, 220, 120) : GetColor(240, 90, 80);
			DrawBoxAA(feet.x - w * 0.5f + 1.0f, by + 1.0f, feet.x - w * 0.5f + 1.0f + (w - 2.0f) * rate, by + 5.0f, col, TRUE);
		}
	}

	for (const auto& p : projectiles_)
	{
		const Vec2 pos = p.from + (p.to - p.from) * p.t + off;
		SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
		DrawCircleAA(pos.x, pos.y, 12.0f, 16, Game::ToColor(p.color), TRUE);
		DrawCircleAA(pos.x, pos.y, 6.0f, 12, GetColor(255, 255, 255), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	Effects::GetInstance().SetDrawOffset(off);
	Effects::GetInstance().Draw();
	Effects::GetInstance().SetDrawOffset({});
}

void DungeonScene::DrawHud()
{
	const int fontS = Font::Get(FontSize::Small);

	//階の表示(文字の幅に合わせた枠)
	const std::wstring floorText = (stage_ == Stage::kInfinite ? L"むげん B" : L"B") + std::to_wstring(floor_) + L"F";
	const float floorW = static_cast<float>(Font::Width(floorText, FontSize::Medium)) + 40.0f;
	DrawUtil::Panel(16.0f, 14.0f, 16.0f + floorW, 74.0f, { 20, 18, 34 }, 210, { 255, 220, 120 });
	Font::DrawCenter(16.0f + floorW * 0.5f, 44.0f, floorText.c_str(), GetColor(255, 230, 120), FontSize::Medium, GetColor(30, 20, 10));

	constexpr float kPanelRight = 340.0f;
	float y = 86.0f;
	for (const Unit& u : units_)
	{
		if (!u.IsParty() || !u.isAlive) continue;
		DrawUtil::Panel(16.0f, y, kPanelRight, y + 58.0f, { 20, 18, 34 }, 200, u.faction == Faction::Player ? Game::Rgb{ 255, 220, 120 } : Game::Rgb{ 120, 200, 255 });
		const std::wstring name = u.name + L"  Lv" + std::to_wstring(u.level);
		Font::DrawFit(30.0f, y + 6.0f, name, u.isShiny ? GetColor(255, 240, 140) : GetColor(255, 255, 255), FontSize::Small, GetColor(20, 20, 30), kPanelRight - 46.0f);

		//HPの数字は右にそろえ、残りをバーにする
		const std::wstring hp = std::to_wstring(u.hp) + L"/" + std::to_wstring(u.maxHp);
		const float hpX = kPanelRight - 14.0f - Font::Width(hp, FontSize::Small);
		const float barRight = hpX - 10.0f;
		const float rate = Clamp01(static_cast<float>(u.hp) / u.maxHp);
		const unsigned int col = rate > 0.5f ? GetColor(90, 220, 110) : (rate > 0.25f ? GetColor(240, 200, 60) : GetColor(240, 80, 70));
		DrawRoundRectAA(30.0f, y + 36.0f, barRight, y + 48.0f, 5.0f, 5.0f, 8, GetColor(40, 40, 50), TRUE);
		if (rate > 0.0f) DrawRoundRectAA(31.0f, y + 37.0f, 31.0f + (barRight - 32.0f) * rate, y + 47.0f, 4.0f, 4.0f, 8, col, TRUE);
		DrawStringToHandle(static_cast<int>(hpX), static_cast<int>(y + 29.0f), hp.c_str(), GetColor(255, 255, 255), fontS, GetColor(20, 20, 30));
		y += 66.0f;
	}

	const wchar_t* tactics = tactics_ == Tactics::Attack ? L"さくせん：ガンガンいこうぜ" : L"さくせん：いのちだいじに";
	if (AllyCount() > 0)
	{
		Font::DrawFit(20.0f, y + 2.0f, tactics, GetColor(170, 220, 255), FontSize::Small, GetColor(20, 20, 30), 310.0f);
		y += 30.0f;
	}
	const std::wstring personality = L"せいかく：" + std::wstring(kPersonalities[personality_].name);
	Font::DrawFit(20.0f, y + 2.0f, personality, GetColor(200, 190, 230), FontSize::Small, GetColor(20, 20, 30), 310.0f);

	if (state_ == State::Input) ButtonGuide::DrawCenter(640.0f, 702.0f, L"{MOVE}：いどう　{A}：こうげき・かいだん　{Y}＋いどう：むきだけ かえる　{M}：メニュー", GetColor(170, 160, 190), FontSize::Small, GetColor(10, 10, 20));
}

void DungeonScene::DrawMinimap()
{
	constexpr float kCell = 4.0f;
	const float ox = Game::kScreenWidth - DungeonMap::kWidth * kCell - 16.0f;
	const float oy = 14.0f;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
	DrawBoxAA(ox - 6.0f, oy - 6.0f, ox + DungeonMap::kWidth * kCell + 6.0f, oy + DungeonMap::kHeight * kCell + 6.0f, GetColor(10, 8, 20), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	for (int y = 0; y < DungeonMap::kHeight; y++)
	{
		for (int x = 0; x < DungeonMap::kWidth; x++)
		{
			const Cell c = { x, y };
			if (!map_.IsExplored(c) || !map_.IsWalkable(c)) continue;
			unsigned int col = map_.IsVisible(c) ? GetColor(150, 160, 190) : GetColor(85, 90, 115);
			if (map_.Get(c) == Tile::Stairs) col = GetColor(255, 220, 80);
			if (GateAt(c) >= 0) col = GetColor(120, 210, 255);
			DrawBoxAA(ox + x * kCell, oy + y * kCell, ox + (x + 1) * kCell, oy + (y + 1) * kCell, col, TRUE);
		}
	}
	for (const Unit& u : units_)
	{
		if (!u.isAlive) continue;
		if (u.faction == Faction::Enemy && !map_.IsVisible(u.cell)) continue;
		const unsigned int col = u.faction == Faction::Player ? GetColor(255, 255, 255) : (u.faction == Faction::Ally ? GetColor(100, 200, 255) : GetColor(255, 80, 80));
		DrawCircleAA(ox + (u.cell.x + 0.5f) * kCell, oy + (u.cell.y + 0.5f) * kCell, kCell * 0.9f, 8, col, TRUE);
	}
}

void DungeonScene::DrawMessages()
{
	if (messages_.empty()) return;
	const float x = 360.0f;
	const float h = 30.0f;
	const float y0 = Game::kScreenHeight - 40.0f - messages_.size() * h;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
	DrawRoundRectAA(x - 14.0f, y0 - 10.0f, x + kMessageWidth + 14.0f, Game::kScreenHeight - 36.0f, 12.0f, 12.0f, 8, GetColor(10, 8, 20), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	for (size_t i = 0; i < messages_.size(); i++)
	{
		const Message& m = messages_[i];
		const float slide = (1.0f - EaseOutCubic(m.time * 5.0f)) * 30.0f;
		Font::DrawFit(x + slide, y0 + i * h, m.text, m.color, FontSize::Small, GetColor(10, 10, 20), kMessageWidth - slide);
	}
}

void DungeonScene::DrawRecruit()
{
	const MonsterData m = pendingUnit_.Data();
	const float pop = EaseOutBack(stateTime_ * 3.0f);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const float cx = 640.0f;
	const float cy = 340.0f;
	const float w = 440.0f * pop;
	const float h = 210.0f * pop;
	if (pop < 0.05f) return;
	DrawUtil::Panel(cx - w, cy - h, cx + w, cy + h, { 30, 26, 50 }, 240, { 255, 220, 120 });

	SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
	DrawCircleAA(cx - 290.0f * pop, cy + 10.0f, 80.0f * pop, 32, Game::ToColor(m.color), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	const float rise = std::fabs(std::sin(time_ * 3.0f)) * 8.0f;
	UnitDraw::Monster({ cx - 290.0f * pop, cy + 70.0f - rise }, 70.0f * pop, m, time_, 1.0f, false);

	const int x = static_cast<int>(cx - 180.0f * pop);
	const int fontS = Font::Get(FontSize::Small);
	const float textW = cx + w - 24.0f - x;
	//長い名前(色違いなど)は小さい文字にする
	const std::wstring riseText = pendingUnit_.name + L"が おきあがり";
	const bool isLong = Font::Width(riseText, FontSize::Medium) > textW;
	Font::DrawFit(static_cast<float>(x), cy - (isLong ? 140.0f : 150.0f), riseText, GetColor(255, 230, 140), isLong ? FontSize::Small : FontSize::Medium, GetColor(20, 20, 30), textW);
	Font::DrawFit(static_cast<float>(x), cy - 100.0f, L"なかまに なりたそうに こちらを みている！", GetColor(255, 255, 255), FontSize::Small, GetColor(20, 20, 30), textW);
	//仲間になったときの強さ(主人公と同じレベル)
	const Unit preview = MakeAllyFrom(pendingUnit_);
	const std::wstring stat = L"Lv" + std::to_wstring(preview.level) + L"　HP " + std::to_wstring(preview.maxHp) + L"　こうげき " + std::to_wstring(preview.atk) + L"　ぼうぎょ " + std::to_wstring(preview.def);
	Font::DrawFit(static_cast<float>(x), cy - 50.0f, stat, GetColor(200, 220, 255), FontSize::Small, GetColor(20, 20, 30), textW);
	Font::DrawFit(static_cast<float>(x), cy - 18.0f, std::wstring(L"とくぎ：") + MonsterTable::SkillName(m.skill), GetColor(200, 220, 255), FontSize::Small, GetColor(20, 20, 30), textW);
	DrawStringToHandle(x, static_cast<int>(cy + 30.0f), L"なかまに しますか？", GetColor(255, 255, 255), Font::Get(FontSize::Medium), GetColor(20, 20, 30));

	const wchar_t* choices[2] = { L"はい", L"いいえ" };
	for (int i = 0; i < 2; i++)
	{
		const float bx = static_cast<float>(x) + 50.0f + i * 180.0f;
		const float by = cy + 120.0f;
		const bool isSel = recruitCursor_ == i;
		DrawUtil::Panel(bx - 72.0f, by - 26.0f, bx + 72.0f, by + 26.0f, isSel ? Game::Rgb{ 200, 80, 90 } : Game::Rgb{ 50, 46, 70 }, 240, isSel ? Game::Rgb{ 255, 230, 120 } : Game::Rgb{ 120, 110, 150 });
		Font::DrawCenter(bx, by, choices[i], GetColor(255, 255, 255), FontSize::Medium, GetColor(20, 20, 30));
	}

	//ことわる前の確認
	if (!isRecruitConfirm_) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawUtil::Window(330.0f, 250.0f, 950.0f, 470.0f);
	Font::DrawCenterFit(640.0f, 300.0f, L"ほんとうに なかまに しませんか？", GetColor(255, 220, 200), FontSize::Medium, GetColor(20, 15, 30), 580.0f);
	Font::DrawCenterFit(640.0f, 345.0f, pendingUnit_.name + L"は もう もどって こないかも…", GetColor(200, 190, 220), FontSize::Small, GetColor(20, 15, 30), 580.0f);
	const wchar_t* confirm[2] = { L"なかまに しない", L"もどる" };
	for (int i = 0; i < 2; i++)
	{
		const float bx = 520.0f + i * 240.0f;
		const float by = 415.0f;
		const bool isSel = recruitConfirmCursor_ == i;
		DrawUtil::Panel(bx - 105.0f, by - 26.0f, bx + 105.0f, by + 26.0f, isSel ? Game::Rgb{ 200, 80, 90 } : Game::Rgb{ 50, 46, 70 }, 240, isSel ? Game::Rgb{ 255, 230, 120 } : Game::Rgb{ 120, 110, 150 });
		Font::DrawCenterFit(bx, by, confirm[i], GetColor(255, 255, 255), FontSize::Small, GetColor(20, 20, 30), 190.0f);
	}
}

void DungeonScene::DrawLevelChoice()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const float pop = EaseOutBack(stateTime_ * 3.0f);
	if (pop < 0.05f) return;
	DrawUtil::Window(640.0f - 330.0f * pop, 220.0f, 640.0f + 330.0f * pop, 470.0f);
	const std::wstring title = L"レベル" + std::to_wstring(units_[0].level) + L"！ どちらを のばす？";
	Font::DrawCenter(640.0f, 270.0f, title.c_str(), GetColor(255, 230, 120), FontSize::Medium, GetColor(20, 15, 30));
	for (int i = 0; i < 2; i++)
	{
		const float bx = 500.0f + i * 280.0f;
		const bool isSel = i == choiceCursor_;
		const float s = isSel ? 1.0f + 0.04f * std::sin(time_ * 8.0f) : 1.0f;
		DrawUtil::Panel(bx - 120.0f * s, 330.0f, bx + 120.0f * s, 420.0f, isSel ? Game::Rgb{ 70, 60, 120 } : Game::Rgb{ 40, 36, 60 }, 240,
			isSel ? Game::Rgb{ 255, 230, 120 } : Game::Rgb{ 110, 100, 140 });
		Font::DrawCenterFit(bx, 375.0f, kGrowth[choiceOptions_[i]].name, GetColor(255, 255, 255), FontSize::Medium, GetColor(20, 15, 30), 215.0f);
	}
}

void DungeonScene::DrawMenu()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawUtil::Window(390.0f, 140.0f, 890.0f, 590.0f);
	Font::DrawCenter(640.0f, 185.0f, L"メニュー", GetColor(255, 230, 140), FontSize::Medium, GetColor(20, 20, 30));
	const wchar_t* items[6] =
	{
		tactics_ == Tactics::Attack ? L"さくせん：ガンガンいこうぜ" : L"さくせん：いのちだいじに",
		L"つよさを みる",
		L"だいじな もの",
		L"せってい",
		L"ダンジョンから でる",
		L"とじる",
	};
	for (int i = 0; i < 6; i++)
	{
		const float y = 250.0f + i * 50.0f;
		const bool isSel = menuCursor_ == i;
		if (isSel) DrawUtil::CursorBeside(650.0f, y, items[i], FontSize::Small, time_);
		Font::DrawCenter(650.0f, y, items[i], isSel ? GetColor(255, 255, 255) : GetColor(170, 160, 190), FontSize::Small, GetColor(20, 20, 30));
	}
	ButtonGuide::DrawCenter(640.0f, 555.0f, L"{A}：えらぶ　{B}：もどる", GetColor(150, 140, 170), FontSize::Small, GetColor(20, 20, 30));
}

void DungeonScene::DrawLeaveConfirm()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const unsigned int edge = GetColor(20, 15, 30);
	DrawUtil::Window(300.0f, 230.0f, 980.0f, 490.0f);
	Font::DrawCenterFit(640.0f, 288.0f, L"ダンジョンから でて 村へ かえる？", GetColor(255, 230, 160), FontSize::Medium, edge, 620.0f);
	Font::DrawCenterFit(640.0f, 340.0f, L"ひろった そざい・おまもりは ぜんぶ もちかえれる", GetColor(200, 220, 240), FontSize::Small, edge, 620.0f);
	const wchar_t* choices[2] = { L"かえる", L"やめる" };
	for (int i = 0; i < 2; i++)
	{
		const float bx = 540.0f + i * 200.0f;
		const bool isSel = i == leaveCursor_;
		DrawUtil::Panel(bx - 80.0f, 392.0f, bx + 80.0f, 448.0f, isSel ? Game::Rgb{ 70, 110, 190 } : Game::Rgb{ 50, 45, 70 }, 240,
			isSel ? Game::Rgb{ 255, 230, 120 } : Game::Rgb{ 120, 110, 150 });
		Font::DrawCenter(bx, 420.0f, choices[i], GetColor(255, 255, 255), FontSize::Medium, edge);
	}
}

void DungeonScene::DrawStatus()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawUtil::Window(200.0f, 90.0f, 1080.0f, 630.0f);

	const Unit& hero = units_[0];
	const int fontS = Font::Get(FontSize::Small);
	const unsigned int white = GetColor(240, 235, 255);
	const unsigned int edge = GetColor(20, 15, 30);
	Font::DrawCenter(640.0f, 130.0f, L"つよさ", GetColor(255, 230, 140), FontSize::Medium, edge);

	const std::wstring lines[] =
	{
		L"レベル " + std::to_wstring(hero.level) + L"　けいけんち " + std::to_wstring(hero.exp) + L"/" + std::to_wstring(hero.NextExp()),
		L"HP " + std::to_wstring(hero.hp) + L"/" + std::to_wstring(hero.maxHp) + L"　こうげき " + std::to_wstring(hero.atk) + L"　ぼうぎょ " + std::to_wstring(hero.def),
		L"かいしん " + std::to_wstring(kBaseCrit + baseCrit_ + bonus_.Get(SkillKind::Crit)) + L"%",
		L"せいかく：" + std::wstring(kPersonalities[personality_].name) + L"（" + kPersonalities[personality_].desc + L"）",
	};
	for (int i = 0; i < 4; i++) Font::DrawFit(250.0f, 170.0f + i * 36.0f, lines[i], white, FontSize::Small, edge, 780.0f);

	//装備とお守り
	const SaveSlot& s = SaveManager::GetInstance().Current();
	auto partText = [&s](int id, bool isShiny, EquipPart part) -> std::wstring
	{
		const EquipSeries* e = id >= 0 ? EquipTable::Find(id) : nullptr;
		return e ? EquipTable::PartName(*e, part, isShiny) : L"なし";
	};
	const std::wstring equip = L"うえ：" + partText(s.equipUpper, s.isEquipUpperShiny, EquipPart::Upper) +
		L"　した：" + partText(s.equipLower, s.isEquipLowerShiny, EquipPart::Lower);
	Font::DrawFit(250.0f, 322.0f, equip, white, FontSize::Small, edge, 780.0f);

	const Charm* charm = Save().run.runCharm >= 0 ? &Save().run.foundCharms[Save().run.runCharm] :
		(s.equippedCharm >= 0 && s.equippedCharm < static_cast<int>(s.charms.size()) ? &s.charms[s.equippedCharm] : nullptr);
	Font::DrawFit(250.0f, 358.0f, L"おまもり：" + (charm ? charm->Name() : std::wstring(L"なし")) + (KeyItem::OwnedCount(s) > 0 ? L"　／　だいじな もの " + std::to_wstring(KeyItem::OwnedCount(s)) + L"こ" : std::wstring()), white, FontSize::Small, edge, 780.0f);

	//発動中のスキル(5行ずつ、ふだんは2列)
	constexpr int kSkillRows = 5;
	std::vector<std::pair<std::wstring, bool>> skills;
	for (int k = static_cast<int>(SkillKind::Atk); k < static_cast<int>(SkillKind::Num); k++)
	{
		const int v = bonus_.Get(static_cast<SkillKind>(k));
		if (v != 0) skills.push_back({ SkillInfo::Describe({ static_cast<SkillKind>(k), v }), v > 0 });
	}
	const int cols = std::max(2, (static_cast<int>(skills.size()) + kSkillRows - 1) / kSkillRows);
	const float colW = 790.0f / cols;
	DrawStringToHandle(250, 404, L"はつどうちゅうの スキル", GetColor(255, 230, 140), fontS, edge);
	for (size_t i = 0; i < skills.size(); i++)
	{
		const float x = 262.0f + (i / kSkillRows) * colW;
		const float y = 438.0f + (i % kSkillRows) * 29.0f;
		Font::DrawFit(x, y, skills[i].first, skills[i].second ? GetColor(200, 230, 255) : GetColor(255, 160, 160), FontSize::Small, edge, colW - 14.0f);
	}
	if (skills.empty()) DrawStringToHandle(262, 438, L"なし", GetColor(150, 140, 170), fontS, edge);

	ButtonGuide::DrawCenter(640.0f, 606.0f, L"{A} / {B}：もどる", GetColor(150, 140, 170), FontSize::Small, edge);
}

void DungeonScene::DrawRest()
{
	DrawUtil::Gradient(0, 0, Game::kScreenWidth, Game::kScreenHeight, { 30, 40, 70 }, { 10, 12, 25 });

	//たき火
	const float fx = 640.0f;
	const float fy = 420.0f;
	SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
	DrawCircleAA(fx, fy, 140.0f + std::sin(time_ * 9.0f) * 8.0f, 40, GetColor(255, 120, 40), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawOvalAA(fx, fy, 22.0f, 38.0f + std::sin(time_ * 14.0f) * 4.0f, 20, GetColor(255, 170, 60), TRUE);
	DrawOvalAA(fx, fy + 8.0f, 12.0f, 20.0f, 16, GetColor(255, 235, 140), TRUE);
	UnitDraw::Hero({ fx - 140.0f, fy + 40.0f }, 52.0f, time_, 1.0f, false);
	float ax = fx + 120.0f;
	for (const Unit& u : units_)
	{
		if (u.faction != Faction::Ally || !u.isAlive) continue;
		UnitDraw::Monster({ ax, fy + 46.0f }, 38.0f, u.Data(), time_, -1.0f, false);
		ax += 90.0f;
	}

	const std::wstring title = L"むげん B" + std::to_wstring(floor_) + L"F　きゅうけいじょ";
	Font::DrawCenterFit(640.0f, 80.0f, title, GetColor(255, 230, 140), FontSize::Large, GetColor(30, 20, 10), 1200.0f);

	const RunData& r = Save().run;
	int mats = 0;
	for (int m : r.materials) mats += m;
	for (int m : r.shinyMaterials) mats += m;
	const std::wstring info = L"これまでに ひろった そざい " + std::to_wstring(mats) + L"こ　おまもり " + std::to_wstring(r.foundCharms.size()) + L"こ";
	Font::DrawCenterFit(640.0f, 140.0f, info + L"　（たきびで みんな ぜんかい）", GetColor(220, 220, 240), FontSize::Small, GetColor(20, 15, 30), 1200.0f);

	DrawUtil::Window(400.0f, 180.0f, 880.0f, 360.0f);
	const wchar_t* items[3] = { L"つぎへ すすむ", L"おまもりを つけかえる", L"ちじょうへ かえる（もちかえる）" };
	for (int i = 0; i < 3; i++)
	{
		const float y = 220.0f + i * 52.0f;
		const bool isSel = menuCursor_ == i && state_ == State::Rest;
		if (isSel) DrawUtil::Cursor(445.0f, y, time_);
		const bool isDisabled = i == 1 && r.foundCharms.empty();
		Font::DrawFit(480.0f, y - 14.0f, items[i], isDisabled ? GetColor(110, 100, 130) : (isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230)),
			FontSize::Small, GetColor(20, 15, 30), 380.0f);
	}
	Font::DrawCenterFit(640.0f, 690.0f, L"すすむと もっと よい おまもりや そざいが でやすく なる。ちからつきても ひろった ものは もちかえれる", GetColor(200, 180, 160), FontSize::Small, GetColor(20, 15, 30), 1220.0f);
}

void DungeonScene::DrawCharmSwap()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const SaveSlot& s = SaveManager::GetInstance().Current();
	const RunData& r = s.run;
	DrawUtil::Window(120.0f, 90.0f, 1160.0f, 640.0f);
	Font::DrawCenter(640.0f, 125.0f, L"どの おまもりを つける？", GetColor(255, 230, 140), FontSize::Medium, GetColor(20, 15, 30));

	const int fontS = Font::Get(FontSize::Small);
	const int num = static_cast<int>(r.foundCharms.size()) + 1;
	const int first = std::max(0, std::min(swapCursor_ - 5, num - 11));
	for (int i = first; i < std::min(num, first + 11); i++)
	{
		const float y = 175.0f + (i - first) * 38.0f;
		const bool isSel = i == swapCursor_;
		if (isSel) DrawUtil::Cursor(150.0f, y, time_);
		const bool isNow = (i - 1) == r.runCharm;
		std::wstring text;
		unsigned int col = GetColor(210, 200, 230);
		if (i == 0)
		{
			const bool hasBase = s.equippedCharm >= 0 && s.equippedCharm < static_cast<int>(s.charms.size());
			text = L"きょてんで つけた もの：" + (hasBase ? s.charms[s.equippedCharm].Name() : std::wstring(L"なし"));
		}
		else
		{
			const Charm& c = r.foundCharms[i - 1];
			text = c.Name();
			col = Game::ToColor(CharmMaker::RarityColor(c.rarity));
		}
		if (isNow) text += L"（いま）";
		Font::DrawFit(185.0f, y - 14.0f, text, col, FontSize::Small, GetColor(20, 15, 30), 500.0f);
	}

	//選んでいるお守りの中身
	const Charm* c = nullptr;
	if (swapCursor_ == 0) { if (s.equippedCharm >= 0 && s.equippedCharm < static_cast<int>(s.charms.size())) c = &s.charms[s.equippedCharm]; }
	else c = &r.foundCharms[swapCursor_ - 1];
	DrawUtil::Panel(700.0f, 160.0f, 1130.0f, 600.0f, { 36, 32, 56 }, 240, { 120, 110, 150 });
	if (c)
	{
		Font::DrawCenterFit(915.0f, 200.0f, c->Name(), Game::ToColor(CharmMaker::RarityColor(c->rarity)), FontSize::Small, GetColor(20, 15, 30), 410.0f);
		Font::DrawFit(730.0f, 250.0f, L"メイン：" + SkillInfo::Describe(c->main), GetColor(255, 220, 160), FontSize::Small, GetColor(20, 15, 30), 385.0f);
		for (size_t k = 0; k < c->subs.size(); k++)
		{
			Font::DrawFit(730.0f, 300.0f + k * 40.0f, L"サブ：" + SkillInfo::Describe(c->subs[k]), GetColor(200, 230, 255), FontSize::Small, GetColor(20, 15, 30), 385.0f);
		}
		if (c->rare.kind != SkillKind::None)
		{
			Font::DrawFit(730.0f, 440.0f, L"レア：" + SkillInfo::Describe(c->rare), GetColor(255, 200, 255), FontSize::Small, GetColor(20, 15, 30), 385.0f);
		}
	}
	else
	{
		Font::DrawCenter(915.0f, 380.0f, L"おまもり なし", GetColor(150, 140, 170), FontSize::Small, GetColor(20, 15, 30));
	}
	ButtonGuide::DrawCenter(640.0f, 615.0f, L"{A}：つける　{B}：もどる", GetColor(150, 140, 170), FontSize::Small, GetColor(20, 15, 30));
}

void DungeonScene::DrawResult()
{
	const unsigned int edge = GetColor(30, 10, 10);
	if (state_ == State::GameOver)
	{
		const int alpha = static_cast<int>(235 * Clamp01(stateTime_ / 1.0f));
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
		DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(40, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		Font::DrawCenterScaled(640.0f, 250.0f, EaseOutBack(stateTime_), L"ゲームオーバー", GetColor(255, 90, 90), FontSize::Huge, edge);
		const std::wstring info = L"B" + std::to_wstring(floor_) + L"F で ちからつきた　レベル " + std::to_wstring(units_[0].level);
		Font::DrawCenterFit(640.0f, 350.0f, info, GetColor(255, 230, 230), FontSize::Medium, edge, 1200.0f);
		const std::wstring got = L"もちかえった もの：そざい " + std::to_wstring(resultMaterials_) + L"こ　おまもり " + std::to_wstring(resultCharms_) + L"こ";
		Font::DrawCenter(640.0f, 400.0f, got.c_str(), GetColor(255, 240, 200), FontSize::Small, edge);
		if (gotKeyItem_ >= 0)
		{
			Font::DrawCenterFit(640.0f, 445.0f, std::wstring(L"だいじな もの「") + KeyItem::Get(gotKeyItem_).name + L"」は てばなさずに すんだ", GetColor(255, 220, 140), FontSize::Small, edge, 1200.0f);
		}
	}
	else
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(240 * Clamp01(stateTime_ * 2.0f)));
		DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		if (RandI(0, 5) == 0) Effects::GetInstance().ConfettiBurst(units_[0].drawPos + Vec2(RandF(-500.0f, 500.0f), 400.0f), 6);

		const bool isInfiniteClear = state_ == State::Clear && stage_ == Stage::kInfinite;
		const wchar_t* title = isInfiniteClear ? L"むげんの めいきゅう とうは！" : (state_ == State::Clear ? L"ダンジョン クリア！" : L"ぶじに かえってきた！");
		const float titleFit = std::min(1.0f, 1200.0f / Font::Width(title, FontSize::Huge));
		Font::DrawCenterScaled(640.0f, 200.0f, EaseOutElastic(stateTime_) * titleFit, title, GetColor(255, 230, 80), FontSize::Huge, GetColor(90, 40, 10));
		const std::wstring info = L"もちかえった もの：そざい " + std::to_wstring(resultMaterials_) + L"こ　おまもり " + std::to_wstring(resultCharms_) + L"こ";
		Font::DrawCenterFit(640.0f, 310.0f, info, GetColor(255, 255, 255), FontSize::Medium, GetColor(60, 30, 10), 1200.0f);
		if (isInfiniteClear && isFirstClear_)
		{
			//とくべつな ほうび
			const std::wstring got = std::wstring(L"とくべつな もの「") + Relic::Name() + L"」を てにいれた！";
			Font::DrawCenterFit(640.0f, 380.0f, got, GetColor(255, 220, 140), FontSize::Medium, GetColor(60, 30, 10), 1200.0f);
			std::wstring effects;
			for (const SkillValue& v : Relic::Skills()) effects += (effects.empty() ? L"" : L"　") + SkillInfo::Describe(v);
			Font::DrawCenterFit(640.0f, 430.0f, L"ずっと はつどう：" + effects, GetColor(230, 220, 255), FontSize::Small, GetColor(30, 20, 40), 1200.0f);
			DrawUtil::Star(640.0f - Font::Width(got, FontSize::Medium) * 0.5f - 34.0f, 380.0f, 18.0f, time_ * 2.0f, GetColor(255, 220, 80));
		}
		else if (state_ == State::Clear && isFirstClear_)
		{
			Font::DrawCenter(640.0f, 380.0f, L"あたらしい せかいが ひらかれた！", GetColor(255, 220, 140), FontSize::Medium, GetColor(60, 30, 10));
			Font::DrawCenterFit(640.0f, 430.0f, L"むげんの めいきゅう・いろちがいの モンスター・おまもりが かいほう された", GetColor(230, 220, 255), FontSize::Small, GetColor(30, 20, 40), 1200.0f);
		}

		//とちゅうの ぬしの だいじな もの(むげん とうはは うえで だしている)
		if (gotKeyItem_ >= 0 && !isInfiniteClear)
		{
			const float y = (state_ == State::Clear && isFirstClear_) ? 470.0f : 380.0f;
			const std::wstring got = std::wstring(L"だいじな もの「") + KeyItem::Get(gotKeyItem_).name + L"」を てにいれた！（" + KeyItem::EffectText(gotKeyItem_) + L"）";
			Font::DrawCenterFit(640.0f, y, got, GetColor(255, 220, 140), FontSize::Small, GetColor(60, 30, 10), 1200.0f);
		}
	}
	if (stateTime_ > 2.0f) ButtonGuide::DrawCenter(640.0f, 520.0f, L"{A}：むらへ もどる", GetColor(255, 255, 255), FontSize::Small, edge);
}
