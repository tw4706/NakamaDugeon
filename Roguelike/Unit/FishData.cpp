#include "FishData.h"
#include<DxLib.h>
#include<cmath>

using namespace MathUtil;

namespace
{
	//★ごとの でやすさ
	constexpr float kRarityWeight[6] = { 0.0f, 40.0f, 24.0f, 12.0f, 5.0f, 1.5f };
}

const std::vector<FishData>& FishTable::All()
{
	static const std::vector<FishData> kTable =
	{
		{ L"メダカ", L"いけの すみで むれて およぐ ちいさな さかな。つりばりに かかるのは ちょっと めずらしい。", 1, 2, 4, 0.05f, FishShape::Normal, { 200, 190, 150 }, { 240, 235, 210 }, false, false, false, { SkillKind::Regen, 1 }, L"メダカの つくだに" },
		{ L"フナ", L"どこの いけにも いる ぎんいろの さかな。つりの きほんは フナから。", 1, 10, 28, 0.15f, FishShape::Normal, { 150, 150, 120 }, { 215, 210, 180 }, false, false, false, { SkillKind::Hp, 5 }, L"フナの しおやき" },
		{ L"ドジョウ", L"どろの なかに ひそむ ほそながい さかな。ひげで えさを さがす。", 1, 8, 16, 0.2f, FishShape::Long, { 140, 120, 80 }, { 200, 180, 130 }, true, true, false, { SkillKind::Resist, 10 }, L"ドジョウじる" },
		{ L"オイカワ", L"なつに なると からだが にじいろに かがやく。すばしっこい。", 2, 10, 18, 0.35f, FishShape::Normal, { 110, 160, 190 }, { 240, 140, 120 }, false, true, false, { SkillKind::Swift, 10 }, L"オイカワの からあげ" },
		{ L"ブルーギル", L"まるい からだの さかな。なんでも たべる くいしんぼう。", 2, 10, 22, 0.3f, FishShape::Round, { 100, 130, 110 }, { 230, 190, 110 }, false, false, false, { SkillKind::Def, 1 }, L"ブルーギルの フライ" },
		{ L"コイ", L"いけの おおもの。ちからが つよく、ひきが おもい。", 2, 30, 75, 0.45f, FishShape::Normal, { 120, 110, 80 }, { 210, 190, 140 }, true, false, false, { SkillKind::Hp, 10 }, L"コイこく" },
		{ L"キンギョ", L"だれかが はなした キンギョが いけで おおきく そだった。", 3, 5, 18, 0.25f, FishShape::Round, { 240, 90, 50 }, { 255, 200, 160 }, false, false, false, { SkillKind::Recruit, 5 }, L"キンギョの おかし" },
		{ L"ナマズ", L"よるに うごきだす ひげの さかな。じしんを しらせると いう はなしも。", 3, 30, 65, 0.5f, FishShape::Long, { 90, 85, 70 }, { 190, 180, 160 }, true, false, false, { SkillKind::Vision, 1 }, L"ナマズの てんぷら" },
		{ L"ニジマス", L"よこに にじいろの すじが はしる。つれると とても うれしい。", 3, 25, 55, 0.55f, FishShape::Normal, { 120, 140, 100 }, { 235, 130, 150 }, false, true, false, { SkillKind::Exp, 10 }, L"ニジマスの ムニエル" },
		{ L"ウナギ", L"にょろにょろと にげる ほそながい さかな。つかむのも ひとくろう。", 3, 40, 95, 0.6f, FishShape::Long, { 60, 70, 60 }, { 180, 190, 150 }, false, false, false, { SkillKind::Atk, 2 }, L"ウナギの かばやき" },
		{ L"ライギョ", L"いけの ギャング。くちが おおきく、かかると はげしく あばれる。", 4, 50, 90, 0.72f, FishShape::Long, { 80, 90, 60 }, { 170, 160, 110 }, false, true, false, { SkillKind::Crit, 5 }, L"ライギョの ステーキ" },
		{ L"プルンギョ", L"プルンが いけに すみついて さかなに なった？ さわると ぷるぷる。", 4, 15, 40, 0.65f, FishShape::Jelly, { 110, 200, 230 }, { 210, 245, 255 }, false, false, false, { SkillKind::Treasure, 10 }, L"プルンギョの ゼリー" },
		{ L"ニシキゴイ", L"あかと しろの もようが うつくしい コイ。むらの じまんの たからもの。", 5, 50, 90, 0.75f, FishShape::Normal, { 245, 245, 240 }, { 230, 70, 50 }, true, true, false, { SkillKind::Shiny, 30 }, L"ニシキゴイの おさしみ" },
		{ L"イトウ", L"まぼろしの おおきな さかな。ゆうしゃが いけを しずかに させてから すがたを みせた。", 4, 60, 130, 0.82f, FishShape::Normal, { 120, 90, 80 }, { 200, 120, 100 }, false, true, true, { SkillKind::Atk, 3 }, L"イトウの まるやき" },
		{ L"いけの ぬし", L"むらの いけに むかしから すむ きょだいな ナマズ。つりあげた ものは だれも いない…？", 5, 120, 220, 0.95f, FishShape::Long, { 50, 45, 60 }, { 160, 150, 190 }, true, true, true, { SkillKind::Exp, 25 }, L"ぬしの なべ" },
	};
	return kTable;
}

bool FishTable::IsAvailable(int index, bool isCleared)
{
	return isCleared || !All()[index].needsClear;
}

int FishTable::Pick(bool isCleared)
{
	const auto& all = All();
	float total = 0.0f;
	for (int i = 0; i < static_cast<int>(all.size()); i++)
	{
		if (IsAvailable(i, isCleared)) total += kRarityWeight[all[i].rarity];
	}
	float r = RandF(0.0f, total);
	for (int i = 0; i < static_cast<int>(all.size()); i++)
	{
		if (!IsAvailable(i, isCleared)) continue;
		r -= kRarityWeight[all[i].rarity];
		if (r <= 0.0f) return i;
	}
	return 0;
}

int FishTable::RollSize(int index)
{
	const FishData& f = All()[index];
	const float t = std::pow(RandF(0.0f, 1.0f), 1.8f);
	return f.minSize + static_cast<int>(std::round((f.maxSize - f.minSize) * t));
}

SkillValue FishTable::MealSkill(int index, int size)
{
	//いちばん おおきいと 1.5ばい(きりすて)
	const FishData& f = All()[index];
	const float ratio = f.maxSize > f.minSize ? Clamp01(static_cast<float>(size - f.minSize) / (f.maxSize - f.minSize)) : 0.0f;
	return { f.meal.kind, f.meal.value + static_cast<int>(f.meal.value * 0.5f * ratio) };
}

std::wstring FishTable::MealText(int index, int size)
{
	return SkillInfo::Describe(MealSkill(index, size));
}

void FishDraw::Fish(const Vec2& center, float length, const FishData& data, float time, float facing, bool isShadow)
{
	const float f = facing >= 0.0f ? 1.0f : -1.0f;
	const Game::Rgb shadow = { 25, 22, 35 };
	const unsigned int body = Game::ToColor(isShadow ? shadow : data.color);
	const unsigned int belly = Game::ToColor(isShadow ? shadow : data.belly);
	const unsigned int fin = Game::ToColor(isShadow ? shadow : Game::Rgb{ data.color.r * 3 / 4, data.color.g * 3 / 4, data.color.b * 3 / 4 });

	//かたちごとの からだの おおきさ
	float a = length * 0.36f;
	float b = length * 0.16f;
	float tailLen = length * 0.24f;
	switch (data.shape)
	{
	case FishShape::Long: a = length * 0.44f; b = length * 0.075f; tailLen = length * 0.1f; break;
	case FishShape::Round: a = length * 0.3f; b = length * 0.25f; tailLen = length * 0.2f; break;
	case FishShape::Jelly: a = length * 0.3f; b = length * 0.24f; tailLen = length * 0.18f; break;
	default: break;
	}

	const float wag = std::sin(time * 7.0f) * b * 0.35f;
	const Vec2 c = center + Vec2(tailLen * 0.4f * f, 0.0f);
	const float rear = c.x - a * 0.92f * f;
	const float front = c.x + a * f;

	if (data.shape == FishShape::Jelly && !isShadow) SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);

	//しっぽ
	DrawTriangleAA(rear + 4.0f * f, c.y, rear - tailLen * f, c.y - b * 0.95f + wag, rear - tailLen * 0.8f * f, c.y + wag * 0.3f, fin, TRUE);
	DrawTriangleAA(rear + 4.0f * f, c.y, rear - tailLen * f, c.y + b * 0.95f + wag, rear - tailLen * 0.8f * f, c.y + wag * 0.3f, fin, TRUE);

	//せびれ
	if (data.shape == FishShape::Long)
	{
		DrawBoxAA(std::min(rear, front - a * 0.6f * f), c.y - b * 1.25f, std::max(rear, front - a * 0.6f * f), c.y - b * 0.6f, fin, TRUE);
	}
	else
	{
		DrawTriangleAA(c.x - a * 0.45f * f, c.y - b * 0.7f, c.x + a * 0.25f * f, c.y - b * 0.85f, c.x - a * 0.25f * f, c.y - b * 1.45f, fin, TRUE);
	}

	//からだと おなか
	DrawOvalAA(c.x, c.y, a, b, 32, body, TRUE);
	DrawOvalAA(c.x + a * 0.05f * f, c.y + b * 0.38f, a * 0.8f, b * 0.5f, 24, belly, TRUE);

	//もよう
	if (data.hasSpots && !isShadow)
	{
		for (int k = 0; k < 4; k++)
		{
			const float sx = c.x + (-0.55f + k * 0.32f) * a * f;
			const float sy = c.y - b * (0.2f + 0.15f * (k % 2));
			DrawOvalAA(sx, sy, a * 0.13f, b * 0.24f, 12, belly, TRUE);
		}
	}

	//むなびれ
	DrawTriangleAA(c.x + a * 0.25f * f, c.y + b * 0.2f, c.x - a * 0.05f * f, c.y + b * 0.25f, c.x + a * 0.02f * f, c.y + b * 0.75f + wag * 0.5f, fin, TRUE);

	if (data.shape == FishShape::Jelly && !isShadow)
	{
		SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
		DrawOvalAA(c.x + a * 0.1f * f, c.y - b * 0.45f, a * 0.4f, b * 0.2f, 16, GetColor(255, 255, 255), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	if (isShadow) return;

	//ひげ
	if (data.hasWhisker)
	{
		const unsigned int whisker = GetColor(60, 50, 40);
		const float thick = std::max(1.0f, length * 0.012f);
		DrawLineAA(front - 3.0f * f, c.y + b * 0.1f, front + length * 0.08f * f, c.y + b * 0.6f + wag * 0.3f, whisker, thick);
		DrawLineAA(front - 3.0f * f, c.y + b * 0.25f, front + length * 0.05f * f, c.y + b * 0.95f + wag * 0.3f, whisker, thick);
	}

	//め
	const float er = std::max(2.0f, std::min(b * 0.28f, length * 0.05f));
	const float ex = front - a * 0.22f * f;
	const float ey = c.y - b * 0.22f;
	DrawCircleAA(ex, ey, er, 12, GetColor(255, 255, 255), TRUE);
	DrawCircleAA(ex + er * 0.25f * f, ey, er * 0.55f, 10, GetColor(20, 20, 30), TRUE);
}
