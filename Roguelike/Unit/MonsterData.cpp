#include "MonsterData.h"
#include "MathUtil.h"
#include<fstream>
#include<sstream>
#include<Windows.h>
#include<algorithm>

namespace
{
	std::vector<MonsterData> table;

	std::wstring ToWide(const std::string& s)
	{
		const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
		std::wstring w(len > 0 ? len - 1 : 0, L'\0');
		if (len > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
		return w;
	}
}

bool MonsterTable::Load(const char* path)
{
	table.clear();
	std::ifstream file(path);
	if (!file) return false;

	std::string line;
	std::getline(file, line);	//見出し
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty()) continue;

		std::vector<std::string> cols;
		std::stringstream ss(line);
		std::string col;
		while (std::getline(ss, col, ',')) cols.push_back(col);
		if (cols.size() < 17) continue;

		MonsterData m;
		m.id = std::stoi(cols[0]);
		m.name = ToWide(cols[1]);
		m.hp = std::stoi(cols[2]);
		m.atk = std::stoi(cols[3]);
		m.def = std::stoi(cols[4]);
		m.exp = std::stoi(cols[5]);
		m.minFloor = std::stoi(cols[6]);
		m.maxFloor = std::stoi(cols[7]);
		m.infMinFloor = std::stoi(cols[8]);
		m.infMaxFloor = std::stoi(cols[9]);
		m.recruitRate = std::stoi(cols[10]);
		m.color = { std::stoi(cols[11]), std::stoi(cols[12]), std::stoi(cols[13]) };
		m.shape = static_cast<MonsterShape>(std::stoi(cols[14]));
		m.skill = static_cast<Skill>(std::stoi(cols[15]));
		m.type = static_cast<MonsterType>(std::stoi(cols[16]));
		if (cols.size() >= 18) m.desc = ToWide(cols[17]);
		table.push_back(m);
	}
	return !table.empty();
}

const MonsterData& MonsterTable::Get(int id)
{
	return table[id];
}

const std::vector<MonsterData>& MonsterTable::All()
{
	return table;
}

namespace
{
	//ステージごとの出る階
	bool IsInRange(const MonsterData& m, int floor, bool isInfinite)
	{
		const int lo = isInfinite ? m.infMinFloor : m.minFloor;
		const int hi = isInfinite ? m.infMaxFloor : m.maxFloor;
		return lo > 0 && floor >= lo && floor <= hi;
	}
}

int MonsterTable::PickForFloor(int floor, bool isInfinite)
{
	//まれにレアが出る
	if (MathUtil::RandI(0, 99) < 4)
	{
		for (const auto& m : table)
		{
			if (m.type == MonsterType::Rare && IsInRange(m, floor, isInfinite)) return m.id;
		}
	}

	std::vector<int> candidates;
	for (const auto& m : table)
	{
		if (m.type == MonsterType::Normal && IsInRange(m, floor, isInfinite)) candidates.push_back(m.id);
	}
	if (candidates.empty()) return 0;
	return candidates[MathUtil::RandI(0, static_cast<int>(candidates.size()) - 1)];
}

int MonsterTable::BossFor(int floor, bool isInfinite)
{
	for (const auto& m : table)
	{
		if (m.type == MonsterType::Boss && IsInRange(m, floor, isInfinite)) return m.id;
	}
	return -1;
}

const wchar_t* MonsterTable::SkillName(Skill skill)
{
	switch (skill)
	{
	case Skill::Heal:	return L"かいふく";
	case Skill::Ranged:	return L"とおくから こうげき";
	case Skill::Double:	return L"2かい こうげき";
	case Skill::Drain:	return L"HPを すいとる";
	case Skill::Poison:	return L"どくに する";
	case Skill::Freeze:	return L"まひ させる";
	case Skill::Burn:	return L"もやす";
	case Skill::Wide:	return L"まわりを まとめて こうげき";
	case Skill::Summon:	return L"てしたを よぶ";
	default:			return L"とくぎ なし";
	}
}

MonsterData MonsterTable::ShinyVariant(int id)
{
	MonsterData m = table[id];

	//色を入れかえて明るくする
	m.color = { std::min(255, m.color.b + 60), std::min(255, m.color.r + 40), std::min(255, m.color.g + 60) };
	m.hp = m.hp * 13 / 10;
	m.atk = m.atk * 13 / 10;
	m.def = m.def * 13 / 10;
	m.exp *= 3;
	m.recruitRate /= 2;
	return m;
}
