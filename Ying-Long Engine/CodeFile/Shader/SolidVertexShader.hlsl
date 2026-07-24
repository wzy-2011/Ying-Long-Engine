/**
 * @file SolidVertexShader.hlsl
 * @brief 纯色顶点着色器 / Solid color vertex shader
 *
 * 该着色器用于纯色渲染，将顶点从模型空间变换到裁剪空间，
 * 并将顶点颜色直接传递给像素着色器。
 *
 * This shader is used for solid color rendering. It transforms vertices
 * from model space to clip space and passes vertex colors directly
 * to the pixel shader.
 *
 * 输入 / Input:
 *   - position: 顶点位置（模型空间）/ Vertex position (model space)
 *   - color: 顶点颜色 / Vertex color
 *
 * 输出 / Output:
 *   - position: 裁剪空间位置（SV_Position）/ Clip-space position (SV_Position)
 *   - color: 插值后的颜色 / Interpolated color
 *
 * 常量缓冲区 / Constant Buffers:
 *   - TransformCB: 变换矩阵 / Transform matrices
 */

/**
 * @brief 变换常量缓冲区 / Transform constant buffer
 *
 * 包含模型矩阵和模型-视图-投影矩阵，
 * 用于将顶点从模型空间变换到裁剪空间。
 *
 * Contains the model matrix and model-view-projection matrix
 * for transforming vertices from model space to clip space.
 */
cbuffer TransformCB : register(b2)
{
    matrix model;              ///< 模型矩阵（模型空间 -> 世界空间）/ Model matrix (model -> world space)
    matrix modelViewProject;   ///< 模型-视图-投影矩阵（模型空间 -> 裁剪空间）/ Model-View-Projection matrix (model -> clip space)
}

/**
 * @brief 顶点着色器输入结构 / Vertex shader input structure
 *
 * 定义从顶点缓冲区读取的顶点属性数据。
 * Defines the vertex attribute data read from the vertex buffer.
 */
struct VertexInput
{
    float3 position : Position;   ///< 顶点位置（模型空间）/ Vertex position (model space)
    float3 color : Color;         ///< 顶点颜色 / Vertex color
};

/**
 * @brief 像素着色器输入结构 / Pixel shader input structure
 *
 * 定义传递给像素着色器的顶点插值数据。
 * Defines the interpolated vertex data passed to the pixel shader.
 */
struct PixelInput
{
    float4 position : SV_Position;   ///< 裁剪空间位置（系统值）/ Clip-space position (system value)
    float3 color : Color;            ///< 插值后的颜色 / Interpolated color
};

/**
 * @brief 纯色顶点着色器主函数 / Solid color vertex shader main function
 *
 * 对每个顶点执行空间变换，将顶点从模型空间变换到裁剪空间，
 * 并将顶点颜色直接传递给像素着色器。
 *
 * Performs space transformation for each vertex, transforming from
 * model space to clip space, and passes vertex color directly
 * to the pixel shader.
 *
 * @param input 顶点输入数据 / Vertex input data
 * @return 顶点输出数据（插值后传递给像素着色器）/ Vertex output data (interpolated to pixel shader)
 */
PixelInput main(VertexInput input)
{
    PixelInput output;

    // 将顶点位置从模型空间变换到裁剪空间
    // Transform vertex position from model space to clip space
    output.position = mul(float4(input.position, 1.0f), modelViewProject);

    // 直接传递顶点颜色
    // Pass through vertex color
    output.color = input.color;

    return output;
}
