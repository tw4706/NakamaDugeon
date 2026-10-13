#pragma once
#include<string>
#include<vector>

/// <summary>
/// アップデート1回分のお知らせ
/// </summary>
struct UpdateEntry
{
	std::wstring version;
	std::vector<std::wstring> lines;
};

/// <summary>
/// アップデート情報(data/Updates.csv)
/// </summary>
namespace UpdateLog
{
	bool Load(const char* path);

	/// <summary>
	/// 古い順
	/// </summary>
	const std::vector<UpdateEntry>& All();

	/// <summary>
	/// いちばん新しいバージョン(無ければ空)
	/// </summary>
	std::wstring Latest();

	/// <summary>
	/// パッチか(1.0.3 のような3けた)。ステージ・モンスターの追加はアップデート(1.1 のような2けた)
	/// </summary>
	bool IsPatch(const std::wstring& version);

	/// <summary>
	/// くらべる用の数(1.1.4 → 10104)
	/// </summary>
	int VersionCode(const std::wstring& version);
}
