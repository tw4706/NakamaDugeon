#include "ButtonGuide.h"
#include "SaveData.h"
#include<DxLib.h>
#include<vector>

namespace
{
	enum class Glyph
	{
		Text,
		A,
		B,
		Y,
		Menu,
		Move,
		UpDown,
		LeftRight,
		PageL,
		PageR,
	};

	struct Part
	{
		Glyph glyph;
		std::wstring text;
	};

	std::vector<Part> Split(const std::wstring& text)
	{
		std::vector<Part> parts;
		std::wstring plain;
		size_t i = 0;
		while (i < text.size())
		{
			if (text[i] == L'{')
			{
				const size_t end = text.find(L'}', i);
				if (end != std::wstring::npos)
				{
					const std::wstring key = text.substr(i + 1, end - i - 1);
					Glyph g = Glyph::Text;
					if (key == L"A") g = Glyph::A;
					else if (key == L"B") g = Glyph::B;
					else if (key == L"Y") g = Glyph::Y;
					else if (key == L"M") g = Glyph::Menu;
					else if (key == L"MOVE") g = Glyph::Move;
					else if (key == L"UD") g = Glyph::UpDown;
					else if (key == L"LR") g = Glyph::LeftRight;
					else if (key == L"L") g = Glyph::PageL;
					else if (key == L"R") g = Glyph::PageR;
					if (g != Glyph::Text)
					{
						if (!plain.empty()) parts.push_back({ Glyph::Text, plain });
						plain.clear();
						parts.push_back({ g, L"" });
						i = end + 1;
						continue;
					}
				}
			}
			plain += text[i];
			i++;
		}
		if (!plain.empty()) parts.push_back({ Glyph::Text, plain });
		return parts;
	}

	//キーボードの キーの なまえ
	const wchar_t* KeyName(Glyph g)
	{
		switch (g)
		{
		case Glyph::A: return L"SPACE";
		case Glyph::B: return L"X";
		case Glyph::Y: return L"C";
		case Glyph::Menu: return L"X";
		case Glyph::Move: return L"WASD";
		case Glyph::UpDown: return L"↑↓";
		case Glyph::LeftRight: return L"←→";
		case Glyph::PageL: return L"Q";
		case Glyph::PageR: return L"E";
		default: return L"";
		}
	}

	float FontHeight(FontSize size)
	{
		return static_cast<float>(GetFontSizeToHandle(Font::Get(size)));
	}

	float GlyphWidth(Glyph g, FontSize size, bool isPad)
	{
		const float h = FontHeight(size);
		if (!isPad) return Font::Width(KeyName(g), size) * 0.8f + h * 0.6f;
		if (g == Glyph::Menu) return h * 1.25f;
		if (g == Glyph::PageL || g == Glyph::PageR) return h * 1.7f;
		return h * 1.1f;
	}

	//Xbox の ABXY
	void FaceButton(float cx, float cy, float r, const wchar_t* letter, unsigned int color, FontSize size)
	{
		DrawCircleAA(cx, cy + 1.5f, r, 24, GetColor(10, 10, 14), TRUE);
		DrawCircleAA(cx, cy, r, 24, GetColor(45, 46, 52), TRUE);
		DrawCircleAA(cx, cy, r, 24, color, FALSE, 2.0f);
		const int font = Font::Get(size);
		const int w = GetDrawStringWidthToHandle(letter, 1, font);
		const int h = GetFontSizeToHandle(font);
		DrawExtendStringToHandle(static_cast<int>(cx - w * 0.4f), static_cast<int>(cy - h * 0.4f), 0.8, 0.8, letter, color, font);
	}

	void DrawGlyph(Glyph g, float x, float cy, FontSize size, bool isPad)
	{
		const float h = FontHeight(size);
		const float w = GlyphWidth(g, size, isPad);
		if (!isPad)
		{
			//キーの ふた
			const float x1 = x + 2.0f;
			const float x2 = x + w - 2.0f;
			DrawRoundRectAA(x1, cy - h * 0.5f + 2.5f, x2, cy + h * 0.5f + 2.5f, 6.0f, 6.0f, 8, GetColor(110, 110, 125), TRUE);
			DrawRoundRectAA(x1, cy - h * 0.5f, x2, cy + h * 0.5f, 6.0f, 6.0f, 8, GetColor(235, 235, 242), TRUE);
			const int font = Font::Get(size);
			const wchar_t* name = KeyName(g);
			const float tw = Font::Width(name, size) * 0.8f;
			DrawExtendStringToHandle(static_cast<int>((x1 + x2) * 0.5f - tw * 0.5f), static_cast<int>(cy - h * 0.4f), 0.8, 0.8, name, GetColor(40, 40, 50), font);
			return;
		}

		const float cx = x + w * 0.5f;
		const float r = h * 0.48f;
		switch (g)
		{
		case Glyph::A: FaceButton(cx, cy, r, L"A", GetColor(110, 200, 80), size); break;
		case Glyph::B: FaceButton(cx, cy, r, L"B", GetColor(235, 80, 80), size); break;
		case Glyph::Y: FaceButton(cx, cy, r, L"Y", GetColor(245, 200, 60), size); break;
		case Glyph::Menu:
		{
			DrawCircleAA(cx, cy + 1.5f, r, 24, GetColor(10, 10, 14), TRUE);
			DrawCircleAA(cx, cy, r, 24, GetColor(45, 46, 52), TRUE);
			DrawCircleAA(cx, cy, r, 24, GetColor(200, 200, 210), FALSE, 2.0f);
			for (int k = -1; k <= 1; k++) DrawLineAA(cx - r * 0.45f, cy + k * r * 0.35f, cx + r * 0.45f, cy + k * r * 0.35f, GetColor(230, 230, 240), 2.0f);
			break;
		}
		case Glyph::Move:
		{
			//ひだりスティック
			DrawCircleAA(cx, cy + 1.5f, r, 24, GetColor(10, 10, 14), TRUE);
			DrawCircleAA(cx, cy, r, 24, GetColor(70, 72, 80), TRUE);
			DrawCircleAA(cx, cy, r * 0.7f, 24, GetColor(45, 46, 52), TRUE);
			const int font = Font::Get(size);
			const int lw = GetDrawStringWidthToHandle(L"L", 1, font);
			DrawExtendStringToHandle(static_cast<int>(cx - lw * 0.35f), static_cast<int>(cy - h * 0.35f), 0.7, 0.7, L"L", GetColor(230, 230, 240), font);
			break;
		}
		case Glyph::PageL:
		case Glyph::PageR:
		{
			//かたの ボタン
			const float bw = w * 0.45f;
			DrawRoundRectAA(cx - bw, cy - h * 0.4f + 1.5f, cx + bw, cy + h * 0.4f + 1.5f, 8.0f, 8.0f, 8, GetColor(10, 10, 14), TRUE);
			DrawRoundRectAA(cx - bw, cy - h * 0.4f, cx + bw, cy + h * 0.4f, 8.0f, 8.0f, 8, GetColor(45, 46, 52), TRUE);
			DrawRoundRectAA(cx - bw, cy - h * 0.4f, cx + bw, cy + h * 0.4f, 8.0f, 8.0f, 8, GetColor(200, 200, 210), FALSE, 2.0f);
			const wchar_t* label = g == Glyph::PageL ? L"LB" : L"RB";
			const int font = Font::Get(size);
			const float lw = static_cast<float>(GetDrawStringWidthToHandle(label, 2, font)) * 0.65f;
			DrawExtendStringToHandle(static_cast<int>(cx - lw * 0.5f), static_cast<int>(cy - h * 0.33f), 0.65, 0.65, label, GetColor(230, 230, 240), font);
			break;
		}
		case Glyph::UpDown:
		case Glyph::LeftRight:
		{
			//じゅうじキー(つかう向きを あかるく)
			const float a = r * 0.95f;
			const float t = r * 0.38f;
			const unsigned int dim = GetColor(70, 72, 80);
			const unsigned int lit = GetColor(235, 235, 245);
			DrawBoxAA(cx - t, cy - a, cx + t, cy + a, g == Glyph::UpDown ? lit : dim, TRUE);
			DrawBoxAA(cx - a, cy - t, cx + a, cy + t, g == Glyph::LeftRight ? lit : dim, TRUE);
			DrawBoxAA(cx - t, cy - t, cx + t, cy + t, GetColor(150, 150, 160), TRUE);
			break;
		}
		default: break;
		}
	}
}

bool ButtonGuide::IsPad()
{
	const int style = SaveManager::GetInstance().GetSettings().buttonStyle;
	if (style == 1) return false;
	if (style == 2) return true;
	return Input::GetInstance().IsUsingPad();
}

std::wstring ButtonGuide::Name(Button button)
{
	const bool isPad = IsPad();
	switch (button)
	{
	case Button::Decide: return isPad ? L"Aボタン" : L"SPACE";
	case Button::Cancel: return isPad ? L"Bボタン" : L"X";
	case Button::Turn: return isPad ? L"Yボタン" : L"C";
	case Button::Menu: return isPad ? L"メニューボタン" : L"X";
	default: return L"";
	}
}

float ButtonGuide::Width(const std::wstring& text, FontSize size)
{
	const bool isPad = IsPad();
	float w = 0.0f;
	for (const Part& p : Split(text))
	{
		w += p.glyph == Glyph::Text ? static_cast<float>(Font::Width(p.text, size)) : GlyphWidth(p.glyph, size, isPad);
	}
	return w;
}

void ButtonGuide::Draw(float x, float y, const std::wstring& text, unsigned int color, FontSize size, unsigned int edgeColor)
{
	const bool isPad = IsPad();
	const int font = Font::Get(size);
	const float h = FontHeight(size);
	for (const Part& p : Split(text))
	{
		if (p.glyph == Glyph::Text)
		{
			DrawStringToHandle(static_cast<int>(x), static_cast<int>(y - h * 0.5f), p.text.c_str(), color, font, edgeColor);
			x += static_cast<float>(Font::Width(p.text, size));
			continue;
		}
		DrawGlyph(p.glyph, x, y, size, isPad);
		x += GlyphWidth(p.glyph, size, isPad);
	}
}

void ButtonGuide::DrawCenter(float x, float y, const std::wstring& text, unsigned int color, FontSize size, unsigned int edgeColor)
{
	Draw(x - Width(text, size) * 0.5f, y, text, color, size, edgeColor);
}
