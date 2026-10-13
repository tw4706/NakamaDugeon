#pragma once

/// <summary>
/// 遊べるステージ
/// </summary>
namespace Stage
{
	constexpr int kNormal = 0;		//はじまりのどうくつ(12階・ボスあり)
	constexpr int kInfinite = 1;	//むげんのめいきゅう(クリア後)
	constexpr int kNum = 2;

	constexpr int kNormalLastFloor = 12;
	constexpr int kInfiniteLastFloor = 100;	//むげんの めいきゅうの さいご(テンクウノオウ)

	/// <summary>
	/// むげんの めいきゅうのエリア名(20階ごと)
	/// </summary>
	inline const wchar_t* InfiniteArea(int floor)
	{
		static const wchar_t* kNames[5] = { L"まよいの いりぐち", L"こおりの どうくつ", L"ようがんの ちか", L"やみの しんでん", L"てんくうの とう" };
		const int index = (floor - 1) / 20;
		return kNames[index < 0 ? 0 : (index > 4 ? 4 : index)];
	}

	//無限ダンジョンで休憩する間隔
	constexpr int kRestInterval = 5;

	//無限ダンジョンで強敵が出る間隔
	constexpr int kEliteInterval = 10;

	inline const wchar_t* Name(int stage)
	{
		return stage == kInfinite ? L"むげんの めいきゅう" : L"はじまりの どうくつ";
	}
}
