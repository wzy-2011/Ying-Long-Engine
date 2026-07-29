/**
 * @file GBufferVertexShader.hlsl
 * @brief Geometry Pass vertex shader for Deferred Rendering
 *
 * Transforms vertices to clip space and passes world-space position
 * and normal to the Geometry Pass pixel shader.
 */

cbuffer TransformConstantBuffer : register(b2)
{
    matrix Model;
    matrix ModelViewProject;
}

struct GBufferVSInput
{
    float3 Position : Position;
    float3 Normal : Normal;
    float2 TextureCoord : TextureCoord;
};

struct GBufferVSOutput
{
    float3 WorldPosition : WORLD_POSITION;
    float3 Normal : NORMAL;
    float4 Position : SV_Position;
    float2 TexCoord : TextureCoord;
};

GBufferVSOutput main(GBufferVSInput input)
{
    GBufferVSOutput output;

    output.WorldPosition = (float3)mul(float4(input.Position, 1.0f), Model);
    output.Normal = normalize(mul(float4(input.Normal, 0.0f), Model).xyz);
    output.Position = mul(float4(input.Position, 1.0f), ModelViewProject);
    output.TexCoord = input.TextureCoord;

    return output;
}
