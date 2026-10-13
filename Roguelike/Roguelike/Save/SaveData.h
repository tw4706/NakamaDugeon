#pragma once
#include"Charm.h"
#include"EquipData.h"
#include<vector>
#include<array>
#include<string>

/// <summary>
/// 図鑑の記録
/// </summary>
struct ZukanEntry
{
	bool isSeen = false;
	bool isDefeated = false;
	bool isRecruited = false;
};

/// <summary>
/// 冒険中の仲間
/// </summary>
struct RunAlly
{
	int monsterId = 0;
	bool isShiny = false;
	int level = 1;
	int exp = 0;
	int hp = 1;
	int maxHp = 1;
	int atk = 1;
	int def = 0;
};

/// <summary>
/// 冒険の途中データ(階を降りた時に保存)
/// </summary>
struct RunData
{
	bool isActive = false;
	int stage = 0;
	int floor = 1;
	int turn = 0;
	int tactics = 0;

	int heroLevel = 1;
	int heroExp = 0;
	int heroHp = 1;
	int heroMaxHp = 1;
	int heroAtk = 1;
	int heroDef = 0;
	int heroCrit = 0;
	int personality = 0;
	bool isGutsUsed = false;

	std::vector<RunAlly> allies;

	//この冒険で拾ったもの(持ち帰るまでは仮)
	std::vector<int> materials;
	std::vector<int> shinyMaterials;
	std::vector<Charm> foundCharms;

	//冒険中に付けているお守り(-1: 拠点で付けたもの)
	int runCharm = -1;
};

/// <summary>
/// 冒険をまたいで引き継ぐ主人公の育ち
/// </summary>
struct HeroProgress
{
	int level = 1;
	int exp = 0;
	int maxHp = 0;	//0はまだ育っていない(初期値を使う)
	int atk = 0;
	int def = 0;
	int crit = 0;
};

/// <summary>
/// クリアで解放される新要素(見たかどうかのビット)
/// </summary>
namespace NewsBit
{
	constexpr int kInfinite = 1 << 0;		//むげんの めいきゅう
	constexpr int kCharm = 1 << 1;			//かじやの おまもり
	constexpr int kShinyZukan = 1 << 2;		//いろちがいの ずかん
	constexpr int kClearBanner = 1 << 3;	//クリア後の案内
}

/// <summary>
/// ぼうけんに つれていく なかま
/// </summary>
struct PartyMember
{
	int monsterId = 0;
	bool isShiny = false;
};

/// <summary>
/// セーブ枠1つ分
/// </summary>
struct SaveSlot
{
	bool exists = false;
	int playSeconds = 0;
	long long lastPlayed = 0;
	bool isCleared = false;
	int clearCount = 0;
	int deepestNormal = 0;
	int deepestInfinite = 0;
	int lastInfinite = 0;	//むげんの めいきゅうで 前回 おわった階(0は なし)
	HeroProgress hero;
	int seenNews = 0;	//NewsBit
	bool hasRelic = false;	//むげんの めいきゅう B100F の ほうび(ずっと効く)
	int keyItems = 0;	//ぬしを たおして もらった だいじな もの(KeyItem の ばんごうの ビット)
	std::vector<PartyMember> lastParty;	//まえに つれていった なかま

	/// <summary>
	/// 解放ずみで、まだ見ていない新要素か
	/// </summary>
	bool IsNew(int bit) const { return isCleared && (seenNews & bit) == 0; }

	std::vector<ZukanEntry> zukan;
	std::vector<ZukanEntry> zukanShiny;

	std::vector<int> materials;
	std::vector<int> shinyMaterials;

	//装備の強化レベル(-1は持っていない)
	std::vector<int> upperLv;
	std::vector<int> lowerLv;
	std::vector<int> shinyUpperLv;
	std::vector<int> shinyLowerLv;

	//付けている装備(モンスターID、-1は無し)
	int equipUpper = -1;
	bool isEquipUpperShiny = false;
	int equipLower = -1;
	bool isEquipLowerShiny = false;

	std::vector<Charm> charms;
	int equippedCharm = -1;
	int shards = 0;

	//お守りを分解して得た スキルだま(スキルごと)
	int orbs[static_cast<int>(SkillKind::Num)] = {};

	RunData run;

	/// <summary>
	/// モンスター数に合わせて配列をそろえる
	/// </summary>
	void Fit(int monsterNum);

	/// <summary>
	/// 拠点で付けている装備とお守りの補正
	/// </summary>
	HeroBonus GearBonus(const Charm* overrideCharm) const;
};

/// <summary>
/// むげんの めいきゅう B100F を たおした ほうび
/// </summary>
namespace Relic
{
	const wchar_t* Name();

	/// <summary>
	/// ずっと効くスキル
	/// </summary>
	const std::vector<SkillValue>& Skills();
}

/// <summary>
/// ぬしを はじめて たおすと もらえる だいじな もの(もっているだけで ずっと効く)
/// </summary>
namespace KeyItem
{
	struct Def
	{
		const wchar_t* name;
		const wchar_t* text;	//せつめい
		int bossId;				//くれる ぬし
		std::vector<SkillValue> skills;	//ゆうしゃに 効く
		int allyHpRate;			//なかまの さいだいHP +%
		int allyAtkRate;		//なかまの こうげき +%
	};

	int Count();
	const Def& Get(int index);

	/// <summary>
	/// その ぬしが くれる もの(なければ -1)
	/// </summary>
	int ForBoss(int monsterId);

	bool IsOwned(const SaveSlot& s, int index);
	void Give(SaveSlot& s, int index);
	int OwnedCount(const SaveSlot& s);

	/// <summary>
	/// こうかを みじかく(例: こうげき+2　なかまの さいだいHP+10%)
	/// </summary>
	std::wstring EffectText(int index);

	int AllyHpRate(const SaveSlot& s);
	int AllyAtkRate(const SaveSlot& s);
}

/// <summary>
/// 設定(全セーブ共通)
/// </summary>
struct Settings
{
	int bgmVolume = 7;	//0〜10
	int seVolume = 8;	//0〜10
	bool isFullScreen = false;
	int readNotices = 0;	//読んだお知らせの いちばん新しい バージョン(VersionCode)

	//ボタンの ひょうじ(0:じどう 1:キーボード 2:Xbox)
	int buttonStyle = 0;
	int moveSpeed = 1;	//いどうの はやさ(0:ゆっくり 1:ふつう 2:はやい)
	bool isShake = true;	//がめんの ゆれ
	bool isVibration = true;	//コントローラーの しんどう
	bool isMute = false;	//しょうおん

	/// <summary>
	/// じっさいに ならす おおきさ(しょうおん中は 0)
	/// </summary>
	int BgmLevel() const { return isMute ? 0 : bgmVolume; }
	int SeLevel() const { return isMute ? 0 : seVolume; }

	/// <summary>
	/// いどうの はやさの ばいりつ
	/// </summary>
	float MoveRate() const { return moveSpeed <= 0 ? 0.75f : (moveSpeed >= 2 ? 1.35f : 1.0f); }
};

/// <summary>
/// セーブの読み書き
/// </summary>
class SaveManager
{
public:
	static constexpr int kSlotNum = 3;

	static SaveManager& GetInstance();

private:
	SaveManager() = default;
	SaveManager(const SaveManager&) = delete;
	SaveManager& operator=(const SaveManager&) = delete;

public:
	/// <summary>
	/// 全枠と設定の読み込み
	/// </summary>
	void LoadAll();

	const SaveSlot& Slot(int index) const { return slots_[index]; }

	/// <summary>
	/// 遊ぶ枠を決める(空なら新しく作る)
	/// </summary>
	void Select(int index);

	int CurrentIndex() const { return current_; }
	SaveSlot& Current() { return slots_[current_]; }

	/// <summary>
	/// 今の枠を書き込む
	/// </summary>
	void Save();

	void Delete(int index);

	void AddPlayTime(float seconds);

	Settings& GetSettings() { return settings_; }
	void SaveSettings();

	/// <summary>
	/// いどう「はやい」が つかえるか(クリア後)
	/// </summary>
	bool IsFastMoveUnlocked() const;

	/// <summary>
	/// いま つかう いどうの はやさの ばいりつ(ロック中の「はやい」は ふつう)
	/// </summary>
	float MoveRate() const;

private:
	bool LoadSlot(int index);

	std::array<SaveSlot, kSlotNum> slots_;
	int current_ = -1;
	float playTimeAcc_ = 0.0f;
	Settings settings_;
};
