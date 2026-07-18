#pragma kernel TileLightCulling

static const int TILE_SIZE = 16;
static const int MAX_LIGHTS_PER_TILE = 64;
static const int MAX_POINT_LIGHTS = 1000;
static const int MAX_SPOT_LIGHTS = 1000;

struct PointLight
{
    float3 Position;
    float pad0;
    float3 Color;
    float Intensity;
};

struct SpotLight
{
    float3 Position;
    float Intensity;
    float3 Color;
    float InnerConeAngle;
    float3 Direction;
    float OuterConeAngle;
};

cbuffer LightCountConstantBuffer : register(b0)
{
    int PointLightCount;
    int SpotLightCount;
    float3 CameraPosition;
};

cbuffer TileCullingConstantBuffer : register(b1)
{
    float4x4 ViewProj;
    float4x4 InvViewProj;
    float2 ScreenSize;
};

StructuredBuffer<PointLight> PointLightBuffer : register(t0);
StructuredBuffer<SpotLight> SpotLightBuffer : register(t1);

RWStructuredBuffer<int> PointLightTileIndices : register(u0);
RWStructuredBuffer<int> SpotLightTileIndices : register(u1);
RWStructuredBuffer<int> TileLightCounts : register(u2);

float3 ScreenToWorld(float2 screenPos, float depth)
{
    float4 clipPos = float4(
        (screenPos.x / ScreenSize.x) * 2.0f - 1.0f,
        -(screenPos.y / ScreenSize.y) * 2.0f + 1.0f,
        depth,
        1.0f
    );
    
    float4 worldPos = mul(InvViewProj, clipPos);
    return worldPos.xyz / worldPos.w;
}

bool PointLightAABBTest(float3 lightPos, float lightRadius, float3 tileMin, float3 tileMax)
{
    float3 closestPoint = clamp(lightPos, tileMin, tileMax);
    float distanceSq = dot(closestPoint - lightPos, closestPoint - lightPos);
    return distanceSq < lightRadius * lightRadius;
}

[numthreads(1, 1, 1)]
void TileLightCulling(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint numTilesX = uint((ScreenSize.x + TILE_SIZE - 1) / TILE_SIZE);
    uint numTilesY = uint((ScreenSize.y + TILE_SIZE - 1) / TILE_SIZE);
    
    uint tileIndex = dispatchThreadId.x;
    if (tileIndex >= numTilesX * numTilesY)
        return;
    
    uint tileX = tileIndex % numTilesX;
    uint tileY = tileIndex / numTilesX;
    
    float tileLeft = float(tileX) * TILE_SIZE;
    float tileTop = float(tileY) * TILE_SIZE;
    float tileRight = min(tileLeft + TILE_SIZE, ScreenSize.x);
    float tileBottom = min(tileTop + TILE_SIZE, ScreenSize.y);
    
    float3 tileNearMin = ScreenToWorld(float2(tileLeft, tileTop), 0.0f);
    float3 tileNearMax = ScreenToWorld(float2(tileRight, tileBottom), 0.0f);
    float3 tileFarMin = ScreenToWorld(float2(tileLeft, tileTop), 1.0f);
    float3 tileFarMax = ScreenToWorld(float2(tileRight, tileBottom), 1.0f);
    
    float3 tileMin = min(min(tileNearMin, tileNearMax), min(tileFarMin, tileFarMax));
    float3 tileMax = max(max(tileNearMin, tileNearMax), max(tileFarMin, tileFarMax));
    
    int pointLightCount = 0;
    int spotLightCount = 0;
    
    for (int i = 0; i < PointLightCount; i++)
    {
        PointLight light = PointLightBuffer[i];
        float lightRadius = sqrt(light.Intensity / 0.01f);
        
        if (PointLightAABBTest(light.Position, lightRadius, tileMin, tileMax))
        {
            if (pointLightCount < MAX_LIGHTS_PER_TILE)
            {
                PointLightTileIndices[tileIndex * MAX_LIGHTS_PER_TILE + pointLightCount] = i;
                pointLightCount++;
            }
        }
    }
    
    for (int i = 0; i < SpotLightCount; i++)
    {
        SpotLight light = SpotLightBuffer[i];
        float lightRadius = sqrt(light.Intensity / 0.01f);
        
        if (PointLightAABBTest(light.Position, lightRadius, tileMin, tileMax))
        {
            if (spotLightCount < MAX_LIGHTS_PER_TILE)
            {
                SpotLightTileIndices[tileIndex * MAX_LIGHTS_PER_TILE + spotLightCount] = i;
                spotLightCount++;
            }
        }
    }
    
    TileLightCounts[tileIndex * 2] = pointLightCount;
    TileLightCounts[tileIndex * 2 + 1] = spotLightCount;
}