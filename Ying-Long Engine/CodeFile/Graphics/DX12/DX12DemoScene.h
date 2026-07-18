/**
 * @file DX12DemoScene.h
 * @brief DX12 演示场景 / DX12 Demo Scene
 *
 * 简单的 DX12 测试场景，用于验证渲染管线是否正常工作。
 * 包含三角形、立方体等基本图元，支持相机控制和灯光数据传递。
 *
 * Simple DX12 test scene for verifying the render pipeline works correctly.
 * Contains basic primitives like triangles and boxes, supports camera control
 * and light data propagation.
 *
 * @note 这是演示/测试用场景。实际项目中应使用 ECS Scene + MeshRendererSystem。
 *       This is a demo/test scene. Real projects should use ECS Scene + MeshRendererSystem.
 */
#pragma once

#include "DX12Primitives.h"
#include "../Camera/Camera.h"

struct ImGuiContext;

namespace YingLong
{
    /**
     * @brief DX12 演示场景类 / DX12 Demo Scene Class
     *
     * 用于测试 DX12 渲染的简单场景，包含三角形和立方体图元。
     * 提供相机控制、灯光数据传递和 ImGui 调试面板。
     *
     * Simple scene for testing DX12 rendering, contains triangle and box primitives.
     * Provides camera control, light data propagation, and ImGui debug panel.
     */
    class DX12DemoScene
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         * @param core DX12 核心对象引用 / DX12 core object reference
         */
        DX12DemoScene(DX12Core& core);

        /**
         * @brief 析构函数 / Destructor
         */
        ~DX12DemoScene();

        /**
         * @brief 更新场景 / Update scene
         * @param deltaTime 增量时间（秒） / Delta time in seconds
         */
        void Update(float deltaTime);

        /**
         * @brief 渲染场景 / Render scene
         * @param commandList 图形命令列表指针 / Graphics command list pointer
         */
        void Render(ID3D12GraphicsCommandList* commandList);

        /**
         * @brief 设置相机 / Set camera
         * @param camera 相机指针 / Camera pointer
         */
        void SetCamera(Camera* camera);

        /**
         * @brief 更新相机矩阵 / Update camera matrices
         *
         * 从当前相机重新计算视图矩阵和投影矩阵，并传递给所有图元。
         * Recalculates view and projection matrices from current camera
         * and propagates to all primitives.
         */
        void UpdateCameraMatrices();

        /**
         * @brief 更新宽高比 / Update aspect ratio
         * @param width 宽度 / Width
         * @param height 高度 / Height
         */
        void UpdateAspectRatio(float width, float height);

        /**
         * @brief 设置光源计数和相机位置数据 / Set light count and camera position data
         *
         * 将光源计数和相机位置数据传递给场景中所有图元。
         * Propagates light count and camera position data to all primitives in the scene.
         *
         * @param data 光源计数常量缓冲区数据 / Light count constant buffer data
         */
        void SetLightCountData(const DX12LightCountCB& data);

        /**
         * @brief 生成 ImGui 控制面板 / Spawn ImGui control window
         */
        void SpawnControlWindow();

        /**
         * @brief 添加三角形图元 / Add triangle primitive
         * @param triangle 三角形唯一指针 / Triangle unique pointer
         */
        void AddTriangle(std::unique_ptr<DX12Triangle> triangle);

        /**
         * @brief 添加立方体图元 / Add box primitive
         * @param box 立方体唯一指针 / Box unique pointer
         */
        void AddBox(std::unique_ptr<DX12Box> box);

        /**
         * @brief 添加球体图元（用于点光源可视化）/ Add sphere primitive (for point light visualization)
         * @param sphere 球体唯一指针 / Sphere unique pointer
         */
        void AddSphere(std::unique_ptr<DX12Sphere> sphere);

        /**
         * @brief 添加锥体线框图元（用于聚光灯可视化）
         *        Add wireframe cone primitive (for spot light visualization)
         * @param cone 锥体线框唯一指针 / Wireframe cone unique pointer
         */
        void AddWireframeCone(std::unique_ptr<DX12WireframeCone> cone);

        /**
         * @brief 同步点光源可视化 / Sync point light visualization
         *
         * 根据点光源数据更新球体的位置和颜色，使其与实际点光源参数实时同步。
         * Updates sphere positions and colors based on point light data,
         * keeping them in real-time sync with actual point light parameters.
         *
         * @param data 点光源数据向量 / Point light data vector
         * @param states 点光源状态列表 / Point light state list
         */
        void SyncPointLightVisualization(const std::vector<DX12PointLightData>& data,
            const std::vector<struct DX12PointLightState>& states);

        /**
         * @brief 同步聚光灯可视化 / Sync spot light visualization
         *
         * 根据聚光灯数据更新锥体线框的位置、方向和锥角，使其与实际聚光灯参数实时同步。
         * Updates wireframe cone positions, orientations, and angles based on spot light data,
         * keeping them in real-time sync with actual spot light parameters.
         *
         * @param data 聚光灯数据向量 / Spot light data vector
         * @param states 聚光灯状态列表 / Spot light state list
         */
        void SyncSpotLightVisualization(const std::vector<DX12SpotLightData>& data,
            const std::vector<struct DX12SpotLightState>& states);

        /**
         * @brief 获取球体图元数量 / Get sphere primitive count
         * @return size_t 球体数量 / Sphere count
         */
        size_t GetSphereCount() const { return Spheres.size(); }

        /**
         * @brief 获取锥体线框图元数量 / Get wireframe cone count
         * @return size_t 锥体线框数量 / Wireframe cone count
         */
        size_t GetWireframeConeCount() const { return WireframeCones.size(); }

    private:
        DX12Core& Core;   ///< DX12 核心对象引用 / DX12 core object reference
        Camera* pCamera;  ///< 相机指针 / Camera pointer

        // Scene objects / 场景对象
        std::vector<std::unique_ptr<DX12Triangle>> Triangles;  ///< 三角形图元列表 / Triangle primitive list
        std::vector<std::unique_ptr<DX12Box>> Boxes;           ///< 立方体图元列表 / Box primitive list
        std::vector<std::unique_ptr<DX12Sphere>> Spheres;      ///< 球体图元列表（点光源可视化）/ Sphere primitive list (point light visualization)
        std::vector<std::unique_ptr<DX12WireframeCone>> WireframeCones;  ///< 锥体线框图元列表（聚光灯可视化）/ Wireframe cone primitive list (spot light visualization)

        // Camera matrices / 相机矩阵
        float ViewMatrix[16];        ///< 视图矩阵 / View matrix
        float ProjectionMatrix[16];  ///< 投影矩阵 / Projection matrix
    };
}
