#include "SaveData.h"
#include "MonsterData.h"
#include "Stage.h"
#include "FishData.h"
#include<fstream>
#include<algorithm>
#include<ctime>
#include<Windows.h>

namespace
{
	constexpr const char* kDir = "savedata";
	constexpr const char* kVersion = "NAKAMA1";

	std::string SlotPath(int index)
	{
		return std::string(kDir) + "/slot" + std::to_string(index + 1) + ".dat";
	}

	//------------------------------//
	// 書き込み
	//------------------------------//

	void WriteInts(std::ofstream& f, const std::vector<int>& v)
	{
		f << v.size();
		for (int x : v) f << ' ' << x;
		f << '\n';
	}

	void WriteZukan(std::ofstream& f, const std::vector<ZukanEntry>& v)
	{
		f << v.size();
		for (const auto& z : v) f << ' ' << (z.isSeen ? 1 : 0) << (z.isDefeated ? 1 : 0) << (z.isRecruited ? 1 : 0);
		f << '\n';
	}

	void WriteCharm(std::ofstream& f, const Charm& c)
	{
		f << c.rarity << ' ' << static_cast<int>(c.main.kind) << ' ' << c.main.value << ' ' << c.subs.size();
		for (const auto& s : c.subs) f << ' ' << static_cast<int>(s.kind) << ' ' << s.value;
		f << ' ' << static_cast<int>(c.rare.kind) << ' ' << c.rare.value << ' ' << (c.isLocked ? 1 : 0) << '\n';
	}

	void WriteCharms(std::ofstream& f, const std::vector<Charm>& v)
	{
		f << v.size() << '\n';
		for (const auto& c : v) WriteCharm(f, c);
	}

	//------------------------------//
	// 読み込み
	//------------------------------//

	std::vector<int> ReadInts(std::ifstream& f)
	{
		size_t n = 0;
		f >> n;
		std::vector<int> v(n);
		for (auto& x : v) f >> x;
		return v;
	}

	std::vector<ZukanEntry> ReadZukan(std::ifstream& f)
	{
		size_t n = 0;
		f >> n;
		std::vector<ZukanEntry> v(n);
		for (auto& z : v)
		{
			std::string bits;
			f >> bits;
			if (bits.size() >= 3)
			{
				z.isSeen = bits[0] == '1';
				z.isDefeated = bits[1] == '1';
				z.isRecruited = bits[2] == '1';
			}
		}
		return v;
	}

	Charm ReadCharm(std::ifstream& f)
	{
		Charm c;
		int kind = 0;
		size_t subNum = 0;
		f >> c.rarity >> kind >> c.main.value >> subNum;
		c.main.kind = static_cast<SkillKind>(kind);
		for (size_t i = 0; i < subNum; i++)
		{
			SkillValue s;
			f >> kind >> s.value;
			s.kind = static_cast<SkillKind>(kind);
			c.subs.push_back(s);
		}
		int locked = 0;
		f >> kind >> c.rare.value >> locked;
		c.rare.kind = static_cast<SkillKind>(kind);
		c.isLocked = locked != 0;
		return c;
	}

	std::vector<Charm> ReadCharms(std::ifstream& f)
	{
		size_t n = 0;
		f >> n;
		std::vector<Charm> v;
		for (size_t i = 0; i < n && f; i++) v.push_back(ReadCharm(f));
		return v;
	}
}

//==============================//
// SaveSlot
//==============================//

void SaveSlot::Fit(int monsterNum)
{
	const size_t n = static_cast<size_t>(monsterNum);
	zukan.resize(n);
	zukanShiny.resize(n);
	materials.resize(n, 0);
	shinyMaterials.resize(n, 0);
	upperLv.resize(n, -1);
	lowerLv.resize(n, -1);
	shinyUpperLv.resize(n, -1);
	shinyLowerLv.resize(n, -1);
	run.materials.resize(n, 0);
	run.shinyMaterials.resize(n, 0);
	fish.resize(FishTable::All().size());
}

HeroBonus SaveSlot::GearBonus(const Charm* overrideCharm) const
{
	HeroBonus bonus;
	const EquipSeries* up = equipUpper >= 0 ? EquipTable::Find(equipUpper) : nullptr;
	const EquipSeries* low = equipLower >= 0 ? EquipTable::Find(equipLower) : nullptr;
	if (up)
	{
		const int lv = isEquipUpperShiny ? shinyUpperLv[equipUpper] : upperLv[equipUpper];
		bonus.Add(EquipTable::PartSkill(*up, EquipPart::Upper, isEquipUpperShiny, lv));
	}
	if (low)
	{
		const int lv = isEquipLowerShiny ? shinyLowerLv[equipLower] : lowerLv[equipLower];
		bonus.Add(EquipTable::PartSkill(*low, EquipPart::Lower, isEquipLowerShiny, lv));
	}

	//同じシリーズ(色違いかどうかも同じ)ならシリーズスキル
	if (up && low && equipUpper == equipLower && isEquipUpperShiny == isEquipLowerShiny)
	{
		bonus.Add(EquipTable::SeriesSkill(*up, isEquipUpperShiny));

		//色違いで そろえると ついかの スキル
		if (isEquipUpperShiny)
		{
			const SkillValue extra = EquipTable::ShinyExtra(equipUpper);
			if (extra.kind != SkillKind::None) bonus.Add(extra);
		}
	}

	const Charm* charm = overrideCharm;
	if (!charm && equippedCharm >= 0 && equippedCharm < static_cast<int>(charms.size())) charm = &charms[equippedCharm];
	if (charm) charm->ApplyTo(bonus);

	//だいじな もの(ぬしの ほうび)は もっているだけで効く
	for (int i = 0; i < KeyItem::Count(); i++)
	{
		if (!KeyItem::IsOwned(*this, i)) continue;
		for (const SkillValue& v : KeyItem::Get(i).skills) bonus.Add(v);
	}
	return bonus;
}

const wchar_t* Relic::Name()
{
	return L"えいゆうの あかし";
}

const std::vector<SkillValue>& Relic::Skills()
{
	static const std::vector<SkillValue> kSkills =
	{
		{ SkillKind::Atk, 5 },
		{ SkillKind::Def, 5 },
		{ SkillKind::Hp, 30 },
		{ SkillKind::Exp, 20 },
		{ SkillKind::AllySlot, 1 },
		{ SkillKind::Shiny, 100 },	//色違いが2倍出やすい
	};
	return kSkills;
}

//==============================//
// だいじな もの
//==============================//

namespace
{
	//さいごの ひとつは むげんの めいきゅう B100F の えいゆうの あかし(hasRelic)
	constexpr int kRelicItem = 5;

	const std::vector<KeyItem::Def>& KeyItemTable()
	{
		static const std::vector<KeyItem::Def> kTable =
		{
			{ L"まおうの つの", L"マオウドラゴンの つの。もつ ものに ちからを あたえる。", 19, { { SkillKind::Atk, 2 } }, 0, 0 },
			{ L"おうかん", L"キングプルンの おうかん。なかまを まもる ちからが やどる。", 40, {}, 10, 0 },
			{ L"ひょうがの うろこ", L"とけない こおりの うろこ。みを かたく まもる。", 41, { { SkillKind::Def, 3 }, { SkillKind::Resist, 10 } }, 0, 0 },
			{ L"マジンの かく", L"ようがんの まじんの しんぞう。あつい ちからが みなぎる。", 42, { { SkillKind::Atk, 3 } }, 0, 0 },
			{ L"やみの せいしょ", L"やみの しんかんの せいしょ。なかまの きばを するどくする。", 43, {}, 0, 10 },
			{ L"えいゆうの あかし", L"むげんの めいきゅうを とうはした あかし。", 44, Relic::Skills(), 0, 0 },

			//つりずかんの ほうび(ver1.5)。ばんごうが ずれないよう かならず うしろに たす
			{ L"つりびとの ぼうし", L"いけの さかなを 5しゅるい つった あかし。けいけんが みに つきやすい。", -1, { { SkillKind::Exp, 5 } }, 0, 0, 5 },
			{ L"うきの おまもり", L"いけの さかなを 10しゅるい つった あかし。なかまを やさしく まもる。", -1, {}, 5, 0, 10 },
			{ L"ぬしの うろこ", L"つりずかんを ぜんぶ うめた あかし。いけの ぬしの かたい うろこ。", -1, { { SkillKind::Def, 2 } }, 0, 0, 15 },
		};
		return kTable;
	}
}

int KeyItem::Count()
{
	return static_cast<int>(KeyItemTable().size());
}

const KeyItem::Def& KeyItem::Get(int index)
{
	return KeyItemTable()[index];
}

int KeyItem::ForBoss(int monsterId)
{
	for (int i = 0; i < Count(); i++)
	{
		if (Get(i).bossId == monsterId) return i;
	}
	return -1;
}

bool KeyItem::IsOwned(const SaveSlot& s, int index)
{
	if (index == kRelicItem) return s.hasRelic;
	return (s.keyItems & (1 << index)) != 0;
}

void KeyItem::Give(SaveSlot& s, int index)
{
	if (index == kRelicItem) s.hasRelic = true;
	else s.keyItems |= 1 << index;
}

bool SaveSlot::IsTrainingOpen() const
{
	return KeyItem::IsOwned(*this, KeyItem::ForBoss(Stage::kTrainingUnlockBoss));
}

int KeyItem::OwnedCount(const SaveSlot& s)
{
	int n = 0;
	for (int i = 0; i < Count(); i++) if (IsOwned(s, i)) n++;
	return n;
}

std::wstring KeyItem::EffectText(int index)
{
	const Def& d = Get(index);
	std::wstring text;
	for (const SkillValue& v : d.skills) text += (text.empty() ? L"" : L"　") + SkillInfo::Describe(v);
	if (d.allyHpRate > 0) text += (text.empty() ? L"" : L"　") + std::wstring(L"なかまの さいだいHP +") + std::to_wstring(d.allyHpRate) + L"%";
	if (d.allyAtkRate > 0) text += (text.empty() ? L"" : L"　") + std::wstring(L"なかまの こうげき +") + std::to_wstring(d.allyAtkRate) + L"%";
	return text;
}

int KeyItem::AllyHpRate(const SaveSlot& s)
{
	int rate = 0;
	for (int i = 0; i < Count(); i++) if (IsOwned(s, i)) rate += Get(i).allyHpRate;
	return rate;
}

int KeyItem::AllyAtkRate(const SaveSlot& s)
{
	int rate = 0;
	for (int i = 0; i < Count(); i++) if (IsOwned(s, i)) rate += Get(i).allyAtkRate;
	return rate;
}

int KeyItem::FishKinds(const SaveSlot& s)
{
	int kinds = 0;
	for (const FishRecord& rec : s.fish) if (rec.count > 0) kinds++;
	return kinds;
}

std::vector<int> KeyItem::GiveFishRewards(SaveSlot& s)
{
	std::vector<int> got;
	const int kinds = FishKinds(s);
	for (int i = 0; i < Count(); i++)
	{
		const int need = Get(i).fishKinds;
		if (need <= 0 || kinds < need || IsOwned(s, i)) continue;
		Give(s, i);
		got.push_back(i);
	}
	return got;
}

//==============================//
// SaveManager
//==============================//

SaveManager& SaveManager::GetInstance()
{
	static SaveManager instance;
	return instance;
}

void SaveManager::LoadAll()
{
	CreateDirectoryA(kDir, nullptr);
	const int monsterNum = static_cast<int>(MonsterTable::All().size());
	for (int i = 0; i < kSlotNum; i++)
	{
		slots_[i] = SaveSlot();
		if (!LoadSlot(i)) slots_[i] = SaveSlot();
		slots_[i].Fit(monsterNum);

		//まえに たおした ぬしの だいじな ものも わたす
		for (int k = 0; k < KeyItem::Count(); k++)
		{
			const int boss = KeyItem::Get(k).bossId;
			if (boss >= 0 && boss < static_cast<int>(slots_[i].zukan.size()) && slots_[i].zukan[boss].isDefeated) KeyItem::Give(slots_[i], k);
		}
		KeyItem::GiveFishRewards(slots_[i]);
	}

	std::ifstream f(std::string(kDir) + "/settings.dat");
	if (f)
	{
		int full = 0;
		f >> settings_.bgmVolume >> settings_.seVolume >> full;
		settings_.isFullScreen = full != 0;
		int read = 0;
		if (f >> read) settings_.readNotices = read;

		//あとから増えた せってい
		int style = 0;
		int speed = 1;
		int shake = 1;
		int vibration = 1;
		if (f >> style >> speed >> shake >> vibration)
		{
			settings_.buttonStyle = std::clamp(style, 0, 2);
			settings_.moveSpeed = std::clamp(speed, 0, 2);
			settings_.isShake = shake != 0;
			settings_.isVibration = vibration != 0;
		}
		int mute = 0;
		if (f >> mute) settings_.isMute = mute != 0;
	}
}

bool SaveManager::LoadSlot(int index)
{
	std::ifstream f(SlotPath(index));
	if (!f) return false;

	std::string version;
	f >> version;
	if (version != kVersion) return false;

	SaveSlot& s = slots_[index];
	int cleared = 0;
	f >> s.playSeconds >> s.lastPlayed >> cleared >> s.clearCount >> s.deepestNormal >> s.deepestInfinite;
	s.isCleared = cleared != 0;
	s.zukan = ReadZukan(f);
	s.zukanShiny = ReadZukan(f);
	s.materials = ReadInts(f);
	s.shinyMaterials = ReadInts(f);
	s.upperLv = ReadInts(f);
	s.lowerLv = ReadInts(f);
	s.shinyUpperLv = ReadInts(f);
	s.shinyLowerLv = ReadInts(f);
	int us = 0, ls = 0;
	f >> s.equipUpper >> us >> s.equipLower >> ls;
	s.isEquipUpperShiny = us != 0;
	s.isEquipLowerShiny = ls != 0;
	s.charms = ReadCharms(f);
	f >> s.equippedCharm >> s.shards;

	//冒険の途中データ
	RunData& r = s.run;
	int active = 0, guts = 0;
	f >> active >> r.stage >> r.floor >> r.turn >> r.tactics;
	f >> r.heroLevel >> r.heroExp >> r.heroHp >> r.heroMaxHp >> r.heroAtk >> r.heroDef >> r.heroCrit >> r.personality >> guts;
	r.isActive = active != 0;
	r.isGutsUsed = guts != 0;
	size_t allyNum = 0;
	f >> allyNum;
	for (size_t i = 0; i < allyNum && f; i++)
	{
		RunAlly a;
		int shiny = 0;
		f >> a.monsterId >> shiny >> a.level >> a.exp >> a.hp >> a.maxHp >> a.atk >> a.def;
		a.isShiny = shiny != 0;
		r.allies.push_back(a);
	}
	r.materials = ReadInts(f);
	r.shinyMaterials = ReadInts(f);
	r.foundCharms = ReadCharms(f);
	f >> r.runCharm;
	s.exists = static_cast<bool>(f);

	//あとから増えた項目(古いセーブには無い)
	std::string tag;
	while (s.exists && f >> tag)
	{
		if (tag == "HERO")
		{
			HeroProgress h;
			if (f >> h.level >> h.exp >> h.maxHp >> h.atk >> h.def >> h.crit) s.hero = h;
		}
		else if (tag == "NEWS")
		{
			f >> s.seenNews;
		}
		else if (tag == "PARTY")
		{
			int num = 0;
			f >> num;
			s.lastParty.clear();
			for (int i = 0; i < num; i++)
			{
				PartyMember p;
				int shiny = 0;
				f >> p.monsterId >> shiny;
				p.isShiny = shiny != 0;
				s.lastParty.push_back(p);
			}
		}
		else if (tag == "KEYITEM")
		{
			f >> s.keyItems;
		}
		else if (tag == "RELIC")
		{
			int relic = 0;
			f >> relic;
			s.hasRelic = relic != 0;
		}
		else if (tag == "LASTINF")
		{
			f >> s.lastInfinite;
		}
		else if (tag == "TICKET")
		{
			f >> s.tickets >> r.rank >> r.tickets;
		}
		else if (tag == "ORBS")
		{
			int num = 0;
			f >> num;
			for (int i = 0; i < num; i++)
			{
				int v = 0;
				f >> v;
				if (i < static_cast<int>(SkillKind::Num)) s.orbs[i] = v;
			}
		}
		else if (tag == "FISH")
		{
			int num = 0;
			f >> num;
			s.fish.assign(std::max(0, num), FishRecord());
			for (FishRecord& rec : s.fish) f >> rec.count >> rec.best;
		}
		else if (tag == "FSTOCK")
		{
			int num = 0;
			f >> num;
			if (static_cast<int>(s.fish.size()) < num) s.fish.resize(std::max(0, num));
			for (int i = 0; i < num; i++)
			{
				s.fish[i].stock = ReadInts(f);
				if (static_cast<int>(s.fish[i].stock.size()) > FishTable::kMaxStock) s.fish[i].stock.resize(FishTable::kMaxStock);
			}
		}
		else if (tag == "MEAL")
		{
			f >> s.mealFish >> s.mealSize >> r.mealFish >> r.mealSize;
			const int fishNum = static_cast<int>(FishTable::All().size());
			if (s.mealFish >= fishNum) s.mealFish = -1;
			if (r.mealFish >= fishNum) r.mealFish = -1;
		}
		else
		{
			break;
		}
	}
	return s.exists;
}

void SaveManager::Select(int index)
{
	current_ = index;
	SaveSlot& s = slots_[index];
	if (!s.exists)
	{
		s = SaveSlot();
		s.exists = true;
		s.Fit(static_cast<int>(MonsterTable::All().size()));
		Save();
	}
}

void SaveManager::Save()
{
	if (current_ < 0) return;
	CreateDirectoryA(kDir, nullptr);
	SaveSlot& s = slots_[current_];
	s.lastPlayed = static_cast<long long>(std::time(nullptr));

	std::ofstream f(SlotPath(current_), std::ios::trunc);
	if (!f) return;

	f << kVersion << '\n';
	f << s.playSeconds << ' ' << s.lastPlayed << ' ' << (s.isCleared ? 1 : 0) << ' ' << s.clearCount << ' '
		<< s.deepestNormal << ' ' << s.deepestInfinite << '\n';
	WriteZukan(f, s.zukan);
	WriteZukan(f, s.zukanShiny);
	WriteInts(f, s.materials);
	WriteInts(f, s.shinyMaterials);
	WriteInts(f, s.upperLv);
	WriteInts(f, s.lowerLv);
	WriteInts(f, s.shinyUpperLv);
	WriteInts(f, s.shinyLowerLv);
	f << s.equipUpper << ' ' << (s.isEquipUpperShiny ? 1 : 0) << ' ' << s.equipLower << ' ' << (s.isEquipLowerShiny ? 1 : 0) << '\n';
	WriteCharms(f, s.charms);
	f << s.equippedCharm << ' ' << s.shards << '\n';

	const RunData& r = s.run;
	f << (r.isActive ? 1 : 0) << ' ' << r.stage << ' ' << r.floor << ' ' << r.turn << ' ' << r.tactics << '\n';
	f << r.heroLevel << ' ' << r.heroExp << ' ' << r.heroHp << ' ' << r.heroMaxHp << ' ' << r.heroAtk << ' ' << r.heroDef << ' '
		<< r.heroCrit << ' ' << r.personality << ' ' << (r.isGutsUsed ? 1 : 0) << '\n';
	f << r.allies.size() << '\n';
	for (const auto& a : r.allies)
	{
		f << a.monsterId << ' ' << (a.isShiny ? 1 : 0) << ' ' << a.level << ' ' << a.exp << ' ' << a.hp << ' ' << a.maxHp << ' ' << a.atk << ' ' << a.def << '\n';
	}
	WriteInts(f, r.materials);
	WriteInts(f, r.shinyMaterials);
	WriteCharms(f, r.foundCharms);
	f << r.runCharm << '\n';

	const HeroProgress& h = s.hero;
	f << "HERO " << h.level << ' ' << h.exp << ' ' << h.maxHp << ' ' << h.atk << ' ' << h.def << ' ' << h.crit << '\n';
	f << "NEWS " << s.seenNews << '\n';
	f << "RELIC " << (s.hasRelic ? 1 : 0) << '\n';
	f << "KEYITEM " << s.keyItems << '\n';
	f << "PARTY " << s.lastParty.size();
	for (const PartyMember& p : s.lastParty) f << ' ' << p.monsterId << ' ' << (p.isShiny ? 1 : 0);
	f << '\n';
	f << "LASTINF " << s.lastInfinite << '\n';
	f << "TICKET " << s.tickets << ' ' << r.rank << ' ' << r.tickets << '\n';
	f << "ORBS " << static_cast<int>(SkillKind::Num);
	for (int v : s.orbs) f << ' ' << v;
	f << '\n';
	f << "FISH " << s.fish.size();
	for (const FishRecord& rec : s.fish) f << ' ' << rec.count << ' ' << rec.best;
	f << '\n';
	f << "FSTOCK " << s.fish.size() << '\n';
	for (const FishRecord& rec : s.fish) WriteInts(f, rec.stock);
	f << "MEAL " << s.mealFish << ' ' << s.mealSize << ' ' << r.mealFish << ' ' << r.mealSize << '\n';
}

void SaveManager::Delete(int index)
{
	DeleteFileA(SlotPath(index).c_str());
	slots_[index] = SaveSlot();
	slots_[index].Fit(static_cast<int>(MonsterTable::All().size()));
	if (current_ == index) current_ = -1;
}

void SaveManager::AddPlayTime(float seconds)
{
	if (current_ < 0) return;
	playTimeAcc_ += seconds;
	while (playTimeAcc_ >= 1.0f)
	{
		playTimeAcc_ -= 1.0f;
		slots_[current_].playSeconds++;
	}
}

void SaveManager::SaveSettings()
{
	CreateDirectoryA(kDir, nullptr);
	std::ofstream f(std::string(kDir) + "/settings.dat", std::ios::trunc);
	f << settings_.bgmVolume << ' ' << settings_.seVolume << ' ' << (settings_.isFullScreen ? 1 : 0) << ' ' << settings_.readNotices << '\n';
	f << settings_.buttonStyle << ' ' << settings_.moveSpeed << ' ' << (settings_.isShake ? 1 : 0) << ' ' << (settings_.isVibration ? 1 : 0) << ' ' << (settings_.isMute ? 1 : 0) << '\n';
}

bool SaveManager::IsFastMoveUnlocked() const
{
	//遊んでいる記録、まだ えらんでいなければ どれかの記録が クリアずみ
	if (current_ >= 0) return slots_[current_].exists && slots_[current_].isCleared;
	for (const SaveSlot& s : slots_)
	{
		if (s.exists && s.isCleared) return true;
	}
	return false;
}

float SaveManager::MoveRate() const
{
	if (settings_.moveSpeed >= 2 && !IsFastMoveUnlocked()) return 1.0f;
	return settings_.MoveRate();
}
