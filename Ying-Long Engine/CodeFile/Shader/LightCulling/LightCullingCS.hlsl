/**
 * @file LightCullingCS.hlsl
 * @brief Tile-Based Light Culling compute shader for Deferred Rendering
 *
 * Divides the screen into 16x16 pixel tiles and determines which lights
 * affect each tile. This dramatically reduces the per-pixel light
 * iteration cost in the Lighting Pass by eliminating lights that do
 * not contribute to the current tile.
 *
 * Culling method: Transform light position to clip space, compute
 * screen-space bounding sphere, and check overlap with tile bounds.
 */

#include "../Lighting/Lighting.hlsli"

// Constants
#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 64

// Light culling constants
cbuffer LightCullingConstants : register(b0)
{
    uint2 ScreenDimensions;     // Screen width and height
    uint PointLightCountLocal;
    uint SpotLightCountLocal;
    float4x4 ViewProjMatrix;    // View * Projection matrix
};

// Light data buffers (shared with forward/deferred rendering)
StructuredBuffer<PointLight> PointLightBufferCS : register(t4);
StructuredBuffer<SpotLight> SpotLightBufferCS : register(t5);

// Output: per-tile light index list
// Each tile has MAX_LIGHTS_PER_TILE slots
RWStructuredBuffer<uint> LightIndexList : register(u0);

// Output: per-tile light count
RWStructuredBuffer<uint> LightCountPerTile : register(u1);

// Compute the influence radius for a spotlight (world-space bounding sphere)
float GetSpotLightInfluenceRadius(SpotLight light)
{
    float threshold = 0.001f;
    return sqrt(light.Intensity / threshold);
}

float GetPointLightInfluenceRadius(PointLight light)
{
    float threshold = 0.001f;
    return sqrt(light.Intensity / threshold);
}

// Check if a light's screen-space bounding sphere overlaps with a tile
// Uses conservative screen-space projection for culling
bool LightOverlapsTile(float3 worldPos, float worldRadius,
    float2 tileMin, float2 tileMax, float4x4 viewProj)
{
    // Transform light position to clip space
    float4 clipPos = mul(float4(worldPos, 1.0f), viewProj);

    // Behind camera or too far
    if (clipPos.w <= 0.0f)
        return false;

    // Compute NDC coordinates
    float3 ndc = clipPos.xyz / clipPos.w;

    // Screen-space center (0 to ScreenDimensions)
    float2 screenCenter = (ndc.xy * 0.5f + 0.5f) * (float2)ScreenDimensions;

    // Approximate screen-space radius:
    // At distance = clipPos.w, a sphere of radius worldRadius
    // projects to approximately worldRadius * screenHeight / (2 * clipPos.w)
    // This is a conservative overestimate for culling purposes
    float screenRadius = worldRadius * (float)ScreenDimensions.y / (2.0f * clipPos.w);

    // Check overlap with tile bounds
    if (screenCenter.x + screenRadius < tileMin.x ||
        screenCenter.x - screenRadius > tileMax.x ||
        screenCenter.y + screenRadius < tileMin.y ||
        screenCenter.y - screenRadius > tileMax.y)
    {
        return false;
    }

    return true;
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID,
    uint groupIndex : SV_GroupIndex)
{
    // Only thread 0 in each group does the light culling
    if (groupIndex != 0)
        return;

    uint2 tileCoord = groupID.xy;
    uint tilesX = (ScreenDimensions.x + TILE_SIZE - 1) / TILE_SIZE;
    uint tileIndex = tileCoord.y * tilesX + tileCoord.x;

    // Compute tile bounds in screen space
    float2 tileMin = (float2)tileCoord * (float)TILE_SIZE;
    float2 tileMax = tileMin + (float)TILE_SIZE;

    uint lightCount = 0;
    uint baseIndex = tileIndex * MAX_LIGHTS_PER_TILE;

    // Cull point lights
    for (uint i = 0; i < PointLightCountLocal && lightCount < MAX_LIGHTS_PER_TILE; i++)
    {
        PointLight light = PointLightBufferCS[i];
        float radius = GetPointLightInfluenceRadius(light);

        if (LightOverlapsTile(light.Position, radius, tileMin, tileMax, ViewProjMatrix))
        {
            LightIndexList[baseIndex + lightCount] = i;
            lightCount++;
        }
    }

    // Cull spot lights (using a combined index: 0..PointLightCount-1 for point lights,
    // PointLightCount..PointLightCount+SpotLightCount-1 for spot lights)
    for (uint j = 0; j < SpotLightCountLocal && lightCount < MAX_LIGHTS_PER_TILE; j++)
    {
        SpotLight light = SpotLightBufferCS[j];
        float radius = GetSpotLightInfluenceRadius(light);

        if (LightOverlapsTile(light.Position, radius, tileMin, tileMax, ViewProjMatrix))
        {
            LightIndexList[baseIndex + lightCount] = PointLightCountLocal + j;
            lightCount++;
        }
    }

    LightCountPerTile[tileIndex] = lightCount;
}