#pragma once

/// <summary>
/// だいじな もの の いちらん(村と ダンジョンで つかう)
/// </summary>
class KeyItemMenu
{
public:
	/// <summary>
	/// とじたら true
	/// </summary>
	bool Update();
	void Draw() const;

private:
	int cursor_ = 0;
	float time_ = 0.0f;
};
