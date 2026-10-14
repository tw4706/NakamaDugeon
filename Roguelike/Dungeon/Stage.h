#pragma once

/// <summary>
/// 遊べるステージ
/// </summary>
namespace Stage
{
	constexpr int kNormal = 0;		//はじまりのどうくつ(12階・ボスあり)
	constexpr int kInfinite = 1;	//むげんのめいきゅう(クリア後)
	constexpr int kTraining = 2;	//キラプルンの すみか(レベル上げ・むげん B20F の ぬしを たおすと かいほう)
	constexpr int kAbyss = 3;		//しんえん(むげん B100F の つぎ。えいゆうの あかしで かいほう)
	constexpr int kNum = 4;

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
		if (stage == kAbyss) return L"しんえん";
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

	//------------------------------//
	// しんえん(B101〜)
	//------------------------------//
	constexpr int kAbyssFirstFloor = 101;
	constexpr int kAbyssLastFloor = 200;		//5そう(20かいずつ)。B200 に さいごの ぬし
	constexpr int kAbyssLayerFloors = 20;
	constexpr float kAbyssBaseRate = 1.9f;		//しんえん版の つよさ(もとの なんばい。ver1.7.2 で 1.6 から)
	constexpr float kAbyssFloorRate = 0.008f;	//1かい ふかく なるごとに +0.8%(1.2% から)
	constexpr int kAbyssExpRate = 2;			//けいけんちは 2ばい
	constexpr int kAbyssFinalBoss = 51;		//ヨミノオウ

	/// <summary>
	/// しんえんの そう(20かいごと)の なまえ
	/// </summary>
	inline const wchar_t* AbyssArea(int floor)
	{
		static const wchar_t* kNames[5] = { L"かげの みずうみ", L"ほねの はかば", L"さかさまの とう", L"ゆめの はざま", L"しんえんの そこ" };
		const int index = (floor - kAbyssFirstFloor) / kAbyssLayerFloors;
		return kNames[index < 0 ? 0 : (index > 4 ? 4 : index)];
	}

	/// <summary>
	/// しんえん版の もとに なる まものを えらぶ むげんの 階(B101〜B200 で B61〜B100 へ じゅんに ひろがる)
	/// </summary>
	inline int AbyssSourceFloor(int floor)
	{
		const int span = kAbyssLastFloor - kAbyssFirstFloor;
		const int step = floor - kAbyssFirstFloor;
		return 61 + (step < 0 ? 0 : (step > span ? span : step)) * 39 / span;
	}

	/// <summary>
	/// その そうの さいごの 階で まつ ぬし(しんえん版。いなければ -1)
	/// </summary>
	inline int AbyssBossFor(int floor)
	{
		if (floor == 120) return 41;	//くるった ヒョウガドラゴン
		if (floor == 140) return 44;	//くるった テンクウノオウ
		if (floor == 160) return 42;	//くるった ヨウガンマジン
		if (floor == 180) return 43;	//くるった ヤミノシンカン
		if (floor == 200) return kAbyssFinalBoss;	//ヨミノオウ(しんえんの さいごの ぬし)
		return -1;
	}
}
