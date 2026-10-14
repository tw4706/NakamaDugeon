#pragma once
#include"Game.h"
#include"MathUtil.h"
#include<functional>

/// <summary>
/// しんえんの オーラ(体を くらく そめて、りんかくから モヤが たちのぼる)
/// ピクセルシェーダー(data/shader/aura.hlsl)を つかう。つかえない PC では かんたんな ひょうじに なる
/// </summary>
namespace AuraRenderer
{
	/// <summary>
	/// シェーダーの よみこみと うごくかの たしかめ(DxLib_Init の あと)
	/// </summary>
	bool Init();
	void Terminate();

	/// <summary>
	/// シェーダーが つかえるか
	/// </summary>
	bool IsReady();

	/// <summary>
	/// オーラを つけて かく
	/// </summary>
	/// <param name="feet">あしもとの いち(画面)</param>
	/// <param name="color">オーラの いろ</param>
	/// <param name="time">ゆらぎの 時間</param>
	/// <param name="drawBody">あしもとの いちを うけとって 体を かく</param>
	void Draw(const Vec2& feet, const Game::Rgb& color, float time, const std::function<void(const Vec2&)>& drawBody);
}
