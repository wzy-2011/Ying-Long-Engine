/**
 * @file DX12Types.h
 * @brief DX12 公共类型定义 / DX12 Common Type Definitions
 *
 * 本文件定义了 DX12 渲染器中使用的通用数据结构，包括顶点格式、
 * 变换矩阵、光源数据、材质参数等常量缓冲区结构。
 * 从 DX12Primitives.h 中提取，以降低头文件耦合度。
 *
 * This file defines common data structures used by the DX12 renderer,
 * including vertex format, transform matrices, light data, material
 * parameters, and other constant buffer structures.
 * Extracted from DX12Primitives.h to reduce header coupling.
 */
#pragma once

#include <cstdint>

namespace YingLong
{
    /**
     * @brief DX12 简单顶点结构 / DX12 Simple Vertex Structure
     *
     * 包含位置、法线和纹理坐标的顶点结构，用于简单几何图元。
     * Vertex structure containing position, normal, and texture coordinate,
     * used for simple geometric primitives.
     */
    struct DX12Vertex
    {
        float Position[3];     ///< 顶点位置 / Vertex position
        float Normal[3];       ///< 顶点法线 / Vertex normal
        float TexCoord[2];     ///< 纹理坐标 / Texture coordinate
    };

    /**
     * @brief DX12 变换矩阵结构 / DX12 Transform Matrix Structure
     *
     * 与 PBR 顶点着色器 cbuffer 布局匹配的变换矩阵结构，
     * 包含模型矩阵和 MVP 矩阵。
     *
     * Transform matrix structure matching the PBR vertex shader cbuffer layout,
     * containing the model matrix and MVP matrix.
     */
    struct DX12Transform
    {
        float ModelMatrix[16];         ///< 模型矩阵 / Model matrix
        float ModelViewProjMatrix[16]; ///< 模型视图投影矩阵 / Model-View-Projection matrix
    };

    /**
     * @brief DX12 点光源数据结构 / DX12 Point Light Data Structure
     *
     * 与 HLSL PointLightBuffer 结构匹配。
     * Matches the HLSL PointLightBuffer structure.
     */
    struct DX12PointLightData
    {
        float Position[3];
        float pad0;
        float Color[3];
        float Intensity;
    };

    /**
     * @brief DX12 聚光源数据结构 / DX12 Spot Light Data Structure
     *
     * 与 HLSL SpotLightBuffer 结构匹配。
     * Matches the HLSL SpotLightBuffer structure.
     */
    struct DX12SpotLightData
    {
        float Position[3];
        float Intensity;
        float Color[3];
        float InnerConeAngle;
        float Direction[3];
        float OuterConeAngle;
        float Rotation[3];   // 未使用，仅用于匹配 HLSL struct 大小
        float pad;           // 未使用，仅用于匹配 HLSL struct 大小
    };

    /**
     * @brief DX12 光源计数常量缓冲区结构 / DX12 Light Count Constant Buffer Structure
     *
     * 与 PBR 像素着色器 b0 寄存器匹配。
     * Matches the PBR pixel shader b0 register.
     */
    struct DX12LightCountCB
    {
        int PointLightCount;
        int SpotLightCount;
        float pad[2];
        float CameraPosition[3];
        float pad0;
    };

    /**
     * @brief 光源剔除常量缓冲区结构 / Light Culling Constant Buffer Structure
     *
     * 与 LightCullingCS.hlsl 的 b0 寄存器匹配。
     * Matches the b0 register of LightCullingCS.hlsl.
     */
    struct LightCullingConstantsCB
    {
        uint32_t ScreenWidth;
        uint32_t ScreenHeight;
        uint32_t PointLightCount;
        uint32_t SpotLightCount;
        float ViewProjMatrix[16];  ///< 视图投影矩阵（列主序，转置后）
    };

    /**
     * @brief DX12 材质常量缓冲区结构 / DX12 Material Constant Buffer Structure
     *
     * 与 PBR 像素着色器 b2 寄存器匹配的材质常量缓冲区结构。
     * Matches the PBR pixel shader b2 register.
     */
    struct DX12MaterialCB
    {
        float Albedo[3];            ///< 反照率颜色 / Albedo color
        float Metallic;              ///< 金属度 / Metallic
        float Roughness;             ///< 粗糙度 / Roughness
        float AmbientOcclusion;      ///< 环境光遮蔽 / Ambient occlusion
        int UseAlbedoTexture;        ///< 是否使用反照率纹理 / Whether to use albedo texture
        int UseRoughnessTexture;     ///< 是否使用粗糙度纹理 / Whether to use roughness texture
        int UseMetallicTexture;      ///< 是否使用金属度纹理 / Whether to use metallic texture
        int UseNormalTexture;        ///< 是否使用法线纹理 / Whether to use normal texture
        int UseAOTexture;            ///< 是否使用 AO 纹理 / Whether to use AO texture
        int Unlit;                   ///< 是否无光照（直接输出 Albedo）/ Whether unlit (output Albedo directly)
        float pad[2];                ///< 填充（16字节对齐）/ Padding (16-byte alignment)
    };
}