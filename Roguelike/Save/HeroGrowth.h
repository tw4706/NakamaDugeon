#pragma once
#include"SaveData.h"
#include<array>
#include<algorithm>

/// <summary>
/// ゆうしゃの そだちかた(レベルで かってに のびる ぶん ＋ レベルアップの 2択)
/// クリア後は 2択で えらんだ ぶんを むらで ふりなおせる
/// </summary>
namespace HeroGrowth
{
	//はじめの のうりょく
	constexpr int kBaseHp = 32;
	constexpr int kBaseAtk = 7;
	constexpr int kBaseDef = 2;

	/// <summary>
	/// レベルアップの 2択(1ポイントぶん)
	/// </summary>
	struct Choice
	{
		const wchar_t* name;
		int hp;
		int atk;
		int def;
		int crit;
	};

	constexpr int kNum = 4;
	inline constexpr Choice kChoices[kNum] =
	{
		{ L"こうげき +1", 0, 1, 0, 0 },
		{ L"ぼうぎょ +1", 0, 0, 1, 0 },
		{ L"さいだいHP +4", 4, 0, 0, 0 },
		{ L"かいしん +2%", 0, 0, 0, 2 },
	};

	//------------------------------//
	// レベルで かってに のびる ぶん(1レベルごとに HP+4・こうげき+1、ぐうすうの レベルで ぼうぎょ+1)
	//------------------------------//
	inline int AutoHp(int level) { return kBaseHp + 4 * (level - 1); }
	inline int AutoAtk(int level) { return kBaseAtk + (level - 1); }
	inline int AutoDef(int level) { return kBaseDef + level / 2; }

	/// <summary>
	/// 2択で ふれる ポイントの かず(レベル - 1)
	/// </summary>
	inline int TotalPoints(int level) { return std::max(0, level - 1); }

	/// <summary>
	/// いまの そだちから、2択で えらんだ ポイントを もどす(こうげき・ぼうぎょ・HP・かいしん の じゅん)
	/// </summary>
	inline std::array<int, kNum> PointsOf(const HeroProgress& h)
	{
		std::array<int, kNum> p = {};
		if (h.maxHp <= 0) return p;	//まだ そだっていない
		p[0] = std::max(0, h.atk - AutoAtk(h.level));
		p[1] = std::max(0, h.def - AutoDef(h.level));
		p[2] = std::max(0, (h.maxHp - AutoHp(h.level)) / 4);
		p[3] = std::max(0, h.crit / 2);

		//むかしの セーブで ずれていても ポイントの かずを こえないように
		int over = p[0] + p[1] + p[2] + p[3] - TotalPoints(h.level);
		for (int i = kNum - 1; i >= 0 && over > 0; i--)
		{
			const int cut = std::min(p[i], over);
			p[i] -= cut;
			over -= cut;
		}
		return p;
	}

	/// <summary>
	/// ポイントの ふりかたで そだちを きめなおす
	/// </summary>
	inline void Apply(HeroProgress& h, const std::array<int, kNum>& p)
	{
		h.maxHp = AutoHp(h.level);
		h.atk = AutoAtk(h.level);
		h.def = AutoDef(h.level);
		h.crit = 0;
		for (int i = 0; i < kNum; i++)
		{
			h.maxHp += kChoices[i].hp * p[i];
			h.atk += kChoices[i].atk * p[i];
			h.def += kChoices[i].def * p[i];
			h.crit += kChoices[i].crit * p[i];
		}
	}
}
