static const float PI = 3.1415926535f;
static const float2 DOUBLE_PI = PI * 2;

struct PBRMaterial
{
    float3 Albedo;
    float Metallic;
    float Roughness;
    float AmbientOcclusion;
};

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
    float3 Rotation;
    float pad;
};

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float3 PBRPointLightDirectLight(PointLight light, PBRMaterial material,
    float3 normal, float3 worldPosition, float3 cameraPosition)
{
    float3 toLight = light.Position - worldPosition;
    float distance = length(toLight);

    [branch]
    if (distance > 100.0f)
        return float3(0.0f, 0.0f, 0.0f);

    float attenuation = 1.0f / (distance * distance);

    [branch]
    if (attenuation * light.Intensity < 0.0001f)
        return float3(0.0f, 0.0f, 0.0f);

    float3 N = normal;
    float3 V = normalize(cameraPosition - worldPosition);

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, material.Albedo, material.Metallic);

    float3 L = normalize(toLight);
    float3 H = normalize(V + L);

    float3 radiance = light.Color * light.Intensity * attenuation;

    float NDF = DistributionGGX(N, H, material.Roughness);
    float G = GeometrySmith(N, V, L, material.Roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.00001f;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - material.Metallic;

    float NdotL = max(dot(N, L), 0.0f);

    return (kD * material.Albedo / PI + specular) * radiance * NdotL;
}

float CalculateSpotLightAttenuation(SpotLight light, float3 vertexToLight)
{
    float cosAngle = dot(normalize(-vertexToLight), normalize(light.Direction));

    if (cosAngle < light.OuterConeAngle)
        return 0.0f;

    if (cosAngle > light.InnerConeAngle)
        return 1.0f;

    float delta = light.InnerConeAngle - light.OuterConeAngle;
    return (cosAngle - light.OuterConeAngle) / delta;
}

float3 PBRSpotLightDirectLight(SpotLight light, PBRMaterial material,
    float3 normal, float3 worldPosition, float3 cameraPosition)
{
    float3 toLight = light.Position - worldPosition;
    float distance = length(toLight);

    [branch]
    if (distance > 100.0f)
        return float3(0.0f, 0.0f, 0.0f);

    float distanceAttenuation = 1.0f / (distance * distance);

    float spotAttenuation = CalculateSpotLightAttenuation(light, worldPosition - light.Position);

    // 关键优化：如果像素不在聚光灯锥体内，直接跳过所有PBR计算
    // Key optimization: skip all PBR calculations if pixel is outside the spot light cone
    [branch]
    if (spotAttenuation <= 0.0f)
        return float3(0.0f, 0.0f, 0.0f);

    [branch]
    if (distanceAttenuation * spotAttenuation * light.Intensity < 0.0001f)
        return float3(0.0f, 0.0f, 0.0f);

    float3 N = normal;
    float3 V = normalize(cameraPosition - worldPosition);

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, material.Albedo, material.Metallic);

    float3 L = normalize(toLight);
    float3 H = normalize(V + L);

    float totalAttenuation = distanceAttenuation * spotAttenuation;
    float3 radiance = light.Color * light.Intensity * totalAttenuation;

    float NDF = DistributionGGX(N, H, material.Roughness);
    float G = GeometrySmith(N, V, L, material.Roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - material.Metallic;

    float NdotL = max(dot(N, L), 0.0f);

    return (kD * material.Albedo / PI + specular) * radiance * NdotL;
}

#ifdef DX11_RENDERER

// DX11 path: use constant buffers (cbuffer) to pass light data
// DX11路径：使用常量缓冲区传递光源数据
cbuffer PointLightCB : register(b3)
{
    PointLight PointLightList[50];
    int PointLightCount;
    float3 CameraPosition;
};

cbuffer SpotLightCB : register(b4)
{
    SpotLight SpotLightList[50];
    int SpotLightCount;
    float3 padding;
};

#else

// DX12 path: use StructuredBuffer + LightCountConstantBuffer
// DX12路径：使用结构化缓冲区 + 光源计数常量缓冲区
cbuffer LightCountConstantBuffer : register(b0)
{
    int PointLightCount;
    int SpotLightCount;
    float3 CameraPosition;
};

StructuredBuffer<PointLight> PointLightBuffer : register(t4);
StructuredBuffer<SpotLight> SpotLightBuffer : register(t5);

#endif

float3 Lighting(PBRMaterial material, float3 normal, float3 worldPosition)
{
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < PointLightCount; i++)
    {
#ifdef DX11_RENDERER
        Lo += PBRPointLightDirectLight(PointLightList[i], material, normal, worldPosition, CameraPosition);
#else
        Lo += PBRPointLightDirectLight(PointLightBuffer[i], material, normal, worldPosition, CameraPosition);
#endif
    }

    for (int j = 0; j < SpotLightCount; j++)
    {
#ifdef DX11_RENDERER
        Lo += PBRSpotLightDirectLight(SpotLightList[j], material, normal, worldPosition, CameraPosition);
#else
        Lo += PBRSpotLightDirectLight(SpotLightBuffer[j], material, normal, worldPosition, CameraPosition);
#endif
    }

    float3 ambient = float3(0.03f, 0.03f, 0.03f) * material.Albedo * material.AmbientOcclusion;

    float3 Color = ambient + Lo;
    Color = Color / (Color + float3(1.0f, 1.0f, 1.0f));
    Color = pow(Color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    return Color;
}
