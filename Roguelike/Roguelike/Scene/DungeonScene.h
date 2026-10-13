#pragma once
#include "Scene.h"
#include "DungeonMap.h"
#include "Unit.h"
#include "Skill.h"
#include "SaveData.h"
#include "SettingsMenu.h"
#include "KeyItemMenu.h"
#include<vector>
#include<deque>
#include<string>
#include<array>

struct SaveSlot;
struct Charm;

/// <summary>
/// ダンジョン探索(ターン制)
/// </summary>
class DungeonScene : public Scene
{
public:
	/// <param name="stage">Stage::kNormal / Stage::kInfinite</param>
	/// <param name="isContinue">セーブされた冒険の続きから始めるか</param>
	/// <param name="startFloor">はじめる階(むげんの めいきゅうの つづき)</param>
	DungeonScene(SceneManager& manager, int stage, bool isContinue, int startFloor = 1, const std::vector<PartyMember>& party = {});

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	enum class State
	{
		Input,
		Recruit,
		LevelChoice,
		Menu,
		Status,
		Settings,		//せってい
		KeyItems,		//だいじな もの
		LeaveConfirm,	//ダンジョンから でるか たしかめる
		Descend,
		Rest,
		CharmSwap,
		GameOver,
		Clear,
		Return,
	};

	enum class Tactics
	{
		Attack,		//ガンガンいこうぜ
		Careful,	//いのちだいじに
	};

	struct Message
	{
		std::wstring text;
		unsigned int color;
		float time;
	};

	struct Projectile
	{
		Vec2 from;
		Vec2 to;
		float t;
		Game::Rgb color;
	};

	//------------------------------//
	// 冒険の開始・保存
	//------------------------------//
	SaveSlot& Save();
	void NewRun();
	void LoadRun();
	void SaveRun();
	/// <param name="isDead">死んだときは素材だけ持ち帰る</param>
	void BringBack(bool isDead = false);
	void EndRun();
	void RecomputeBonus();
	void ApplyHeroStats();
	const Charm* RunCharm();

	//------------------------------//
	// 進行
	//------------------------------//
	void StartFloor(bool isFirst);
	Unit MakeEnemy(int monsterId, bool isShiny, bool isElite) const;
	void SpawnEnemy(bool isOutOfSight);
	void UpdateInput();
	void UpdateRecruit();
	void UpdateLevelChoice();
	void UpdateMenu();
	void UpdateLeaveConfirm();
	void UpdateRest();
	void UpdateCharmSwap();
	void EndPlayerTurn();
	void TickStatus();
	void CleanupDead();
	void UpdateZukanSeen();
	void GoNextFloor();

	//------------------------------//
	// 行動
	//------------------------------//
	void ActAlly(int index);
	void ActEnemy(int index);
	bool SkipByStatus(int index);
	bool TryHeal(int index);
	bool TryRanged(int index, int target);

	/// <summary>
	/// となりの相手への攻撃(2回攻撃・まとめて攻撃をふくむ)
	/// </summary>
	void MeleeAttack(int index, int target);

	/// <summary>
	/// ボスが手下を呼ぶ
	/// </summary>
	bool TrySummon(int index);
	void Attack(int attacker, int target, float rate = 1.0f, bool isExtra = false);
	void HeroOnHit(int target, int damage);
	void Damage(int attacker, int target, int amount, bool isCritical);
	void Kill(int target, int killer);
	/// <param name="isFirstShiny">はじめて倒した色違いなら素材が確定</param>
	void DropLoot(const Unit& dead, bool isFirstShiny);
	void GainExp(int index, int exp);
	void MoveUnit(int index, const Cell& to);
	void StepToward(int index, const Cell& to);

	/// <summary>
	/// 仲間を主人公のそばへ移す(空きが無ければfalse)
	/// </summary>
	bool WarpNearHero(int index);

	//------------------------------//
	// ワープゲート(B1Fから先の階へ)
	//------------------------------//
	void PlaceGate(int targetFloor);
	int GateAt(const Cell& c) const;
	bool IsGateOpen(int targetFloor);
	void DrawGates();
	void AcceptRecruit();

	/// <summary>
	/// 仲間のレベルが1つ上がったときの成長
	/// </summary>
	void GrowAlly(Unit& u) const;

	/// <summary>
	/// だいじな もので なかまの さいだいHPを ふやす(なんど よんでも おなじ)
	/// </summary>
	void ApplyAllyHpBonus(Unit& u);

	/// <summary>
	/// 倒した敵から仲間を作る(主人公と同じレベルにする)
	/// </summary>
	Unit MakeAllyFrom(const Unit& enemy) const;

	/// <summary>
	/// えらんだ なかまを ゆうしゃと おなじ レベルで つくる
	/// </summary>
	Unit MakeAllyById(int monsterId, bool isShiny);

	//------------------------------//
	// 判定
	//------------------------------//
	int UnitAt(const Cell& c) const;
	bool IsBlocked(const Cell& c) const;
	bool IsAdjacent(const Cell& a, const Cell& b) const;
	int Distance(const Cell& a, const Cell& b) const;
	int NearestEnemyOf(int index, int range) const;
	int AllyCount() const;
	int MaxAlly() const;
	int ThemeFloor() const;
	bool IsBossAlive() const;
	bool Chance(int percent) const;
	Vec2 CellToPos(const Cell& c) const;

	//------------------------------//
	// 表示
	//------------------------------//
	void AddMessage(const std::wstring& text, unsigned int color);
	void DrawMap();
	void DrawUnits();
	void DrawHud();
	void DrawMinimap();
	void DrawMessages();
	void DrawRecruit();
	void DrawLevelChoice();
	void DrawMenu();
	void DrawLeaveConfirm();
	void DrawStatus();
	void DrawRest();
	void DrawCharmSwap();
	void DrawResult();

	int stage_;
	bool isContinue_;
	int startFloor_ = 1;
	std::vector<PartyMember> party_;	//しゅっぱつ まえに えらんだ なかま

	DungeonMap map_;
	std::vector<Unit> units_;	//0番は主人公
	State state_ = State::Input;
	Tactics tactics_ = Tactics::Attack;
	int floor_ = 1;
	int turn_ = 0;

	//主人公の素の能力(装備・お守り・性格を除く)
	int baseMaxHp_ = 0;
	int baseAtk_ = 0;
	int baseDef_ = 0;
	int baseCrit_ = 0;
	int personality_ = 0;
	bool isGutsUsed_ = false;
	HeroBonus bonus_;

	float time_ = 0.0f;
	float stateTime_ = 0.0f;
	float waitTimer_ = 0.0f;
	float moveHoldTime_ = 0.0f;

	struct Gate
	{
		Cell cell;
		int floor;
	};
	std::vector<Gate> gates_;
	int warpTarget_ = 0;	//ワープ中の行き先(0は無し)
	bool isAttackQueued_ = false;
	Vec2 camera_;

	//仲間になりたいモンスター
	bool hasPendingRecruit_ = false;
	Unit pendingUnit_;
	int recruitCursor_ = 0;
	bool isRecruitConfirm_ = false;		//ことわる前の確認中
	int recruitConfirmCursor_ = 1;

	//レベルアップの2択
	int pendingChoices_ = 0;
	std::array<int, 2> choiceOptions_ = {};
	int choiceCursor_ = 0;

	int menuCursor_ = 0;
	int leaveCursor_ = 1;
	SettingsMenu settingsMenu_{ false };
	KeyItemMenu keyItemMenu_;
	int gotKeyItem_ = -1;	//この ぼうけんで もらった だいじな もの(けっか がめん用)
	int blastDepth_ = 0;	//ばくえんの れんさの ふかさ
	int swapCursor_ = 0;
	int commandTarget_ = -1;
	bool isFirstClear_ = false;
	int resultMaterials_ = 0;
	int resultCharms_ = 0;

	std::deque<Message> messages_;
	std::vector<Projectile> projectiles_;
};
