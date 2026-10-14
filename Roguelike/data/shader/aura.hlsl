// しんえんの オーラ(DxLib / Direct3D11 の 2D シェーダー)
// 体を くらく そめて、りんかくから ゆらめく モヤを 上へ たちのぼらせる

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

// x:時間 y,z:1テクセルの おおきさ w:オーラの こさ
cbuffer cbAura : register(b4)
{
	float4 g_AuraColor;
	float4 g_Param;
};

SamplerState g_Sampler : register(s0);
Texture2D g_Texture : register(t0);

float Hash(float2 p)
{
	return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float Noise(float2 p)
{
	float2 i = floor(p);
	float2 f = frac(p);
	f = f * f * (3.0 - 2.0 * f);
	float a = Hash(i);
	float b = Hash(i + float2(1.0, 0.0));
	float c = Hash(i + float2(0.0, 1.0));
	float d = Hash(i + float2(1.0, 1.0));
	return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output;
	float2 uv = input.TexCoords0;
	float2 texel = g_Param.yz;
	float t = g_Param.x;
	float2 px = uv / texel;

	float4 c = g_Texture.Sample(g_Sampler, uv);

	// 上へ ながれる ノイズで しらべる いちを ずらす(下の 体を ひろって 上に モヤが のびる)
	float n = Noise(px / 9.0 + float2(0.0, t * 2.5));
	float n2 = Noise(px / 4.0 + float2(t * 1.7, t * 3.5));
	float2 wobble = float2((n - 0.5) * 8.0, 4.0 + n * 12.0) * texel;

	float glow = 0.0;
	[unroll]
	for (int k = 0; k < 12; k++)
	{
		float ang = 6.2831853 * k / 12.0;
		float2 dir = float2(cos(ang), sin(ang));
		glow = max(glow, g_Texture.Sample(g_Sampler, uv + dir * texel * 5.0 + wobble).a);
		glow = max(glow, g_Texture.Sample(g_Sampler, uv + dir * texel * 10.0 + wobble).a * 0.7);
		glow = max(glow, g_Texture.Sample(g_Sampler, uv + dir * texel * 16.0 + wobble * 1.6).a * 0.4 * n2);
	}

	float flicker = 0.6 + 0.4 * Noise(px / 5.0 + float2(t * 3.0, -t * 4.0));
	float aura = saturate(glow * flicker - c.a);

	// 体は くらく そまり、オーラの いろが すこし まざる
	float3 body = c.rgb * 0.5 + g_AuraColor.rgb * 0.14;
	float3 auraRgb = g_AuraColor.rgb * (0.65 + 0.55 * flicker);
	float3 rgb = lerp(auraRgb, body, c.a);
	float a = max(c.a, aura * 0.85 * g_Param.w);

	output.Color0 = float4(rgb, a);
	return output;
}
