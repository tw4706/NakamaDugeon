#include "Skill.h"

namespace
{
	struct Entry
	{
		const char* key;
		const wchar_t* name;
		const wchar_t* unit;	//数値の後ろに付ける文字(空なら"Lv")
	};

	const Entry kEntries[static_cast<int>(SkillKind::Num)] =
	{
		{ "none", L"なし", L"" },
		{ "atk", L"こうげき", L"+" },
		{ "def", L"ぼうぎょ", L"+" },
		{ "hp", L"さいだいHP", L"+" },
		{ "crit", L"かいしん", L"%" },
		{ "regen", L"しぜんかいふく", L"+" },
		{ "exp", L"けいけんち", L"%" },
		{ "vision", L"しかい", L"+" },
		{ "resist", L"いじょう たいせい", L"%" },
		{ "poison", L"どく", L"Lv" },
		{ "paralyze", L"まひ", L"Lv" },
		{ "wide", L"こうげき はんい", L"Lv" },
		{ "burn", L"えんじょう", L"Lv" },
		{ "drain", L"きゅうしゅう", L"%" },
		{ "command", L"ごうれい", L"Lv" },
		{ "evade", L"かいひ", L"%" },
		{ "double", L"2かいこうげき", L"%" },
		{ "halfdmg", L"ダメージはんげん", L"%" },
		{ "floorheal", L"かいだんで かいふく", L"" },
		{ "stun", L"ひるませ", L"%" },
		{ "treasure", L"そざいが でやすい", L"%" },
		{ "shiny", L"いろちがい しゅつげん", L"%" },
		{ "allyslot", L"なかまの わく", L"+" },
		{ "recruit", L"スカウト", L"%" },
		{ "luck", L"こううん", L"+" },
		{ "guts", L"ふくつ", L"" },
		{ "intimidate", L"いあつ", L"%" },
		{ "bond", L"きずな", L"%" },
		{ "freeze", L"こおりづけ", L"%" },
		{ "blast", L"ばくえん", L"Lv" },
		{ "lifepact", L"いのちの けいやく", L"%" },
		{ "peak", L"ぜっこうちょう", L"%" },
		{ "venom", L"どくぎり", L"%" },
		{ "counter", L"はんげき", L"%" },
		{ "desperate", L"はいすい", L"%" },
	};
}

SkillKind SkillInfo::FromKey(const std::string& key)
{
	for (int i = 0; i < static_cast<int>(SkillKind::Num); i++)
	{
		if (key == kEntries[i].key) return static_cast<SkillKind>(i);
	}
	return SkillKind::None;
}

const wchar_t* SkillInfo::Name(SkillKind kind)
{
	return kEntries[static_cast<int>(kind)].name;
}

std::wstring SkillInfo::Describe(const SkillValue& s)
{
	const Entry& e = kEntries[static_cast<int>(s.kind)];
	const std::wstring unit = e.unit;
	if (unit.empty()) return e.name;
	if (unit == L"Lv") return std::wstring(e.name) + L" Lv" + std::to_wstring(s.value);
	if (unit == L"%") return std::wstring(e.name) + L" +" + std::to_wstring(s.value) + L"%";
	return std::wstring(e.name) + L" +" + std::to_wstring(s.value);
}

const wchar_t* SkillInfo::Summary(SkillKind kind)
{
	switch (kind)
	{
	case SkillKind::Atk:		return L"こうげきりょくが あがり、てきに あたえる ダメージが ふえる。";
	case SkillKind::Def:		return L"ぼうぎょりょくが あがり、うける ダメージが へる。";
	case SkillKind::Hp:			return L"さいだいHPが ふえる。";
	case SkillKind::Crit:		return L"かいしんの いちげきが でやすく なる。かいしんは あいての ぼうぎょを むしして 1.8ばいの ダメージ。";
	case SkillKind::Regen:		return L"4ターンごとに HPが かいふくする。たたかいの さいちゅうでも きく。";
	case SkillKind::Exp:		return L"てきを たおして もらえる けいけんちが ふえる。";
	case SkillKind::Vision:		return L"ダンジョンで みえる はんいが ひろがる。";
	case SkillKind::Resist:		return L"どく・まひ・えんじょうに かかりにくく なる。";
	case SkillKind::Poison:		return L"こうげきした てきを どくに する。どくは まいターン ダメージ。";
	case SkillKind::Paralyze:	return L"こうげきした てきを まひさせて うごけなく する。ぬしには きかない。";
	case SkillKind::Wide:		return L"ねらった てきの まわりにも ダメージを あたえる。";
	case SkillKind::Burn:		return L"こうげきした てきを もやす。もえている あいだ まいターン ダメージ。";
	case SkillKind::Drain:		return L"あたえた ダメージの いちぶだけ HPが かいふくする。";
	case SkillKind::Command:	return L"なかまが ゆうしゃと おなじ てきを ねらい、なかまの ダメージが あがる。";
	case SkillKind::Evade:		return L"てきの こうげきを よける ことが ある。";
	case SkillKind::Double:		return L"こうげきした あと もう いちど こうげきする ことが ある。";
	case SkillKind::HalfDamage:	return L"うける ダメージが はんぶんに なる ことが ある。";
	case SkillKind::FloorHeal:	return L"かいだんを おりるたびに なかまも ふくめて ぜんかいふく。";
	case SkillKind::Stun:		return L"こうげきした てきを ひるませて 1かい やすませる。ぬしには きかない。";
	case SkillKind::Treasure:	return L"たおした てきが そざいを おとしやすく なる。";
	case SkillKind::Shiny:		return L"いろちがいの モンスターに であいやすく なる（クリア後）。";
	case SkillKind::AllySlot:	return L"つれて いける なかまの かずが ふえる。";
	case SkillKind::Recruit:	return L"たおした モンスターが なかまに なりやすく なる。";
	case SkillKind::Luck:		return L"あたらしく でる おまもりの ★が たかく なりやすい。";
	case SkillKind::Guts:		return L"ぼうけんの なかで 1かいだけ、たおれる こうげきを HP1で たえる。";
	case SkillKind::Intimidate:	return L"となりに いる てきの こうげきりょくを さげる。";
	case SkillKind::Bond:		return L"ちかく（2マス）に いる なかまが おおいほど ゆうしゃの ダメージが あがる。";
	case SkillKind::Freeze:		return L"まひ・ひるみ中の てきに あたえる ダメージが あがる。";
	case SkillKind::Blast:		return L"こうげきで てきを もやし、もえている てきを たおすと ばくはつして まわりに もえうつる。";
	case SkillKind::LifePact:	return L"ゆうしゃか なかまが てきを たおすたびに ゆうしゃの HPが かいふくする。";
	case SkillKind::Peak:		return L"HPが まんたんの あいだ あたえる ダメージが あがる。";
	case SkillKind::VenomEdge:	return L"どくの てきに あたえる ダメージが あがる。";
	case SkillKind::Counter:	return L"てきの こうげきを うけたとき、すぐに やりかえす ことが ある。";
	case SkillKind::Desperate:	return L"HPが のこり 30% いかの とき あたえる ダメージが おおきく あがる。";
	default:					return L"";
	}
}

std::wstring SkillInfo::Explain(const SkillValue& s)
{
	const int v = s.value;
	const std::wstring n = std::to_wstring(v);
	switch (s.kind)
	{
	case SkillKind::Atk:		return L"こうげき +" + n + L"（ダメージが やく " + n + L" ふえる）";
	case SkillKind::Def:		return L"ぼうぎょ +" + n + L"（うける ダメージが やく " + std::to_wstring(v / 2) + (v % 2 ? L".5" : L"") + L" へる）";
	case SkillKind::Hp:			return L"さいだいHP +" + n;
	case SkillKind::Crit:		return L"かいしんの かくりつ +" + n + L"%（もとは 6%）";
	case SkillKind::Regen:		return L"4ターンごとに さいだいHPの " + std::to_wstring(v / 2) + (v % 2 ? L".5" : L"") + L"%（さいてい " + n + L"）かいふく";
	case SkillKind::Exp:		return L"もらえる けいけんちが " + n + L"% ふえる";
	case SkillKind::Vision:		return L"みえる はんいが " + n + L"マス ひろがる";
	case SkillKind::Resist:		return L"どくに なる かくりつ -" + n + L"%、まひ・えんじょう -" + std::to_wstring(v / 2) + L"%";
	case SkillKind::Poison:		return L"Lv" + n + L"：" + std::to_wstring(3 + v) + L"ターン どく（まいターン " + std::to_wstring(1 + v * 2) + L"＋かいの ふかさ÷3）";
	case SkillKind::Paralyze:	return L"Lv" + n + L"：" + std::to_wstring(12 * v) + L"% で 2ターン まひ";
	case SkillKind::Wide:
		if (v <= 1) return L"Lv1：ねらった てきと じぶんの りょうほうの となりに 60% の ダメージ";
		if (v == 2) return L"Lv2：じぶんの まわり 8マス ぜんぶに 60% の ダメージ";
		return L"Lv3：2マス さきまで（みえる てき）60% の ダメージ";
	case SkillKind::Burn:		return L"Lv" + n + L"：3ターン もやす（1ターン こうげき×" + n + L"÷5 の ダメージ）";
	case SkillKind::Drain:		return L"あたえた ダメージの " + n + L"% だけ かいふく";
	case SkillKind::Command:	return L"Lv" + n + L"：なかまの ダメージ +" + std::to_wstring(15 * v) + L"%";
	case SkillKind::Evade:		return n + L"% で てきの こうげきを よける";
	case SkillKind::Double:		return n + L"% で もう いちど こうげき";
	case SkillKind::HalfDamage:	return n + L"% で うける ダメージが はんぶん";
	case SkillKind::FloorHeal:	return L"かいだんを おりると ぜんいん ぜんかいふく";
	case SkillKind::Stun:		return n + L"% で てきを ひるませる";
	case SkillKind::Treasure:	return L"そざいの おちる かくりつ +" + n + L"%（もとは 30%）";
	case SkillKind::Shiny:		return L"いろちがいの でやすさ +" + n + L"%（もとは 1%）";
	case SkillKind::AllySlot:	return L"なかまの わく +" + n + L"（もとは 2たい）";
	case SkillKind::Recruit:	return L"なかまに なる かくりつ +" + n + L"%";
	case SkillKind::Luck:		return L"こううん +" + n + L"：★の たかい おまもりが でやすい";
	case SkillKind::Guts:		return L"1かいだけ HP1で たえる";
	case SkillKind::Intimidate:	return L"となりの てきの こうげき -" + n + L"%";
	case SkillKind::Bond:		return L"ちかくの なかま 1たいごとに ダメージ +" + n + L"%";
	case SkillKind::Freeze:		return L"まひ・ひるみ中の てきへ ダメージ +" + n + L"%";
	case SkillKind::Blast:		return L"えんじょう Lv" + n + L"　もえている てきを たおすと ばくはつ（まわりに こうげきの 80% と えんじょう）";
	case SkillKind::LifePact:	return L"てきを たおすたびに さいだいHPの " + n + L"% かいふく";
	case SkillKind::Peak:		return L"HPが まんたんの とき ダメージ +" + n + L"%";
	case SkillKind::VenomEdge:	return L"どくの てきへ ダメージ +" + n + L"%";
	case SkillKind::Counter:	return n + L"% で はんげき（ふつうの こうげき）";
	case SkillKind::Desperate:	return L"HP 30% いかで ダメージ +" + n + L"%";
	default:					return L"";
	}
}

bool SkillInfo::IsLevelSkill(SkillKind kind)
{
	return kind == SkillKind::Poison || kind == SkillKind::Paralyze || kind == SkillKind::Wide ||
		kind == SkillKind::Burn || kind == SkillKind::Command;
}

bool SkillInfo::IsMain(SkillKind kind)
{
	return kind >= SkillKind::Poison && kind <= SkillKind::Command;
}

bool SkillInfo::IsRare(SkillKind kind)
{
	return kind >= SkillKind::Shiny && kind <= SkillKind::Guts;
}
