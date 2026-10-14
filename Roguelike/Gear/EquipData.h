#pragma once
#include"Skill.h"
#include<string>
#include<vector>

/// <summary>
/// 装備の部位
/// </summary>
enum class EquipPart
{
	Upper,
	Lower,
};

/// <summary>
/// そうびの しゅるい(ふつう・いろちがい・しんえん)。セーブにも この かずで のこる
/// </summary>
namespace EquipVar
{
	constexpr int kNormal = 0;
	constexpr int kShiny = 1;		//いろちがいの そざいで つくる(×1.5)
	constexpr int kAbyss = 2;		//しんえんの そざいで つくる(×1.8＋しんえんスキル)
	constexpr int kNum = 3;
}

/// <summary>
/// モンスター1体ぶんの装備シリーズ
/// </summary>
struct EquipSeries
{
	int monsterId;
	std::wstring material;
	std::wstring upperName;
	SkillValue upperSkill;
	std::wstring lowerName;
	SkillValue lowerSkill;
	std::wstring seriesName;
	SkillValue seriesSkill;
};

namespace EquipTable
{
	bool Load(const char* path);

	const std::vector<EquipSeries>& All();

	/// <summary>
	/// モンスターのシリーズ(無ければnullptr)
	/// </summary>
	const EquipSeries* Find(int monsterId);

	std::wstring PartName(const EquipSeries& s, EquipPart part, int variant);
	std::wstring SeriesName(const EquipSeries& s, int variant);
	std::wstring MaterialName(const EquipSeries& s, int variant);

	/// <summary>
	/// 強化レベル込みの部位スキル
	/// </summary>
	SkillValue PartSkill(const EquipSeries& s, EquipPart part, int variant, int level);

	SkillValue SeriesSkill(const EquipSeries& s, int variant);

	/// <summary>
	/// 色違いの シリーズだけに つく ついかの スキル(なければ None)
	/// </summary>
	SkillValue ShinyExtra(int monsterId);

	/// <summary>
	/// しんえんの シリーズだけに つく しんえんスキル
	/// </summary>
	SkillValue AbyssExtra(int monsterId);

	/// <summary>
	/// その しゅるいで そろえたときの ついかの スキル(ふつうは None)
	/// </summary>
	SkillValue VariantExtra(int monsterId, int variant);

	/// <summary>
	/// しんえんの そうびが ある モンスター(しんえんに でる まもの・ぬし)
	/// </summary>
	bool HasAbyss(int monsterId);

	/// <summary>
	/// 作る・強化に必要な素材数(level: 今のレベル、-1は未所持)
	/// </summary>
	int Cost(int level);

	constexpr int kMaxLevel = 5;
}
