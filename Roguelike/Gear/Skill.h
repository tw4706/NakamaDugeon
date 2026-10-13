#pragma once
#include<string>

/// <summary>
/// 装備・お守り・性格で付くスキル
/// </summary>
enum class SkillKind
{
	None,

	//数値を伸ばすもの(サブ)
	Atk,		//攻撃+
	Def,		//防御+
	Hp,			//最大HP+
	Crit,		//会心率+(%)
	Regen,		//自然回復量+
	Exp,		//経験値+(%)
	Vision,		//視界+
	Resist,		//状態異常に強い(%)

	//戦い方を変えるもの(メイン)
	Poison,		//攻撃で毒
	Paralyze,	//攻撃でまひ
	Wide,		//攻撃範囲拡大
	Burn,		//攻撃で炎上
	Drain,		//与ダメージの一部を回復(%)
	Command,	//号令: 仲間が同じ敵を狙い、ダメージ+

	//装備のシリーズ用
	Evade,		//回避率(%)
	Double,		//2回攻撃率(%)
	HalfDamage,	//被ダメ半減率(%)
	FloorHeal,	//階を降りると全員回復
	Stun,		//攻撃で相手が1ターン止まる率(%)
	Treasure,	//素材が落ちやすい(%)

	//レア
	Shiny,		//色違い出現率+(%)
	AllySlot,	//仲間の枠+1
	Recruit,	//仲間になる確率+(%)
	Luck,		//レアなお守りが出やすい
	Guts,		//1回だけ倒れてもHP1で耐える

	//ボス・色違いの そうびの スキル(ver1.2)
	Intimidate,	//いあつ: となりの敵の こうげき-(%)
	Bond,		//きずな: ちかくの なかま1体ごとに 与ダメ+(%)
	Freeze,		//こおりづけ: まひ・ひるみ中の敵へ 与ダメ+(%)
	Blast,		//ばくえん: えんじょう(Lv)＋もえている敵を倒すと ばくはつ
	LifePact,	//いのちの けいやく: 敵を倒すと 最大HPの(%)回復
	Peak,		//ぜっこうちょう: HPが満タンのとき 与ダメ+(%)
	VenomEdge,	//どくぎり: どくの敵へ 与ダメ+(%)
	Counter,	//はんげき: こうげきを うけると (%)で はんげき
	Desperate,	//はいすい: HP30%以下で 与ダメ+(%)

	Num,
};

/// <summary>
/// スキル1つ分
/// </summary>
struct SkillValue
{
	SkillKind kind = SkillKind::None;
	int value = 0;
};

/// <summary>
/// 主人公にかかる補正の合計
/// </summary>
struct HeroBonus
{
	int values[static_cast<int>(SkillKind::Num)] = {};

	void Add(const SkillValue& s) { values[static_cast<int>(s.kind)] += s.value; }
	int Get(SkillKind kind) const { return values[static_cast<int>(kind)]; }
};

namespace SkillInfo
{
	/// <summary>
	/// CSV用の名前から変換
	/// </summary>
	SkillKind FromKey(const std::string& key);

	const wchar_t* Name(SkillKind kind);

	/// <summary>
	/// 効果の説明(例: こうげき+3)
	/// </summary>
	std::wstring Describe(const SkillValue& s);

	/// <summary>
	/// どんなスキルか(数値なし)
	/// </summary>
	const wchar_t* Summary(SkillKind kind);

	/// <summary>
	/// その数値のときの くわしい効果
	/// </summary>
	std::wstring Explain(const SkillValue& s);

	/// <summary>
	/// Lvで効果が変わるスキルか(Lv1〜3を見せる)
	/// </summary>
	bool IsLevelSkill(SkillKind kind);

	bool IsMain(SkillKind kind);
	bool IsRare(SkillKind kind);
}
