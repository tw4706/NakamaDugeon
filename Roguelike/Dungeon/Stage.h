#pragma once

/// <summary>
/// 遊べるステージ
/// </summary>
namespace Stage
{
	constexpr int kNormal = 0;		//はじまりのどうくつ(12階・ボスあり)
	constexpr int kInfinite = 1;	//むげんのめいきゅう(クリア後)
	constexpr int kTraining = 2;	//キラプルンの すみか(レベル上げ・むげん B20F の ぬしを たおすと かいほう)
	constexpr int kNum = 3;

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
		if (stage == kTraining) return L"キラプルンの すみか";
		return stage == kInfinite ? L"むげんの めいきゅう" : L"はじまりの どうくつ";
	}

	//------------------------------//
	// キラプルンの すみか
	//------------------------------//
	constexpr int kTrainingLastFloor = 5;
	constexpr int kTrainingUnlockBoss = 40;	//キングプルン(むげん B20F)
	constexpr int kKiraPurun = 18;
	constexpr int kKiraPercent = 50;		//キラプルンが でる わりあい
	constexpr int kTrainingOverRate = 10;	//あがりやすい レベルを こえたら けいけんちは 1/10

	/// <summary>
	/// むずかしさ(しょきゅう・ちゅうきゅう・じょうきゅう)
	/// </summary>
	struct TrainingRank
	{
		const wchar_t* name;
		int ticketCost;		//つかう きっぷの まい数
		int levelCap;		//この レベルまでは あがりやすい
		int baseFloor;		//ほかの まものは むげんの この階 + 2×階 あたりが でる
		float kiraHpRate;	//キラプルンの つよさ(もとの なんばい)
		float kiraDefRate;
		int kiraExp;		//キラプルンの けいけんち
	};

	constexpr int kTrainingRankNum = 3;
	inline const TrainingRank& Rank(int rank)
	{
		static const TrainingRank kRanks[kTrainingRankNum] =
		{
			{ L"しょきゅう", 1, 35, 30, 3.0f, 2.0f, 600 },
			{ L"ちゅうきゅう", 2, 50, 50, 5.0f, 3.0f, 1500 },
			{ L"じょうきゅう", 3, 65, 70, 7.0f, 4.0f, 3000 },
		};
		return kRanks[rank < 0 ? 0 : (rank >= kTrainingRankNum ? kTrainingRankNum - 1 : rank)];
	}
}
