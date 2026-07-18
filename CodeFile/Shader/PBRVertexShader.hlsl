/**
 * @file PBRVertexShader.hlsl
 * @brief PBR 材质顶点着色器 / PBR material vertex shader
 *
 * 该着色器负责将顶点从模型空间变换到裁剪空间，
 * 并计算世界空间位置和法线，传递给像素着色器进行 PBR 光照计算。
 *
 * This shader transforms vertices from model space to clip space,
 * and computes world-space position and normal for PBR lighting
 * calculations in the pixel shader.
 *
 * 输入 / Input:
 *   - Position: 顶点位置（模型空间）/ Vertex position (model space)
 *   - Normal: 顶点法线（模型空间）/ Vertex normal (model space)
 *   - TextureCoord: 纹理坐标 / Texture coordinates
 *
 * 输出 / Output:
 *   - WorldPosition: 世界空间位置 / World-space position
 *   - Normal: 世界空间法线 / World-space normal
 *   - Position: 裁剪空间位置（SV_Position）/ Clip-space position (SV_Position)
 *   - TexCoord: 纹理坐标 / Texture coordinates
 *
 * 常量缓冲区 / Constant Buffers:
 *   - b3 (TransformConstantBuffer): 变换矩阵 / Transform matrices
 */

/**
 * @brief 变换常量缓冲区 / Transform constant buffer
 *
 * 包含模型矩阵和模型-视图-投影矩阵，
 * 用于将顶点从模型空间变换到各个坐标空间。
 *
 * Contains the model matrix and model-view-projection matrix
 * for transforming vertices from model space to various coordinate spaces.
 *
 * Register: b2
 */
cbuffer TransformConstantBuffer : register(b2)
{
    matrix Model;              ///< 模型矩阵（模型空间 -> 世界空间）/ Model matrix (model -> world space)
    matrix ModelViewProject;   ///< 模型-视图-投影矩阵（模型空间 -> 裁剪空间）/ Model-View-Projection matrix (model -> clip space)
}

/**
 * @brief PBR 顶点着色器输入结构 / PBR vertex shader input structure
 *
 * 定义从顶点缓冲区读取的顶点属性数据。
 * Defines the vertex attribute data read from the vertex buffer.
 */
struct PBRVertexShaderInput
{
    float3 Position : Position;           ///< 顶点位置（模型空间）/ Vertex position (model space)
    float3 Normal : Normal;               ///< 顶点法线（模型空间）/ Vertex normal (model space)
    float2 TextureCoord : TextureCoord;   ///< 纹理坐标 / Texture coordinates
};

/**
 * @brief PBR 顶点着色器输出结构 / PBR vertex shader output structure
 *
 * 定义传递给像素着色器的顶点插值数据。
 * Defines the interpolated vertex data passed to the pixel shader.
 */
struct PBRVertexShaderOutput
{
    float3 WorldPosition : POSITION;   ///< 世界空间位置 / World-space position
    float3 Normal : NORMAL;            ///< 世界空间法线 / World-space normal
    float4 Position : SV_Position;     ///< 裁剪空间位置（系统值）/ Clip-space position (system value)
    float2 TexCoord : TextureCoord;    ///< 纹理坐标 / Texture coordinates
};

/**
 * @brief PBR 顶点着色器主函数 / PBR vertex shader main function
 *
 * 对每个顶点执行空间变换，计算世界空间位置、法线和裁剪空间位置，
 * 并将纹理坐标直接传递给像素着色器。
 *
 * Performs space transformations for each vertex, computing world-space
 * position, normal, and clip-space position, and passes texture coordinates
 * directly to the pixel shader.
 *
 * @param InputData 顶点输入数据 / Vertex input data
 * @return 顶点输出数据（插值后传递给像素着色器）/ Vertex output data (interpolated to pixel shader)
 */
PBRVertexShaderOutput main(PBRVertexShaderInput InputData)
{
    PBRVertexShaderOutput VertexShaderOutObject;

    // 将顶点位置从模型空间变换到世界空间
    // Transform vertex position from model space to world space
    VertexShaderOutObject.WorldPosition = (float3) mul(float4(InputData.Position, 1.0f), Model);

    // 将法线从模型空间变换到世界空间并归一化
    // Transform normal from model space to world space and normalize
    VertexShaderOutObject.Normal = normalize(mul(float4(InputData.Normal, 0.0f), Model).xyz);

    // 将顶点位置从模型空间直接变换到裁剪空间
    // Transform vertex position directly from model space to clip space
    VertexShaderOutObject.Position = mul(float4(InputData.Position, 1.0f), ModelViewProject);

    // 直接传递纹理坐标
    // Pass through texture coordinates
    VertexShaderOutObject.TexCoord = InputData.TextureCoord;

    return VertexShaderOutObject;
}
