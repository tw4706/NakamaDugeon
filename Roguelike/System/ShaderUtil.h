#pragma once

/// <summary>
/// ピクセルシェーダーの よみこみと、画面を シェーダーで かきうつす 道具
/// </summary>
namespace ShaderUtil
{
	/// <summary>
	/// HLSL を その場で コンパイルして ピクセルシェーダーを つくる(しっぱいは -1)
	/// デバッグ版は エラーを logPath に のこす
	/// </summary>
	int LoadPixelShader(const char* hlslPath, const char* logPath);

	/// <summary>
	/// 画面(テクスチャ)を シェーダーで 四角に かく(アルファ ブレンド)
	/// </summary>
	void DrawWithShader(int pixelShader, int texture, float x, float y, float w, float h);
}
