#include "DrawUtil.h"
#include<DxLib.h>
#include<cmath>
#include<algorithm>
#include<cwchar>

using namespace MathUtil;

Game::Rgb DrawUtil::Lerp(const Game::Rgb& a, const Game::Rgb& b, float t)
{
	return
	{
		static_cast<int>(MathUtil::Lerp(static_cast<float>(a.r), static_cast<float>(b.r), t)),
		static_cast<int>(MathUtil::Lerp(static_cast<float>(a.g), static_cast<float>(b.g), t)),
		static_cast<int>(MathUtil::Lerp(static_cast<float>(a.b), static_cast<float>(b.b), t)),
	};
}

Game::Rgb DrawUtil::Scale(const Game::Rgb& c, float rate)
{
	return
	{
		std::clamp(static_cast<int>(c.r * rate), 0, 255),
		std::clamp(static_cast<int>(c.g * rate), 0, 255),
		std::clamp(static_cast<int>(c.b * rate), 0, 255),
	};
}

void DrawUtil::Gradient(int x1, int y1, int x2, int y2, const Game::Rgb& top, const Game::Rgb& bottom)
{
	constexpr int kBand = 6;
	for (int y = y1; y < y2; y += kBand)
	{
		const float t = static_cast<float>(y - y1) / std::max(1, y2 - y1);
		DrawBox(x1, y, x2, std::min(y + kBand, y2), Game::ToColor(Lerp(top, bottom, t)), TRUE);
	}
}

void DrawUtil::Stripes(float time, const Game::Rgb& base, const Game::Rgb& stripe, float speed)
{
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, Game::ToColor(base), TRUE);

	constexpr float kWidth = 70.0f;
	const unsigned int color = Game::ToColor(stripe);
	const float offset = std::fmod(time * speed, kWidth * 2.0f);
	const float h = static_cast<float>(Game::kScreenHeight);
	for (float x = -h - kWidth * 2.0f + offset; x < Game::kScreenWidth + kWidth; x += kWidth * 2.0f)
	{
		DrawQuadrangleAA(x, h, x + kWidth, h, x + kWidth + h, 0.0f, x + h, 0.0f, color, TRUE);
	}
}

void DrawUtil::Panel(float x1, float y1, float x2, float y2, const Game::Rgb& fill, int alpha, const Game::Rgb& frame)
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawRoundRectAA(x1, y1, x2, y2, 18.0f, 18.0f, 8, Game::ToColor(fill), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawRoundRectAA(x1, y1, x2, y2, 18.0f, 18.0f, 8, Game::ToColor(frame), FALSE, 4.0f);
}

void DrawUtil::Star(float x, float y, float radius, float angle, unsigned int color)
{
	const float inner = radius * 0.45f;
	for (int i = 0; i < 5; i++)
	{
		const float a0 = angle + kPi * 2.0f * i / 5.0f - kPi * 0.5f;
		const float a1 = a0 + kPi / 5.0f;
		const float a2 = a0 - kPi / 5.0f;
		const float ox = x + std::cos(a0) * radius;
		const float oy = y + std::sin(a0) * radius;
		DrawTriangleAA(x, y, ox, oy, x + std::cos(a1) * inner, y + std::sin(a1) * inner, color, TRUE);
		DrawTriangleAA(x, y, ox, oy, x + std::cos(a2) * inner, y + std::sin(a2) * inner, color, TRUE);
	}
}

void DrawUtil::Character(const Vec2& pos, float size, int player, Face face, float squash, const Vec2& look, bool hasShadow)
{
	const Game::Rgb base = Game::PlayerRgb(player);
	const float rx = size / std::sqrt(squash);
	const float ry = size * squash;
	const float cx = pos.x;
	const float cy = pos.y - ry;

	//影
	if (hasShadow)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
		DrawOvalAA(cx, pos.y, rx * 0.9f, size * 0.22f, 24, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	//足
	const unsigned int dark = Game::ToColor(Scale(base, 0.55f));
	DrawOvalAA(cx - rx * 0.45f, pos.y - size * 0.08f, size * 0.32f, size * 0.18f, 16, dark, TRUE);
	DrawOvalAA(cx + rx * 0.45f, pos.y - size * 0.08f, size * 0.32f, size * 0.18f, 16, dark, TRUE);

	//体
	DrawOvalAA(cx, cy, rx + 3.0f, ry + 3.0f, 40, dark, TRUE);
	DrawOvalAA(cx, cy, rx, ry, 40, Game::ToColor(base), TRUE);
	DrawOvalAA(cx, cy + ry * 0.35f, rx * 0.62f, ry * 0.5f, 32, Game::ToColor(Lerp(base, { 255, 255, 255 }, 0.55f)), TRUE);

	//ハイライト
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
	DrawOvalAA(cx - rx * 0.4f, cy - ry * 0.5f, rx * 0.22f, ry * 0.14f, 16, GetColor(255, 255, 255), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	//目
	const float eyeY = cy - ry * 0.18f;
	const float eyeX = rx * 0.32f;
	const float eyeR = size * 0.2f;
	const unsigned int black = GetColor(30, 30, 40);
	const unsigned int white = GetColor(255, 255, 255);
	const Vec2 lk = look * (eyeR * 0.4f);

	for (int side = -1; side <= 1; side += 2)
	{
		const float ex = cx + eyeX * side;
		switch (face)
		{
		case Face::Happy:
			DrawLineAA(ex - eyeR * 0.7f, eyeY + eyeR * 0.2f, ex, eyeY - eyeR * 0.5f, black, size * 0.08f);
			DrawLineAA(ex, eyeY - eyeR * 0.5f, ex + eyeR * 0.7f, eyeY + eyeR * 0.2f, black, size * 0.08f);
			break;
		case Face::Dizzy:
			DrawLineAA(ex - eyeR * 0.6f, eyeY - eyeR * 0.6f, ex + eyeR * 0.6f, eyeY + eyeR * 0.6f, black, size * 0.08f);
			DrawLineAA(ex + eyeR * 0.6f, eyeY - eyeR * 0.6f, ex - eyeR * 0.6f, eyeY + eyeR * 0.6f, black, size * 0.08f);
			break;
		case Face::Sad:
			DrawOvalAA(ex, eyeY, eyeR * 0.75f, eyeR, 20, white, TRUE);
			DrawCircleAA(ex + lk.x, eyeY + eyeR * 0.35f, eyeR * 0.45f, 16, black, TRUE);
			DrawLineAA(ex - eyeR * 0.8f * side, eyeY - eyeR * 1.5f, ex + eyeR * 0.8f * side, eyeY - eyeR * 1.0f, black, size * 0.06f);
			break;
		case Face::Surprised:
			DrawOvalAA(ex, eyeY, eyeR * 0.9f, eyeR * 1.15f, 20, white, TRUE);
			DrawCircleAA(ex, eyeY, eyeR * 0.3f, 16, black, TRUE);
			break;
		case Face::Determined:
			DrawOvalAA(ex, eyeY, eyeR * 0.75f, eyeR, 20, white, TRUE);
			DrawCircleAA(ex + lk.x, eyeY + lk.y, eyeR * 0.5f, 16, black, TRUE);
			DrawLineAA(ex - eyeR * 0.9f * side, eyeY - eyeR * 1.0f, ex + eyeR * 0.9f * side, eyeY - eyeR * 1.6f, black, size * 0.07f);
			break;
		default:
			DrawOvalAA(ex, eyeY, eyeR * 0.75f, eyeR, 20, white, TRUE);
			DrawCircleAA(ex + lk.x, eyeY + lk.y, eyeR * 0.5f, 16, black, TRUE);
			DrawCircleAA(ex + lk.x - eyeR * 0.15f, eyeY + lk.y - eyeR * 0.2f, eyeR * 0.16f, 8, white, TRUE);
			break;
		}
	}

	//ほっぺ
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawOvalAA(cx - rx * 0.62f, eyeY + eyeR * 1.2f, eyeR * 0.6f, eyeR * 0.35f, 12, GetColor(255, 120, 150), TRUE);
	DrawOvalAA(cx + rx * 0.62f, eyeY + eyeR * 1.2f, eyeR * 0.6f, eyeR * 0.35f, 12, GetColor(255, 120, 150), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	//口
	const float mouthY = eyeY + eyeR * 1.4f;
	switch (face)
	{
	case Face::Happy:
		DrawOvalAA(cx, mouthY, eyeR * 0.7f, eyeR * 0.55f, 16, GetColor(150, 30, 50), TRUE);
		break;
	case Face::Sad:
		DrawLineAA(cx - eyeR * 0.5f, mouthY + eyeR * 0.2f, cx, mouthY - eyeR * 0.1f, black, size * 0.05f);
		DrawLineAA(cx, mouthY - eyeR * 0.1f, cx + eyeR * 0.5f, mouthY + eyeR * 0.2f, black, size * 0.05f);
		break;
	case Face::Surprised:
	case Face::Dizzy:
		DrawOvalAA(cx, mouthY, eyeR * 0.35f, eyeR * 0.45f, 16, GetColor(150, 30, 50), TRUE);
		break;
	default:
		DrawLineAA(cx - eyeR * 0.4f, mouthY, cx + eyeR * 0.4f, mouthY, black, size * 0.05f);
		break;
	}
}

void DrawUtil::Crown(float x, float y, float size)
{
	const unsigned int gold = GetColor(255, 210, 40);
	const unsigned int dark = GetColor(200, 130, 20);
	const float w = size;
	const float h = size * 0.7f;
	DrawBoxAA(x - w * 0.5f, y - h * 0.3f, x + w * 0.5f, y, gold, TRUE);
	DrawTriangleAA(x - w * 0.5f, y - h * 0.3f, x - w * 0.5f, y - h, x - w * 0.2f, y - h * 0.3f, gold, TRUE);
	DrawTriangleAA(x - w * 0.25f, y - h * 0.3f, x, y - h * 1.1f, x + w * 0.25f, y - h * 0.3f, gold, TRUE);
	DrawTriangleAA(x + w * 0.2f, y - h * 0.3f, x + w * 0.5f, y - h, x + w * 0.5f, y - h * 0.3f, gold, TRUE);
	DrawBoxAA(x - w * 0.5f, y - h * 0.3f, x + w * 0.5f, y, dark, FALSE, 2.0f);
	DrawCircleAA(x, y - h * 0.15f, size * 0.08f, 12, GetColor(230, 40, 60), TRUE);
	DrawCircleAA(x - w * 0.3f, y - h * 0.15f, size * 0.06f, 12, GetColor(60, 120, 230), TRUE);
	DrawCircleAA(x + w * 0.3f, y - h * 0.15f, size * 0.06f, 12, GetColor(60, 200, 90), TRUE);
}

void DrawUtil::Coin(float x, float y, float radius, float spin, bool isBig)
{
	const float sx = std::max(0.15f, std::fabs(std::cos(spin)));
	const unsigned int edge = isBig ? GetColor(200, 80, 200) : GetColor(200, 130, 20);
	const unsigned int face = isBig ? GetColor(255, 150, 255) : GetColor(255, 210, 50);
	DrawOvalAA(x, y, radius * sx + 2.0f, radius + 2.0f, 24, edge, TRUE);
	DrawOvalAA(x, y, radius * sx, radius, 24, face, TRUE);
	if (isBig)
	{
		DrawUtil::Star(x, y, radius * 0.6f, 0.0f, GetColor(255, 255, 200));
	}
	else
	{
		DrawBoxAA(x - radius * 0.12f * sx, y - radius * 0.5f, x + radius * 0.12f * sx, y + radius * 0.5f, edge, TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
	DrawOvalAA(x - radius * 0.35f * sx, y - radius * 0.4f, radius * 0.18f * sx, radius * 0.25f, 12, GetColor(255, 255, 255), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void DrawUtil::Window(float x1, float y1, float x2, float y2)
{
	Panel(x1, y1, x2, y2, { 26, 22, 44 }, 235, { 255, 220, 120 });
}

void DrawUtil::Cursor(float x, float y, float time)
{
	const float bob = std::sin(time * 8.0f) * 4.0f;
	DrawTriangleAA(x + bob, y - 11.0f, x + bob, y + 11.0f, x + 18.0f + bob, y, GetColor(255, 230, 80), TRUE);
}

void DrawUtil::CursorBeside(float centerX, float y, const wchar_t* text, FontSize size, float time)
{
	const int width = GetDrawStringWidthToHandle(text, static_cast<int>(std::wcslen(text)), Font::Get(size));
	Cursor(centerX - width * 0.5f - 34.0f, y, time);
}

void DrawUtil::NewMark(float x, float y, float time, float size)
{
	const float hop = -std::fabs(std::sin(time * 5.0f)) * size * 0.5f;
	const float cy = y + hop;

	//光
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(90 + 60 * std::sin(time * 6.0f)));
	DrawCircleAA(x, cy, size * 1.6f, 24, GetColor(255, 200, 60), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawCircleAA(x, cy, size + 2.0f, 24, GetColor(150, 30, 30), TRUE);
	DrawCircleAA(x, cy, size, 24, GetColor(255, 220, 60), TRUE);

	//「！」
	const unsigned int red = GetColor(220, 40, 40);
	DrawQuadrangleAA(x - size * 0.2f, cy - size * 0.68f, x + size * 0.2f, cy - size * 0.68f,
		x + size * 0.11f, cy + size * 0.2f, x - size * 0.11f, cy + size * 0.2f, red, TRUE);
	DrawCircleAA(x, cy + size * 0.5f, size * 0.16f, 12, red, TRUE);
}
