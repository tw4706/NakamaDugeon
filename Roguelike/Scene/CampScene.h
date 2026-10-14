#pragma once
#include "Scene.h"
#include "DungeonMap.h"
#include "MathUtil.h"
#include "KeyItemMenu.h"
#include "SaveData.h"
#include "UpdateLog.h"
#include<vector>
#include<string>

/// <summary>
/// 拠点のキャンプ(歩いて各機能を開く)
/// </summary>
class CampScene : public Scene
{
public:
	/// <param name="start">さいしょに たつ マス(しょこ・いけから もどったときなど)</param>
	CampScene(SceneManager& manager, const Cell& start = { 13, 5 });

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	static constexpr int kCols = 26;
	static constexpr int kRows = 15;

	enum class Spot
	{
		None,
		Dungeon,
		Library,
		Smith,
		Fire,
		Notice,
		Pond,
	};

	enum class Mode
	{
		Walk,
		StageSelect,
		Confirm,
		Pause,
		Notice,
		Unlock,		//クリア後の解放の案内
		FloorSelect,	//むげんの めいきゅうの はじめる階
		KeyItems,		//だいじな もの
		PartySelect,	//つれていく なかまを えらぶ
		RankSelect,		//キラプルンの すみかの むずかしさ
		LibraryMenu,	//しょこで よむ ずかんを えらぶ
		FireMenu,		//たきび(りょうり・せってい)
		Cook,			//りょうりする さかなを えらぶ
	};

	/// <summary>
	/// お知らせの1行
	/// </summary>
	struct NoticeRow
	{
		std::wstring text;
		bool isHeader;
		bool isPatch = false;	//見出しがパッチか(アップデートでないか)
	};

	struct Wanderer
	{
		int monsterId;
		bool isShiny;
		Cell cell;
		Vec2 drawPos;
		float timer;
		float facing;
	};

	struct StageChoice
	{
		std::wstring text;
		int stage;		//-1:つづきから -2:やめる
		bool isLocked;
	};

	void BuildMap();
	bool IsBlocked(const Cell& c) const;
	Spot SpotAt(const Cell& c) const;
	Vec2 CellToPos(const Cell& c) const;
	Cell PosToCell(const Vec2& p) const;
	bool IsHitWall(const Vec2& p) const;
	void UpdateWalk();
	void UpdateStageSelect();
	void OpenStageSelect();
	void StartDungeon(int stage, bool isContinue);
	void LaunchDungeon(int stage, bool isContinue);
	void OpenPartySelect();
	void UpdatePartySelect();
	void DrawPartySelect();
	int MaxParty() const;
	void BeginNewRun(int stage, int startFloor);
	void UpdateFloorSelect();
	void DrawFloorSelect();
	void UpdateRankSelect();
	void DrawRankSelect();
	void OpenNotice();
	void UpdateNotice();
	bool HasUnreadNotice() const;

	void DrawGround();
	void DrawBuildings();
	void DrawArchive();
	void DrawSmithy();
	void DrawNoticeBoard();
	void DrawHud();
	void DrawStageSelect();
	void DrawNotice();
	void DrawNewMarks();
	void DrawUnlock();
	void UpdateUnlock();
	void UpdateLibraryMenu();
	void DrawLibraryMenu();
	void UpdateFireMenu();
	void DrawFireMenu();
	void OpenCook();
	void UpdateCook();
	void DrawCook();

	/// <summary>
	/// つぎの ぼうけんで たべる りょうりの せつめい(なければ からっぽ)
	/// </summary>
	std::wstring PendingMealText() const;

	bool blocked_[kRows][kCols] = {};
	Spot spots_[kRows][kCols] = {};
	bool path_[kRows][kCols] = {};

	Cell startCell_;
	Cell heroCell_ = { 13, 6 };
	Vec2 heroPos_;
	float heroFacing_ = 1.0f;

	std::vector<Wanderer> wanderers_;
	Mode mode_ = Mode::Walk;
	std::vector<StageChoice> choices_;
	int cursor_ = 0;
	int pendingStage_ = 0;
	int pendingFloor_ = 1;
	std::vector<int> floorChoices_;		//えらべる はじめる階
	int floorCursor_ = 0;
	int lastFloorChoice_ = 0;			//前回の つづきの階(0は なし)
	int rankCursor_ = 0;				//キラプルンの すみかの むずかしさ
	int stageCursor_ = 0;				//えらんでいる いきさき(かくにん中も おぼえておく)
	int areaCursor_ = 0;				//くわしく 見ている エリア
	int libraryCursor_ = 0;				//しょこの ずかん(0:モンスター 1:つり 2:やめる)
	int fireCursor_ = 0;				//たきび(0:りょうり 1:せってい 2:やめる)
	std::vector<int> cookList_;			//もっている さかな(FishTable の ばんごう)
	int cookCursor_ = 0;
	int cookTop_ = 0;

	/// <summary>
	/// えらんだ いきさきの ステージ(つづきからは その ステージ、やめるは -1)
	/// </summary>
	int ChoiceStage(int index) const;
	void ResetAreaCursor();
	void DrawStageDetail(float left, float top, float right, float bottom);
	float time_ = 0.0f;
	bool isLeaving_ = false;

	//お知らせ
	std::vector<NoticeRow> noticeRows_;
	KeyItemMenu keyItemMenu_;

	//なかまを えらぶ
	std::vector<PartyMember> partyCands_;
	std::vector<PartyMember> partySel_;
	int partyCursor_ = -1;	//-1は「しゅっぱつ」
	std::wstring partyMsg_;
	float partyMsgTime_ = 0.0f;
	int noticeScroll_ = 0;
	float noticeHold_ = 0.0f;	//おしっぱなしで つぎに うごくまでの時間
	bool isNoticeNew_ = false;	//開いたときに未読があったか
	int noticeReadBefore_ = 0;	//開くまえに 読んでいた バージョン(NEW の しるし用)
	int noticeCursor_ = 0;		//えらんでいる バージョン(あたらしい順)
	int noticeListTop_ = 0;		//いちらんの いちばん上
	int noticeDetail_ = -1;		//くわしく 見ている バージョン(-1は いちらん)

	/// <summary>
	/// おしっぱなしでも つづけて うごく 上下の入力(-1/0/1)
	/// </summary>
	int NoticeRepeatDir();
	void OpenNoticeDetail(int index);
	const UpdateEntry& NoticeEntry(int index) const;
	int NoticeCount() const;
};
