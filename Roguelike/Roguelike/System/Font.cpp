#include "Font.h"
#include<DxLib.h>
#include<array>
#include<cwchar>

namespace
{
	constexpr int kSizeTable[static_cast<int>(FontSize::Num)] = { 24, 36, 56, 96, 150 };
	constexpr int kEdgeTable[static_cast<int>(FontSize::Num)] = { 2, 3, 4, 5, 6 };

	std::array<int, static_cast<int>(FontSize::Num)> handles = { -1, -1, -1, -1, -1 };
}

void Font::Init()
{
	for (int i = 0; i < static_cast<int>(FontSize::Num); i++)
	{
		handles[i] = CreateFontToHandle(L"メイリオ", kSizeTable[i], 9, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, kEdgeTable[i]);
	}
}

void Font::Terminate()
{
	for (int& handle : handles)
	{
		DeleteFontToHandle(handle);
		handle = -1;
	}
}

int Font::Get(FontSize size)
{
	return handles[static_cast<int>(size)];
}

int Font::GetPixelSize(FontSize size)
{
	return kSizeTable[static_cast<int>(size)];
}

void Font::DrawCenter(float x, float y, const wchar_t* text, unsigned int color, FontSize size, unsigned int edgeColor)
{
	const int font = Get(size);
	const int width = GetDrawStringWidthToHandle(text, static_cast<int>(std::wcslen(text)), font);
	const int height = GetFontSizeToHandle(font);
	DrawStringToHandle(static_cast<int>(x) - width / 2, static_cast<int>(y) - height / 2, text, color, font, edgeColor);
}

void Font::DrawCenterScaled(float x, float y, float scale, const wchar_t* text, unsigned int color, FontSize size, unsigned int edgeColor)
{
	if (scale <= 0.01f) return;
	const int font = Get(size);
	const int width = GetDrawStringWidthToHandle(text, static_cast<int>(std::wcslen(text)), font);
	const int height = GetFontSizeToHandle(font);
	DrawExtendStringToHandle(static_cast<int>(x - width * scale * 0.5f), static_cast<int>(y - height * scale * 0.5f),
		scale, scale, text, color, font, edgeColor);
}

namespace
{
	//これより小さくは縮めない
	constexpr float kMinScale = 0.72f;

	/// <summary>
	/// 収まる倍率と、必要なら末尾を切った文字
	/// </summary>
	std::wstring FitText(const std::wstring& text, int font, float maxWidth, float& scale)
	{
		const int width = GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.size()), font);
		scale = 1.0f;
		if (width <= maxWidth || width <= 0) return text;
		scale = maxWidth / width;
		if (scale >= kMinScale) return text;

		scale = kMinScale;
		std::wstring cut = text;
		while (cut.size() > 1)
		{
			cut.pop_back();
			const std::wstring t = cut + L"…";
			if (GetDrawStringWidthToHandle(t.c_str(), static_cast<int>(t.size()), font) * kMinScale <= maxWidth) return t;
		}
		return cut + L"…";
	}
}

void Font::DrawFit(float x, float y, const std::wstring& text, unsigned int color, FontSize size, unsigned int edgeColor, float maxWidth)
{
	const int font = Get(size);
	float scale = 1.0f;
	const std::wstring t = FitText(text, font, maxWidth, scale);
	if (scale >= 1.0f)
	{
		DrawStringToHandle(static_cast<int>(x), static_cast<int>(y), t.c_str(), color, font, edgeColor);
		return;
	}
	//縮めた分だけ下げて、行の中央をそろえる
	const float h = static_cast<float>(GetFontSizeToHandle(font));
	DrawExtendStringToHandle(static_cast<int>(x), static_cast<int>(y + h * (1.0f - scale) * 0.5f), scale, scale, t.c_str(), color, font, edgeColor);
}

void Font::DrawCenterFit(float x, float y, const std::wstring& text, unsigned int color, FontSize size, unsigned int edgeColor, float maxWidth)
{
	const int font = Get(size);
	float scale = 1.0f;
	const std::wstring t = FitText(text, font, maxWidth, scale);
	const float w = static_cast<float>(GetDrawStringWidthToHandle(t.c_str(), static_cast<int>(t.size()), font)) * scale;
	const float h = static_cast<float>(GetFontSizeToHandle(font)) * scale;
	DrawExtendStringToHandle(static_cast<int>(x - w * 0.5f), static_cast<int>(y - h * 0.5f), scale, scale, t.c_str(), color, font, edgeColor);
}

std::vector<std::wstring> Font::Wrap(const std::wstring& text, FontSize size, float maxWidth)
{
	const int font = Get(size);
	auto width = [font](const std::wstring& s, size_t len) { return GetDrawStringWidthToHandle(s.c_str(), static_cast<int>(len), font); };

	std::vector<std::wstring> out;
	std::wstring rest = text;
	while (!rest.empty())
	{
		if (width(rest, rest.size()) <= maxWidth)
		{
			out.push_back(rest);
			break;
		}
		size_t cut = 1;
		while (cut < rest.size() && width(rest, cut + 1) <= maxWidth) cut++;
		const size_t space = rest.find_last_of(L" 　・、", cut - 1);
		if (space != std::wstring::npos && space > cut / 2) cut = space + 1;

		//「。」「、」などが 行の あたまに こないように 前の行へ
		while (cut < rest.size() && std::wstring(L"。、，．」』）！？ー…").find(rest[cut]) != std::wstring::npos) cut++;
		out.push_back(rest.substr(0, cut));
		rest = rest.substr(cut);
	}
	return out;
}

int Font::Width(const std::wstring& text, FontSize size)
{
	return GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.size()), Get(size));
}

void Font::DrawCenterAlpha(float x, float y, float scale, int alpha, const wchar_t* text, unsigned int color, FontSize size, unsigned int edgeColor)
{
	if (alpha <= 0) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawCenterScaled(x, y, scale, text, color, size, edgeColor);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
