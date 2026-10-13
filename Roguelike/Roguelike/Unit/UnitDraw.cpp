#include "Unit.h"
#include "DrawUtil.h"
#include<DxLib.h>
#include<cmath>

using namespace MathUtil;

namespace
{
	void Eyes(float cx, float cy, float gap, float r, float facing, bool isHurt)
	{
		const unsigned int ink = GetColor(30, 30, 40);
		for (int side = -1; side <= 1; side += 2)
		{
			const float ex = cx + gap * side;
			if (isHurt)
			{
				DrawLineAA(ex - r, cy - r, ex + r, cy + r, ink, r * 0.5f);
				DrawLineAA(ex + r, cy - r, ex - r, cy + r, ink, r * 0.5f);
				continue;
			}
			DrawOvalAA(ex, cy, r * 0.8f, r, 12, GetColor(255, 255, 255), TRUE);
			DrawCircleAA(ex + facing * r * 0.25f, cy + r * 0.1f, r * 0.5f, 10, ink, TRUE);
		}
	}

	void Shadow(const Vec2& feet, float w)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
		DrawOvalAA(feet.x, feet.y, w, w * 0.3f, 16, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//ほのお(下が太く、上がとがる)
	void Flame(float x, float y, float w, float h, float t)
	{
		const float sway = std::sin(t * 3.0f) * w * 0.3f;
		SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
		DrawTriangleAA(x - w, y, x + w, y, x + sway, y - h, GetColor(255, 110, 30), TRUE);
		DrawCircleAA(x, y, w, 12, GetColor(255, 110, 30), TRUE);
		DrawTriangleAA(x - w * 0.5f, y, x + w * 0.5f, y, x + sway * 0.6f, y - h * 0.6f, GetColor(255, 220, 90), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//こおりの けっしょう
	void Crystal(float x, float y, float w, float h, float lean)
	{
		const float tx = x + lean;
		DrawQuadrangleAA(x - w, y, tx - w * 0.6f, y - h * 0.75f, tx, y - h, tx + w * 0.6f, y - h * 0.75f, GetColor(170, 225, 255), TRUE);
		DrawQuadrangleAA(x - w, y, tx - w * 0.6f, y - h * 0.75f, tx, y - h, x, y, GetColor(225, 245, 255), TRUE);
		DrawLineAA(x + w, y, tx + w * 0.6f, y - h * 0.75f, GetColor(110, 170, 230), 1.0f);
	}

	//いなずま
	void Bolt(float x1, float y1, float x2, float y2, float t, unsigned int color)
	{
		float px = x1;
		float py = y1;
		for (int k = 1; k <= 4; k++)
		{
			const float r = k / 4.0f;
			const float zig = (k == 4) ? 0.0f : ((k % 2) ? 1.0f : -1.0f) * 6.0f * (0.6f + 0.4f * std::sin(t * 9.0f + k));
			const float nx = x1 + (x2 - x1) * r + zig;
			const float ny = y1 + (y2 - y1) * r;
			DrawLineAA(px, py, nx, ny, color, 2.5f);
			px = nx;
			py = ny;
		}
	}

	void Halo(float x, float y, float r, unsigned int color)
	{
		DrawOvalAA(x, y, r, r * 0.3f, 20, color, FALSE, 3.0f);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
		DrawOvalAA(x, y, r * 1.1f, r * 0.4f, 20, color, FALSE, 5.0f);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	void AngelWings(const Vec2& c, float r, float t, unsigned int color)
	{
		const float flap = std::sin(t * 2.0f) * r * 0.15f;
		for (int side = -1; side <= 1; side += 2)
		{
			for (int k = 0; k < 3; k++)
			{
				const float len = r * (1.1f - k * 0.2f);
				DrawOvalAA(c.x + side * (r * 0.7f + len * 0.45f), c.y - r * 0.25f + k * r * 0.2f - flap * (1.0f - k * 0.3f), len * 0.5f, r * 0.16f, 16, color, TRUE);
			}
		}
	}

	void Crown(float x, float y, float w, unsigned int color)
	{
		DrawBoxAA(x - w, y - w * 0.35f, x + w, y, color, TRUE);
		for (int k = -1; k <= 1; k++)
		{
			const float px = x + k * w * 0.75f;
			DrawTriangleAA(px - w * 0.3f, y - w * 0.3f, px + w * 0.3f, y - w * 0.3f, px, y - w * (k == 0 ? 1.1f : 0.9f), color, TRUE);
			DrawCircleAA(px, y - w * (k == 0 ? 1.1f : 0.9f), w * 0.08f, 8, color, TRUE);
		}
		DrawCircleAA(x, y - w * 0.17f, w * 0.15f, 10, GetColor(230, 50, 70), TRUE);
		DrawCircleAA(x - w * 0.6f, y - w * 0.17f, w * 0.1f, 8, GetColor(60, 120, 230), TRUE);
		DrawCircleAA(x + w * 0.6f, y - w * 0.17f, w * 0.1f, 8, GetColor(60, 120, 230), TRUE);
	}

	void Aura(const Vec2& c, float r, float t, unsigned int color)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(70 + 40 * std::sin(t * 1.5f)));
		DrawCircleAA(c.x, c.y, r * 1.35f, 24, color, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		for (int k = 0; k < 4; k++)
		{
			const float a = std::fmod(t * 0.4f + k * 0.25f, 1.0f);
			const float px = c.x + std::sin(k * 2.1f) * r * 0.9f;
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(180 * (1.0f - a)));
			DrawCircleAA(px, c.y + r * 0.5f - a * r * 1.6f, r * 0.12f * (1.0f - a * 0.5f), 10, color, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}
	}

	/// <summary>
	/// 体の うしろに かく かざり(けはい・はね)
	/// </summary>
	void DecorBack(int id, const Vec2& feet, float size, float t)
	{
		const unsigned int snow = GetColor(250, 252, 255);
		switch (id)
		{
		case 30: Aura({ feet.x, feet.y - size * 0.55f }, size * 0.7f, t, GetColor(90, 30, 130)); break;
		case 32: Aura({ feet.x, feet.y - size * 0.8f }, size * 0.55f, t, GetColor(120, 40, 160)); break;
		case 33: Aura({ feet.x, feet.y - size * 0.5f }, size * 0.5f, t, GetColor(60, 20, 90)); break;
		case 43: Aura({ feet.x, feet.y - size * 0.75f }, size * 0.55f, t, GetColor(100, 40, 160)); break;
		case 35: AngelWings({ feet.x, feet.y - size * 0.6f }, size * 0.6f, t, snow); break;
		case 37: AngelWings({ feet.x, feet.y - size * 0.95f }, size * 0.5f, t, snow); break;
		case 44: AngelWings({ feet.x, feet.y - size * 0.95f }, size * 0.66f, t, GetColor(255, 245, 210)); break;
		case 40:
			DrawTriangleAA(feet.x - size * 0.75f, feet.y - size * 0.05f, feet.x + size * 0.75f, feet.y - size * 0.05f, feet.x, feet.y - size * 1.1f, GetColor(180, 30, 50), TRUE);
			break;
		default: break;
		}
	}

	/// <summary>
	/// 追加モンスターの かざり(IDごと)
	/// </summary>
	void Decor(int id, const Vec2& feet, float size, float t, float facing, const Vec2& top, const Vec2& body, float bodyR, const Vec2& head)
	{
		const unsigned int snow = GetColor(250, 252, 255);
		const unsigned int gold = GetColor(250, 210, 70);
		const unsigned int ice = GetColor(200, 240, 255);
		switch (id)
		{
		case 20:	//ユキプルン: ゆきの ぼうし
			DrawOvalAA(top.x, top.y + size * 0.12f, size * 0.38f, size * 0.16f, 20, snow, TRUE);
			DrawCircleAA(top.x + size * 0.05f, top.y - size * 0.08f, size * 0.12f, 12, snow, TRUE);
			for (int k = 0; k < 3; k++)
			{
				const float a = t * 0.5f + k * 2.09f;
				DrawUtil::Star(body.x + std::cos(a) * bodyR * 1.3f, body.y + std::sin(a) * bodyR * 0.5f, size * 0.07f, a, ice);
			}
			break;
		case 21:	//フブキバット: つらら と ふぶき
			for (int side = -1; side <= 1; side += 2)
			{
				for (int k = 0; k < 2; k++)
				{
					const float x = body.x + side * size * (0.55f + k * 0.2f);
					DrawTriangleAA(x - size * 0.05f, body.y + size * 0.1f, x + size * 0.05f, body.y + size * 0.1f, x, body.y + size * (0.35f - k * 0.08f), ice, TRUE);
				}
			}
			for (int k = 0; k < 5; k++)
			{
				const float a = std::fmod(t * 0.3f + k * 0.2f, 1.0f);
				DrawCircleAA(body.x - size * 0.8f + a * size * 1.6f, body.y - size * 0.5f + std::sin(k * 1.7f + a * 6.0f) * size * 0.4f, size * 0.035f, 6, snow, TRUE);
			}
			break;
		case 22:	//コオリゴーレム: かたの けっしょう
			Crystal(body.x - bodyR * 0.85f, body.y - size * 0.3f, size * 0.12f, size * 0.55f, -size * 0.1f);
			Crystal(body.x + bodyR * 0.85f, body.y - size * 0.3f, size * 0.12f, size * 0.55f, size * 0.1f);
			Crystal(top.x - size * 0.1f, top.y + size * 0.05f, size * 0.08f, size * 0.3f, 0.0f);
			break;
		case 23:	//アイスウルフ: せなかの けっしょう
			for (int k = 0; k < 3; k++)
			{
				Crystal(body.x - facing * size * (0.3f - k * 0.22f), body.y - size * 0.25f, size * 0.09f, size * (0.3f + k * 0.08f), -facing * size * 0.06f);
			}
			break;
		case 24:	//ユキノセイ: ゆきの かんむり
			for (int k = -1; k <= 1; k++) Crystal(top.x + k * size * 0.13f, top.y + size * 0.08f, size * 0.05f, size * (k == 0 ? 0.28f : 0.18f), k * size * 0.04f);
			DrawUtil::Star(body.x, body.y + size * 0.15f, size * 0.08f, t, ice);
			break;
		case 25:	//マグマプルン: ひびと あわ
			SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
			DrawLineAA(body.x - bodyR * 0.6f, body.y + size * 0.2f, body.x - bodyR * 0.2f, body.y + size * 0.05f, GetColor(255, 220, 80), 2.5f);
			DrawLineAA(body.x - bodyR * 0.2f, body.y + size * 0.05f, body.x + bodyR * 0.1f, body.y + size * 0.28f, GetColor(255, 220, 80), 2.5f);
			DrawLineAA(body.x + bodyR * 0.4f, body.y - size * 0.15f, body.x + bodyR * 0.7f, body.y + size * 0.05f, GetColor(255, 220, 80), 2.5f);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			for (int k = 0; k < 3; k++)
			{
				const float a = std::fmod(t * 0.35f + k * 0.33f, 1.0f);
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(220 * (1.0f - a)));
				DrawCircleAA(top.x + (k - 1) * size * 0.2f, top.y + size * 0.1f - a * size * 0.5f, size * 0.06f, 8, GetColor(255, 150, 60), TRUE);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			}
			break;
		case 26:	//ヒクイドリ: ほのおの とさか
			Flame(top.x - facing * size * 0.05f, top.y + size * 0.08f, size * 0.09f, size * 0.32f, t);
			Flame(body.x - facing * size * 0.65f, body.y - size * 0.2f, size * 0.09f, size * 0.3f, t + 1.0f);
			break;
		case 27:	//ヨウガンゴーレム: ようがんの ひび
			SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(180 + 60 * std::sin(t)));
			DrawLineAA(body.x - bodyR * 0.5f, body.y - size * 0.3f, body.x - bodyR * 0.1f, body.y, GetColor(255, 160, 40), 3.0f);
			DrawLineAA(body.x - bodyR * 0.1f, body.y, body.x - bodyR * 0.3f, body.y + size * 0.35f, GetColor(255, 160, 40), 3.0f);
			DrawLineAA(body.x + bodyR * 0.2f, body.y - size * 0.1f, body.x + bodyR * 0.55f, body.y + size * 0.2f, GetColor(255, 160, 40), 3.0f);
			DrawLineAA(head.x - size * 0.2f, head.y - size * 0.15f, head.x + size * 0.15f, head.y - size * 0.2f, GetColor(255, 160, 40), 2.5f);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			Flame(top.x + size * 0.15f, top.y + size * 0.05f, size * 0.07f, size * 0.22f, t);
			break;
		case 28:	//カエンオオカミ: ほのおの たてがみ と しっぽ
			for (int k = 0; k < 3; k++) Flame(head.x - facing * size * (0.15f + k * 0.12f), head.y - size * 0.1f + k * size * 0.08f, size * 0.08f, size * 0.3f, t + k);
			Flame(body.x - facing * size * 0.85f, body.y - size * 0.25f, size * 0.1f, size * 0.35f, t + 2.0f);
			break;
		case 29:	//ヒノセイレイ: ほのおの かみ
			Flame(top.x, top.y + size * 0.12f, size * 0.16f, size * 0.4f, t);
			Flame(top.x - size * 0.15f, top.y + size * 0.18f, size * 0.08f, size * 0.25f, t + 1.0f);
			Flame(top.x + size * 0.15f, top.y + size * 0.18f, size * 0.08f, size * 0.25f, t + 2.0f);
			break;
		case 30:	//ヤミプルン: つの と やみの けはい
			for (int side = -1; side <= 1; side += 2)
			{
				DrawTriangleAA(body.x + side * bodyR * 0.35f, top.y + size * 0.35f, body.x + side * bodyR * 0.65f, top.y + size * 0.42f, body.x + side * bodyR * 0.7f, top.y - size * 0.05f, GetColor(40, 20, 50), TRUE);
			}
			break;
		case 31:	//シニガミバット: おおがま
		{
			const float sx = body.x + facing * size * 0.5f;
			DrawLineAA(sx, body.y + size * 0.5f, sx - facing * size * 0.1f, body.y - size * 0.7f, GetColor(110, 80, 60), size * 0.06f);
			DrawQuadrangleAA(sx - facing * size * 0.1f, body.y - size * 0.7f, sx - facing * size * 0.65f, body.y - size * 0.6f, sx - facing * size * 0.75f, body.y - size * 0.4f, sx - facing * size * 0.1f, body.y - size * 0.6f, GetColor(200, 205, 215), TRUE);
			DrawOvalAA(top.x, top.y + size * 0.15f, size * 0.3f, size * 0.12f, 16, GetColor(30, 20, 40), TRUE);
			break;
		}
		case 32:	//ノロイノヨロイ: のろいの けはい
			break;
		case 33:	//ヤミオオカミ: とげの たてがみ と あかい め
			for (int k = 0; k < 4; k++)
			{
				const float x = body.x - facing * size * (0.35f - k * 0.2f);
				DrawTriangleAA(x - size * 0.08f, body.y - size * 0.25f, x + size * 0.08f, body.y - size * 0.25f, x - facing * size * 0.06f, body.y - size * 0.55f, GetColor(40, 25, 60), TRUE);
			}
			SetDrawBlendMode(DX_BLENDMODE_ADD, 160);
			DrawCircleAA(head.x + facing * size * 0.1f, head.y - size * 0.02f, size * 0.14f, 12, GetColor(255, 40, 60), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			break;
		case 34:	//ボウレイ: くさり と ランタン
		{
			for (int k = 0; k < 5; k++)
			{
				DrawCircleAA(body.x - facing * size * (0.3f + k * 0.1f), body.y + size * (0.3f + k * 0.06f), size * 0.04f, 8, GetColor(150, 150, 160), FALSE, 2.0f);
			}
			const float lx = body.x + facing * size * 0.6f;
			const float ly = body.y + std::sin(t) * size * 0.05f;
			DrawLineAA(lx, ly - size * 0.25f, lx, ly - size * 0.1f, GetColor(80, 70, 60), 2.0f);
			DrawBoxAA(lx - size * 0.1f, ly - size * 0.1f, lx + size * 0.1f, ly + size * 0.15f, GetColor(80, 70, 60), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
			DrawCircleAA(lx, ly + size * 0.03f, size * 0.07f, 10, GetColor(120, 255, 180), TRUE);
			DrawCircleAA(lx, ly + size * 0.03f, size * 0.2f, 12, GetColor(40, 120, 80), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			break;
		}
		case 35:	//ソラプルン: てんしの はね と わっか
			Halo(top.x, top.y - size * 0.1f, size * 0.22f, gold);
			break;
		case 36:	//カミナリバット: いなずま
			Bolt(body.x - size * 0.6f, body.y - size * 0.6f, body.x - size * 0.4f, body.y + size * 0.1f, t, GetColor(255, 240, 120));
			Bolt(body.x + size * 0.6f, body.y - size * 0.6f, body.x + size * 0.4f, body.y + size * 0.1f, t + 1.0f, GetColor(255, 240, 120));
			DrawTriangleAA(top.x - size * 0.06f, top.y + size * 0.05f, top.x + size * 0.08f, top.y + size * 0.05f, top.x - size * 0.02f, top.y - size * 0.25f, GetColor(255, 230, 80), TRUE);
			break;
		case 37:	//テンシノヨロイ: はね と わっか
			Halo(top.x, top.y + size * 0.05f, size * 0.25f, gold);
			break;
		case 38:	//ライジュウ: いなずまの しっぽ
			Bolt(body.x - facing * size * 0.55f, body.y, body.x - facing * size * 0.95f, body.y - size * 0.6f, t, GetColor(255, 240, 120));
			Bolt(head.x, head.y - size * 0.3f, head.x + facing * size * 0.1f, head.y - size * 0.6f, t + 2.0f, GetColor(255, 255, 200));
			if (std::fmod(t, 3.0f) < 0.4f)
			{
				SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
				DrawCircleAA(body.x, body.y, bodyR * 1.2f, 20, GetColor(255, 240, 120), TRUE);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			}
			break;
		case 39:	//ホシノセイレイ: ほしの かざり
			DrawUtil::Star(top.x, top.y - size * 0.05f, size * 0.14f, 0.0f, gold);
			for (int k = 0; k < 3; k++)
			{
				const float a = t * 0.6f + k * 2.09f;
				DrawUtil::Star(body.x + std::cos(a) * size * 0.6f, body.y + std::sin(a) * size * 0.25f, size * 0.06f, a, GetColor(255, 240, 180));
			}
			break;
		case 40:	//キングプルン: おうかん と マント
			Crown(top.x, top.y + size * 0.22f, size * 0.34f, gold);
			break;
		case 41:	//ヒョウガドラゴン: こおりの とげ と いき
			for (int k = 0; k < 3; k++) Crystal(body.x - facing * size * (0.25f - k * 0.2f), body.y - size * 0.35f, size * 0.08f, size * (0.3f + k * 0.07f), -facing * size * 0.05f);
			Crystal(top.x, top.y + size * 0.1f, size * 0.06f, size * 0.25f, -facing * size * 0.05f);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(100 + 60 * std::sin(t)));
			DrawOvalAA(head.x + facing * size * 0.45f, head.y + size * 0.05f, size * 0.2f, size * 0.1f, 16, ice, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			break;
		case 42:	//ヨウガンマジン: もえる かた
			Flame(body.x - bodyR * 0.85f, body.y - size * 0.45f, size * 0.1f, size * 0.35f, t);
			Flame(body.x + bodyR * 0.85f, body.y - size * 0.45f, size * 0.1f, size * 0.35f, t + 1.5f);
			SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(160 + 60 * std::sin(t)));
			DrawLineAA(body.x - size * 0.2f, body.y - size * 0.3f, body.x + size * 0.05f, body.y - size * 0.05f, GetColor(255, 160, 40), 3.0f);
			DrawLineAA(body.x + size * 0.05f, body.y - size * 0.05f, body.x - size * 0.05f, body.y + size * 0.25f, GetColor(255, 160, 40), 3.0f);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			break;
		case 43:	//ヤミノシンカン: やみの けはい
			break;
		case 44:	//テンクウノオウ: きんの わっか と はね
			Halo(top.x, top.y - size * 0.05f, size * 0.25f, gold);
			Crown(head.x, head.y - size * 0.22f, size * 0.15f, gold);
			break;
		default:
			break;
		}
	}
}

void UnitDraw::Hero(const Vec2& feet, float size, float time, float facing, bool isHurt)
{
	Shadow(feet, size * 0.7f);
	const float bob = std::fabs(std::sin(time * 6.0f)) * size * 0.05f;
	const float cy = feet.y - size * 0.75f - bob;

	//マント
	DrawTriangleAA(feet.x - size * 0.55f, feet.y - size * 0.1f, feet.x + size * 0.55f, feet.y - size * 0.1f, feet.x, cy - size * 0.3f, GetColor(200, 40, 60), TRUE);
	//体
	DrawOvalAA(feet.x, cy + size * 0.2f, size * 0.42f, size * 0.45f, 24, GetColor(60, 100, 200), TRUE);
	//顔
	DrawCircleAA(feet.x, cy - size * 0.25f, size * 0.38f, 24, GetColor(255, 220, 190), TRUE);
	//髪
	DrawOvalAA(feet.x - facing * size * 0.05f, cy - size * 0.5f, size * 0.42f, size * 0.22f, 20, GetColor(90, 60, 40), TRUE);
	Eyes(feet.x + facing * size * 0.05f, cy - size * 0.22f, size * 0.14f, size * 0.07f, facing, isHurt);
	//剣
	const float sx = feet.x + facing * size * 0.5f;
	DrawLineAA(sx, cy + size * 0.3f, sx + facing * size * 0.25f, cy - size * 0.4f, GetColor(220, 230, 240), size * 0.1f);
	DrawLineAA(sx - size * 0.12f, cy + size * 0.18f, sx + size * 0.12f, cy + size * 0.22f, GetColor(200, 160, 40), size * 0.08f);
}

void UnitDraw::Monster(const Vec2& feet, float size, const MonsterData& data, float time, float facing, bool isHurt)
{
	const Game::Rgb base = data.color;
	const unsigned int col = Game::ToColor(base);
	const unsigned int dark = Game::ToColor(DrawUtil::Scale(base, 0.6f));
	const unsigned int light = Game::ToColor(DrawUtil::Lerp(base, { 255, 255, 255 }, 0.5f));
	const float t = time * 5.0f + data.id;

	//かざりを付ける位置(頭の上・体の中心・体の大きさ・頭)
	Vec2 top = feet;
	Vec2 body = feet;
	float bodyR = size * 0.5f;
	Vec2 head = feet;

	//ずかんの かげ(まだ みていない)には かざりを つけない
	const bool isSilhouette = base.r < 40 && base.g < 40 && base.b < 40;
	if (!isSilhouette) DecorBack(data.id, feet, size, t);

	switch (data.shape)
	{
	case MonsterShape::Slime:
	{
		Shadow(feet, size * 0.75f);
		const float sq = 1.0f + std::sin(t) * 0.08f;
		const float rx = size * 0.7f / sq;
		const float ry = size * 0.55f * sq;
		const float cy = feet.y - ry;
		DrawOvalAA(feet.x, cy, rx + 2.0f, ry + 2.0f, 24, dark, TRUE);
		DrawOvalAA(feet.x, cy, rx, ry, 24, col, TRUE);
		DrawTriangleAA(feet.x - size * 0.15f, cy - ry * 0.85f, feet.x + size * 0.15f, cy - ry * 0.85f, feet.x, cy - ry * 1.35f, col, TRUE);
		DrawOvalAA(feet.x - rx * 0.4f, cy - ry * 0.4f, rx * 0.2f, ry * 0.15f, 12, light, TRUE);
		Eyes(feet.x + facing * size * 0.08f, cy, size * 0.2f, size * 0.1f, facing, isHurt);
		top = { feet.x, cy - ry * 1.3f };
		body = { feet.x, cy };
		bodyR = rx;
		head = body;
		break;
	}
	case MonsterShape::Bat:
	{
		const float fly = std::sin(t * 1.5f) * size * 0.12f;
		Shadow(feet, size * 0.5f);
		const float cy = feet.y - size * 0.9f + fly;
		const float flap = std::sin(t * 4.0f) * size * 0.25f;
		DrawTriangleAA(feet.x - size * 0.2f, cy, feet.x - size * 0.95f, cy - size * 0.3f + flap, feet.x - size * 0.6f, cy + size * 0.25f, dark, TRUE);
		DrawTriangleAA(feet.x + size * 0.2f, cy, feet.x + size * 0.95f, cy - size * 0.3f + flap, feet.x + size * 0.6f, cy + size * 0.25f, dark, TRUE);
		DrawCircleAA(feet.x, cy, size * 0.35f, 20, col, TRUE);
		DrawTriangleAA(feet.x - size * 0.3f, cy - size * 0.2f, feet.x - size * 0.15f, cy - size * 0.5f, feet.x - size * 0.05f, cy - size * 0.25f, col, TRUE);
		DrawTriangleAA(feet.x + size * 0.3f, cy - size * 0.2f, feet.x + size * 0.15f, cy - size * 0.5f, feet.x + size * 0.05f, cy - size * 0.25f, col, TRUE);
		Eyes(feet.x, cy, size * 0.13f, size * 0.08f, facing, isHurt);
		top = { feet.x, cy - size * 0.45f };
		body = { feet.x, cy };
		bodyR = size * 0.35f;
		head = body;
		break;
	}
	case MonsterShape::Golem:
	{
		Shadow(feet, size * 0.8f);
		const float cy = feet.y - size * 0.6f;
		DrawBoxAA(feet.x - size * 0.6f, cy - size * 0.45f, feet.x + size * 0.6f, cy + size * 0.55f, dark, TRUE);
		DrawBoxAA(feet.x - size * 0.55f, cy - size * 0.4f, feet.x + size * 0.55f, cy + size * 0.5f, col, TRUE);
		DrawBoxAA(feet.x - size * 0.35f, cy - size * 0.85f, feet.x + size * 0.35f, cy - size * 0.35f, col, TRUE);
		DrawBoxAA(feet.x - size * 0.85f, cy - size * 0.3f, feet.x - size * 0.6f, cy + size * 0.3f, dark, TRUE);
		DrawBoxAA(feet.x + size * 0.6f, cy - size * 0.3f, feet.x + size * 0.85f, cy + size * 0.3f, dark, TRUE);
		DrawLineAA(feet.x - size * 0.3f, cy, feet.x + size * 0.1f, cy + size * 0.3f, dark, 2.0f);
		Eyes(feet.x + facing * size * 0.05f, cy - size * 0.6f, size * 0.15f, size * 0.08f, facing, isHurt);
		top = { feet.x, cy - size * 0.85f };
		body = { feet.x, cy };
		bodyR = size * 0.6f;
		head = { feet.x, cy - size * 0.6f };
		break;
	}
	case MonsterShape::Ghost:
	{
		const float fly = std::sin(t) * size * 0.1f;
		Shadow(feet, size * 0.45f);
		const float cy = feet.y - size * 0.8f + fly;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
		DrawCircleAA(feet.x, cy, size * 0.5f, 24, col, TRUE);
		DrawBoxAA(feet.x - size * 0.5f, cy, feet.x + size * 0.5f, cy + size * 0.45f, col, TRUE);
		for (int k = 0; k < 3; k++)
		{
			const float wx = feet.x - size * 0.5f + size * (k + 0.5f) / 3.0f;
			DrawTriangleAA(wx - size * 0.17f, cy + size * 0.44f, wx + size * 0.17f, cy + size * 0.44f, wx + std::sin(t + k) * size * 0.08f, cy + size * 0.7f, col, TRUE);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 70);
		DrawCircleAA(feet.x, cy, size * 0.65f, 24, col, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		Eyes(feet.x + facing * size * 0.06f, cy - size * 0.05f, size * 0.17f, size * 0.09f, facing, isHurt);
		top = { feet.x, cy - size * 0.5f };
		body = { feet.x, cy };
		bodyR = size * 0.5f;
		head = body;
		break;
	}
	case MonsterShape::Beast:
	{
		Shadow(feet, size * 0.75f);
		const float step = std::sin(t * 1.5f) * size * 0.05f;
		const float cy = feet.y - size * 0.5f;
		for (int k = -1; k <= 1; k += 2)
		{
			DrawBoxAA(feet.x + k * size * 0.35f - size * 0.08f, cy + size * 0.1f, feet.x + k * size * 0.35f + size * 0.08f, feet.y - (k > 0 ? step : -step), dark, TRUE);
		}
		DrawOvalAA(feet.x, cy, size * 0.6f, size * 0.35f, 24, col, TRUE);
		const float hx = feet.x + facing * size * 0.5f;
		DrawCircleAA(hx, cy - size * 0.3f, size * 0.32f, 20, col, TRUE);
		DrawTriangleAA(hx - size * 0.25f, cy - size * 0.45f, hx - size * 0.1f, cy - size * 0.8f, hx, cy - size * 0.5f, dark, TRUE);
		DrawTriangleAA(hx + size * 0.25f, cy - size * 0.45f, hx + size * 0.1f, cy - size * 0.8f, hx, cy - size * 0.5f, dark, TRUE);
		DrawLineAA(feet.x - facing * size * 0.55f, cy, feet.x - facing * size * 0.85f, cy - size * 0.3f + step * 3.0f, dark, size * 0.1f);
		Eyes(hx + facing * size * 0.05f, cy - size * 0.32f, size * 0.12f, size * 0.07f, facing, isHurt);
		top = { hx, cy - size * 0.62f };
		body = { feet.x, cy };
		bodyR = size * 0.6f;
		head = { hx, cy - size * 0.3f };
		break;
	}
	case MonsterShape::Dragon:
	{
		Shadow(feet, size * 0.9f);
		const float cy = feet.y - size * 0.6f;
		const float flap = std::sin(t * 2.0f) * size * 0.15f;
		DrawTriangleAA(feet.x - size * 0.2f, cy - size * 0.2f, feet.x - size * 1.0f, cy - size * 0.8f + flap, feet.x - size * 0.6f, cy + size * 0.1f, dark, TRUE);
		DrawTriangleAA(feet.x + size * 0.2f, cy - size * 0.2f, feet.x + size * 1.0f, cy - size * 0.8f + flap, feet.x + size * 0.6f, cy + size * 0.1f, dark, TRUE);
		DrawOvalAA(feet.x, cy, size * 0.55f, size * 0.5f, 24, col, TRUE);
		DrawOvalAA(feet.x, cy + size * 0.15f, size * 0.32f, size * 0.3f, 20, light, TRUE);
		const float hx = feet.x + facing * size * 0.35f;
		DrawCircleAA(hx, cy - size * 0.55f, size * 0.33f, 20, col, TRUE);
		DrawTriangleAA(hx - size * 0.2f, cy - size * 0.75f, hx - size * 0.1f, cy - size * 1.05f, hx, cy - size * 0.8f, light, TRUE);
		DrawTriangleAA(hx + size * 0.2f, cy - size * 0.75f, hx + size * 0.1f, cy - size * 1.05f, hx, cy - size * 0.8f, light, TRUE);
		Eyes(hx + facing * size * 0.06f, cy - size * 0.58f, size * 0.12f, size * 0.07f, facing, isHurt);
		top = { hx, cy - size * 0.95f };
		body = { feet.x, cy };
		bodyR = size * 0.55f;
		head = { hx, cy - size * 0.55f };
		break;
	}
	case MonsterShape::Bird:
	{
		Shadow(feet, size * 0.6f);
		const float hop = std::fabs(std::sin(t * 1.2f)) * size * 0.06f;
		const float cy = feet.y - size * 0.8f - hop;
		const unsigned int leg = GetColor(230, 170, 60);
		DrawLineAA(feet.x - size * 0.12f, cy + size * 0.2f, feet.x - size * 0.15f, feet.y, leg, size * 0.06f);
		DrawLineAA(feet.x + size * 0.12f, cy + size * 0.2f, feet.x + size * 0.15f, feet.y, leg, size * 0.06f);
		//しっぽ
		DrawTriangleAA(feet.x - facing * size * 0.3f, cy - size * 0.05f, feet.x - facing * size * 0.3f, cy + size * 0.2f, feet.x - facing * size * 0.85f, cy - size * 0.35f, dark, TRUE);
		DrawOvalAA(feet.x, cy, size * 0.45f, size * 0.36f, 24, col, TRUE);
		DrawOvalAA(feet.x + facing * size * 0.1f, cy + size * 0.1f, size * 0.25f, size * 0.2f, 16, light, TRUE);
		//つばさ
		const float flap = std::sin(t * 2.0f) * size * 0.05f;
		DrawOvalAA(feet.x - facing * size * 0.1f, cy - size * 0.02f + flap, size * 0.28f, size * 0.18f, 16, dark, TRUE);
		const float hx = feet.x + facing * size * 0.32f;
		const float hy = cy - size * 0.42f;
		DrawCircleAA(hx, hy, size * 0.24f, 20, col, TRUE);
		DrawTriangleAA(hx + facing * size * 0.18f, hy - size * 0.06f, hx + facing * size * 0.18f, hy + size * 0.08f, hx + facing * size * 0.45f, hy + size * 0.02f, GetColor(250, 200, 70), TRUE);
		Eyes(hx + facing * size * 0.06f, hy - size * 0.03f, size * 0.09f, size * 0.065f, facing, isHurt);
		top = { hx, hy - size * 0.24f };
		body = { feet.x, cy };
		bodyR = size * 0.45f;
		head = { hx, hy };
		break;
	}
	case MonsterShape::Knight:
	{
		Shadow(feet, size * 0.7f);
		const float step = std::sin(t * 1.2f) * size * 0.03f;
		const float cy = feet.y - size * 0.75f;
		const unsigned int metal = col;
		//あし
		DrawBoxAA(feet.x - size * 0.3f, cy + size * 0.3f, feet.x - size * 0.08f, feet.y - step, dark, TRUE);
		DrawBoxAA(feet.x + size * 0.08f, cy + size * 0.3f, feet.x + size * 0.3f, feet.y + step, dark, TRUE);
		//どう
		DrawQuadrangleAA(feet.x - size * 0.45f, cy - size * 0.3f, feet.x + size * 0.45f, cy - size * 0.3f, feet.x + size * 0.35f, cy + size * 0.35f, feet.x - size * 0.35f, cy + size * 0.35f, metal, TRUE);
		DrawLineAA(feet.x, cy - size * 0.25f, feet.x, cy + size * 0.3f, dark, 2.0f);
		DrawLineAA(feet.x - size * 0.3f, cy, feet.x + size * 0.3f, cy, light, 2.0f);
		//かた
		DrawCircleAA(feet.x - size * 0.45f, cy - size * 0.25f, size * 0.18f, 16, light, TRUE);
		DrawCircleAA(feet.x + size * 0.45f, cy - size * 0.25f, size * 0.18f, 16, light, TRUE);
		//かぶと
		const float hy = cy - size * 0.55f;
		DrawCircleAA(feet.x, hy, size * 0.28f, 20, metal, TRUE);
		DrawBoxAA(feet.x - size * 0.28f, hy, feet.x + size * 0.28f, hy + size * 0.18f, metal, TRUE);
		DrawBoxAA(feet.x - size * 0.22f, hy - size * 0.02f, feet.x + size * 0.22f, hy + size * 0.06f, GetColor(20, 20, 30), TRUE);
		DrawTriangleAA(feet.x - size * 0.05f, hy - size * 0.26f, feet.x + size * 0.05f, hy - size * 0.26f, feet.x - facing * size * 0.15f, hy - size * 0.55f, GetColor(200, 50, 60), TRUE);
		if (isHurt) Eyes(feet.x, hy + size * 0.02f, size * 0.1f, size * 0.05f, facing, true);
		else
		{
			SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
			DrawCircleAA(feet.x - size * 0.09f + facing * size * 0.03f, hy + size * 0.02f, size * 0.04f, 8, light, TRUE);
			DrawCircleAA(feet.x + size * 0.09f + facing * size * 0.03f, hy + size * 0.02f, size * 0.04f, 8, light, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}
		//けん と たて
		const float sx = feet.x + facing * size * 0.55f;
		DrawLineAA(sx, cy + size * 0.2f, sx, cy - size * 0.75f, GetColor(220, 225, 235), size * 0.08f);
		DrawLineAA(sx - size * 0.12f, cy + size * 0.1f, sx + size * 0.12f, cy + size * 0.1f, GetColor(200, 160, 40), size * 0.06f);
		const float shx = feet.x - facing * size * 0.5f;
		DrawOvalAA(shx, cy + size * 0.05f, size * 0.2f, size * 0.3f, 16, dark, TRUE);
		DrawOvalAA(shx, cy + size * 0.05f, size * 0.14f, size * 0.23f, 16, light, TRUE);
		top = { feet.x, hy - size * 0.3f };
		body = { feet.x, cy };
		bodyR = size * 0.5f;
		head = { feet.x, hy };
		break;
	}
	case MonsterShape::Spirit:
	{
		const float fly = std::sin(t * 0.8f) * size * 0.1f;
		Shadow(feet, size * 0.4f);
		const float cy = feet.y - size * 0.85f + fly;
		//はね
		const float flap = std::sin(t * 3.0f) * 0.25f + 0.75f;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
		for (int side = -1; side <= 1; side += 2)
		{
			DrawOvalAA(feet.x + side * size * 0.38f * flap, cy - size * 0.2f, size * 0.3f * flap, size * 0.18f, 16, light, TRUE);
			DrawOvalAA(feet.x + side * size * 0.3f * flap, cy + size * 0.08f, size * 0.22f * flap, size * 0.12f, 16, light, TRUE);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		//ひかり
		SetDrawBlendMode(DX_BLENDMODE_ADD, 60);
		DrawCircleAA(feet.x, cy, size * 0.6f, 24, col, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		//ドレス
		DrawTriangleAA(feet.x - size * 0.3f, cy + size * 0.4f, feet.x + size * 0.3f, cy + size * 0.4f, feet.x, cy - size * 0.2f, col, TRUE);
		DrawTriangleAA(feet.x - size * 0.15f, cy + size * 0.4f, feet.x + size * 0.15f, cy + size * 0.4f, feet.x, cy, light, TRUE);
		const float hy = cy - size * 0.32f;
		DrawCircleAA(feet.x, hy, size * 0.22f, 20, GetColor(255, 235, 215), TRUE);
		DrawOvalAA(feet.x, hy - size * 0.12f, size * 0.25f, size * 0.13f, 16, col, TRUE);
		DrawCircleAA(feet.x - facing * size * 0.2f, hy + size * 0.02f, size * 0.1f, 12, col, TRUE);
		Eyes(feet.x + facing * size * 0.04f, hy + size * 0.02f, size * 0.08f, size * 0.055f, facing, isHurt);
		top = { feet.x, hy - size * 0.22f };
		body = { feet.x, cy };
		bodyR = size * 0.35f;
		head = { feet.x, hy };
		break;
	}
	case MonsterShape::Giant:
	{
		Shadow(feet, size * 0.9f);
		const float breath = std::sin(t * 0.8f) * size * 0.03f;
		const float cy = feet.y - size * 0.7f;
		//あし
		DrawBoxAA(feet.x - size * 0.4f, cy + size * 0.3f, feet.x - size * 0.12f, feet.y, dark, TRUE);
		DrawBoxAA(feet.x + size * 0.12f, cy + size * 0.3f, feet.x + size * 0.4f, feet.y, dark, TRUE);
		//うで
		for (int side = -1; side <= 1; side += 2)
		{
			DrawLineAA(feet.x + side * size * 0.55f, cy - size * 0.35f, feet.x + side * size * 0.8f, cy + size * 0.15f + breath, dark, size * 0.22f);
			DrawCircleAA(feet.x + side * size * 0.82f, cy + size * 0.25f + breath, size * 0.17f, 16, col, TRUE);
		}
		//からだ
		DrawQuadrangleAA(feet.x - size * 0.7f, cy - size * 0.5f + breath, feet.x + size * 0.7f, cy - size * 0.5f + breath, feet.x + size * 0.42f, cy + size * 0.4f, feet.x - size * 0.42f, cy + size * 0.4f, col, TRUE);
		DrawOvalAA(feet.x, cy + size * 0.05f, size * 0.3f, size * 0.25f, 20, light, TRUE);
		//あたま と つの
		const float hy = cy - size * 0.68f + breath;
		DrawCircleAA(feet.x, hy, size * 0.25f, 20, col, TRUE);
		for (int side = -1; side <= 1; side += 2)
		{
			DrawTriangleAA(feet.x + side * size * 0.12f, hy - size * 0.15f, feet.x + side * size * 0.25f, hy - size * 0.08f, feet.x + side * size * 0.38f, hy - size * 0.5f, GetColor(240, 230, 200), TRUE);
		}
		if (isHurt) Eyes(feet.x, hy, size * 0.1f, size * 0.06f, facing, true);
		else
		{
			SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
			DrawOvalAA(feet.x - size * 0.09f + facing * size * 0.03f, hy, size * 0.06f, size * 0.035f, 8, GetColor(255, 220, 80), TRUE);
			DrawOvalAA(feet.x + size * 0.09f + facing * size * 0.03f, hy, size * 0.06f, size * 0.035f, 8, GetColor(255, 220, 80), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}
		DrawLineAA(feet.x - size * 0.08f, hy + size * 0.13f, feet.x + size * 0.08f, hy + size * 0.13f, GetColor(30, 20, 20), 2.0f);
		top = { feet.x, hy - size * 0.45f };
		body = { feet.x, cy };
		bodyR = size * 0.7f;
		head = { feet.x, hy };
		break;
	}
	case MonsterShape::Priest:
	{
		const float fly = std::sin(t * 0.7f) * size * 0.06f;
		Shadow(feet, size * 0.6f);
		const float cy = feet.y - size * 0.7f + fly;
		//ローブ
		DrawQuadrangleAA(feet.x - size * 0.25f, cy - size * 0.35f, feet.x + size * 0.25f, cy - size * 0.35f, feet.x + size * 0.55f, feet.y - size * 0.05f + fly, feet.x - size * 0.55f, feet.y - size * 0.05f + fly, col, TRUE);
		DrawQuadrangleAA(feet.x - size * 0.08f, cy - size * 0.3f, feet.x + size * 0.08f, cy - size * 0.3f, feet.x + size * 0.14f, feet.y - size * 0.05f + fly, feet.x - size * 0.14f, feet.y - size * 0.05f + fly, GetColor(220, 190, 90), TRUE);
		for (int k = 0; k < 4; k++)
		{
			const float bx = feet.x - size * 0.45f + k * size * 0.3f;
			DrawTriangleAA(bx, feet.y - size * 0.06f + fly, bx + size * 0.3f, feet.y - size * 0.06f + fly, bx + size * 0.15f, feet.y + size * 0.04f + fly, col, TRUE);
		}
		//フード
		const float hy = cy - size * 0.5f;
		DrawCircleAA(feet.x, hy, size * 0.3f, 20, dark, TRUE);
		DrawTriangleAA(feet.x - size * 0.3f, hy, feet.x + size * 0.3f, hy, feet.x - facing * size * 0.1f, hy - size * 0.55f, dark, TRUE);
		DrawCircleAA(feet.x + facing * size * 0.04f, hy + size * 0.04f, size * 0.2f, 20, GetColor(15, 10, 20), TRUE);
		if (isHurt) Eyes(feet.x + facing * size * 0.04f, hy + size * 0.04f, size * 0.08f, size * 0.05f, facing, true);
		else
		{
			SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
			DrawCircleAA(feet.x - size * 0.06f + facing * size * 0.06f, hy + size * 0.04f, size * 0.04f, 8, light, TRUE);
			DrawCircleAA(feet.x + size * 0.08f + facing * size * 0.06f, hy + size * 0.04f, size * 0.04f, 8, light, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}
		//つえ
		const float sx = feet.x + facing * size * 0.6f;
		DrawLineAA(sx, feet.y - size * 0.02f, sx, cy - size * 0.85f, GetColor(120, 90, 60), size * 0.07f);
		DrawCircleAA(sx, cy - size * 0.95f, size * 0.12f, 16, GetColor(200, 170, 70), FALSE, 3.0f);
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(160 + 80 * std::sin(t)));
		DrawCircleAA(sx, cy - size * 0.95f, size * 0.12f, 16, light, TRUE);
		DrawCircleAA(sx, cy - size * 0.95f, size * 0.22f, 16, col, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		top = { feet.x - facing * size * 0.1f, hy - size * 0.55f };
		body = { feet.x, cy };
		bodyR = size * 0.5f;
		head = { feet.x, hy };
		break;
	}
	}

	if (!isSilhouette) Decor(data.id, feet, size, t, facing, top, body, bodyR, head);
}
