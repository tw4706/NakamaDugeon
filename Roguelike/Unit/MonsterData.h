#pragma once
#include"Game.h"
#include<string>
#include<vector>

/// <summary>
/// 見た目の形
/// </summary>
enum class MonsterShape
{
	Slime,
	Bat,
	Golem,
	Ghost,
	Beast,
	Dragon,
	Bird,		//とり
	Knight,		//よろい
	Spirit,		//せいれい
	Giant,		//まじん
	Priest,		//しんかん
};

/// <summary>
/// 特技
/// </summary>
enum class Skill
{
	None,
	Heal,		//弱った味方を回復
	Ranged,		//離れた相手を攻撃
	Double,		//2回攻撃
	Drain,		//与えたダメージの半分回復
	Poison,		//攻撃で毒にする
	Freeze,		//攻撃でまひさせる
	Burn,		//攻撃で炎上させる
	Wide,		//まわりの相手をまとめて攻撃
	Summon,		//手下を呼ぶ(ボス)
};

/// <summary>
/// 出現の種類
/// </summary>
enum class MonsterType
{
	Normal,
	Rare,
	Boss,
};

struct MonsterData
{
	int id;
	std::wstring name;
	int hp;
	int atk;
	int def;
	int exp;
	int minFloor;
	int maxFloor;
	int infMinFloor;	//むげんの めいきゅうで出る階(0は出ない。ボスはその階のぬし)
	int infMaxFloor;
	int recruitRate;	//仲間になる確率(%)
	Game::Rgb color;
	MonsterShape shape;
	Skill skill;
	MonsterType type;
	std::wstring desc;
};

namespace MonsterTable
{
	/// <summary>
	/// CSVの読み込み
	/// </summary>
	bool Load(const char* path);

	const MonsterData& Get(int id);
	const std::vector<MonsterData>& All();

	/// <summary>
	/// その階に出るモンスターを選ぶ
	/// </summary>
	int PickForFloor(int floor, bool isInfinite);

	/// <summary>
	/// その階のボス(いなければ-1)
	/// </summary>
	int BossFor(int floor, bool isInfinite);

	const wchar_t* SkillName(Skill skill);

	/// <summary>
	/// 色違いのデータ(色と能力を変えたもの)
	/// </summary>
	MonsterData ShinyVariant(int id);
}
