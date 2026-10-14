#include "EquipData.h"
#include "MonsterData.h"
#include "Stage.h"
#include<fstream>
#include<sstream>
#include<algorithm>
#include<Windows.h>

namespace
{
	std::vector<EquipSeries> table;

	std::wstring ToWide(const std::string& s)
	{
		const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
		std::wstring w(len > 0 ? len - 1 : 0, L'\0');
		if (len > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
		return w;
	}

	//しゅるいごとの つよさ(いろちがい ×1.5・しんえん ×1.8)
	float VariantRate(int variant)
	{
		if (variant == EquipVar::kAbyss) return 1.8f;
		return variant == EquipVar::kShiny ? 1.5f : 1.0f;
	}

	//レベルで伸びない(オンオフだけの)スキル
	bool IsFixed(SkillKind kind)
	{
		return kind == SkillKind::FloorHeal || kind == SkillKind::Guts || kind == SkillKind::AllySlot ||
			kind == SkillKind::Poison || kind == SkillKind::Paralyze || kind == SkillKind::Burn ||
			kind == SkillKind::Wide || kind == SkillKind::Command || kind == SkillKind::Luck || kind == SkillKind::Blast ||
			kind == SkillKind::Guard || kind == SkillKind::StairSense || kind == SkillKind::TreasureSense || kind == SkillKind::FirstStrike;
	}
}

bool EquipTable::Load(const char* path)
{
	table.clear();
	std::ifstream file(path);
	if (!file) return false;

	std::string line;
	std::getline(file, line);
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty()) continue;

		std::vector<std::string> c;
		std::stringstream ss(line);
		std::string col;
		while (std::getline(ss, col, ',')) c.push_back(col);
		if (c.size() < 11) continue;

		EquipSeries s;
		s.monsterId = std::stoi(c[0]);
		s.material = ToWide(c[1]);
		s.upperName = ToWide(c[2]);
		s.upperSkill = { SkillInfo::FromKey(c[3]), std::stoi(c[4]) };
		s.lowerName = ToWide(c[5]);
		s.lowerSkill = { SkillInfo::FromKey(c[6]), std::stoi(c[7]) };
		s.seriesName = ToWide(c[8]);
		s.seriesSkill = { SkillInfo::FromKey(c[9]), std::stoi(c[10]) };
		table.push_back(s);
	}
	return !table.empty();
}

const std::vector<EquipSeries>& EquipTable::All()
{
	return table;
}

const EquipSeries* EquipTable::Find(int monsterId)
{
	for (const auto& s : table)
	{
		if (s.monsterId == monsterId) return &s;
	}
	return nullptr;
}

std::wstring EquipTable::PartName(const EquipSeries& s, EquipPart part, int variant)
{
	const std::wstring& base = part == EquipPart::Upper ? s.upperName : s.lowerName;
	if (variant == EquipVar::kAbyss) return L"しんえんの" + base;
	return variant == EquipVar::kShiny ? L"かがやく" + base : base;
}

std::wstring EquipTable::SeriesName(const EquipSeries& s, int variant)
{
	if (variant == EquipVar::kAbyss) return L"しんえんの " + s.seriesName;
	return variant == EquipVar::kShiny ? L"かがやく " + s.seriesName : s.seriesName;
}

std::wstring EquipTable::MaterialName(const EquipSeries& s, int variant)
{
	if (variant == EquipVar::kAbyss) return L"しんえんの" + s.material;
	return variant == EquipVar::kShiny ? L"きらめく" + s.material : s.material;
}

SkillValue EquipTable::PartSkill(const EquipSeries& s, EquipPart part, int variant, int level)
{
	SkillValue v = part == EquipPart::Upper ? s.upperSkill : s.lowerSkill;
	if (IsFixed(v.kind)) return v;

	//強化1回につき25%ずつ伸びる(最低+1)
	const float rate = (1.0f + 0.25f * std::max(0, level)) * VariantRate(variant);
	v.value = std::max(v.value + std::max(0, level), static_cast<int>(v.value * rate + 0.5f));
	return v;
}

SkillValue EquipTable::SeriesSkill(const EquipSeries& s, int variant)
{
	SkillValue v = s.seriesSkill;
	const bool isSpecial = variant != EquipVar::kNormal;
	if (isSpecial && !IsFixed(v.kind)) v.value = static_cast<int>(v.value * 1.5f + 0.5f);
	if (isSpecial && IsFixed(v.kind) && v.kind != SkillKind::FloorHeal) v.value += 1;
	return v;
}

int EquipTable::Cost(int level)
{
	//作る: 3個、強化: 今のレベル+2個
	return level < 0 ? 3 : level + 2;
}

SkillValue EquipTable::ShinyExtra(int monsterId)
{
	//シリーズの とくちょうに あわせた ついかの スキル
	switch (monsterId)
	{
	//まもりの シリーズ: はんげき
	case 0: case 4: case 10: case 16: case 20: case 22: case 27: case 32: case 37:
		return { SkillKind::Counter, 20 };
	//よける シリーズ: はんげき
	case 1: case 7: case 34:
		return { SkillKind::Counter, 15 };
	//こうげきの シリーズ: はいすい
	case 5: case 6: case 9: case 13: case 21: case 25: case 26: case 28: case 33: case 38:
		return { SkillKind::Desperate, 50 };
	//どくの シリーズ: どくぎり
	case 12: case 30:
		return { SkillKind::VenomEdge, 30 };
	//まひ・ひるませの シリーズ: こおりづけ
	case 8: case 17: case 23: case 31: case 36:
		return { SkillKind::Freeze, 30 };
	//かいふく・はんいの シリーズ: いのちの けいやく
	case 3: case 15: case 24: case 29: case 35:
		return { SkillKind::LifePact, 3 };
	//なかま・そだてる シリーズ: きずな
	case 2: case 11: case 14: case 39:
		return { SkillKind::Bond, 5 };
	//きらきら: ぜっこうちょう
	case 18:
		return { SkillKind::Peak, 15 };
	default:
		return { SkillKind::None, 0 };
	}
}

SkillValue EquipTable::AbyssExtra(int monsterId)
{
	//しんえんの ぬしの シリーズ
	if (monsterId == 41) return { SkillKind::AbyssGuard, 25 };
	if (monsterId == 44) return { SkillKind::AbyssPower, 25 };
	if (monsterId == Stage::kAbyssFinalBoss) return { SkillKind::AbyssGuard, 30 };

	//いろちがいの ついかスキルと おなじ なかまわけ: せめ→やみの ちから、まもり→やみの よろい、ほか→ふかみの めぐみ
	switch (ShinyExtra(monsterId).kind)
	{
	case SkillKind::Desperate:
	case SkillKind::VenomEdge:
	case SkillKind::Freeze:
		return { SkillKind::AbyssPower, 20 };
	case SkillKind::Counter:
		return { SkillKind::AbyssGuard, 20 };
	default:
		return { SkillKind::DeepBlessing, 10 };
	}
}

SkillValue EquipTable::VariantExtra(int monsterId, int variant)
{
	if (variant == EquipVar::kShiny) return ShinyExtra(monsterId);
	if (variant == EquipVar::kAbyss) return AbyssExtra(monsterId);
	return { SkillKind::None, 0 };
}

bool EquipTable::HasAbyss(int monsterId)
{
	//しんえんに でる まもの(むげん B61〜100 の ふつうの まもの)と しんえんの ぬし
	if (monsterId < 0 || monsterId >= static_cast<int>(MonsterTable::All().size())) return false;
	for (int f = Stage::kAbyssFirstFloor; f <= Stage::kAbyssLastFloor; f++)
	{
		if (Stage::AbyssBossFor(f) == monsterId) return true;
	}
	const MonsterData& m = MonsterTable::Get(monsterId);
	return m.type == MonsterType::Normal && m.infMaxFloor >= Stage::AbyssSourceFloor(Stage::kAbyssFirstFloor);
}
