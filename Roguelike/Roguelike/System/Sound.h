#pragma once
#include<array>

constexpr int kBgmVolume = 160;
constexpr int kSEVolume = 230;

/// <summary>
/// 効果音
/// </summary>
enum class SE
{
	Cursor,
	Decide,
	Cancel,
	Count,
	Start,
	Finish,
	Coin,
	BigCoin,
	Explosion,
	Hit,
	Pop,
	Pump,
	Dash,
	Splash,
	Bang,
	Tick,
	Ready,
	Fanfare,
	Drumroll,
	Crash,
	Stun,
	Jump,
	Point,
	Whoosh,
	Fall,
	Buzzer,
	Signal,
	Shiny,		//きらきら(ワープなど)
	ShinyJackpot,	//色違いの登場(パチンコの あたりの ような おと)
	Num,
};

/// <summary>
/// BGM
/// </summary>
enum class BGM
{
	Title,
	Village,	//キャンプ・休憩所
	Dungeon,
	Boss,
	Victory,	//クリア・帰還
	Num,
	None,
};

/// <summary>
/// 音の生成と再生
/// 音源ファイルを使わずに起動時に波形を作る
/// </summary>
class Sound
{
public:
	static Sound& GetInstance();

private:
	Sound() = default;
	Sound(const Sound&) = delete;
	Sound& operator=(const Sound&) = delete;

public:
	void Init();
	void Terminate();

	/// <summary>
	/// 効果音の再生
	/// </summary>
	/// <param name="pitch">再生速度の倍率</param>
	void PlaySE(SE se, float pitch = 1.0f);

	void PlayBGM(BGM bgm);
	void StopBGM();

	/// <summary>
	/// 音量の設定(0〜10)
	/// </summary>
	void SetVolume(int bgm, int se);
private:
	std::array<int, static_cast<int>(SE::Num)> seHandles_ = {};
	std::array<int, static_cast<int>(BGM::Num)> bgmHandles_ = {};
	BGM currentBgm_ = BGM::None;
	int bgmVolume_ = kBgmVolume;
	int seVolume_ = kSEVolume;
};
