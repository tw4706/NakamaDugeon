#include "AuraRenderer.h"
#include "ShaderUtil.h"
#include<DxLib.h>
#include<fstream>
#include<cmath>

namespace
{
	constexpr const char* kShaderPath = "data/shader/aura.hlsl";
	constexpr const char* kLogPath = "aura_log.txt";

	//体を いちど かく 下書きの 画面(あしもとは まんなかの 下のほう)
	constexpr int kScratchW = 256;
	constexpr int kScratchH = 256;
	constexpr float kLocalFeetX = 128.0f;
	constexpr float kLocalFeetY = 208.0f;
	constexpr int kConstSlot = 4;		//aura.hlsl の register(b4)

	struct AuraConst
	{
		float color[4];
		float param[4];		//x:時間 y,z:1テクセル w:こさ
	};

	int pixelShader = -1;
	int constBuffer = -1;
	int scratch = -1;
	bool isReady = false;

	/// <summary>
	/// 下書きの 画面を シェーダーで かきうつす(左上を x,y に)
	/// </summary>
	void DrawScratch(float x, float y, const Game::Rgb& color, float time, float strength)
	{
		AuraConst* c = static_cast<AuraConst*>(GetBufferShaderConstantBuffer(constBuffer));
		c->color[0] = color.r / 255.0f;
		c->color[1] = color.g / 255.0f;
		c->color[2] = color.b / 255.0f;
		c->color[3] = 1.0f;
		c->param[0] = time;
		c->param[1] = 1.0f / kScratchW;
		c->param[2] = 1.0f / kScratchH;
		c->param[3] = strength;
		UpdateShaderConstantBuffer(constBuffer);
		SetShaderConstantBuffer(constBuffer, DX_SHADERTYPE_PIXEL, kConstSlot);
		ShaderUtil::DrawWithShader(pixelShader, scratch, x, y, static_cast<float>(kScratchW), static_cast<float>(kScratchH));
	}

	/// <summary>
	/// 体を 下書きの 画面に かく(もとの かき先に もどす)
	/// </summary>
	void RenderBody(const std::function<void(const Vec2&)>& drawBody)
	{
		const int prev = GetDrawScreen();
		FillGraph(scratch, 0, 0, 0, 0);
		SetDrawScreen(scratch);
		drawBody(Vec2(kLocalFeetX, kLocalFeetY));
		SetDrawScreen(prev);
	}

	/// <summary>
	/// じっさいに かいて、体は くらく・まわりは オーラの いろに なるか たしかめる
	/// </summary>
	bool SelfTest()
	{
		const int prev = GetDrawScreen();
		const int test = MakeScreen(64, 64, TRUE);
		if (test < 0) return false;

		RenderBody([](const Vec2& f) { DrawBox(static_cast<int>(f.x) - 8, static_cast<int>(f.y) - 16, static_cast<int>(f.x) + 8, static_cast<int>(f.y), GetColor(255, 255, 255), TRUE); });
		FillGraph(test, 0, 0, 0, 0);
		SetDrawScreen(test);
		DrawScratch(32.0f - kLocalFeetX, 48.0f - kLocalFeetY, { 255, 0, 0 }, 0.0f, 1.0f);

		int br = 0, bg = 0, bb = 0, ar = 0, ag = 0, ab = 0;
		GetColor2(GetPixel(32, 40), &br, &bg, &bb);	//体の なか
		GetColor2(GetPixel(32, 29), &ar, &ag, &ab);	//体の すこし 上(オーラ)
		SetDrawScreen(prev);
		DeleteGraph(test);

		const bool isBodyDark = br < 200 && bg < 200 && br > 20;
		const bool isAuraRed = ar > 40 && ar > ag * 2;
#ifdef _DEBUG
		//デバッグでは けっかを ファイルに のこす(アプリの ログは きっている)
		std::ofstream log(kLogPath);
		log << "body " << br << "," << bg << "," << bb << " aura " << ar << "," << ag << "," << ab
			<< " -> " << (isBodyDark && isAuraRed ? "ok" : "fallback") << "\n";
#endif
		return isBodyDark && isAuraRed;
	}
}

bool AuraRenderer::Init()
{
	pixelShader = ShaderUtil::LoadPixelShader(kShaderPath, kLogPath);
	constBuffer = CreateShaderConstantBuffer(sizeof(AuraConst));
	scratch = MakeScreen(kScratchW, kScratchH, TRUE);
	isReady = pixelShader >= 0 && constBuffer >= 0 && scratch >= 0;
	if (isReady) isReady = SelfTest();
	return isReady;
}

void AuraRenderer::Terminate()
{
	if (pixelShader >= 0) DeleteShader(pixelShader);
	if (constBuffer >= 0) DeleteShaderConstantBuffer(constBuffer);
	if (scratch >= 0) DeleteGraph(scratch);
	pixelShader = constBuffer = scratch = -1;
	isReady = false;
}

bool AuraRenderer::IsReady()
{
	return isReady;
}

void AuraRenderer::Draw(const Vec2& feet, const Game::Rgb& color, float time, const std::function<void(const Vec2&)>& drawBody)
{
	if (!isReady)
	{
		//かんたんな ひょうじ: くらく そめて、まわりに うすい ひかり
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(70 + 30 * std::sin(time * 5.0f)));
		DrawOvalAA(feet.x, feet.y - 26.0f, 30.0f, 38.0f, 24, Game::ToColor(color), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		SetDrawBright(110, 95, 130);
		drawBody(feet);
		SetDrawBright(255, 255, 255);
		return;
	}

	RenderBody(drawBody);
	DrawScratch(feet.x - kLocalFeetX, feet.y - kLocalFeetY, color, time, 1.0f);
}
