// ドット絵に する(DxLib / Direct3D11 の 2D シェーダー)
// 小さく かいた 体を ぼかさずに ひろげ、いろを すこし へらして、まわりに 1ドットの ふちを つける

struct PS_INPUT
{
	float4 Position   : SV_POSITION;	// DxLib の 2D 頂点シェーダーの 出力と そろえる
	float4 Diffuse    : COLOR0;
	float4 Specular   : COLOR1;
	float2 TexCoords0 : TEXCOORD0;
	float2 TexCoords1 : TEXCOORD1;
};

struct PS_OUTPUT
{
	float4 Color0 : SV_TARGET0;
};

// x,y:下書きの 大きさ(ドット)　g_Ink:ふちの いろ
cbuffer cbPixel : register(b4)
{
	float4 g_Size;
	float4 g_Ink;
};

Texture2D g_Texture : register(t0);

float4 Dot(int2 p)
{
	return g_Texture.Load(int3(p, 0));
}

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output;
	int2 p = int2(input.TexCoords0 * g_Size.xy);
	float4 c = Dot(p);

	// まわり: となりに 体が あれば 1ドットの ふち(となりの いろを くらく したもの と インクの あいだ)
	float4 result = float4(0.0, 0.0, 0.0, 0.0);
	float4 n0 = Dot(p + int2(1, 0));
	float4 n1 = Dot(p - int2(1, 0));
	float4 n2 = Dot(p + int2(0, 1));
	float4 n3 = Dot(p - int2(0, 1));
	if (n3.a >= 0.5) result = float4(lerp(g_Ink.rgb, n3.rgb * 0.3, 0.5), 1.0);
	if (n2.a >= 0.5) result = float4(lerp(g_Ink.rgb, n2.rgb * 0.3, 0.5), 1.0);
	if (n1.a >= 0.5) result = float4(lerp(g_Ink.rgb, n1.rgb * 0.3, 0.5), 1.0);
	if (n0.a >= 0.5) result = float4(lerp(g_Ink.rgb, n0.rgb * 0.3, 0.5), 1.0);

	// 体: はんぶん いじょう こい ドットだけ のこして、いろの だんかいを へらす
	if (c.a >= 0.5) result = float4(floor(c.rgb * 16.0 + 0.5) / 16.0, 1.0);

	output.Color0 = result;
	return output;
}
