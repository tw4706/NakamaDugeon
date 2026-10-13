#pragma once
#include"Game.h"
#include"MathUtil.h"
#include"Font.h"

/// <summary>
/// キャラクターの表情
/// </summary>
enum class Face
{
	Normal,
	Happy,
	Sad,
	Dizzy,
	Surprised,
	Determined,
};

namespace DrawUtil
{
	Game::Rgb Lerp(const Game::Rgb& a, const Game::Rgb& b, float t);
	Game::Rgb Scale(const Game::Rgb& c, float rate);

	/// <summary>
	/// 上下グラデーションの四角形
	/// </summary>
	void Gradient(int x1, int y1, int x2, int y2, const Game::Rgb& top, const Game::Rgb& bottom);

	/// <summary>
	/// 斜めに流れるストライプ背景
	/// </summary>
	void Stripes(float time, const Game::Rgb& base, const Game::Rgb& stripe, float speed);

	/// <summary>
	/// 角丸のパネル
	/// </summary>
	void Panel(float x1, float y1, float x2, float y2, const Game::Rgb& fill, int alpha, const Game::Rgb& frame);

	void Star(float x, float y, float radius, float angle, unsigned int color);

	/// <summary>
	/// キャラクター
	/// </summary>
	/// <param name="pos">足元の位置</param>
	/// <param name="size">体の半径</param>
	/// <param name="squash">縦のつぶれ(1で通常)</param>
	/// <param name="look">目の向き</param>
	void Character(const Vec2& pos, float size, int player, Face face, float squash = 1.0f, const Vec2& look = Vec2(), bool hasShadow = true);

	/// <summary>
	/// 王冠
	/// </summary>
	void Crown(float x, float y, float size);

	/// <summary>
	/// コイン
	/// </summary>
	void Coin(float x, float y, float radius, float spin, bool isBig);

	/// <summary>
	/// メニュー用のウィンドウ
	/// </summary>
	void Window(float x1, float y1, float x2, float y2);

	/// <summary>
	/// 選択カーソル(右向きの三角)
	/// </summary>
	void Cursor(float x, float y, float time);

	/// <summary>
	/// 中央揃えの文字の左にカーソル
	/// </summary>
	void CursorBeside(float centerX, float y, const wchar_t* text, FontSize size, float time);

	/// <summary>
	/// 新しい要素の「！」マーク(はねる)
	/// </summary>
	void NewMark(float x, float y, float time, float size = 16.0f);
}
