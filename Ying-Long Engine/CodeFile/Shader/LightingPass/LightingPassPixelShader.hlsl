/**
 * @file LightingPassPixelShader.hlsl
 * @brief Lighting Pass pixel shader for Deferred Rendering
 *
 * Reads surface attributes from the G-Buffer and performs PBR lighting
 * calculations using Tile-Based Light Culling. The compute shader
 * (LightCullingCS.hlsl) pre-computes which lights affect each 16x16 tile,
 * and this pixel shader iterates only those lights.
 *
 * G-Buffer inputs (SRVs):
 *   t0: Albedo + AO
 *   t1: Normal + Roughness
 *   t2: Position + Metallic
 *   t3: Emissive (reserved)
 *
 * Light data (StructuredBuffer, shared with forward path):
 *   t4-t5: PointLight + SpotLight StructuredBuffers
 *
 * Tile-Based Light Culling results:
 *   t6: LightIndexList (per-tile light indices)
 *   t7: LightCountPerTile (per-tile light counts)
 */

#include "../Lighting/Lighting.hlsli"

// Tile-Based Light Culling constants
#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 64

// G-Buffer textures
Texture2D AlbedoAOTexture : register(t0);
Texture2D NormalRoughnessTexture : register(t1);
Texture2D PositionMetallicTexture : register(t2);
Texture2D EmissiveTexture : register(t3);

// Tile-Based Light Culling output buffers
StructuredBuffer<uint> LightIndexList : register(t6);
StructuredBuffer<uint> LightCountPerTile : register(t7);

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

    // Sample G-Buffer using Load (integer pixel coordinates, no filtering)
    uint2 pixelCoord = uint2(input.Position.xy);
    float4 albedoAO = AlbedoAOTexture.Load(int3(pixelCoord, 0));
    float4 normalRoughness = NormalRoughnessTexture.Load(int3(pixelCoord, 0));
    float4 positionMetallic = PositionMetallicTexture.Load(int3(pixelCoord, 0));
    float4 emissiveData = EmissiveTexture.Load(int3(pixelCoord, 0));

    // Unlit pixels: output Emissive color directly (point light indicator spheres, etc.)
    if (emissiveData.a > 0.5f)
    {
        output.FinalColor = float4(emissiveData.rgb, 1.0f);
        return output;
    }

    float3 albedo = albedoAO.rgb;
    float ao = albedoAO.a;

    float3 normal = normalRoughness.xyz;
    float roughness = normalRoughness.w;

    float3 worldPosition = positionMetallic.xyz;
    float metallic = positionMetallic.w;

    // Skip pixels with no geometry (normal = 0 means background)
    if (length(normal) < 0.001f)
    {
        output.FinalColor = float4(0.1f, 0.1f, 0.1f, 1.0f);
        return output;
    }

    normal = normalize(normal);

    // Build PBR material
    PBRMaterial material;
    material.Albedo = albedo;
    material.Metallic = metallic;
    material.Roughness = roughness;
    material.AmbientOcclusion = ao;

    // ========================================================================
    // Tile-Based Light Culling: iterate only lights affecting this tile
    // 基于 Tile 的光源剔除：仅遍历影响当前 Tile 的光源
    // ========================================================================
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // Compute tile index from pixel position
    uint2 tileCoord = pixelCoord / TILE_SIZE;

    // Calculate tilesX from screen dimensions
    // Screen dimensions come from the G-Buffer texture dimensions
    uint2 screenDim;
    AlbedoAOTexture.GetDimensions(screenDim.x, screenDim.y);
    uint tilesX = (screenDim.x + TILE_SIZE - 1) / TILE_SIZE;
    uint tileIndex = tileCoord.y * tilesX + tileCoord.x;

    // Read light list for this tile
    uint lightCount = LightCountPerTile[tileIndex];
    uint baseIndex = tileIndex * MAX_LIGHTS_PER_TILE;

    for (uint i = 0; i < lightCount; i++)
    {
        uint lightIdx = LightIndexList[baseIndex + i];

        if (lightIdx < (uint)PointLightCount)
        {
            // Point light
            Lo += PBRPointLightDirectLight(
                PointLightBuffer[lightIdx],
                material, normal, worldPosition, CameraPosition);
        }
        else
        {
            // Spot light (index offset by PointLightCount)
            uint spotIdx = lightIdx - (uint)PointLightCount;
            if (spotIdx < (uint)SpotLightCount)
            {
                Lo += PBRSpotLightDirectLight(
                    SpotLightBuffer[spotIdx],
                    material, normal, worldPosition, CameraPosition);
            }
        }
    }

    // Ambient term
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * material.Albedo * material.AmbientOcclusion;

    float3 Color = ambient + Lo;
    // HDR tonemapping (Reinhard)
    Color = Color / (Color + float3(1.0f, 1.0f, 1.0f));
    // Gamma correction
    Color = pow(Color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    output.FinalColor = float4(Color, 1.0f);
    return output;
}