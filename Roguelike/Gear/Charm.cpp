#include "Charm.h"
#include "MathUtil.h"
#include<algorithm>

using namespace MathUtil;

namespace
{
	const std::vector<SkillKind> kMains =
	{
		SkillKind::Poison, SkillKind::Paralyze, SkillKind::Wide, SkillKind::Burn, SkillKind::Drain, SkillKind::Command,
	};

	struct SubDef
	{
		SkillKind kind;
		int base;	//★1の値
		int step;	//★1つごとに増える値
	};

	const SubDef kSubs[] =
	{
		{ SkillKind::Atk, 1, 2 },		//ver1.7.1: ★ごとの のびを +1 → +2(★5で やく 9)
		{ SkillKind::Def, 1, 2 },
		{ SkillKind::Hp, 4, 6 },		//+3 → +6(★5で やく 28)
		{ SkillKind::Crit, 2, 2 },
		{ SkillKind::Regen, 1, 1 },		//★で のびなかったのを なおした
		{ SkillKind::Exp, 5, 4 },
		{ SkillKind::Vision, 1, 0 },
		{ SkillKind::Resist, 10, 8 },
	};

	struct RareDef
	{
		SkillKind kind;
		int value;
		int weight;
	};

	const RareDef kRares[] =
	{
		{ SkillKind::Shiny, 100, 1 },		//色違い2倍(超レア)
		{ SkillKind::AllySlot, 1, 2 },
		{ SkillKind::Recruit, 10, 4 },
		{ SkillKind::Luck, 1, 4 },
		{ SkillKind::Guts, 1, 3 },
	};

	const wchar_t* MainNoun(SkillKind kind)
	{
		switch (kind)
		{
		case SkillKind::Poison:		return L"どく";
		case SkillKind::Paralyze:	return L"しびれ";
		case SkillKind::Wide:		return L"せんぷう";
		case SkillKind::Burn:		return L"ほのお";
		case SkillKind::Drain:		return L"ちすい";
		case SkillKind::Command:	return L"しきかん";
		default:					return L"ふしぎ";
		}
	}
}

void Charm::ApplyTo(HeroBonus& bonus) const
{
	SkillValue m = main;

	//吸収はLvを%に直す
	if (m.kind == SkillKind::Drain) m.value *= 8;
	bonus.Add(m);
	for (const auto& s : subs) bonus.Add(s);
	if (rare.kind != SkillKind::None) bonus.Add(rare);
	if (abyss.kind != SkillKind::None) bonus.Add(abyss);
}

std::wstring Charm::Name() const
{
	return std::wstring(L"★") + std::to_wstring(rarity) + L" " + MainNoun(main.kind) + L"の おまもり";
}

Charm CharmMaker::Make(int power, int luck, SkillKind fixedMain)
{
	Charm c;

	//レア度: 深いほど・運が良いほど上がりやすい
	const int roll = RandI(0, 999) + power * 40 + luck * 120;
	if (roll >= 1150) c.rarity = 5;
	else if (roll >= 920) c.rarity = 4;
	else if (roll >= 650) c.rarity = 3;
	else if (roll >= 350) c.rarity = 2;
	else c.rarity = 1;

	//メイン
	c.main.kind = fixedMain != SkillKind::None ? fixedMain : kMains[RandI(0, static_cast<int>(kMains.size()) - 1)];
	const int lvRoll = RandI(0, 99) + c.rarity * 12;
	c.main.value = lvRoll >= 95 ? 3 : (lvRoll >= 60 ? 2 : 1);

	//サブ(★で数が増える、同じ種類は付かない)
	const int subNum = c.rarity >= 5 ? 3 : (c.rarity >= 3 ? 2 : 1);
	std::vector<int> pool;
	for (int i = 0; i < static_cast<int>(std::size(kSubs)); i++) pool.push_back(i);
	for (int k = 0; k < subNum && !pool.empty(); k++)
	{
		const int pick = RandI(0, static_cast<int>(pool.size()) - 1);
		const SubDef& d = kSubs[pool[pick]];
		pool.erase(pool.begin() + pick);

		int value = d.base + d.step * (c.rarity - 1);
		value = std::max(1, static_cast<int>(value * RandF(0.7f, 1.3f) + 0.5f));
		c.subs.push_back({ d.kind, value });
	}

	//レア(超低確率。付くとサブが1つ減る)
	if (RandI(0, 999) < 8 + luck * 6 + c.rarity * 2)
	{
		int total = 0;
		for (const auto& r : kRares) total += r.weight;
		int w = RandI(0, total - 1);
		for (const auto& r : kRares)
		{
			if (w < r.weight) { c.rare = { r.kind, r.value }; break; }
			w -= r.weight;
		}
		if (c.subs.size() > 1) c.subs.pop_back();
	}
	return c;
}

int CharmMaker::ShardValue(const Charm& c)
{
	static const int kTable[7] = { 0, 1, 2, 4, 8, 16, 32 };
	return kTable[std::clamp(c.rarity, 1, 6)] + (c.rare.kind != SkillKind::None ? 10 : 0);
}

Game::Rgb CharmMaker::RarityColor(int rarity)
{
	static const Game::Rgb kTable[7] =
	{
		{ 200, 200, 200 }, { 220, 220, 220 }, { 120, 220, 120 }, { 100, 170, 255 }, { 220, 120, 255 }, { 255, 200, 60 }, { 255, 90, 170 },
	};
	return kTable[std::clamp(rarity, 1, 6)];
}

const std::vector<SkillKind>& CharmMaker::MainKinds()
{
	return kMains;
}

const std::vector<SkillKind>& CharmMaker::SubKinds()
{
	static std::vector<SkillKind> kinds;
	if (kinds.empty()) for (const auto& d : kSubs) kinds.push_back(d.kind);
	return kinds;
}

const std::vector<SkillKind>& CharmMaker::RareKinds()
{
	static std::vector<SkillKind> kinds;
	if (kinds.empty()) for (const auto& r : kRares) kinds.push_back(r.kind);
	return kinds;
}

std::vector<SkillValue> CharmMaker::Orbs(const Charm& c)
{
	std::vector<SkillValue> out;
	out.push_back({ c.main.kind, c.main.value });
	for (const auto& s : c.subs) out.push_back({ s.kind, 1 });
	if (c.rare.kind != SkillKind::None) out.push_back({ c.rare.kind, 1 });
	return out;
}

Charm CharmMaker::MakeWith(int power, SkillKind main, const std::vector<SkillKind>& subs, SkillKind rare)
{
	Charm c = Make(power, 0, main);

	//レア
	if (rare != SkillKind::None)
	{
		for (const auto& r : kRares)
		{
			if (r.kind == rare) c.rare = { r.kind, r.value };
		}
	}

	//サブ: えらんだものを先に、のこりはランダム
	const int subNum = std::max(static_cast<int>(subs.size()), static_cast<int>(c.subs.size()));
	std::vector<SkillValue> result;
	auto valueOf = [&](const SubDef& d)
	{
		const int value = d.base + d.step * (c.rarity - 1);
		return std::max(1, static_cast<int>(value * RandF(0.85f, 1.3f) + 0.5f));
	};
	std::vector<int> pool;
	for (int i = 0; i < static_cast<int>(std::size(kSubs)); i++)
	{
		if (std::find(subs.begin(), subs.end(), kSubs[i].kind) != subs.end()) result.push_back({ kSubs[i].kind, valueOf(kSubs[i]) });
		else pool.push_back(i);
	}
	while (static_cast<int>(result.size()) < subNum && !pool.empty())
	{
		const int pick = RandI(0, static_cast<int>(pool.size()) - 1);
		result.push_back({ kSubs[pool[pick]].kind, valueOf(kSubs[pool[pick]]) });
		pool.erase(pool.begin() + pick);
	}
	c.subs = result;
	return c;
}

Charm CharmMaker::MakeAbyss(int power, int luck)
{
	//★5 の つくりかたを もとに、メインの Lv が たかめ
	Charm c = Make(std::max(power, 30), luck);
	c.rarity = 6;
	const int lvRoll = RandI(0, 99) + c.rarity * 12;
	c.main.value = lvRoll >= 95 ? 3 : 2;

	//サブは ★6 の あたいで つくりなおし、レアが ついても 3つ のこす
	std::vector<int> pool;
	for (int i = 0; i < static_cast<int>(std::size(kSubs)); i++)
	{
		bool isUsed = false;
		for (const SkillValue& s : c.subs) if (s.kind == kSubs[i].kind) isUsed = true;
		if (!isUsed) pool.push_back(i);
	}
	while (c.subs.size() < 3 && !pool.empty())
	{
		const int pick = RandI(0, static_cast<int>(pool.size()) - 1);
		c.subs.push_back({ kSubs[pool[pick]].kind, 0 });
		pool.erase(pool.begin() + pick);
	}
	for (SkillValue& s : c.subs)
	{
		for (const SubDef& d : kSubs)
		{
			if (d.kind != s.kind) continue;
			const int value = d.base + d.step * (c.rarity - 1);
			s.value = std::max(1, static_cast<int>(value * RandF(0.85f, 1.3f) + 0.5f));
		}
	}

	//しんえんスキル(しんえんの なかで きく)
	struct AbyssDef { SkillKind kind; int min; int max; };
	static const AbyssDef kAbyss[] =
	{
		{ SkillKind::AbyssPower, 20, 30 },
		{ SkillKind::AbyssGuard, 20, 30 },
		{ SkillKind::DeepBlessing, 10, 18 },
	};
	const AbyssDef& d = kAbyss[RandI(0, static_cast<int>(std::size(kAbyss)) - 1)];
	c.abyss = { d.kind, RandI(d.min, d.max) };

	//だいじな ものなので さいしょから ロック
	c.isLocked = true;
	return c;
}
