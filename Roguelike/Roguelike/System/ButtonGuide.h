#pragma once
#include"Font.h"
#include"Input.h"
#include<string>

/// <summary>
/// ボタンの ひょうじ(キーボード / Xbox コントローラー)
/// 文字の中の {A} {B} {Y} {M} {MOVE} {UD} {LR} {L} {R} を ボタンの絵に かえて かく
/// </summary>
namespace ButtonGuide
{
	/// <summary>
	/// コントローラーの ひょうじにするか(せっていと さいごに さわった機器で きまる)
	/// </summary>
	bool IsPad();

	/// <summary>
	/// メッセージ用の ボタン名(例: "Aボタン" / "SPACE")
	/// </summary>
	std::wstring Name(Button button);

	float Width(const std::wstring& text, FontSize size);

	/// <summary>
	/// 左はし x、たての まんなか y に かく
	/// </summary>
	void Draw(float x, float y, const std::wstring& text, unsigned int color, FontSize size, unsigned int edgeColor);
	void DrawCenter(float x, float y, const std::wstring& text, unsigned int color, FontSize size, unsigned int edgeColor);
}
