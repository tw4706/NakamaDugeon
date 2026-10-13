#include "SmithScene.h"
#include "SceneManager.h"
#include "CampScene.h"
#include "SaveData.h"
#include "MonsterData.h"
#include "EquipData.h"
#include "Charm.h"
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

namespace
{
	constexpr int kVisibleRows = 9;
	constexpr int kEquipJump = 5;	//そうび一覧で ←→ の とぶ かず
	constexpr int kMaxCharm = 100;

	//錬成に必要なかけら
	constexpr int kCraftCost = 10;
	constexpr int kHighCraftCost = 40;
	constexpr int kAlchemyCost = 20;

	//れんきんに必要なスキルだま(メイン・サブ・レア)
	constexpr int kMainOrbs = 3;
	constexpr int kSubOrbs = 2;
	constexpr int kRareOrbs = 3;
	constexpr int kAlchemyRows = 5;

	//お守り一覧の先頭にある錬成の行
	constexpr int kCraftRows = 3;

	//その下に「まとめて ぶんかい」の行、そのあとに お守り
	constexpr int kBulkRow = kCraftRows;
	constexpr int kTopRows = kCraftRows + 1;

	//お守りの ならびかえ
	const wchar_t* kSortNames[] = { L"てにいれた じゅん", L"あたらしい じゅん", L"★が たかい じゅん", L"★が ひくい じゅん", L"スキル じゅん" };
	constexpr int kSortNum = static_cast<int>(std::size(kSortNames));

	//まとめて ぶんかいの メニュー
	enum BulkItem
	{
		kBulkBreak,
		kBulkStar1,
		kBulkStar2,
		kBulkStar3,
		kBulkStar4,
		kBulkClear,
		kBulkBack,
		kBulkNum,
	};

	//トップの項目(つくる・おまもり・スキル・でる)
	constexpr int kTopItems = 4;

	//スキルの せつめい(こうげき〜ふくつ)
	constexpr int kFirstSkill = static_cast<int>(SkillKind::Atk);
	constexpr int kSkillNum = static_cast<int>(SkillKind::Num) - kFirstSkill;
	constexpr int kSkillRows = 11;

	//お守りの サブ・レアに付くスキル
	const SkillKind kCharmSubs[] = { SkillKind::Atk, SkillKind::Def, SkillKind::Hp, SkillKind::Crit, SkillKind::Regen, SkillKind::Exp, SkillKind::Vision, SkillKind::Resist };
	const SkillKind kCharmRares[] = { SkillKind::Shiny, SkillKind::AllySlot, SkillKind::Recruit, SkillKind::Luck, SkillKind::Guts };

	//性格で付くスキル
	const SkillKind kPersonalitySkills[] = { SkillKind::Atk, SkillKind::Def, SkillKind::Hp, SkillKind::Evade, SkillKind::Recruit, SkillKind::Exp };
}

SmithScene::SmithScene(SceneManager& manager) :
	Scene(manager)
{
}

void SmithScene::Init()
{
	BuildRows();
	BuildRecommend();
}

void SmithScene::BuildRows()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	rows_.clear();
	for (const EquipSeries& e : EquipTable::All())
	{
		const bool isKnown = e.monsterId < static_cast<int>(s.zukan.size()) && s.zukan[e.monsterId].isSeen;
		rows_.push_back({ e.monsterId, false, isKnown });
	}

	//色違いシリーズはクリア後
	if (s.isCleared)
	{
		for (const EquipSeries& e : EquipTable::All())
		{
			const bool isKnown = e.monsterId < static_cast<int>(s.zukanShiny.size()) && s.zukanShiny[e.monsterId].isSeen;
			rows_.push_back({ e.monsterId, true, isKnown });
		}
	}
}

void SmithScene::BuildRecommend()
{
	//ねらいごとに つよい じゅんの シリーズ(みつけた モンスターの中で いちばん上を おすすめ)
	struct Pick
	{
		const wchar_t* label;
		std::vector<int> ids;
	};
	const Pick kPicks[] =
	{
		{ L"ぬし戦", { 42, 28, 25, 13, 5 } },
		{ L"かりょく", { 44, 38, 33, 19, 6 } },
		{ L"まもり", { 37, 32, 27, 22, 16, 10, 4 } },
		{ L"かいふく", { 43, 31, 34, 17, 7, 3 } },
		{ L"そだて", { 39, 11 } },
	};
	const SaveSlot& s = SaveManager::GetInstance().Current();
	recommend_.assign(s.zukan.size(), L"");
	for (const Pick& p : kPicks)
	{
		for (int id : p.ids)
		{
			if (id >= static_cast<int>(s.zukan.size()) || !s.zukan[id].isSeen || !EquipTable::Find(id)) continue;
			if (!recommend_[id].empty()) recommend_[id] += L"・";
			recommend_[id] += p.label;
			break;
		}
	}
}

void SmithScene::Notice(const std::wstring& text)
{
	notice_ = text;
	noticeTime_ = 2.2f;
}

void SmithScene::Update()
{
	time_ += Game::kFrameTime;
	noticeTime_ = std::max(0.0f, noticeTime_ - Game::kFrameTime);
	if (isLeaving_) return;

	switch (mode_)
	{
	case Mode::Top:			UpdateTop(); break;
	case Mode::Equip:		UpdateEquip(); break;
	case Mode::EquipAction:	UpdateEquipAction(); break;
	case Mode::Charm:		UpdateCharm(); break;
	case Mode::CharmAction:	UpdateCharmAction(); break;
	case Mode::BulkMenu:	UpdateBulkMenu(); break;
	case Mode::BulkConfirm:	UpdateBulkConfirm(); break;
	case Mode::Alchemy:		UpdateAlchemy(); break;
	case Mode::SkillBook:	UpdateSkillBook(); break;
	}
}

//==============================//
// トップ
//==============================//

void SmithScene::UpdateTop()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	if (input.IsAnyTriggered(Button::Up)) { topCursor_ = (topCursor_ + kTopItems - 1) % kTopItems; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { topCursor_ = (topCursor_ + 1) % kTopItems; sound.PlaySE(SE::Cursor); }

	const bool isLeave = input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && topCursor_ == kTopItems - 1);
	if (isLeave)
	{
		isLeaving_ = true;
		sound.PlaySE(SE::Cancel);
		SaveManager::GetInstance().Save();
		sceneManager_.ChangeSceneWithFade(std::make_shared<CampScene>(sceneManager_));
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	if (topCursor_ == 0)
	{
		mode_ = Mode::Equip;
		sound.PlaySE(SE::Decide);
	}
	else if (topCursor_ == 2)
	{
		mode_ = Mode::SkillBook;
		sound.PlaySE(SE::Decide);
	}
	else if (SaveManager::GetInstance().Current().isCleared)
	{
		mode_ = Mode::Charm;
		RebuildCharmOrder();
		sound.PlaySE(SE::Decide);

		//おまもりを見たので「！」を消す
		SaveSlot& s = SaveManager::GetInstance().Current();
		if (s.IsNew(NewsBit::kCharm))
		{
			s.seenNews |= NewsBit::kCharm;
			SaveManager::GetInstance().Save();
		}
	}
	else
	{
		sound.PlaySE(SE::Buzzer);
		Notice(L"おまもりは ダンジョンを クリアすると あつかえるように なる");
	}
}

//==============================//
// 装備
//==============================//

void SmithScene::UpdateEquip()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = static_cast<int>(rows_.size());
	if (input.IsAnyTriggered(Button::Up)) { row_ = (row_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { row_ = (row_ + 1) % num; sound.PlaySE(SE::Cursor); }
	//←→で 5こずつ とぶ(はしでは はんたいがわへ)
	const int jump = (input.IsAnyTriggered(Button::Right) ? kEquipJump : 0) - (input.IsAnyTriggered(Button::Left) ? kEquipJump : 0);
	if (jump != 0)
	{
		const int edgeRow = jump > 0 ? num - 1 : 0;
		row_ = row_ == edgeRow ? (num - 1 - edgeRow) : std::clamp(row_ + jump, 0, num - 1);
		sound.PlaySE(SE::Cursor);
	}
	if (input.IsAnyTriggered(Button::Turn)) { part_ = 1 - part_; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel)) { mode_ = Mode::Top; sound.PlaySE(SE::Cancel); return; }
	if (!input.IsAnyTriggered(Button::Decide)) return;

	const Row& r = rows_[row_];
	if (!r.isKnown) { sound.PlaySE(SE::Buzzer); return; }

	//選べる行動を並べる
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const auto& lvs = part_ == 0 ? (r.isShiny ? s.shinyUpperLv : s.upperLv) : (r.isShiny ? s.shinyLowerLv : s.lowerLv);
	const int lv = lvs[r.monsterId];
	const int equipped = part_ == 0 ? s.equipUpper : s.equipLower;
	const bool isEquippedShiny = part_ == 0 ? s.isEquipUpperShiny : s.isEquipLowerShiny;
	const bool isEquipped = equipped == r.monsterId && isEquippedShiny == r.isShiny;

	actions_.clear();
	if (lv < 0)
	{
		actions_.push_back(L"つくる（そざい " + std::to_wstring(EquipTable::Cost(-1)) + L"こ）");
	}
	else
	{
		actions_.push_back(isEquipped ? L"はずす" : L"そうびする");
		if (lv < EquipTable::kMaxLevel) actions_.push_back(L"きょうか +" + std::to_wstring(lv + 1) + L"（そざい " + std::to_wstring(EquipTable::Cost(lv)) + L"こ）");
	}
	actions_.push_back(L"やめる");
	actionCursor_ = 0;
	mode_ = Mode::EquipAction;
	sound.PlaySE(SE::Decide);
}

void SmithScene::UpdateEquipAction()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = static_cast<int>(actions_.size());
	if (input.IsAnyTriggered(Button::Up)) { actionCursor_ = (actionCursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { actionCursor_ = (actionCursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && actionCursor_ == num - 1))
	{
		mode_ = Mode::Equip;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	SaveSlot& s = SaveManager::GetInstance().Current();
	const Row& r = rows_[row_];
	const EquipSeries* e = EquipTable::Find(r.monsterId);
	auto& lvs = part_ == 0 ? (r.isShiny ? s.shinyUpperLv : s.upperLv) : (r.isShiny ? s.shinyLowerLv : s.lowerLv);
	auto& mats = r.isShiny ? s.shinyMaterials : s.materials;
	int& lv = lvs[r.monsterId];
	const EquipPart part = part_ == 0 ? EquipPart::Upper : EquipPart::Lower;

	//作る・強化
	const bool isMake = lv < 0 && actionCursor_ == 0;
	const bool isUpgrade = lv >= 0 && actionCursor_ == 1 && lv < EquipTable::kMaxLevel;
	if (isMake || isUpgrade)
	{
		const int cost = EquipTable::Cost(lv);
		if (mats[r.monsterId] < cost)
		{
			sound.PlaySE(SE::Buzzer);
			Notice(L"そざいが たりない…（" + EquipTable::MaterialName(*e, r.isShiny) + L" が " + std::to_wstring(cost) + L"こ ひつよう）");
			return;
		}
		mats[r.monsterId] -= cost;
		lv++;
		sound.PlaySE(SE::Bang);
		sound.PlaySE(SE::Point);
		Effects::GetInstance().Burst({ 960.0f, 300.0f }, 30, { 255, 200, 120 }, 150.0f, 450.0f, 6.0f, 0.5f, ParticleShape::Spark);
		Effects::GetInstance().Shake(5.0f, 0.2f);
		Notice(EquipTable::PartName(*e, part, r.isShiny) + (isMake ? L" を つくった！" : L" を +" + std::to_wstring(lv) + L" に きょうかした！"));
		SaveManager::GetInstance().Save();
		mode_ = Mode::Equip;
		return;
	}

	//装備・はずす
	if (lv >= 0 && actionCursor_ == 0)
	{
		int& equipped = part_ == 0 ? s.equipUpper : s.equipLower;
		bool& isShiny = part_ == 0 ? s.isEquipUpperShiny : s.isEquipLowerShiny;
		if (equipped == r.monsterId && isShiny == r.isShiny)
		{
			equipped = -1;
			sound.PlaySE(SE::Cancel);
		}
		else
		{
			equipped = r.monsterId;
			isShiny = r.isShiny;
			sound.PlaySE(SE::Ready);
			Notice(EquipTable::PartName(*e, part, r.isShiny) + L" を そうびした");
		}
		SaveManager::GetInstance().Save();
		mode_ = Mode::Equip;
	}
}

//==============================//
// お守り
//==============================//

void SmithScene::UpdateCharm()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	SaveSlot& s = SaveManager::GetInstance().Current();
	const int num = kTopRows + static_cast<int>(s.charms.size());
	if (input.IsAnyTriggered(Button::Up)) { charmCursor_ = (charmCursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { charmCursor_ = (charmCursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel)) { mode_ = Mode::Top; sound.PlaySE(SE::Cancel); return; }

	//←→で ならびかえ(えらんでいる お守りは そのまま)
	const int sortDir = (input.IsAnyTriggered(Button::Right) ? 1 : 0) - (input.IsAnyTriggered(Button::Left) ? 1 : 0);
	if (sortDir != 0)
	{
		const int keep = CharmIndexAt(charmCursor_);
		sortMode_ = (sortMode_ + kSortNum + sortDir) % kSortNum;
		RebuildCharmOrder();
		if (keep >= 0) charmCursor_ = RowOfCharm(keep);
		sound.PlaySE(SE::Cursor);
	}

	//{Y}で まとめて ぶんかい する お守りに チェック
	if (input.IsAnyTriggered(Button::Turn))
	{
		const int index = CharmIndexAt(charmCursor_);
		if (index < 0) return;
		if (s.charms[index].isLocked || s.equippedCharm == index)
		{
			sound.PlaySE(SE::Buzzer);
			Notice(L"ロック中・そうびちゅうの おまもりは えらべない");
			return;
		}
		selected_[index] = !selected_[index];
		sound.PlaySE(SE::Cursor);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	//まとめて ぶんかい
	if (charmCursor_ == kBulkRow)
	{
		mode_ = Mode::BulkMenu;
		bulkCursor_ = 0;
		sound.PlaySE(SE::Decide);
		return;
	}

	//錬成
	if (charmCursor_ < kCraftRows)
	{
		if (static_cast<int>(s.charms.size()) >= kMaxCharm)
		{
			sound.PlaySE(SE::Buzzer);
			Notice(L"おまもりが いっぱい（" + std::to_wstring(kMaxCharm) + L"こ まで）。ぶんかい してね");
			return;
		}
		if (charmCursor_ == 0) Craft(kCraftCost, 0, -1);
		else if (charmCursor_ == 1) Craft(kHighCraftCost, 8, -1);
		else
		{
			mode_ = Mode::Alchemy;
			alchemyRow_ = 0;
			sound.PlaySE(SE::Decide);
		}
		return;
	}

	//お守りの操作
	const int index = CharmIndexAt(charmCursor_);
	if (index < 0) return;
	const Charm& c = s.charms[index];
	actions_.clear();
	actions_.push_back(s.equippedCharm == index ? L"はずす" : L"そうびする");
	actions_.push_back(c.isLocked ? L"ロックを はずす" : L"ロックする");
	actions_.push_back(L"ぶんかい（かけら +" + std::to_wstring(CharmMaker::ShardValue(c)) + L"・スキルだま）");
	actions_.push_back(L"やめる");
	actionCursor_ = 0;
	mode_ = Mode::CharmAction;
	sound.PlaySE(SE::Decide);
}

void SmithScene::RebuildCharmOrder()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const int n = static_cast<int>(s.charms.size());
	selected_.resize(n, 0);
	charmOrder_.resize(n);
	for (int i = 0; i < n; i++) charmOrder_[i] = i;

	auto skillKey = [&s](int i) { return static_cast<int>(s.charms[i].main.kind) * 10 - s.charms[i].main.value; };
	switch (sortMode_)
	{
	case 1: std::reverse(charmOrder_.begin(), charmOrder_.end()); break;
	case 2: std::stable_sort(charmOrder_.begin(), charmOrder_.end(), [&s](int a, int b) { return s.charms[a].rarity > s.charms[b].rarity; }); break;
	case 3: std::stable_sort(charmOrder_.begin(), charmOrder_.end(), [&s](int a, int b) { return s.charms[a].rarity < s.charms[b].rarity; }); break;
	case 4: std::stable_sort(charmOrder_.begin(), charmOrder_.end(), [&](int a, int b) { return skillKey(a) < skillKey(b); }); break;
	default: break;
	}
}

int SmithScene::CharmIndexAt(int row) const
{
	const int i = row - kTopRows;
	if (i < 0 || i >= static_cast<int>(charmOrder_.size())) return -1;
	return charmOrder_[i];
}

int SmithScene::RowOfCharm(int index) const
{
	for (size_t i = 0; i < charmOrder_.size(); i++)
	{
		if (charmOrder_[i] == index) return kTopRows + static_cast<int>(i);
	}
	return 0;
}

void SmithScene::RemoveCharm(int index)
{
	SaveSlot& s = SaveManager::GetInstance().Current();
	s.charms.erase(s.charms.begin() + index);
	if (index < static_cast<int>(selected_.size())) selected_.erase(selected_.begin() + index);
	if (s.equippedCharm > index) s.equippedCharm--;
}

int SmithScene::SelectedCount() const
{
	int n = 0;
	for (char c : selected_) if (c) n++;
	return n;
}

void SmithScene::UpdateBulkMenu()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	SaveSlot& s = SaveManager::GetInstance().Current();
	if (input.IsAnyTriggered(Button::Up)) { bulkCursor_ = (bulkCursor_ + kBulkNum - 1) % kBulkNum; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { bulkCursor_ = (bulkCursor_ + 1) % kBulkNum; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && bulkCursor_ == kBulkBack))
	{
		mode_ = Mode::Charm;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	if (bulkCursor_ == kBulkBreak)
	{
		if (SelectedCount() == 0)
		{
			sound.PlaySE(SE::Buzzer);
			Notice(L"まだ えらんで いない（" + ButtonGuide::Name(Button::Turn) + L"で えらべる）");
			return;
		}
		mode_ = Mode::BulkConfirm;
		confirmCursor_ = 1;
		sound.PlaySE(SE::Decide);
		return;
	}
	if (bulkCursor_ == kBulkClear)
	{
		std::fill(selected_.begin(), selected_.end(), 0);
		sound.PlaySE(SE::Cancel);
		Notice(L"えらぶのを ぜんぶ やめた");
		return;
	}

	//★○いかを まとめて えらぶ(ロック中・そうびちゅうは のぞく)
	const int maxStar = bulkCursor_ - kBulkStar1 + 1;
	int added = 0;
	for (int i = 0; i < static_cast<int>(s.charms.size()); i++)
	{
		const Charm& c = s.charms[i];
		if (c.rarity > maxStar || c.isLocked || s.equippedCharm == i || selected_[i]) continue;
		selected_[i] = 1;
		added++;
	}
	sound.PlaySE(added > 0 ? SE::Point : SE::Buzzer);
	Notice(L"★" + std::to_wstring(maxStar) + L"いかを " + std::to_wstring(added) + L"こ えらんだ（ぜんぶで " + std::to_wstring(SelectedCount()) + L"こ）");
}

void SmithScene::UpdateBulkConfirm()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	if (input.IsAnyTriggered(Button::Left) || input.IsAnyTriggered(Button::Right) ||
		input.IsAnyTriggered(Button::Up) || input.IsAnyTriggered(Button::Down))
	{
		confirmCursor_ = 1 - confirmCursor_;
		sound.PlaySE(SE::Cursor);
	}
	if (input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && confirmCursor_ == 1))
	{
		mode_ = Mode::BulkMenu;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	//うしろから けして ばんごうが ずれないように
	SaveSlot& s = SaveManager::GetInstance().Current();
	int count = 0;
	int shards = 0;
	int orbNum = 0;
	for (int i = static_cast<int>(s.charms.size()) - 1; i >= 0; i--)
	{
		if (i >= static_cast<int>(selected_.size()) || !selected_[i]) continue;
		const Charm& c = s.charms[i];
		if (c.isLocked || s.equippedCharm == i) continue;
		shards += CharmMaker::ShardValue(c);
		for (const SkillValue& o : CharmMaker::Orbs(c))
		{
			s.orbs[static_cast<int>(o.kind)] += o.value;
			orbNum += o.value;
		}
		RemoveCharm(i);
		count++;
	}
	s.shards += shards;
	RebuildCharmOrder();
	charmCursor_ = std::min(charmCursor_, kTopRows + static_cast<int>(s.charms.size()) - 1);
	mode_ = Mode::Charm;

	sound.PlaySE(SE::BigCoin);
	Effects::GetInstance().Burst({ 960.0f, 300.0f }, 40, { 200, 220, 255 }, 120.0f, 420.0f, 7.0f, 0.6f, ParticleShape::Spark);
	Notice(std::to_wstring(count) + L"こ ぶんかい：かけら +" + std::to_wstring(shards) + L"・スキルだま +" + std::to_wstring(orbNum));
	SaveManager::GetInstance().Save();
}

void SmithScene::Craft(int cost, int power, int fixedMain)
{
	SaveSlot& s = SaveManager::GetInstance().Current();
	Sound& sound = Sound::GetInstance();
	if (s.shards < cost)
	{
		sound.PlaySE(SE::Buzzer);
		Notice(L"かけらが たりない…（" + std::to_wstring(cost) + L"こ ひつよう）");
		return;
	}
	s.shards -= cost;
	const SkillKind main = fixedMain >= 0 ? CharmMaker::MainKinds()[fixedMain] : SkillKind::None;
	const Charm c = CharmMaker::Make(power, 0, main);
	s.charms.push_back(c);
	RebuildCharmOrder();
	charmCursor_ = RowOfCharm(static_cast<int>(s.charms.size()) - 1);

	sound.PlaySE(SE::BigCoin);
	if (c.rarity >= 4) sound.PlaySE(SE::Fanfare);
	Effects& fx = Effects::GetInstance();
	fx.Burst({ 960.0f, 300.0f }, 20 + c.rarity * 8, CharmMaker::RarityColor(c.rarity), 150.0f, 450.0f, 10.0f, 0.8f, ParticleShape::Star);
	fx.Flash(CharmMaker::RarityColor(c.rarity), 60 + c.rarity * 20);
	Notice(c.Name() + L" が できた！");
	SaveManager::GetInstance().Save();
}

void SmithScene::UpdateCharmAction()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	const int num = static_cast<int>(actions_.size());
	if (input.IsAnyTriggered(Button::Up)) { actionCursor_ = (actionCursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { actionCursor_ = (actionCursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel) || (input.IsAnyTriggered(Button::Decide) && actionCursor_ == num - 1))
	{
		mode_ = Mode::Charm;
		sound.PlaySE(SE::Cancel);
		return;
	}
	if (!input.IsAnyTriggered(Button::Decide)) return;

	SaveSlot& s = SaveManager::GetInstance().Current();
	const int index = CharmIndexAt(charmCursor_);
	if (index < 0) { mode_ = Mode::Charm; return; }
	Charm& c = s.charms[index];

	if (actionCursor_ == 0)
	{
		s.equippedCharm = s.equippedCharm == index ? -1 : index;
		sound.PlaySE(s.equippedCharm >= 0 ? SE::Ready : SE::Cancel);
		if (s.equippedCharm >= 0) Notice(c.Name() + L" を そうびした");
	}
	else if (actionCursor_ == 1)
	{
		c.isLocked = !c.isLocked;
		sound.PlaySE(SE::Cursor);
	}
	else if (actionCursor_ == 2)
	{
		if (c.isLocked || s.equippedCharm == index)
		{
			sound.PlaySE(SE::Buzzer);
			Notice(L"ロック中・そうびちゅうの おまもりは ぶんかい できない");
			return;
		}
		const int shards = CharmMaker::ShardValue(c);
		s.shards += shards;
		int orbNum = 0;
		for (const SkillValue& o : CharmMaker::Orbs(c))
		{
			s.orbs[static_cast<int>(o.kind)] += o.value;
			orbNum += o.value;
		}
		RemoveCharm(index);
		RebuildCharmOrder();
		charmCursor_ = std::min(charmCursor_, kTopRows + static_cast<int>(s.charms.size()) - 1);
		sound.PlaySE(SE::Pop);
		Effects::GetInstance().Burst({ 960.0f, 300.0f }, 20, { 200, 220, 255 }, 100.0f, 350.0f, 6.0f, 0.5f, ParticleShape::Spark);
		Notice(L"かけら " + std::to_wstring(shards) + L"こ と スキルだま " + std::to_wstring(orbNum) + L"こ を てにいれた");
	}
	SaveManager::GetInstance().Save();
	mode_ = Mode::Charm;
}

SkillKind SmithScene::AlchemyKind(int row) const
{
	const int pick = alchemyPick_[row];
	if (row == 0) return CharmMaker::MainKinds()[pick];
	if (pick == 0) return SkillKind::None;
	if (row == 3) return CharmMaker::RareKinds()[pick - 1];
	return CharmMaker::SubKinds()[pick - 1];
}

int SmithScene::AlchemyNeed(int row) const
{
	if (AlchemyKind(row) == SkillKind::None) return 0;
	if (row == 0) return kMainOrbs;
	if (row == 3) return kRareOrbs;
	return kSubOrbs;
}

void SmithScene::UpdateAlchemy()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	if (input.IsAnyTriggered(Button::Up)) { alchemyRow_ = (alchemyRow_ + kAlchemyRows - 1) % kAlchemyRows; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { alchemyRow_ = (alchemyRow_ + 1) % kAlchemyRows; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel)) { mode_ = Mode::Charm; sound.PlaySE(SE::Cancel); return; }

	//←→で スキルを かえる
	const int dir = input.IsAnyTriggered(Button::Right) ? 1 : (input.IsAnyTriggered(Button::Left) ? -1 : 0);
	if (dir != 0 && alchemyRow_ < 4)
	{
		int num = static_cast<int>(CharmMaker::MainKinds().size());
		if (alchemyRow_ == 1 || alchemyRow_ == 2) num = static_cast<int>(CharmMaker::SubKinds().size()) + 1;
		if (alchemyRow_ == 3) num = static_cast<int>(CharmMaker::RareKinds().size()) + 1;
		int& pick = alchemyPick_[alchemyRow_];
		pick = (pick + num + dir) % num;

		//サブ1とサブ2は おなじに しない
		const int other = alchemyRow_ == 1 ? 2 : 1;
		if ((alchemyRow_ == 1 || alchemyRow_ == 2) && pick != 0 && pick == alchemyPick_[other]) pick = (pick + num + dir) % num;
		sound.PlaySE(SE::Cursor);
	}

	if (!input.IsAnyTriggered(Button::Decide)) return;
	if (alchemyRow_ < 4)
	{
		alchemyRow_++;
		sound.PlaySE(SE::Cursor);
		return;
	}

	SaveSlot& s = SaveManager::GetInstance().Current();
	if (static_cast<int>(s.charms.size()) >= kMaxCharm)
	{
		sound.PlaySE(SE::Buzzer);
		Notice(L"おまもりが いっぱい（" + std::to_wstring(kMaxCharm) + L"こ まで）。ぶんかい してね");
		return;
	}
	if (s.shards < kAlchemyCost)
	{
		sound.PlaySE(SE::Buzzer);
		Notice(L"かけらが たりない…（" + std::to_wstring(kAlchemyCost) + L"こ ひつよう）");
		return;
	}
	for (int row = 0; row < 4; row++)
	{
		const SkillKind k = AlchemyKind(row);
		if (k != SkillKind::None && s.orbs[static_cast<int>(k)] < AlchemyNeed(row))
		{
			sound.PlaySE(SE::Buzzer);
			Notice(std::wstring(SkillInfo::Name(k)) + L"の スキルだまが たりない…");
			return;
		}
	}

	//だまと かけらを つかって つくる
	std::vector<SkillKind> subs;
	for (int row = 0; row < 4; row++)
	{
		const SkillKind k = AlchemyKind(row);
		if (k == SkillKind::None) continue;
		s.orbs[static_cast<int>(k)] -= AlchemyNeed(row);
		if (row == 1 || row == 2) subs.push_back(k);
	}
	s.shards -= kAlchemyCost;
	const Charm c = CharmMaker::MakeWith(4, AlchemyKind(0), subs, AlchemyKind(3));
	s.charms.push_back(c);
	RebuildCharmOrder();
	charmCursor_ = RowOfCharm(static_cast<int>(s.charms.size()) - 1);
	mode_ = Mode::Charm;

	sound.PlaySE(SE::BigCoin);
	if (c.rarity >= 4) sound.PlaySE(SE::Fanfare);
	Effects& fx = Effects::GetInstance();
	fx.Burst({ 960.0f, 300.0f }, 20 + c.rarity * 8, CharmMaker::RarityColor(c.rarity), 150.0f, 450.0f, 10.0f, 0.8f, ParticleShape::Star);
	fx.Flash(CharmMaker::RarityColor(c.rarity), 60 + c.rarity * 20);
	Notice(c.Name() + L" が できた！");
	SaveManager::GetInstance().Save();
}

//==============================//
// 表示
//==============================//

void SmithScene::Draw()
{
	DrawUtil::Gradient(0, 0, Game::kScreenWidth, Game::kScreenHeight, { 70, 40, 30 }, { 25, 15, 15 });

	//炉の明かり
	for (int k = 0; k < 5; k++)
	{
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(14 + 6 * std::sin(time_ * 6.0f)));
		DrawCircleAA(1180.0f, 680.0f, 90.0f + k * 50.0f, 40, GetColor(255, 110, 40), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	switch (mode_)
	{
	case Mode::Top: DrawTop(); break;
	case Mode::Equip:
	case Mode::EquipAction: DrawEquip(); break;
	case Mode::SkillBook: DrawSkillBook(); break;
	default: DrawCharm(); break;
	}
	if (mode_ == Mode::EquipAction || mode_ == Mode::CharmAction) DrawActionMenu();
	if (mode_ == Mode::Alchemy) DrawAlchemy();
	if (mode_ == Mode::BulkMenu || mode_ == Mode::BulkConfirm) DrawBulk();

	Effects::GetInstance().Draw();
	if (noticeTime_ > 0.0f)
	{
		DrawUtil::Panel(240.0f, 620.0f, 1040.0f, 676.0f, { 20, 15, 30 }, static_cast<int>(220 * std::min(1.0f, noticeTime_)), { 255, 220, 120 });
		Font::DrawCenterFit(640.0f, 648.0f, notice_, GetColor(255, 240, 180), FontSize::Small, GetColor(20, 15, 30), 770.0f);
	}
}

void SmithScene::DrawTop()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	Font::DrawCenter(640.0f, 90.0f, L"かじや", GetColor(255, 220, 140), FontSize::Large, GetColor(40, 20, 10));
	Font::DrawCenter(640.0f, 150.0f, L"「よう、なにを する？」", GetColor(240, 230, 220), FontSize::Small, GetColor(40, 20, 10));

	DrawUtil::Window(400.0f, 196.0f, 880.0f, 468.0f);
	const wchar_t* items[kTopItems] = { L"そうびを つくる・つける", s.isCleared ? L"おまもり" : L"？？？（クリアで かいほう）", L"スキルの せつめい", L"でる" };
	for (int i = 0; i < kTopItems; i++)
	{
		const float y = 240.0f + i * 58.0f;
		const bool isSel = i == topCursor_;
		if (isSel) DrawUtil::Cursor(436.0f, y, time_);
		const unsigned int col = (i == 1 && !s.isCleared) ? GetColor(120, 110, 140) : (isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230));
		Font::DrawFit(470.0f, y - 14.0f, items[i], col, FontSize::Small, GetColor(20, 15, 30), 390.0f);
		if (i == 1 && s.IsNew(NewsBit::kCharm))
		{
			DrawUtil::NewMark(470.0f + Font::Width(items[i], FontSize::Small) + 26.0f, y, time_, 13.0f);
		}
	}

	//今の装備
	const HeroBonus b = s.GearBonus(nullptr);
	DrawUtil::Window(240.0f, 488.0f, 1040.0f, 652.0f);
	auto partText = [&s](int id, bool isShiny, EquipPart part) -> std::wstring
	{
		const EquipSeries* e = id >= 0 ? EquipTable::Find(id) : nullptr;
		return e ? EquipTable::PartName(*e, part, isShiny) : L"なし";
	};
	const std::wstring equip = L"うえ：" + partText(s.equipUpper, s.isEquipUpperShiny, EquipPart::Upper) +
		L"　した：" + partText(s.equipLower, s.isEquipLowerShiny, EquipPart::Lower);
	Font::DrawFit(270.0f, 508.0f, equip, GetColor(240, 235, 255), FontSize::Small, GetColor(20, 15, 30), 750.0f);
	const bool hasCharm = s.equippedCharm >= 0 && s.equippedCharm < static_cast<int>(s.charms.size());
	Font::DrawFit(270.0f, 544.0f, L"おまもり：" + (hasCharm ? s.charms[s.equippedCharm].Name() : std::wstring(L"なし")) + (KeyItem::OwnedCount(s) > 0 ? L"　／　だいじな もの " + std::to_wstring(KeyItem::OwnedCount(s)) + L"こ" : std::wstring()), GetColor(240, 235, 255), FontSize::Small, GetColor(20, 15, 30), 750.0f);

	std::wstring skills;
	for (int k = static_cast<int>(SkillKind::Atk); k < static_cast<int>(SkillKind::Num); k++)
	{
		const int v = b.Get(static_cast<SkillKind>(k));
		if (v == 0) continue;
		if (!skills.empty()) skills += L"　";
		skills += SkillInfo::Describe({ static_cast<SkillKind>(k), v });
	}
	//スキルは2行まで折り返す
	const auto lines = Font::Wrap(L"スキル：" + (skills.empty() ? std::wstring(L"なし") : skills), FontSize::Small, 750.0f);
	for (size_t i = 0; i < lines.size() && i < 2; i++)
	{
		std::wstring line = lines[i];
		if (i == 1 && lines.size() > 2) line += lines[2];
		Font::DrawFit(270.0f + (i > 0 ? 24.0f : 0.0f), 580.0f + i * 30.0f, line, GetColor(200, 230, 255), FontSize::Small, GetColor(20, 15, 30), i > 0 ? 726.0f : 750.0f);
	}
}

void SmithScene::DrawEquip()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const int fontS = Font::Get(FontSize::Small);
	const unsigned int edge = GetColor(20, 15, 30);
	Font::DrawCenter(350.0f, 36.0f, L"そうびを つくる・つける", GetColor(255, 220, 140), FontSize::Medium, edge);

	DrawUtil::Window(30.0f, 70.0f, 680.0f, 600.0f);
	DrawStringToHandle(96, 84, L"シリーズ", GetColor(200, 190, 220), fontS, edge);
	Font::DrawCenter(390.0f, 96.0f, L"うえ", GetColor(200, 190, 220), FontSize::Small, edge);
	Font::DrawCenter(495.0f, 96.0f, L"した", GetColor(200, 190, 220), FontSize::Small, edge);
	Font::DrawCenter(605.0f, 96.0f, L"そざい", GetColor(200, 190, 220), FontSize::Small, edge);
	DrawLineAA(45.0f, 114.0f, 665.0f, 114.0f, GetColor(90, 80, 110), 1.0f);

	const int font = Font::Get(FontSize::Small);
	const int first = std::max(0, std::min(row_ - kVisibleRows / 2, static_cast<int>(rows_.size()) - kVisibleRows));
	for (int i = first; i < std::min(static_cast<int>(rows_.size()), first + kVisibleRows); i++)
	{
		const Row& r = rows_[i];
		const float y = 142.0f + (i - first) * 50.0f;
		const bool isSel = i == row_;
		if (isSel)
		{
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
			DrawBoxAA(45.0f, y - 23.0f, 665.0f, y + 23.0f, GetColor(255, 220, 120), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}
		else if ((i % 2) == 0)
		{
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 25);
			DrawBoxAA(45.0f, y - 23.0f, 665.0f, y + 23.0f, GetColor(200, 190, 255), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}

		//モンスターの かお
		MonsterData icon = r.isShiny ? MonsterTable::ShinyVariant(r.monsterId) : MonsterTable::Get(r.monsterId);
		if (!r.isKnown) SetDrawBright(40, 36, 55);
		UnitDraw::Monster({ 70.0f, y + 14.0f }, 15.0f, icon, r.isKnown ? time_ : 0.0f, 1.0f, false);
		SetDrawBright(255, 255, 255);

		const EquipSeries* e = EquipTable::Find(r.monsterId);
		if (!r.isKnown || !e)
		{
			DrawStringToHandle(96, static_cast<int>(y - 13.0f), L"？？？", GetColor(120, 110, 140), fontS, edge);
			continue;
		}

		//おすすめは 名前の下に 小さな ふだ
		const std::wstring& rec = r.monsterId < static_cast<int>(recommend_.size()) ? recommend_[r.monsterId] : std::wstring();
		const unsigned int nameCol = r.isShiny ? GetColor(255, 230, 120) : GetColor(240, 235, 255);
		Font::DrawFit(96.0f, rec.empty() ? y - 13.0f : y - 21.0f, EquipTable::SeriesName(*e, r.isShiny), nameCol, FontSize::Small, edge, 230.0f);
		if (!rec.empty())
		{
			const std::wstring tag = L"おすすめ：" + rec;
			const float tw = GetDrawStringWidthToHandle(tag.c_str(), static_cast<int>(tag.size()), font) * 0.6f;
			DrawRoundRectAA(96.0f, y + 4.0f, 104.0f + tw, y + 21.0f, 7.0f, 7.0f, 8, GetColor(230, 120, 50), TRUE);
			DrawExtendStringToHandle(100, static_cast<int>(y + 5.0f), 0.6, 0.6, tag.c_str(), GetColor(255, 250, 235), font);
		}

		const int mat = (r.isShiny ? s.shinyMaterials : s.materials)[r.monsterId];
		for (int p = 0; p < 2; p++)
		{
			const auto& lvs = p == 0 ? (r.isShiny ? s.shinyUpperLv : s.upperLv) : (r.isShiny ? s.shinyLowerLv : s.lowerLv);
			const int lv = lvs[r.monsterId];
			const int equipped = p == 0 ? s.equipUpper : s.equipLower;
			const bool isEqShiny = p == 0 ? s.isEquipUpperShiny : s.isEquipLowerShiny;
			const bool isEquipped = lv >= 0 && equipped == r.monsterId && isEqShiny == r.isShiny;
			const bool canPay = lv < EquipTable::kMaxLevel && mat >= EquipTable::Cost(lv);
			const float px = 390.0f + p * 105.0f;

			//もっている: みどりの ふだ、つくれる: みどりの わく、まだ: はいいろ
			if (lv >= 0)
			{
				DrawRoundRectAA(px - 44.0f, y - 17.0f, px + 44.0f, y + 17.0f, 8.0f, 8.0f, 8, isEquipped ? GetColor(40, 90, 130) : GetColor(40, 95, 60), TRUE);
				Font::DrawCenter(px, y, (L"+" + std::to_wstring(lv)).c_str(), GetColor(255, 255, 255), FontSize::Small, edge);
				if (canPay) DrawExtendStringToHandle(static_cast<int>(px + 20.0f), static_cast<int>(y - 9.0f), 0.7, 0.7, L"▲", GetColor(150, 255, 150), font, edge);
				if (isEquipped) DrawExtendStringToHandle(static_cast<int>(px - 40.0f), static_cast<int>(y - 9.0f), 0.7, 0.7, L"E", GetColor(170, 235, 255), font, edge);
			}
			else if (canPay)
			{
				DrawRoundRectAA(px - 44.0f, y - 17.0f, px + 44.0f, y + 17.0f, 8.0f, 8.0f, 8, GetColor(110, 220, 130), FALSE, 2.0f);
				const float mw = GetDrawStringWidthToHandle(L"つくれる", 4, font) * 0.75f;
				DrawExtendStringToHandle(static_cast<int>(px - mw * 0.5f), static_cast<int>(y - 9.0f), 0.75, 0.75, L"つくれる", GetColor(150, 255, 160), font, edge);
			}
			else
			{
				Font::DrawCenter(px, y, L"－", GetColor(110, 100, 130), FontSize::Small, edge);
			}
			const bool isCell = isSel && p == part_;
			if (isCell) DrawRoundRectAA(px - 47.0f, y - 20.0f, px + 47.0f, y + 20.0f, 9.0f, 9.0f, 8, GetColor(255, 230, 80), FALSE, 3.0f);
		}
		Font::DrawCenter(605.0f, y, (L"×" + std::to_wstring(mat)).c_str(), mat > 0 ? GetColor(200, 255, 200) : GetColor(120, 110, 140), FontSize::Small, edge);
	}

	//マークの いみ
	{
		const float ly = 698.0f;
		const unsigned int legend = GetColor(190, 180, 210);
		float lx = 40.0f;
		auto label = [&](const wchar_t* text)
		{
			DrawStringToHandle(static_cast<int>(lx), static_cast<int>(ly - 12.0f), text, legend, fontS, edge);
			lx += Font::Width(text, FontSize::Small) + 30.0f;
		};
		DrawRoundRectAA(lx, ly - 11.0f, lx + 26.0f, ly + 11.0f, 6.0f, 6.0f, 8, GetColor(40, 95, 60), TRUE);
		lx += 34.0f;
		label(L"もっている");
		DrawRoundRectAA(lx, ly - 11.0f, lx + 26.0f, ly + 11.0f, 6.0f, 6.0f, 8, GetColor(110, 220, 130), FALSE, 2.0f);
		lx += 34.0f;
		label(L"つくれる");
		DrawRoundRectAA(lx, ly - 11.0f, lx + 26.0f, ly + 11.0f, 6.0f, 6.0f, 8, GetColor(40, 90, 130), TRUE);
		lx += 34.0f;
		label(L"そうびちゅう");
		Font::DrawCenter(lx + 8.0f, ly - 2.0f, L"▲", GetColor(150, 255, 150), FontSize::Small, edge);
		lx += 26.0f;
		label(L"きょうか できる");
	}

	//選んでいる装備の説明
	DrawUtil::Window(700.0f, 70.0f, 1250.0f, 600.0f);
	const Row& r = rows_[row_];
	const EquipSeries* e = EquipTable::Find(r.monsterId);
	if (!r.isKnown || !e)
	{
		Font::DrawCenter(975.0f, 300.0f, L"まだ みつけていない モンスター", GetColor(150, 140, 170), FontSize::Small, edge);
		return;
	}
	const MonsterData m = r.isShiny ? MonsterTable::ShinyVariant(r.monsterId) : MonsterTable::Get(r.monsterId);
	UnitDraw::Monster({ 975.0f, 210.0f }, 56.0f, m, time_, 1.0f, false);

	//おすすめの ふだ
	if (r.monsterId < static_cast<int>(recommend_.size()) && !recommend_[r.monsterId].empty())
	{
		const std::wstring tag = L"おすすめ：" + recommend_[r.monsterId];
		const int font = Font::Get(FontSize::Small);
		const float tw = GetDrawStringWidthToHandle(tag.c_str(), static_cast<int>(tag.size()), font) * 0.75f;
		DrawRoundRectAA(1232.0f - tw - 16.0f, 86.0f, 1232.0f, 112.0f, 12.0f, 12.0f, 8, GetColor(230, 120, 50), TRUE);
		DrawExtendStringToHandle(static_cast<int>(1232.0f - tw - 8.0f), 90, 0.75, 0.75, tag.c_str(), GetColor(255, 250, 235), font);
	}

	const EquipPart part = part_ == 0 ? EquipPart::Upper : EquipPart::Lower;
	const auto& lvs = part_ == 0 ? (r.isShiny ? s.shinyUpperLv : s.upperLv) : (r.isShiny ? s.shinyLowerLv : s.lowerLv);
	const int lv = lvs[r.monsterId];
	Font::DrawCenterFit(975.0f, 250.0f, EquipTable::PartName(*e, part, r.isShiny), GetColor(255, 240, 160), FontSize::Small, edge, 500.0f);

	const SkillValue now = EquipTable::PartSkill(*e, part, r.isShiny, std::max(0, lv));
	Font::DrawFit(730.0f, 280.0f, L"スキル：" + SkillInfo::Describe(now), GetColor(200, 230, 255), FontSize::Small, edge, 495.0f);
	if (lv >= 0 && lv < EquipTable::kMaxLevel)
	{
		const std::wstring next = L"　→ つぎ：" + SkillInfo::Describe(EquipTable::PartSkill(*e, part, r.isShiny, lv + 1));
		Font::DrawFit(730.0f, 312.0f, next, GetColor(160, 200, 240), FontSize::Small, edge, 495.0f);
	}

	const std::wstring cost = lv >= EquipTable::kMaxLevel ? L"さいだいまで きょうか ずみ" :
		(lv < 0 ? L"つくる：" : L"きょうか：") + EquipTable::MaterialName(*e, r.isShiny) + L" ×" + std::to_wstring(EquipTable::Cost(lv));
	Font::DrawFit(730.0f, 350.0f, cost, GetColor(200, 255, 200), FontSize::Small, edge, 495.0f);

	Font::DrawFit(730.0f, 405.0f, L"シリーズスキル（うえ・した そろえる）", GetColor(255, 220, 140), FontSize::Small, edge, 495.0f);
	const SkillValue series = EquipTable::SeriesSkill(*e, r.isShiny);
	Font::DrawFit(750.0f, 440.0f, EquipTable::SeriesName(*e, r.isShiny) + L"：", GetColor(255, 240, 200), FontSize::Small, edge, 475.0f);
	std::wstring seriesText = SkillInfo::Describe(series);
	const SkillValue extra = r.isShiny ? EquipTable::ShinyExtra(r.monsterId) : SkillValue();
	if (extra.kind != SkillKind::None) seriesText += L"　＋ " + SkillInfo::Describe(extra);
	Font::DrawFit(770.0f, 472.0f, seriesText, extra.kind != SkillKind::None ? GetColor(255, 220, 120) : GetColor(255, 240, 200), FontSize::Small, edge, 455.0f);

	const bool isSetActive = s.equipUpper == r.monsterId && s.equipLower == r.monsterId &&
		s.isEquipUpperShiny == r.isShiny && s.isEquipLowerShiny == r.isShiny;
	if (isSetActive) DrawStringToHandle(750, 512, L"★ はつどう ちゅう！", GetColor(255, 230, 80), fontS, edge);

	//えらんでいる部位のスキルの効果
	const std::wstring effect = L"こうか：" + SkillInfo::Explain(now);
	if (isSetActive)
	{
		Font::DrawFit(730.0f, 540.0f, effect, GetColor(180, 220, 200), FontSize::Small, edge, 495.0f);
	}
	else
	{
		//2ぎょうまで
		const auto lines = Font::Wrap(effect, FontSize::Small, 495.0f);
		for (size_t i = 0; i < lines.size() && i < 2; i++)
		{
			Font::DrawFit(730.0f + (i > 0 ? 24.0f : 0.0f), 506.0f + i * 27.0f, lines[i], GetColor(180, 220, 200), FontSize::Small, edge, i > 0 ? 471.0f : 495.0f);
		}
	}

	ButtonGuide::DrawCenter(975.0f, 580.0f, L"{LR}：5こ　{Y}：うえ/した　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}

void SmithScene::DrawCharm()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const int fontS = Font::Get(FontSize::Small);
	const unsigned int edge = GetColor(20, 15, 30);
	Font::DrawCenter(350.0f, 36.0f, L"おまもり", GetColor(255, 220, 140), FontSize::Medium, edge);
	const std::wstring shard = L"かけら：" + std::to_wstring(s.shards) + L"こ　おまもり " + std::to_wstring(s.charms.size()) + L"/" + std::to_wstring(kMaxCharm);
	DrawStringToHandle(720, 24, shard.c_str(), GetColor(200, 230, 255), fontS, edge);

	DrawUtil::Window(30.0f, 70.0f, 680.0f, 600.0f);
	const std::wstring crafts[kCraftRows] =
	{
		L"れんせい（かけら " + std::to_wstring(kCraftCost) + L"）",
		L"こうきゅう れんせい（かけら " + std::to_wstring(kHighCraftCost) + L"）",
		L"れんきん（スキルだま ＋ かけら " + std::to_wstring(kAlchemyCost) + L"）",
	};
	const int total = kTopRows + static_cast<int>(s.charms.size());
	const int first = std::max(0, std::min(charmCursor_ - kVisibleRows / 2, total - kVisibleRows - 1));
	const int selectedNum = SelectedCount();
	for (int i = first; i < std::min(total, first + kVisibleRows + 1); i++)
	{
		const float y = 105.0f + (i - first) * 48.0f;
		const bool isSel = i == charmCursor_;
		if (isSel) DrawUtil::Cursor(50.0f, y, time_);
		if (i < kCraftRows)
		{
			Font::DrawFit(85.0f, y - 13.0f, crafts[i], isSel ? GetColor(255, 240, 150) : GetColor(255, 200, 140), FontSize::Small, edge, 580.0f);
			continue;
		}
		if (i == kBulkRow)
		{
			const std::wstring bulk = L"まとめて ぶんかい（えらんだ " + std::to_wstring(selectedNum) + L"こ）";
			Font::DrawFit(85.0f, y - 13.0f, bulk, isSel ? GetColor(255, 240, 150) : GetColor(200, 220, 255), FontSize::Small, edge, 580.0f);
			DrawLineAA(60.0f, y + 24.0f, 650.0f, y + 24.0f, GetColor(90, 80, 110), 1.0f);
			continue;
		}
		const int index = CharmIndexAt(i);
		const Charm& c = s.charms[index];

		//まとめて ぶんかい の チェック
		const bool isChecked = index < static_cast<int>(selected_.size()) && selected_[index];
		DrawBoxAA(84.0f, y - 10.0f, 104.0f, y + 10.0f, isChecked ? GetColor(255, 120, 100) : GetColor(110, 100, 130), FALSE, 2.0f);
		if (isChecked)
		{
			DrawLineAA(88.0f, y, 93.0f, y + 6.0f, GetColor(255, 140, 120), 3.0f);
			DrawLineAA(93.0f, y + 6.0f, 101.0f, y - 7.0f, GetColor(255, 140, 120), 3.0f);
		}
		std::wstring text = c.Name();
		if (c.rare.kind != SkillKind::None) text += L" ◆";
		Font::DrawFit(116.0f, y - 13.0f, text, Game::ToColor(CharmMaker::RarityColor(c.rarity)), FontSize::Small, edge, 420.0f);
		if (s.equippedCharm == index) Font::DrawCenter(560.0f, y, L"E", GetColor(120, 230, 255), FontSize::Small, edge);
		if (c.isLocked) Font::DrawCenter(615.0f, y, L"ロック", GetColor(255, 220, 120), FontSize::Small, edge);
	}
	if (s.charms.empty())
	{
		Font::DrawCenter(355.0f, 340.0f, L"まだ おまもりを もっていない", GetColor(150, 140, 170), FontSize::Small, edge);
	}

	//ならびかえ
	ButtonGuide::Draw(40.0f, 698.0f, std::wstring(L"{LR}：ならび「") + kSortNames[sortMode_] + L"」　{Y}：チェック", GetColor(190, 180, 210), FontSize::Small, edge);

	//詳しい中身
	DrawUtil::Window(700.0f, 70.0f, 1250.0f, 600.0f);
	if (charmCursor_ == kBulkRow)
	{
		Font::DrawFit(730.0f, 110.0f, L"チェックした おまもりを まとめて ぶんかい", GetColor(240, 235, 255), FontSize::Small, edge, 495.0f);
		ButtonGuide::Draw(730.0f, 166.0f, L"おまもりの 行で {Y} を おすと チェック", GetColor(180, 170, 200), FontSize::Small, edge);
		Font::DrawFit(730.0f, 200.0f, L"「★2いかを ぜんぶ えらぶ」なども つかえる", GetColor(180, 170, 200), FontSize::Small, edge, 495.0f);
		Font::DrawFit(730.0f, 234.0f, L"ロック中・そうびちゅうは えらべない", GetColor(180, 170, 200), FontSize::Small, edge, 495.0f);
		Font::DrawFit(730.0f, 290.0f, L"えらんでいる おまもり：" + std::to_wstring(selectedNum) + L"こ", GetColor(255, 220, 140), FontSize::Small, edge, 495.0f);
	}
	else if (charmCursor_ < kCraftRows)
	{
		const wchar_t* desc[kCraftRows] =
		{
			L"ランダムな おまもりを つくる",
			L"レアどの たかい おまもりが でやすい",
			L"スキルだまで スキルを えらんで つくる",
		};
		Font::DrawFit(730.0f, 110.0f, desc[charmCursor_], GetColor(240, 235, 255), FontSize::Small, edge, 495.0f);
		DrawStringToHandle(730, 152, L"おまもりを ぶんかいすると", GetColor(180, 170, 200), fontS, edge);
		DrawStringToHandle(730, 182, L"かけら と スキルだま が てにはいる", GetColor(180, 170, 200), fontS, edge);
		Font::DrawFit(730.0f, 212.0f, L"かけら ★1=1 ★2=2 ★3=4 ★4=8 ★5=16", GetColor(180, 170, 200), FontSize::Small, edge, 495.0f);
		Font::DrawFit(730.0f, 240.0f, L"（レアつきは +10）", GetColor(180, 170, 200), FontSize::Small, edge, 495.0f);
		Font::DrawFit(730.0f, 268.0f, L"スキルだま：メインは Lvの かず、サブ・レアは 1こ", GetColor(180, 170, 200), FontSize::Small, edge, 495.0f);

		//もっている スキルだま
		Font::DrawFit(730.0f, 312.0f, L"もっている スキルだま", GetColor(255, 220, 140), FontSize::Small, edge, 495.0f);
		std::wstring list;
		for (int k = 0; k < static_cast<int>(SkillKind::Num); k++)
		{
			if (s.orbs[k] <= 0) continue;
			if (!list.empty()) list += L"　";
			list += std::wstring(SkillInfo::Name(static_cast<SkillKind>(k))) + L"×" + std::to_wstring(s.orbs[k]);
		}
		if (list.empty()) list = L"まだ ない";
		float ly = 346.0f;
		for (const std::wstring& line : Font::Wrap(list, FontSize::Small, 495.0f))
		{
			if (ly > 530.0f) break;
			Font::DrawFit(730.0f, ly, line, GetColor(200, 230, 255), FontSize::Small, edge, 495.0f);
			ly += 30.0f;
		}
	}
	else
	{
		const Charm& c = s.charms[CharmIndexAt(charmCursor_)];
		Font::DrawCenterFit(975.0f, 110.0f, c.Name(), Game::ToColor(CharmMaker::RarityColor(c.rarity)), FontSize::Small, edge, 500.0f);
		for (int k = 0; k < c.rarity; k++) DrawUtil::Star(900.0f + k * 38.0f, 160.0f, 14.0f, 0.0f, Game::ToColor(CharmMaker::RarityColor(c.rarity)));
		Font::DrawFit(730.0f, 210.0f, L"メイン：" + SkillInfo::Describe(c.main), GetColor(255, 220, 160), FontSize::Small, edge, 495.0f);
		for (size_t k = 0; k < c.subs.size(); k++)
		{
			Font::DrawFit(730.0f, 260.0f + k * 40.0f, L"サブ：" + SkillInfo::Describe(c.subs[k]), GetColor(200, 230, 255), FontSize::Small, edge, 495.0f);
		}
		if (c.rare.kind != SkillKind::None)
		{
			Font::DrawFit(730.0f, 400.0f, L"レア：" + SkillInfo::Describe(c.rare), GetColor(255, 200, 255), FontSize::Small, edge, 495.0f);
		}
	}
	ButtonGuide::DrawCenter(975.0f, 570.0f, L"{A}：えらぶ　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}

void SmithScene::DrawBulk()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const unsigned int edge = GetColor(20, 15, 30);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const int count = SelectedCount();
	const std::wstring items[kBulkNum] =
	{
		L"えらんだ おまもりを ぶんかい（" + std::to_wstring(count) + L"こ）",
		L"★1を ぜんぶ えらぶ",
		L"★2いかを ぜんぶ えらぶ",
		L"★3いかを ぜんぶ えらぶ",
		L"★4いかを ぜんぶ えらぶ",
		L"えらぶのを ぜんぶ やめる",
		L"やめる",
	};
	DrawUtil::Window(390.0f, 150.0f, 890.0f, 225.0f + static_cast<float>(kBulkNum) * 50.0f);
	Font::DrawCenter(640.0f, 185.0f, L"まとめて ぶんかい", GetColor(255, 220, 140), FontSize::Small, edge);
	for (int i = 0; i < kBulkNum; i++)
	{
		const float y = 235.0f + i * 50.0f;
		const bool isSel = i == bulkCursor_;
		if (isSel && mode_ == Mode::BulkMenu) DrawUtil::Cursor(425.0f, y, time_);
		Font::DrawFit(460.0f, y - 13.0f, items[i], isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230), FontSize::Small, edge, 400.0f);
	}
	if (mode_ != Mode::BulkConfirm) return;

	//かくにん(もらえる かずも 見せる)
	int shards = 0;
	int orbNum = 0;
	for (int i = 0; i < static_cast<int>(s.charms.size()) && i < static_cast<int>(selected_.size()); i++)
	{
		if (!selected_[i]) continue;
		shards += CharmMaker::ShardValue(s.charms[i]);
		for (const SkillValue& o : CharmMaker::Orbs(s.charms[i])) orbNum += o.value;
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawUtil::Window(320.0f, 250.0f, 960.0f, 490.0f);
	Font::DrawCenterFit(640.0f, 300.0f, L"ほんとうに " + std::to_wstring(count) + L"こ ぶんかい する？", GetColor(255, 200, 200), FontSize::Medium, edge, 600.0f);
	Font::DrawCenterFit(640.0f, 350.0f, L"かけら +" + std::to_wstring(shards) + L"　スキルだま +" + std::to_wstring(orbNum) + L"（もとに もどせない）", GetColor(200, 220, 240), FontSize::Small, edge, 600.0f);
	const wchar_t* choices[2] = { L"ぶんかい", L"やめる" };
	for (int i = 0; i < 2; i++)
	{
		const float bx = 540.0f + i * 200.0f;
		const bool isSel = i == confirmCursor_;
		DrawUtil::Panel(bx - 80.0f, 395.0f, bx + 80.0f, 450.0f, isSel ? Game::Rgb{ 190, 60, 70 } : Game::Rgb{ 50, 45, 70 }, 240,
			isSel ? Game::Rgb{ 255, 230, 120 } : Game::Rgb{ 120, 110, 150 });
		Font::DrawCenterFit(bx, 422.0f, choices[i], GetColor(255, 255, 255), FontSize::Medium, edge, 150.0f);
	}
}

void SmithScene::DrawActionMenu()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawUtil::Window(420.0f, 220.0f, 860.0f, 260.0f + actions_.size() * 52.0f);
	for (size_t i = 0; i < actions_.size(); i++)
	{
		const float y = 255.0f + i * 52.0f;
		const bool isSel = static_cast<int>(i) == actionCursor_;
		if (isSel) DrawUtil::Cursor(465.0f, y, time_);
		Font::DrawFit(500.0f, y - 13.0f, actions_[i], isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230), FontSize::Small, GetColor(20, 15, 30), 340.0f);
	}
}

void SmithScene::DrawAlchemy()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const unsigned int edge = GetColor(20, 15, 30);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawUtil::Window(200.0f, 90.0f, 1080.0f, 600.0f);
	Font::DrawCenter(640.0f, 126.0f, L"れんきん", GetColor(255, 220, 140), FontSize::Medium, edge);
	Font::DrawCenter(640.0f, 170.0f, L"スキルだまを つかって、つけたい スキルを えらぶ", GetColor(200, 190, 220), FontSize::Small, edge);

	const wchar_t* labels[4] = { L"メイン", L"サブ1", L"サブ2", L"レア" };
	for (int row = 0; row < 4; row++)
	{
		const float y = 212.0f + row * 52.0f;
		const bool isSel = row == alchemyRow_;
		if (isSel) DrawUtil::Cursor(240.0f, y, time_);
		Font::DrawFit(275.0f, y - 13.0f, labels[row], GetColor(255, 220, 160), FontSize::Small, edge, 110.0f);

		const SkillKind k = AlchemyKind(row);
		const std::wstring name = k == SkillKind::None ? L"なし（ランダム）" : SkillInfo::Name(k);
		const unsigned int col = isSel ? GetColor(255, 240, 150) : GetColor(220, 210, 240);
		Font::DrawCenterFit(560.0f, y, name, col, FontSize::Small, edge, 260.0f);
		if (isSel)
		{
			const float bob = std::sin(time_ * 6.0f) * 3.0f;
			Font::DrawCenter(410.0f - bob, y, L"◀", GetColor(255, 220, 120), FontSize::Small, edge);
			Font::DrawCenter(710.0f + bob, y, L"▶", GetColor(255, 220, 120), FontSize::Small, edge);
		}

		//ひつような だま
		if (k != SkillKind::None)
		{
			const int have = s.orbs[static_cast<int>(k)];
			const int need = AlchemyNeed(row);
			const std::wstring orb = L"だま " + std::to_wstring(have) + L" / " + std::to_wstring(need);
			Font::DrawFit(770.0f, y - 13.0f, orb, have >= need ? GetColor(140, 230, 160) : GetColor(255, 120, 120), FontSize::Small, edge, 160.0f);
		}

		//こうか
		if (isSel && k != SkillKind::None)
		{
			const std::wstring how = row == 0 ? std::wstring(L"Lvは ランダム（1〜3）") : (row == 3 ? SkillInfo::Explain({ k, k == SkillKind::Shiny ? 100 : (k == SkillKind::Recruit ? 10 : 1) }) : std::wstring(L"あたいは ★で きまる"));
			Font::DrawCenterFit(640.0f, 482.0f, std::wstring(SkillInfo::Summary(k)), GetColor(200, 230, 255), FontSize::Small, edge, 820.0f);
			Font::DrawCenterFit(640.0f, 514.0f, how, GetColor(170, 200, 230), FontSize::Small, edge, 820.0f);
		}
	}

	//けってい
	const float y = 430.0f;
	const bool isSel = alchemyRow_ == 4;
	const std::wstring go = L"れんきん する（かけら " + std::to_wstring(s.shards) + L" / " + std::to_wstring(kAlchemyCost) + L"）";
	if (isSel)
	{
		DrawUtil::CursorBeside(640.0f, y, go.c_str(), FontSize::Small, time_);
		Font::DrawCenter(640.0f, 490.0f, L"★は ランダム。えらばなかった サブも ★に おうじて つく", GetColor(200, 190, 220), FontSize::Small, edge);
	}
	Font::DrawCenter(640.0f, y, go.c_str(), isSel ? GetColor(255, 240, 150) : GetColor(255, 200, 140), FontSize::Small, edge);

	ButtonGuide::DrawCenter(640.0f, 560.0f, L"{UD}：えらぶ　{LR}：かえる　{B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}

//==============================//
// スキルの せつめい
//==============================//

void SmithScene::UpdateSkillBook()
{
	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	if (input.IsAnyTriggered(Button::Up)) { skillCursor_ = (skillCursor_ + kSkillNum - 1) % kSkillNum; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { skillCursor_ = (skillCursor_ + 1) % kSkillNum; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Cancel) || input.IsAnyTriggered(Button::Decide))
	{
		mode_ = Mode::Top;
		sound.PlaySE(SE::Cancel);
	}
}

std::vector<std::wstring> SmithScene::SkillSources(int kind) const
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const SkillKind k = static_cast<SkillKind>(kind);
	std::vector<std::wstring> out;

	//装備(見つけたモンスターのものだけ名前を出す)
	std::wstring upper;
	std::wstring lower;
	std::wstring series;
	int unknown = 0;
	auto add = [](std::wstring& list, const std::wstring& name) { list += (list.empty() ? L"" : L"・") + name; };
	for (const EquipSeries& e : EquipTable::All())
	{
		const bool isMatch = e.upperSkill.kind == k || e.lowerSkill.kind == k || e.seriesSkill.kind == k;
		if (!isMatch) continue;
		const bool isKnown = e.monsterId < static_cast<int>(s.zukan.size()) && s.zukan[e.monsterId].isSeen;
		if (!isKnown) { unknown++; continue; }
		if (e.upperSkill.kind == k) add(upper, e.upperName);
		if (e.lowerSkill.kind == k) add(lower, e.lowerName);
		if (e.seriesSkill.kind == k) add(series, e.seriesName);
	}
	if (!upper.empty()) out.push_back(L"うえ：" + upper);
	if (!lower.empty()) out.push_back(L"した：" + lower);
	if (!series.empty()) out.push_back(L"シリーズ：" + series);

	//色違いの シリーズだけの スキル(クリア後)
	std::wstring shinySeries;
	if (s.isCleared)
	{
		for (const EquipSeries& e : EquipTable::All())
		{
			if (EquipTable::ShinyExtra(e.monsterId).kind != k) continue;
			const bool isKnown = e.monsterId < static_cast<int>(s.zukanShiny.size()) && s.zukanShiny[e.monsterId].isSeen;
			if (isKnown) add(shinySeries, EquipTable::SeriesName(e, true));
			else unknown++;
		}
	}
	if (!shinySeries.empty()) out.push_back(L"いろちがいの シリーズ：" + shinySeries);
	if (unknown > 0) out.push_back(L"まだ みつけていない モンスターの そうび：" + std::to_wstring(unknown) + L"こ");

	//お守り(クリア前は かくす)
	const auto& mains = CharmMaker::MainKinds();
	const bool isCharm = std::find(mains.begin(), mains.end(), k) != mains.end() ||
		std::find(std::begin(kCharmSubs), std::end(kCharmSubs), k) != std::end(kCharmSubs) ||
		std::find(std::begin(kCharmRares), std::end(kCharmRares), k) != std::end(kCharmRares);
	if (isCharm && !s.isCleared)
	{
		out.push_back(L"？？？（ダンジョンを クリアすると わかる）");
	}
	else if (isCharm)
	{
		if (std::find(mains.begin(), mains.end(), k) != mains.end()) out.push_back(L"おまもりの メインスキル（れんきん でも えらべる）");
		if (std::find(std::begin(kCharmSubs), std::end(kCharmSubs), k) != std::end(kCharmSubs)) out.push_back(L"おまもりの サブスキル（れんきん でも えらべる）");
		if (std::find(std::begin(kCharmRares), std::end(kCharmRares), k) != std::end(kCharmRares)) out.push_back(L"おまもりの レアスキル（とても まれ。れんきん でも えらべる）");
	}

	if (std::find(std::begin(kPersonalitySkills), std::end(kPersonalitySkills), k) != std::end(kPersonalitySkills))
	{
		out.push_back(L"せいかく（ぼうけんごとに ランダム）");
	}
	for (int i = 0; i < KeyItem::Count(); i++)
	{
		if (!KeyItem::IsOwned(s, i)) continue;
		for (const SkillValue& v : KeyItem::Get(i).skills) if (v.kind == k) out.push_back(std::wstring(L"だいじな もの「") + KeyItem::Get(i).name + L"」");
	}
	if (out.empty()) out.push_back(L"てにいれる ほうほうは まだ わからない");
	return out;
}

void SmithScene::DrawSkillBook()
{
	const SaveSlot& s = SaveManager::GetInstance().Current();
	const HeroBonus b = s.GearBonus(nullptr);
	const unsigned int edge = GetColor(20, 15, 30);
	Font::DrawCenter(350.0f, 36.0f, L"スキルの せつめい", GetColor(255, 220, 140), FontSize::Medium, edge);
	Font::DrawFit(720.0f, 24.0f, L"いま はつどう している ものは きいろ", GetColor(200, 190, 220), FontSize::Small, edge, 520.0f);

	//一覧
	DrawUtil::Window(30.0f, 70.0f, 560.0f, 600.0f);
	const int first = std::clamp(skillCursor_ - kSkillRows / 2, 0, kSkillNum - kSkillRows);
	for (int i = first; i < first + kSkillRows; i++)
	{
		const SkillKind k = static_cast<SkillKind>(kFirstSkill + i);
		const int v = b.Get(k);
		const float y = 108.0f + (i - first) * 44.0f;
		const bool isSel = i == skillCursor_;
		if (isSel) DrawUtil::Cursor(50.0f, y, time_);
		const unsigned int col = v > 0 ? GetColor(255, 225, 120) : (isSel ? GetColor(255, 255, 255) : GetColor(200, 190, 220));
		Font::DrawFit(85.0f, y - 13.0f, SkillInfo::Name(k), col, FontSize::Small, edge, 300.0f);
		if (v > 0)
		{
			const std::wstring now = SkillInfo::IsLevelSkill(k) ? L"Lv" + std::to_wstring(v) : (k == SkillKind::FloorHeal || k == SkillKind::Guts ? L"あり" : L"+" + std::to_wstring(v));
			Font::DrawFit(540.0f - Font::Width(now, FontSize::Small), y - 13.0f, now, col, FontSize::Small, edge, 140.0f);
		}
	}

	//スクロールのしるし
	const float bob = std::sin(time_ * 6.0f) * 3.0f;
	if (first > 0) DrawTriangleAA(295.0f, 76.0f + bob, 285.0f, 90.0f + bob, 305.0f, 90.0f + bob, GetColor(255, 230, 120), TRUE);
	if (first + kSkillRows < kSkillNum) DrawTriangleAA(285.0f, 580.0f - bob, 305.0f, 580.0f - bob, 295.0f, 594.0f - bob, GetColor(255, 230, 120), TRUE);

	//くわしい説明
	const SkillKind k = static_cast<SkillKind>(kFirstSkill + skillCursor_);
	const int v = b.Get(k);
	DrawUtil::Window(580.0f, 70.0f, 1250.0f, 600.0f);
	Font::DrawCenterFit(915.0f, 108.0f, SkillInfo::Name(k), GetColor(255, 230, 140), FontSize::Medium, edge, 600.0f);

	float y = 140.0f;
	const auto summary = Font::Wrap(SkillInfo::Summary(k), FontSize::Small, 610.0f);
	for (size_t i = 0; i < summary.size() && i < 3; i++, y += 30.0f) Font::DrawFit(610.0f, y, summary[i], GetColor(235, 230, 250), FontSize::Small, edge, 610.0f);

	y += 14.0f;
	Font::DrawFit(610.0f, y, L"いまの こうか", GetColor(255, 220, 140), FontSize::Small, edge, 610.0f);
	y += 30.0f;
	Font::DrawFit(630.0f, y, v > 0 ? SkillInfo::Explain({ k, v }) : std::wstring(L"いまは ついていない"), v > 0 ? GetColor(255, 240, 170) : GetColor(150, 140, 170), FontSize::Small, edge, 590.0f);
	y += 40.0f;

	//Lvで変わるスキルは Lv1〜3
	if (SkillInfo::IsLevelSkill(k))
	{
		Font::DrawFit(610.0f, y, L"Lvごとの こうか", GetColor(255, 220, 140), FontSize::Small, edge, 610.0f);
		y += 30.0f;
		for (int lv = 1; lv <= 3; lv++, y += 28.0f)
		{
			Font::DrawFit(630.0f, y, SkillInfo::Explain({ k, lv }), lv == v ? GetColor(255, 240, 170) : GetColor(200, 230, 255), FontSize::Small, edge, 590.0f);
		}
		y += 12.0f;
	}

	//てにいれかた
	Font::DrawFit(610.0f, y, L"てにいれかた", GetColor(255, 220, 140), FontSize::Small, edge, 610.0f);
	y += 30.0f;
	for (const std::wstring& src : SkillSources(static_cast<int>(k)))
	{
		const auto lines = Font::Wrap(L"・" + src, FontSize::Small, 590.0f);
		for (size_t i = 0; i < lines.size(); i++)
		{
			if (y > 560.0f) break;
			Font::DrawFit(630.0f + (i > 0 ? 24.0f : 0.0f), y, lines[i], GetColor(210, 200, 230), FontSize::Small, edge, 590.0f);
			y += 28.0f;
		}
	}

	ButtonGuide::DrawCenter(640.0f, 630.0f, L"{UD}：えらぶ　{A} / {B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}
