#pragma once
#include<string>

/// <summary>
/// せっていの がめん(タイトル・村・ダンジョンで つかう)
/// </summary>
class SettingsMenu
{
public:
	enum class Result
	{
		None,
		Close,		//とじた
		Delete,		//「きろくを けす」を えらんだ
	};

	/// <param name="hasDelete">「きろくを けす」を出すか</param>
	explicit SettingsMenu(bool hasDelete);

	Result Update();
	void Draw() const;

	/// <summary>
	/// 下に出す 一言(数秒で きえる)
	/// </summary>
	void Notice(const std::wstring& text);

private:
	enum Item
	{
		kBgm,
		kSe,
		kMute,
		kButton,
		kSpeed,
		kShake,
		kVibration,
		kScreen,
		kControls,
		kDelete,
		kBack,
		kItemNum,
	};

	int ItemAt(int row) const;
	int RowNum() const;
	void DrawControls() const;

	bool hasDelete_;
	bool isControls_ = false;
	int cursor_ = 0;
	float time_ = 0.0f;
	std::wstring notice_;
	float noticeTime_ = 0.0f;
};
