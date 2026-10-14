#include "Sprite.h"
#include<DxLib.h>
#include<array>
#include<vector>
#include<unordered_map>
#include<algorithm>
#include<cmath>

namespace
{
	constexpr int kCols = 12;
	constexpr int kRows = 11;
	constexpr int kTileNum = kCols * kRows;

	const wchar_t* kPaths[static_cast<int>(SpriteSheet::Num)] =
	{
		L"data/gfx/kenney_tiny_dungeon.png",
		L"data/gfx/kenney_tiny_town.png",
	};

	struct SheetData
	{
		int soft = -1;	//ぬりかえ用の もとの ピクセル
		std::array<int, kTileNum> tiles = {};
	};

	std::array<SheetData, static_cast<int>(SpriteSheet::Num)> gSheets;
	std::unordered_map<unsigned long long, int> gTinted;

	bool IsSkin(int r, int g, int b)
	{
		return r > 180 && g > 120 && b > 80 && r > g && g > b && r - b > 60;
	}

	unsigned long long TintKey(SpriteSheet sheet, int index, const Game::Rgb& c, float strength, bool keepSkin)
	{
		const unsigned long long s = static_cast<unsigned long long>(std::clamp(strength, 0.0f, 1.0f) * 100.0f);
		return (static_cast<unsigned long long>(sheet) << 56) | (static_cast<unsigned long long>(index) << 44) |
			(static_cast<unsigned long long>(c.r & 0xff) << 36) | (static_cast<unsigned long long>(c.g & 0xff) << 28) |
			(static_cast<unsigned long long>(c.b & 0xff) << 20) | (s << 8) | (keepSkin ? 1ull : 0ull);
	}
}

bool Sprite::Init()
{
	for (int i = 0; i < static_cast<int>(SpriteSheet::Num); i++)
	{
		SheetData& sd = gSheets[i];
		if (LoadDivGraph(kPaths[i], kTileNum, kCols, kRows, kTileSize, kTileSize, sd.tiles.data()) == -1) return false;
		sd.soft = LoadSoftImage(kPaths[i]);
		if (sd.soft == -1) return false;
	}
	return true;
}

void Sprite::Terminate()
{
	for (auto& [key, handle] : gTinted) DeleteGraph(handle);
	gTinted.clear();
	for (SheetData& sd : gSheets)
	{
		for (int& h : sd.tiles) { if (h > 0) DeleteGraph(h); h = 0; }
		if (sd.soft != -1) DeleteSoftImage(sd.soft);
		sd.soft = -1;
	}
}

int Sprite::Tile(SpriteSheet sheet, int index)
{
	return gSheets[static_cast<int>(sheet)].tiles[std::clamp(index, 0, kTileNum - 1)];
}

int Sprite::Tinted(SpriteSheet sheet, int index, const Game::Rgb& color, float strength, bool keepSkin)
{
	index = std::clamp(index, 0, kTileNum - 1);
	if (strength <= 0.0f) return Tile(sheet, index);
	const unsigned long long key = TintKey(sheet, index, color, strength, keepSkin);
	const auto found = gTinted.find(key);
	if (found != gTinted.end()) return found->second;

	const SheetData& sd = gSheets[static_cast<int>(sheet)];
	const int sx = (index % kCols) * kTileSize;
	const int sy = (index / kCols) * kTileSize;
	const int img = MakeARGB8ColorSoftImage(kTileSize, kTileSize);
	for (int y = 0; y < kTileSize; y++)
	{
		for (int x = 0; x < kTileSize; x++)
		{
			int r = 0, g = 0, b = 0, a = 0;
			GetPixelSoftImage(sd.soft, sx + x, sy + y, &r, &g, &b, &a);
			const float lum = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;

			//ふちどり(くらい いろ)と はだは そのまま
			if (a > 0 && lum >= 0.22f && !(keepSkin && IsSkin(r, g, b)))
			{
				//あかるさの もようを のこして その いろに する
				const float k = 0.45f + 0.85f * lum;
				const float tr = std::min(255.0f, color.r * k);
				const float tg = std::min(255.0f, color.g * k);
				const float tb = std::min(255.0f, color.b * k);
				r = static_cast<int>(r + (tr - r) * strength);
				g = static_cast<int>(g + (tg - g) * strength);
				b = static_cast<int>(b + (tb - b) * strength);
			}
			DrawPixelSoftImage(img, x, y, r, g, b, a);
		}
	}
	const int handle = CreateGraphFromSoftImage(img);
	DeleteSoftImage(img);
	gTinted[key] = handle;
	return handle;
}

void Sprite::DrawRect(int handle, float x, float y, float w, float h, bool flipX)
{
	const int mode = GetDrawMode();
	SetDrawMode(DX_DRAWMODE_NEAREST);
	if (flipX) DrawExtendGraphF(x + w, y, x, y + h, handle, TRUE);
	else DrawExtendGraphF(x, y, x + w, y + h, handle, TRUE);
	SetDrawMode(mode);
}

void Sprite::DrawFeet(int handle, const Vec2& feet, float scaleX, float scaleY, bool flipX, float angle)
{
	const int mode = GetDrawMode();
	SetDrawMode(DX_DRAWMODE_NEAREST);
	DrawRotaGraph3F(feet.x, feet.y, kTileSize * 0.5f, static_cast<float>(kTileSize), scaleX, scaleY, angle, handle, TRUE, flipX ? TRUE : FALSE);
	SetDrawMode(mode);
}
