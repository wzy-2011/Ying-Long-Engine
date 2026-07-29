/**
 * @file LightingPassPixelShader.hlsl
 * @brief Lighting Pass pixel shader for Deferred Rendering
 *
 * Reads surface attributes from the G-Buffer and performs PBR lighting
 * calculations for all lights. Outputs the final lit color to the
 * scene render target.
 *
 * G-Buffer inputs (SRVs):
 *   t0: Albedo + AO
 *   t1: Normal + Roughness
 *   t2: Position + Metallic
 *   t3: Emissive (reserved)
 *
 * Light data (StructuredBuffer, shared with forward path):
 *   t4-t5: PointLight + SpotLight StructuredBuffers
 */

#include "../Lighting/Lighting.hlsli"

// G-Buffer textures
Texture2D AlbedoAOTexture : register(t0);
Texture2D NormalRoughnessTexture : register(t1);
Texture2D PositionMetallicTexture : register(t2);
Texture2D EmissiveTexture : register(t3);

SamplerState PointClampSampler : register(s0);

struct LightingPassPSInput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

struct LightingPassPSOutput
{
    float4 FinalColor : SV_Target;
};

LightingPassPSOutput main(LightingPassPSInput input)
{
    LightingPassPSOutput output;

    // Sample G-Buffer
    float4 albedoAO = AlbedoAOTexture.Load(int3(input.Position.xy, 0));
    float4 normalRoughness = NormalRoughnessTexture.Load(int3(input.Position.xy, 0));
    float4 positionMetallic = PositionMetallicTexture.Load(int3(input.Position.xy, 0));

    float3 albedo = albedoAO.rgb;
    float ao = albedoAO.a;

    float3 normal = normalRoughness.xyz;
    float roughness = normalRoughness.w;

    float3 worldPosition = positionMetallic.xyz;
    float metallic = positionMetallic.w;

    // Skip pixels with no geometry (normal = 0 means background)
    if (length(normal) < 0.001f)
    {
        discard;
    }

    normal = normalize(normal);

    // Build PBR material
    PBRMaterial material;
    material.Albedo = albedo;
    material.Metallic = metallic;
    material.Roughness = roughness;
    material.AmbientOcclusion = ao;

    // Apply PBR lighting using the shared Lighting() function
    float3 finalColor = Lighting(material, normal, worldPosition);

    output.FinalColor = float4(finalColor, 1.0f);
    return output;
}
