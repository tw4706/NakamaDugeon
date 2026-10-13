#pragma once
#include<string>
#include<vector>

/// <summary>
/// フォントの大きさ
/// </summary>
enum class FontSize
{
	Small,		//24
	Medium,		//36
	Large,		//56
	Huge,		//96
	Giant,		//150
	Num,
};

namespace Font
{
	void Init();
	void Terminate();

	int Get(FontSize size);
	int GetPixelSize(FontSize size);

	/// <summary>
	/// 中央揃えで描画
	/// </summary>
	void DrawCenter(float x, float y, const wchar_t* text, unsigned int color, FontSize size, unsigned int edgeColor);

	/// <summary>
	/// 中央揃え・拡大して描画
	/// </summary>
	void DrawCenterScaled(float x, float y, float scale, const wchar_t* text, unsigned int color, FontSize size, unsigned int edgeColor);

	/// <summary>
	/// 中央揃え・拡大・半透明で描画
	/// </summary>
	void DrawCenterAlpha(float x, float y, float scale, int alpha, const wchar_t* text, unsigned int color, FontSize size, unsigned int edgeColor);

	/// <summary>
	/// 幅に収まるように縮めて描画(左上基準)。縮めすぎになるときは末尾を「…」にする
	/// </summary>
	void DrawFit(float x, float y, const std::wstring& text, unsigned int color, FontSize size, unsigned int edgeColor, float maxWidth);

	/// <summary>
	/// 幅に収まるように縮めて中央揃えで描画
	/// </summary>
	void DrawCenterFit(float x, float y, const std::wstring& text, unsigned int color, FontSize size, unsigned int edgeColor, float maxWidth);

	/// <summary>
	/// 幅に収まるように折り返す(なるべく空白で切る)
	/// </summary>
	std::vector<std::wstring> Wrap(const std::wstring& text, FontSize size, float maxWidth);

	int Width(const std::wstring& text, FontSize size);
}
