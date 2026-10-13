#pragma once
#include "Scene.h"
#include "Skill.h"
#include<vector>
#include<string>

/// <summary>
/// かじや(装備の作成・強化・装備、お守りの装備・錬成・分解)
/// </summary>
class SmithScene : public Scene
{
public:
	SmithScene(SceneManager& manager);

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	enum class Mode
	{
		Top,
		Equip,
		EquipAction,
		Charm,
		CharmAction,
		BulkMenu,		//まとめて えらぶ・ぶんかい
		BulkConfirm,	//まとめて ぶんかいの かくにん
		Alchemy,		//れんきん(スキルだまで えらんで つくる)
		SkillBook,		//スキルの せつめい
	};

	//装備の1行(モンスターと色違いかどうか)
	struct Row
	{
		int monsterId;
		bool isShiny;
		bool isKnown;
	};

	void BuildRows();
	void BuildRecommend();
	void UpdateTop();
	void UpdateEquip();
	void UpdateEquipAction();
	void UpdateCharm();
	void UpdateCharmAction();
	void UpdateBulkMenu();
	void UpdateBulkConfirm();
	void DrawBulk();

	/// <summary>
	/// ならびかえに あわせて お守りの じゅんばんを つくりなおす
	/// </summary>
	void RebuildCharmOrder();

	/// <summary>
	/// その行の お守りの ばんごう(お守りの行で なければ -1)
	/// </summary>
	int CharmIndexAt(int row) const;
	int RowOfCharm(int index) const;
	void RemoveCharm(int index);
	int SelectedCount() const;
	void UpdateAlchemy();
	void DrawAlchemy();

	/// <summary>
	/// れんきんで えらんでいる スキル(行ごと)
	/// </summary>
	SkillKind AlchemyKind(int row) const;
	int AlchemyNeed(int row) const;
	void Craft(int cost, int power, int fixedMain);
	void Notice(const std::wstring& text);

	void DrawTop();
	void DrawEquip();
	void DrawCharm();
	void DrawActionMenu();
	void UpdateSkillBook();
	void DrawSkillBook();

	/// <summary>
	/// そのスキルが付くもの(見つけていない装備は数だけ)
	/// </summary>
	std::vector<std::wstring> SkillSources(int kind) const;

	Mode mode_ = Mode::Top;
	float time_ = 0.0f;
	int topCursor_ = 0;
	int row_ = 0;
	int part_ = 0;
	int charmCursor_ = 0;
	int sortMode_ = 0;
	std::vector<int> charmOrder_;		//ひょうじする じゅんの お守りの ばんごう
	std::vector<char> selected_;		//まとめて ぶんかい する お守り
	int bulkCursor_ = 0;
	int confirmCursor_ = 1;
	int actionCursor_ = 0;
	int alchemyRow_ = 0;
	int alchemyPick_[4] = {};	//メイン・サブ1・サブ2・レア(サブとレアは0で なし)
	int skillCursor_ = 0;
	std::vector<Row> rows_;
	std::vector<std::wstring> recommend_;	//モンスターごとの おすすめの りゆう(空なら なし)
	std::vector<std::wstring> actions_;
	std::wstring notice_;
	float noticeTime_ = 0.0f;
	bool isLeaving_ = false;
};
