#pragma once
#include"Skill.h"
#include"Game.h"
#include<vector>
#include<string>

/// <summary>
/// お守り(ランダムな性能)
/// </summary>
struct Charm
{
	int rarity = 1;				//★1〜5(★6 は しんえん げんてい)
	SkillValue main;			//メインスキル(Lv1〜3)
	std::vector<SkillValue> subs;	//サブスキル(1〜3個)
	SkillValue rare;			//レアスキル(無いことが多い)
	SkillValue abyss;			//しんえんスキル(★6 だけ)
	bool isLocked = false;

	/// <summary>
	/// 主人公にかかる補正を足す
	/// </summary>
	void ApplyTo(HeroBonus& bonus) const;

	/// <summary>
	/// 表示用の名前(例: ★3 どくのおまもり)
	/// </summary>
	std::wstring Name() const;
};

namespace CharmMaker
{
	/// <summary>
	/// ランダムに作る
	/// </summary>
	/// <param name="power">深さや錬成のランク(0〜)。高いほどレア度が上がりやすい</param>
	/// <param name="luck">幸運スキルの値</param>
	/// <param name="fixedMain">メインスキルの指定(Noneでランダム)</param>
	Charm Make(int power, int luck, SkillKind fixedMain = SkillKind::None);

	/// <summary>
	/// 分解で得られるかけらの数
	/// </summary>
	int ShardValue(const Charm& c);

	/// <summary>
	/// 分解で得られるスキルだま(メインはLvの数、サブ・レアは1こずつ)
	/// </summary>
	std::vector<SkillValue> Orbs(const Charm& c);

	/// <summary>
	/// スキルを えらんで つくる(れんきん)
	/// </summary>
	/// <param name="subs">かならず付けるサブ(2こまで)</param>
	/// <param name="rare">かならず付けるレア(Noneで なし)</param>
	/// <summary>
	/// ★6 の おまもり(しんえん げんてい ドロップ。★5 と おなじ すうちに しんえんスキルが 1つ)
	/// </summary>
	Charm MakeAbyss(int power, int luck);

	Charm MakeWith(int power, SkillKind main, const std::vector<SkillKind>& subs, SkillKind rare);

	Game::Rgb RarityColor(int rarity);

	/// <summary>
	/// メインスキルの候補
	/// </summary>
	const std::vector<SkillKind>& MainKinds();
	const std::vector<SkillKind>& SubKinds();
	const std::vector<SkillKind>& RareKinds();
}
