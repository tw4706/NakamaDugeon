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

	std::wstring PartName(const EquipSeries& s, EquipPart part, bool isShiny);
	std::wstring SeriesName(const EquipSeries& s, bool isShiny);
	std::wstring MaterialName(const EquipSeries& s, bool isShiny);

	/// <summary>
	/// 強化レベル込みの部位スキル
	/// </summary>
	SkillValue PartSkill(const EquipSeries& s, EquipPart part, bool isShiny, int level);

	SkillValue SeriesSkill(const EquipSeries& s, bool isShiny);

	/// <summary>
	/// 色違いの シリーズだけに つく ついかの スキル(なければ None)
	/// </summary>
	SkillValue ShinyExtra(int monsterId);

	/// <summary>
	/// 作る・強化に必要な素材数(level: 今のレベル、-1は未所持)
	/// </summary>
	int Cost(int level);

	constexpr int kMaxLevel = 5;
}
