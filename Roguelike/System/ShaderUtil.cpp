#include "ShaderUtil.h"
#include<Windows.h>
#include<d3dcompiler.h>
#include<DxLib.h>
#include<fstream>
#include<sstream>
#include<string>

#pragma comment(lib, "d3dcompiler.lib")

int ShaderUtil::LoadPixelShader(const char* hlslPath, const char* logPath)
{
	std::ifstream file(hlslPath, std::ios::binary);
	if (!file) return -1;
	std::stringstream ss;
	ss << file.rdbuf();
	const std::string src = ss.str();

	ID3DBlob* code = nullptr;
	ID3DBlob* errors = nullptr;
	const HRESULT hr = D3DCompile(src.data(), src.size(), hlslPath, nullptr, nullptr, "main", "ps_4_0",
		D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors);
	if (errors)
	{
#ifdef _DEBUG
		std::ofstream log(logPath);
		log << static_cast<const char*>(errors->GetBufferPointer()) << "\n";
#else
		(void)logPath;
#endif
		errors->Release();
	}
	if (FAILED(hr) || !code) return -1;
	const int handle = LoadPixelShaderFromMem(code->GetBufferPointer(), static_cast<int>(code->GetBufferSize()));
	code->Release();
	return handle;
}

void ShaderUtil::DrawWithShader(int pixelShader, int texture, float x, float y, float w, float h)
{
	VERTEX2DSHADER v[6] = {};
	const float xs[4] = { x, x + w, x, x + w };
	const float ys[4] = { y, y, y + h, y + h };
	const float us[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
	const float vs[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	const int order[6] = { 0, 1, 2, 2, 1, 3 };
	for (int i = 0; i < 6; i++)
	{
		const int k = order[i];
		v[i].pos = VGet(xs[k], ys[k], 0.0f);
		v[i].rhw = 1.0f;
		v[i].dif = GetColorU8(255, 255, 255, 255);
		v[i].spc = GetColorU8(0, 0, 0, 0);
		v[i].u = v[i].su = us[k];
		v[i].v = v[i].sv = vs[k];
	}

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	SetUsePixelShader(pixelShader);
	SetUseTextureToShader(0, texture);
	DrawPolygon2DToShader(v, 2);
	SetUseTextureToShader(0, -1);
	SetUsePixelShader(-1);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
