#pragma once
#include"DungeonMap.h"
#include"MonsterData.h"
#include"MathUtil.h"
#include<string>

enum class Faction
{
	Player,
	Ally,
	Enemy,
};

/// <summary>
/// ダンジョンにいるキャラクター(主人公・仲間・敵)
/// </summary>
struct Unit
{
	int monsterId = -1;		//-1は主人公
	std::wstring name;
	Faction faction = Faction::Enemy;
	Cell cell;
	Vec2 drawPos;
	Cell dir = { 1, 0 };	//向いている方向

	int hp = 1;
	int maxHp = 1;
	int atk = 1;
	int def = 0;
	int level = 1;
	int exp = 0;
	Skill skill = Skill::None;
	bool isAlive = true;
	bool isShiny = false;
	bool isElite = false;		//無限ダンジョンの強敵
	bool isNoticed = false;		//色違い・強敵の登場演出を出したか
	int bonusHp = 0;			//だいじな もので ふえた さいだいHP(なかま)

	//状態異常(残りターン)
	int poisonTurns = 0;
	int poisonDamage = 0;
	int burnTurns = 0;
	int burnDamage = 0;
	int paralyzeTurns = 0;
	bool isStunned = false;

	//演出
	float hitFlash = 0.0f;
	float attackAnim = 0.0f;
	Vec2 attackDir;
	float facing = 1.0f;

	bool IsParty() const { return faction != Faction::Enemy; }
	bool IsBoss() const { return monsterId >= 0 && MonsterTable::Get(monsterId).type == MonsterType::Boss; }

	/// <summary>
	/// 次のレベルまでに必要な経験値
	/// </summary>
	int NextExp() const { return level * level * 6 + 4; }

	/// <summary>
	/// 見た目と基本能力のデータ(色違いを反映)
	/// </summary>
	MonsterData Data() const { return isShiny ? MonsterTable::ShinyVariant(monsterId) : MonsterTable::Get(monsterId); }
};

namespace UnitDraw
{
	/// <summary>
	/// 主人公の描画
	/// </summary>
	void Hero(const Vec2& feet, float size, float time, float facing, bool isHurt);

	/// <summary>
	/// モンスターの描画
	/// </summary>
	void Monster(const Vec2& feet, float size, const MonsterData& data, float time, float facing, bool isHurt);
}
