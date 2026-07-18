/**
 * @file SolidPixelShader.hlsl
 * @brief 纯色像素着色器 / Solid color pixel shader
 *
 * 该着色器直接输出常量缓冲区中定义的纯色，不进行任何光照计算。
 * 适用于调试几何体、线框渲染等简单的纯色渲染场景。
 *
 * This shader directly outputs the solid color defined in the constant buffer
 * without performing any lighting calculations. Suitable for simple solid color
 * rendering scenarios such as debug geometry and wireframe rendering.
 *
 * 输入 / Input:
 *   (无顶点属性输入，颜色来自常量缓冲区)
 *   (No vertex attribute input, color comes from constant buffer)
 *
 * 输出 / Output:
 *   - SV_Target: 最终像素颜色 / Final pixel color
 *
 * 常量缓冲区 / Constant Buffers:
 *   - b0 (PixelInput): 纯色值 / Solid color value
 */

/**
 * @brief 像素输入常量缓冲区 / Pixel input constant buffer
 *
 * 存储要输出的纯色值。
 * Stores the solid color value to output.
 *
 * Register: b0
 */
cbuffer PixelInput : register(b0)
{
    float3 color;   ///< 纯色值（RGB）/ Solid color value (RGB)
}

/**
 * @brief 纯色像素着色器主函数 / Solid color pixel shader main function
 *
 * 直接输出常量缓冲区中定义的纯色，Alpha 值设为 1.0（完全不透明）。
 *
 * Directly outputs the solid color defined in the constant buffer,
 * with alpha set to 1.0 (fully opaque).
 *
 * @return 最终像素颜色（RGBA）/ Final pixel color (RGBA)
 */
float4 main() : SV_Target
{
    // 输出纯色，Alpha 通道设为 1.0
    // Output solid color with alpha channel set to 1.0
    return float4(color, 1.0f);
}
