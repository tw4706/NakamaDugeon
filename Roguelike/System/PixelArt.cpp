#include "PixelArt.h"
#include "ShaderUtil.h"
#include<DxLib.h>
#include<fstream>

namespace
{
	constexpr const char* kShaderPath = "data/shader/pixel.hlsl";
	constexpr const char* kLogPath = "pixel_log.txt";

	//小さく かく 下書きの 画面(ドット)。あしもとは まんなかの 下のほう
	constexpr int kScratchW = 128;
	constexpr int kScratchH = 128;
	constexpr float kLocalFeetX = 64.0f;
	constexpr float kLocalFeetY = 108.0f;
	constexpr int kConstSlot = 4;		//pixel.hlsl の register(b4)

	struct PixelConst
	{
		float size[4];
		float ink[4];
	};

	int pixelShader = -1;
	int constBuffer = -1;
	int scratch = -1;
	bool isReady = false;

	void RenderBody(int dot, const std::function<void(const Vec2&, float)>& drawBody)
	{
		const int prev = GetDrawScreen();
		FillGraph(scratch, 0, 0, 0, 0);
		SetDrawScreen(scratch);
		drawBody(Vec2(kLocalFeetX, kLocalFeetY), 1.0f / dot);
		SetDrawScreen(prev);
	}

	void DrawScratch(float x, float y, int dot)
	{
		PixelConst* c = static_cast<PixelConst*>(GetBufferShaderConstantBuffer(constBuffer));
		c->size[0] = static_cast<float>(kScratchW);
		c->size[1] = static_cast<float>(kScratchH);
		c->ink[0] = 25.0f / 255.0f;
		c->ink[1] = 18.0f / 255.0f;
		c->ink[2] = 35.0f / 255.0f;
		c->ink[3] = 1.0f;
		UpdateShaderConstantBuffer(constBuffer);
		SetShaderConstantBuffer(constBuffer, DX_SHADERTYPE_PIXEL, kConstSlot);
		ShaderUtil::DrawWithShader(pixelShader, scratch, x, y, static_cast<float>(kScratchW * dot), static_cast<float>(kScratchH * dot));
	}

	/// <summary>
	/// じっさいに かいて、体の いろ・ふち・そとの すきとおりが でるか たしかめる
	/// </summary>
	bool SelfTest()
	{
		const int prev = GetDrawScreen();
		const int test = MakeScreen(64, 64, FALSE);
		if (test < 0) return false;

		//4ドット四方の あかい はこ
		RenderBody(PixelArt::kDot, [](const Vec2& f, float) { DrawBox(static_cast<int>(f.x) - 2, static_cast<int>(f.y) - 4, static_cast<int>(f.x) + 2, static_cast<int>(f.y), GetColor(255, 0, 0), TRUE); });
		SetDrawScreen(test);
		DrawBox(0, 0, 64, 64, GetColor(255, 255, 255), TRUE);
		const float ox = 32.0f - kLocalFeetX * PixelArt::kDot;
		const float oy = 48.0f - kLocalFeetY * PixelArt::kDot;
		DrawScratch(ox, oy, PixelArt::kDot);

		//はこは x 26〜37・y 36〜47。ふちは その 1ドット(3ピクセル) そと
		int r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0, r3 = 0, g3 = 0, b3 = 0;
		GetColor2(GetPixel(31, 42), &r1, &g1, &b1);	//体
		GetColor2(GetPixel(31, 34), &r2, &g2, &b2);	//ふち
		GetColor2(GetPixel(31, 28), &r3, &g3, &b3);	//そと
		SetDrawScreen(prev);
		DeleteGraph(test);

		const bool isBody = r1 > 200 && g1 < 60;
		const bool isInk = r2 < 120 && g2 < 80;
		const bool isClear = r3 > 200 && g3 > 200;
#ifdef _DEBUG
		std::ofstream log(kLogPath);
		log << "body " << r1 << "," << g1 << "," << b1 << " ink " << r2 << "," << g2 << "," << b2 << " clear " << r3 << "," << g3 << "," << b3
			<< " -> " << (isBody && isInk && isClear ? "ok" : "fallback") << "\n";
#endif
		return isBody && isInk && isClear;
	}
}

bool PixelArt::Init()
{
	pixelShader = ShaderUtil::LoadPixelShader(kShaderPath, kLogPath);
	constBuffer = CreateShaderConstantBuffer(sizeof(PixelConst));
	scratch = MakeScreen(kScratchW, kScratchH, TRUE);
	isReady = pixelShader >= 0 && constBuffer >= 0 && scratch >= 0;
	if (isReady) isReady = SelfTest();
	return isReady;
}

void PixelArt::Terminate()
{
	if (pixelShader >= 0) DeleteShader(pixelShader);
	if (constBuffer >= 0) DeleteShaderConstantBuffer(constBuffer);
	if (scratch >= 0) DeleteGraph(scratch);
	pixelShader = constBuffer = scratch = -1;
	isReady = false;
}

bool PixelArt::IsReady()
{
	return isReady;
}

void PixelArt::Draw(const Vec2& feet, int dot, const std::function<void(const Vec2&, float)>& drawBody, const std::function<void()>& beforeComposite)
{
	if (scratch < 0)
	{
		//下書きの 画面も つくれないときは そのまま かく
		drawBody(feet, 1.0f);
		if (beforeComposite) beforeComposite();
		return;
	}

	RenderBody(dot, drawBody);
	if (beforeComposite) beforeComposite();
	const float x = feet.x - kLocalFeetX * dot;
	const float y = feet.y - kLocalFeetY * dot;
	if (isReady)
	{
		DrawScratch(x, y, dot);
		return;
	}

	//シェーダーが つかえないときは ぼかさずに ひろげるだけ
	const int prevMode = GetDrawMode();
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DrawExtendGraphF(x, y, x + kScratchW * dot, y + kScratchH * dot, scratch, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(prevMode);
}
