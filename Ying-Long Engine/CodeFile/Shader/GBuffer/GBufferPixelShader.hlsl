/**
 * @file GBufferPixelShader.hlsl
 * @brief Geometry Pass pixel shader for Deferred Rendering
 *
 * Writes surface attributes (Albedo, Normal, Position, Material params)
 * to the G-Buffer render targets (MRT).
 *
 * G-Buffer layout:
 *   SV_Target0 (RT0): Albedo(RGB) + AO(A)        - R8G8B8A8_UNORM
 *   SV_Target1 (RT1): Normal(RGB) + Roughness(A) - R16G16B16A16_FLOAT
 *   SV_Target2 (RT2): Position(RGB) + Metallic(A) - R16G16B16A16_FLOAT
 *   SV_Target3 (RT3): Emissive(RGB) + Unused(A)  - R8G8B8A8_UNORM (reserved)
 */

#include "../Lighting/Lighting.hlsli"

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
    int Unlit;
}

Texture2D AlbedoTexture : register(t0);
Texture2D MetallicTexture : register(t1);
Texture2D RoughnessTexture : register(t2);
Texture2D NormalTexture : register(t3);
SamplerState Sampler : register(s0);

struct GBufferPSInput
{
    float3 WorldPosition : WORLD_POSITION;
    float3 Normal : NORMAL;
    float4 Position : SV_Position;
    float2 TexCoord : TextureCoord;
};

struct GBufferPSOutput
{
    float4 AlbedoAO : SV_Target0;       // RGB: Albedo(sRGB), A: AO
    float4 NormalRoughness : SV_Target1; // XYZ: World Normal, W: Roughness
    float4 PositionMetallic : SV_Target2; // XYZ: World Position, W: Metallic
    float4 EmissiveUnused : SV_Target3;  // RGB: Emissive, A: unused
};

float3 GetNormalFromMap(GBufferPSInput input)
{
    float3 tangentNormal = NormalTexture.Sample(Sampler, input.TexCoord).rgb * 2.0f - 1.0f;

    float3 Q1 = ddx(input.WorldPosition);
    float3 Q2 = ddy(input.WorldPosition);
    float2 st1 = ddx(input.TexCoord);
    float2 st2 = ddy(input.TexCoord);

    float3 N = normalize(input.Normal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return mul(tangentNormal, TBN);
}

GBufferPSOutput main(GBufferPSInput input)
{
    GBufferPSOutput output;

    // Sample material properties
    float3 albedo = Albedo;
    if (UseAlbedoTexture)
    {
        albedo = pow(AlbedoTexture.Sample(Sampler, input.TexCoord).rgb, float3(2.2f, 2.2f, 2.2f));
    }

    float metallic = Metallic;
    if (UseMetallicTexture)
    {
        metallic = MetallicTexture.Sample(Sampler, input.TexCoord).r;
    }

    float roughness = Roughness;
    if (UseRoughnessTexture)
    {
        roughness = RoughnessTexture.Sample(Sampler, input.TexCoord).r;
    }

    float ao = AmbientOcclusion;

    // Normal (with optional normal map)
    float3 normal = input.Normal;
    if (UseNormalTexture)
    {
        normal = GetNormalFromMap(input);
    }
    normal = normalize(normal);

    // Unlit 模式：直接输出 Albedo 到 Emissive 通道，标记 alpha=1
    // Unlit mode: write Albedo to Emissive channel, mark alpha=1
    if (Unlit)
    {
        output.AlbedoAO = float4(0.0f, 0.0f, 0.0f, 0.0f);
        output.NormalRoughness = float4(0.0f, 0.0f, 1.0f, 0.0f); // 有效法线以避免 Lighting Pass 跳过后台像素
        output.PositionMetallic = float4(0.0f, 0.0f, 0.0f, 0.0f);
        output.EmissiveUnused = float4(albedo, 1.0f); // alpha=1 标记为 Unlit
        return output;
    }

    // Pack into G-Buffer
    output.AlbedoAO = float4(albedo, ao);
    output.NormalRoughness = float4(normal, roughness);
    output.PositionMetallic = float4(input.WorldPosition, metallic);
    output.EmissiveUnused = float4(0.0f, 0.0f, 0.0f, 0.0f);

    return output;
}
