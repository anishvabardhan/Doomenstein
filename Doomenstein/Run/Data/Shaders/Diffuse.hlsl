
#define MAX_POINT_LIGHTS 10
#define MAX_SPOT_LIGHTS 2

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

//------------------------------------------------------------------------------------------------
cbuffer DirectionalLight : register(b1)
{
	float3 SunDirection;
	float SunIntensity;
    float AmbientIntensity;
    float pad0;
    float pad1;
    float pad2;
};

struct PointLightDef
{
    float3	PointPosition;
    float	pad00;
    float3	PointColor;
    float	pad01;
};

struct SpotLightDef
{
    float3 SpotLightPosition;
    float Cutoff;
    float3 SpotLightDirection;
    float pad000;
    float3 SpotLightColor;
    float pad001;
};

cbuffer PointLight : register(b4)
{
    PointLightDef pointLights[MAX_POINT_LIGHTS];
};

cbuffer SpotLight : register(b5)
{
    SpotLightDef spotLights[MAX_SPOT_LIGHTS];
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4 ModelColor;
	float4x4 ModelMatrix;
};

//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 localTangent : TANGENT;
	float3 localBitangent : BITANGENT;
	float3 localNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------

struct v2p_t
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 tangent : TANGENT;
	float4 bitangent : BITANGENT;
	float4 normal : NORMAL;
    float4 fragPosition : POSITION;
};

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 localPosition = float4(input.localPosition, 1);
	float4 worldPosition = mul(ModelMatrix, localPosition);
	float4 viewPosition = mul(ViewMatrix, worldPosition);
	float4 clipPosition = mul(ProjectionMatrix, viewPosition);
	float4 localNormal = float4(input.localNormal, 0);
	float4 worldNormal = mul(ModelMatrix, localNormal);
	
	v2p_t v2p;
	v2p.position = clipPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.tangent = float4(0, 0, 0, 0);
	v2p.bitangent = float4(0, 0, 0, 0);
	v2p.normal = worldNormal;
    v2p.fragPosition = worldPosition;
	return v2p;
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 color = float4(0.0, 0.0, 0.0, 1.0);
    float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
    float4 vertexColor = input.color;
    float4 modelColor = ModelColor;
	
	// DIRECTION LIGHT
	
    float ambient = 0.1;
    float directional = SunIntensity * saturate(dot(normalize(input.normal.xyz), -SunDirection));
    float4 dirLightColor = float4((ambient + directional).xxx, 1);
	color = dirLightColor * textureColor * vertexColor * modelColor;
	
	// POINT LIGHTS
	
    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        float distance = length(pointLights[i].PointPosition - input.fragPosition.xyz);

        float pointAmbient = 0.5;
        float attenuation = 1.0 / (0.1 +  (0.2 * distance) + (0.1 * distance * distance));
        float diffuse = max(dot(normalize(pointLights[i].PointPosition - input.fragPosition.xyz), input.normal.xyz), 0.0);
        diffuse *= attenuation;
        pointAmbient *= attenuation;

        float4 pointLightColor = float4((pointAmbient + diffuse) * float3(pointLights[i].PointColor.xyz), 1);
        pointLightColor = saturate(pointLightColor);
        
        color += pointLightColor * textureColor * vertexColor * modelColor;       
        color = saturate(color);
    }

	// SPOT LIGHT
	
    for (int i = 0; i < MAX_SPOT_LIGHTS; i++)
    {
        float3 L = spotLights[i].SpotLightPosition - input.fragPosition.xyz;
        float distance = length(L);
        L = L / distance;
    
        float attenuation = 1.0 / ((0.1 * distance) + (0.1 * distance * distance));
    
        float minCos = cos(spotLights[i].Cutoff);
        float maxCos = (minCos + 1.0) / 2.0f;
        float cosAngle = dot(spotLights[i].SpotLightDirection.xyz, -L);
        float spotIntensity = smoothstep(minCos, maxCos, cosAngle);
    
        float spotLightAmbient = 0.05;
    
        float3 norm = normalize(input.normal.xyz);
        float diff = max(dot(norm, normalize(L)), 0.0);
        diff *= attenuation * spotIntensity;
    
        float4 spotLightColor = float4((spotLightAmbient + diff) * float3(spotLights[i].SpotLightColor.xyz), 1);
    
        spotLightColor = saturate(spotLightColor);
    
        color += spotLightColor * textureColor * vertexColor * modelColor;
        color = saturate(color);
    }
    
    clip(color.a - 0.01);
	
	return color;
}
