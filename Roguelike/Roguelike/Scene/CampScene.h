#pragma once
#include "Scene.h"
#include "DungeonMap.h"
#include "MathUtil.h"
#include "KeyItemMenu.h"
#include "SaveData.h"
#include<vector>
#include<string>

/// <summary>
/// 拠点のキャンプ(歩いて各機能を開く)
/// </summary>
class CampScene : public Scene
{
public:
	CampScene(SceneManager& manager);

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

	bool blocked_[kRows][kCols] = {};
	Spot spots_[kRows][kCols] = {};
	bool path_[kRows][kCols] = {};

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
};
