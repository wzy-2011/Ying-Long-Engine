/**
 * @file PBRPixelShader.hlsl
 * @brief PBR material pixel shader
 */

#include "Lighting/Lighting.hlsli"

cbuffer MaterialConstantBuffer : register(b1)
{
    float3 Albedo;
    float Metallic;
    float Roughness;
    float AmbientOcclusion;

    int UseAlbedoTexture;
    int UseRoughnessTexture;
    int UseMetallicTexture;
    int UseNormalTexture;
    int UseAOTexture;
};

struct PBRPixelShaderOutput
{
    float4 FinalColor : SV_Target;
};

Texture2D AlbedoTexture : register(t0);
Texture2D MetallicTexture : register(t1);
Texture2D RoughnessTexture : register(t2);
Texture2D NormalTexture : register(t3);
SamplerState Sampler : register(s0);

struct PBRPixelShaderInput
{
    float3 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float4 Position : SV_Position;
    float2 TextureCoord : TextureCoord;
};

float3 GetNormalFromMap(PBRPixelShaderInput InputData)
{
    float3 tangentNormal = NormalTexture.Sample(Sampler, InputData.TextureCoord).rgb * 2.0f - 1.0f;

    float3 Q1 = ddx(InputData.WorldPosition);
    float3 Q2 = ddy(InputData.WorldPosition);
    float2 st1 = ddx(InputData.TextureCoord);
    float2 st2 = ddy(InputData.TextureCoord);

    float3 N = normalize(InputData.Normal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return mul(tangentNormal, TBN);
}

PBRMaterial GenerateMaterial(PBRPixelShaderInput InputData)
{
    PBRMaterial Material;

    if (UseAlbedoTexture)
    {
        Material.Albedo = pow(AlbedoTexture.Sample(Sampler, InputData.TextureCoord).rgb, float3(2.2f, 2.2f, 2.2f));
    }
    else
    {
        Material.Albedo = Albedo;
    }

    if (UseMetallicTexture)
    {
        Material.Metallic = MetallicTexture.Sample(Sampler, InputData.TextureCoord).r;
    }
    else
    {
        Material.Metallic = Metallic;
    }

    if (UseRoughnessTexture)
    {
        Material.Roughness = RoughnessTexture.Sample(Sampler, InputData.TextureCoord).r;
    }
    else
    {
        Material.Roughness = Roughness;
    }

    if (UseAOTexture)
    {
    }
    else
    {
        Material.AmbientOcclusion = AmbientOcclusion;
    }

    return Material;
}

PBRPixelShaderOutput main(PBRPixelShaderInput InputData)
{
    PBRPixelShaderOutput PBRPixelShaderOutObject;

    PBRMaterial Material = GenerateMaterial(InputData);

    float3 Normal = InputData.Normal;
    if (UseNormalTexture)
    {
        Normal = GetNormalFromMap(InputData);
    }

    PBRPixelShaderOutObject.FinalColor = float4(Lighting(Material, Normal, InputData.WorldPosition), 1.0f);

    return PBRPixelShaderOutObject;
}
