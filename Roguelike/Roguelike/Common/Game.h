#pragma once
#include<DxLib.h>

// ゲーム全体で使用する定数
namespace Game
{
	// 画面情報
	constexpr int kScreenWidth = 1280;
	constexpr int kScreenHeight = 720;
	constexpr int kColorBitNum = 32;

	constexpr float kHalf = 0.5f;

	//1フレームの時間
	constexpr float kFrameTime = 1.0f / 60.0f;

	//参加人数
	constexpr int kPlayerNum = 4;

	struct Rgb
	{
		int r;
		int g;
		int b;
	};

	inline unsigned int ToColor(const Rgb& c) { return GetColor(c.r, c.g, c.b); }

	//プレイヤーの色
	inline Rgb PlayerRgb(int index)
	{
		static const Rgb kTable[kPlayerNum] =
		{
			{ 240, 80, 80 },
			{ 70, 140, 245 },
			{ 80, 200, 100 },
			{ 250, 200, 50 },
		};
		return kTable[index];
	}

	inline unsigned int PlayerColor(int index) { return ToColor(PlayerRgb(index)); }

	//プレイヤーの名前
	inline const wchar_t* PlayerName(int index)
	{
		static const wchar_t* kTable[kPlayerNum] = { L"アカ", L"アオ", L"ミドリ", L"キイロ" };
		return kTable[index];
	}

	inline unsigned int WhiteColor() { return GetColor(255, 255, 255); }
	inline unsigned int BlackColor() { return GetColor(0, 0, 0); }
	inline unsigned int YellowColor() { return GetColor(255, 230, 60); }

	//ゲームを終わらせる
	inline bool isExitRequested = false;
}
