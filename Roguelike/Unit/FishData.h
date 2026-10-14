#pragma once
#include"Game.h"
#include"MathUtil.h"
#include"Skill.h"
#include<vector>
#include<string>

/// <summary>
/// さかなの かたち
/// </summary>
enum class FishShape
{
	Normal,
	Long,	//ほそながい(ウナギ・ドジョウ)
	Round,	//まるい(ブルーギル)
	Jelly,	//ぷるぷる(プルンギョ)
};

/// <summary>
/// むらの いけで つれる さかな
/// </summary>
struct FishData
{
	const wchar_t* name;
	const wchar_t* desc;
	int rarity;			//★の かず(1〜5)
	int minSize;		//cm
	int maxSize;
	float difficulty;	//0〜1(おおきいほど あばれる)
	FishShape shape;
	Game::Rgb color;
	Game::Rgb belly;	//おなか・もようの いろ
	bool hasWhisker;	//ひげ
	bool hasSpots;		//もよう
	bool needsClear;	//クリア後に つれる
	SkillValue meal;	//たきびで りょうりして たべると つぎの ぼうけんの あいだ きく
	const wchar_t* dish;	//りょうりの なまえ
};

namespace FishTable
{
	const std::vector<FishData>& All();

	/// <summary>
	/// いま つれる さかなを ひとつ えらぶ(★が おおいほど でにくい)
	/// </summary>
	int Pick(bool isCleared);

	/// <summary>
	/// つれた おおきさ(ちいさいほうが でやすい)
	/// </summary>
	int RollSize(int index);

	/// <summary>
	/// つれるか(クリア前に つれないものは まだ ひみつ)
	/// </summary>
	bool IsAvailable(int index, bool isCleared);

	/// <summary>
	/// りょうりの こうか(おおきい さかなほど すこし つよい)
	/// </summary>
	SkillValue MealSkill(int index, int size);

	/// <summary>
	/// りょうりの こうかを みじかく(例: こうげき+2)
	/// </summary>
	std::wstring MealText(int index, int size);

	/// <summary>
	/// ひとしゅるい あたり もっておける かず
	/// </summary>
	constexpr int kMaxStock = 9;
}

namespace FishDraw
{
	/// <summary>
	/// さかなを かく(右むき、center は からだの まんなか)
	/// </summary>
	/// <param name="length">あたまから しっぽまでの ながさ</param>
	/// <param name="isShadow">まだ つっていない(まっくろな かげ)</param>
	void Fish(const Vec2& center, float length, const FishData& data, float time, float facing, bool isShadow);
}
