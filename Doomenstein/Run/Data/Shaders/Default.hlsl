//------------------------------------------------------------------------------------------------
Texture2D srcTexture : register(t0);
SamplerState diffuseSampler : register(s0);

cbuffer ModelConstants : register(b3)
{
	float4 ModelColor;
	float4x4 ModelMatrix;
};

cbuffer CameraConstants : register(b2)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct v2p_t
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

v2p_t VertexMain(vs_input_t input)
{
	float4 localPosition = float4(input.localPosition, 1);

	float4 worldPosition = mul(ModelMatrix, localPosition);
	float4 viewPosition = mul(ViewMatrix, worldPosition);
	float4 clipPosition = mul(ProjectionMatrix, viewPosition);

	v2p_t v2p;
	v2p.position = clipPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;

	return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 textureColor = srcTexture.Sample(diffuseSampler, input.uv);
    clip(textureColor.a - 0.1);
	return float4(input.color) * ModelColor * textureColor;
}