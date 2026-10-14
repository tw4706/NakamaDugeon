#pragma once
#include"MathUtil.h"
#include<functional>

/// <summary>
/// ずけいで かいた キャラを ドット絵に する
/// 1/3 の 大きさで かいてから 3ばいに ひろげ、1ドットの ふちを つける(data/shader/pixel.hlsl)
/// ゆか・かべの タイル(16ドットを 48ピクセル)と ドットの 大きさを そろえている
/// </summary>
namespace PixelArt
{
	constexpr int kDot = 3;			//1ドットの ピクセル数(ゆかの タイルと おなじ)
	constexpr int kSmallDot = 2;	//小さく かくとき(メニューの なか)
	constexpr float kSmallSize = 28.0f;	//この 大きさ みまんは kSmallDot

	/// <summary>
	/// キャラの 大きさに あった 1ドットの ピクセル数
	/// </summary>
	inline int DotFor(float size) { return size < kSmallSize ? kSmallDot : kDot; }

	bool Init();
	void Terminate();
	bool IsReady();

	/// <summary>
	/// ドット絵で かく
	/// </summary>
	/// <param name="feet">あしもとの いち(画面)</param>
	/// <param name="dot">1ドットの ピクセル数(DotFor)</param>
	/// <param name="drawBody">小さい 画面での あしもとの いちと 大きさの 倍率(1/dot)を うけとって かく</param>
	/// <param name="beforeComposite">体を 画面に のせる まえに かくもの(あしもとの かげ など)</param>
	void Draw(const Vec2& feet, int dot, const std::function<void(const Vec2&, float)>& drawBody, const std::function<void()>& beforeComposite = nullptr);
}
