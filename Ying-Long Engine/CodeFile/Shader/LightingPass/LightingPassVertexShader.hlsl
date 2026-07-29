/**
 * @file LightingPassVertexShader.hlsl
 * @brief Lighting Pass vertex shader for Deferred Rendering
 *
 * Generates a full-screen triangle using SV_VertexID without any
 * vertex buffer. The triangle covers the entire viewport.
 *
 * Vertex ID 0: (-1, -1) bottom-left
 * Vertex ID 1: ( 3, -1) bottom-right (off-screen, clipped)
 * Vertex ID 2: (-1,  3) top-left (off-screen, clipped)
 *
 * UVs are computed to map (0,0) at top-left to (1,1) at bottom-right.
 */

struct LightingPassVSOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

LightingPassVSOutput main(uint vertexID : SV_VertexID)
{
    LightingPassVSOutput output;

    // Generate full-screen triangle
    float x = (vertexID == 1) ? 3.0f : -1.0f;
    float y = (vertexID == 2) ? 3.0f : -1.0f;

    output.Position = float4(x, y, 0.0f, 1.0f);
    output.TexCoord = float2(
        (x + 1.0f) * 0.5f,
        (1.0f - y) * 0.5f  // Flip Y for texture coordinates
    );

    return output;
}
