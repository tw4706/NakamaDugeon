#pragma once
#include"Game.h"
#include"MathUtil.h"

/// <summary>
/// ドット絵の シート(Kenney Tiny Dungeon / Tiny Town、16×16、CC0)
/// </summary>
enum class SpriteSheet
{
	Dungeon,
	Town,
	Num,
};

/// <summary>
/// ドット絵の よみこみと かきかた
/// </summary>
namespace Sprite
{
	constexpr int kTileSize = 16;

	bool Init();
	void Terminate();

	/// <summary>
	/// そのままの タイル
	/// </summary>
	int Tile(SpriteSheet sheet, int index);

	/// <summary>
	/// いろを ぬりかえた タイル(あかるさの もようは のこす。おなじ ものは つくりおき)
	/// </summary>
	/// <param name="strength">0〜1(1で ほぼ その いろ)</param>
	/// <param name="keepSkin">はだの いろは のこす</param>
	int Tinted(SpriteSheet sheet, int index, const Game::Rgb& color, float strength, bool keepSkin = false);

	/// <summary>
	/// 左上と大きさで かく(ドット が ぼやけない)
	/// </summary>
	void DrawRect(int handle, float x, float y, float w, float h, bool flipX = false);

	/// <summary>
	/// 足もと(下の まんなか)を きじゅんに かく
	/// </summary>
	void DrawFeet(int handle, const Vec2& feet, float scaleX, float scaleY, bool flipX = false, float angle = 0.0f);
}
