#include "DX12DemoScene.h"
#include <DirectXMath.h>
#include <ImGui/imgui.h>
#include "../../Application/Application.h"

using namespace DirectX;

namespace YingLong
{
    DX12DemoScene::DX12DemoScene(DX12Core& core)
        : Core(core)
        , pCamera(nullptr)
    {
        // Initialize identity matrices
        memset(ViewMatrix, 0, sizeof(ViewMatrix));
        memset(ProjectionMatrix, 0, sizeof(ProjectionMatrix));
        ViewMatrix[0] = ViewMatrix[5] = ViewMatrix[10] = ViewMatrix[15] = 1.0f;
        ProjectionMatrix[0] = ProjectionMatrix[5] = ProjectionMatrix[10] = ProjectionMatrix[15] = 1.0f;
    }

    DX12DemoScene::~DX12DemoScene() = default;

    void DX12DemoScene::SetCamera(Camera* camera)
    {
        pCamera = camera;
        if (pCamera)
        {
            UpdateCameraMatrices();
        }
    }

    void DX12DemoScene::UpdateCameraMatrices()
    {
        if (!pCamera)
            return;

        // Get camera matrices from the Camera class
        XMMATRIX viewMatrix = pCamera->GetMatrix();
        XMMATRIX projectionMatrix = pCamera->GetProjection();

        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(ViewMatrix), viewMatrix);
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(ProjectionMatrix), projectionMatrix);
    }

    void DX12DemoScene::UpdateAspectRatio(float width, float height)
    {
        if (pCamera && height > 0.0f)
        {
            pCamera->SetResolution(XMFLOAT2{ width, height });
        }
    }

    void DX12DemoScene::SetLightCountData(const DX12LightCountCB& data)
    {
        for (auto& triangle : Triangles)
        {
            if (triangle)
                triangle->SetLightCountData(data);
        }
        for (auto& box : Boxes)
        {
            if (box)
                box->SetLightCountData(data);
        }
        for (auto& sphere : Spheres)
        {
            if (sphere)
                sphere->SetLightCountData(data);
        }
        for (auto& cone : WireframeCones)
        {
            if (cone)
                cone->SetLightCountData(data);
        }
    }

    void DX12DemoScene::AddTriangle(std::unique_ptr<DX12Triangle> triangle)
    {
        Triangles.push_back(std::move(triangle));
    }

    void DX12DemoScene::AddBox(std::unique_ptr<DX12Box> box)
    {
        Boxes.push_back(std::move(box));
    }

    void DX12DemoScene::AddSphere(std::unique_ptr<DX12Sphere> sphere)
    {
        Spheres.push_back(std::move(sphere));
    }

    void DX12DemoScene::AddWireframeCone(std::unique_ptr<DX12WireframeCone> cone)
    {
        WireframeCones.push_back(std::move(cone));
    }

    void DX12DemoScene::Update(float deltaTime)
    {
        // Update camera matrices
        UpdateCameraMatrices();

        // Update all triangles with camera matrices
        for (auto& triangle : Triangles)
        {
            if (triangle)
            {
                triangle->SetViewMatrix(ViewMatrix);
                triangle->SetProjectionMatrix(ProjectionMatrix);
                triangle->Update(deltaTime);
            }
        }

        // Update all boxes with camera matrices
        for (auto& box : Boxes)
        {
            if (box)
            {
                box->SetViewMatrix(ViewMatrix);
                box->SetProjectionMatrix(ProjectionMatrix);
                box->Update(deltaTime);
            }
        }

        // Update all spheres (point light visualization)
        for (auto& sphere : Spheres)
        {
            if (sphere)
            {
                sphere->SetViewMatrix(ViewMatrix);
                sphere->SetProjectionMatrix(ProjectionMatrix);
                sphere->Update(deltaTime);
            }
        }

        // Update all wireframe cones (spot light visualization)
        for (auto& cone : WireframeCones)
        {
            if (cone)
            {
                cone->SetViewMatrix(ViewMatrix);
                cone->SetProjectionMatrix(ProjectionMatrix);
                cone->Update(deltaTime);
            }
        }
    }

    void DX12DemoScene::Render(ID3D12GraphicsCommandList* commandList)
    {
        // Render all triangles
        for (auto& triangle : Triangles)
        {
            if (triangle)
                triangle->Draw(commandList);
        }

        // Render all boxes
        for (auto& box : Boxes)
        {
            if (box)
                box->Draw(commandList);
        }

        // Render all spheres (point light visualization)
        for (auto& sphere : Spheres)
        {
            if (sphere)
                sphere->Draw(commandList);
        }

        // Render all wireframe cones (spot light visualization)
        for (auto& cone : WireframeCones)
        {
            if (cone)
                cone->Draw(commandList);
        }
    }

    void DX12DemoScene::SpawnControlWindow()
    {
        ImGui::Begin("Rotation Control");

        for (size_t i = 0; i < Triangles.size(); i++)
        {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Triangle %d", (int)i))
            {
                float* speed = Triangles[i]->GetRotationSpeed();
                ImGui::SliderFloat("Pitch Speed", &speed[0], -2.0f, 2.0f);
                ImGui::SliderFloat("Yaw Speed", &speed[1], -2.0f, 2.0f);
                ImGui::SliderFloat("Roll Speed", &speed[2], -2.0f, 2.0f);
                ImGui::TreePop();
            }
        }

        for (size_t i = 0; i < Boxes.size(); i++)
        {
            if (ImGui::TreeNode((void*)(intptr_t)(i + 100), "Box %d", (int)i))
            {
                float* speed = Boxes[i]->GetRotationSpeed();
                ImGui::SliderFloat("Pitch Speed", &speed[0], -2.0f, 2.0f);
                ImGui::SliderFloat("Yaw Speed", &speed[1], -2.0f, 2.0f);
                ImGui::SliderFloat("Roll Speed", &speed[2], -2.0f, 2.0f);
                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    void DX12DemoScene::SyncPointLightVisualization(const std::vector<DX12PointLightData>& data,
        const std::vector<DX12PointLightState>& states)
    {
        // 确保球体数量与启用的点光源数量匹配
        // Ensure sphere count matches enabled point light count
        size_t enabledCount = data.size();

        // 等待 GPU 完成后再修改资源，防止 DEVICE_HUNG
        // Wait for GPU before modifying resources to prevent DEVICE_HUNG
        if (Spheres.size() != enabledCount)
        {
            Core.WaitForGPU();
        }

        // 如果球体数量不够，创建新的球体
        // If sphere count is insufficient, create new spheres
        while (Spheres.size() < enabledCount)
        {
            Spheres.push_back(std::make_unique<DX12Sphere>(Core));
        }
        // 如果球体数量过多，删除多余的
        // If sphere count is too many, remove extras
        while (Spheres.size() > enabledCount)
        {
            Spheres.pop_back();
        }

        // 同步球体位置和颜色
        // Sync sphere positions and colors
        size_t sphereIdx = 0;
        for (size_t i = 0; i < states.size() && sphereIdx < Spheres.size(); ++i)
        {
            if (!states[i].Enabled)
                continue;

            auto& sphere = Spheres[sphereIdx];
            if (sphere)
            {
                // 球体位置 = 点光源位置
                // Sphere position = point light position
                sphere->SetPosition(states[i].Position.x, states[i].Position.y, states[i].Position.z);
                // 球体颜色 = 点光源颜色（设置亮度使球体可见）
                // Sphere color = point light color (with brightness for visibility)
                sphere->SetColor(states[i].Color.x, states[i].Color.y, states[i].Color.z, 1.0f);
                sphere->SetRotationSpeed(0.0f, 0.0f, 0.0f);
            }
            sphereIdx++;
        }
    }

    void DX12DemoScene::SyncSpotLightVisualization(const std::vector<DX12SpotLightData>& data,
        const std::vector<DX12SpotLightState>& states)
    {
        // 确保锥体数量与启用的聚光灯数量匹配
        // Ensure cone count matches enabled spot light count
        size_t enabledCount = data.size();

        // 如果锥体数量不匹配，需要等待 GPU 完成当前帧后再修改资源
        // 防止销毁 GPU 仍在引用的资源导致 DEVICE_HUNG
        // If cone count doesn't match, wait for GPU to finish current frame before modifying resources
        // Prevents destroying resources still referenced by GPU, which would cause DEVICE_HUNG
        if (WireframeCones.size() != enabledCount)
        {
            Core.WaitForGPU();
        }

        // 如果锥体数量不够，创建新的锥体
        // If cone count is insufficient, create new cones
        while (WireframeCones.size() < enabledCount)
        {
            // 默认锥体：高度 3.0，半径 0.5（稍后根据角度调整）
            // Default cone: height 3.0, radius 0.5 (adjusted by angle later)
            WireframeCones.push_back(std::make_unique<DX12WireframeCone>(Core, 3.0f, 0.5f, 32));
        }
        // 如果锥体数量过多，删除多余的
        // If cone count is too many, remove extras
        while (WireframeCones.size() > enabledCount)
        {
            WireframeCones.pop_back();
        }

        // 同步锥体位置、方向和锥角
        // Sync cone positions, orientations, and cone angles
        size_t coneIdx = 0;
        for (size_t i = 0; i < states.size() && coneIdx < WireframeCones.size(); ++i)
        {
            if (!states[i].Enabled)
                continue;

            auto& cone = WireframeCones[coneIdx];
            if (cone)
            {
                // 锥体位置 = 聚光灯位置（锥体顶点在光源位置）
                // Cone position = spot light position (cone apex at light position)
                cone->SetPosition(states[i].Position.x, states[i].Position.y, states[i].Position.z);

                // 方向：聚光灯的 Rotation 以度为单位，转换为弧度
                // 锥体默认沿 +Z 方向，需要旋转到聚光灯方向
                // Direction: spot light Rotation is in degrees, convert to radians
                // Cone default is along +Z, need to rotate to spot light direction
                float pitchRad = states[i].Rotation.x / 360.0f * XM_2PI;
                float yawRad   = states[i].Rotation.y / 360.0f * XM_2PI;
                float rollRad  = states[i].Rotation.z / 360.0f * XM_2PI;
                cone->SetRotation(pitchRad, yawRad, rollRad);

                // 锥角：根据外锥角计算锥体高度和底面半径
                // 锥体可视长度固定为 3.0 单位
                // Cone angle: calculate height and radius from outer cone angle
                // Visual cone length is fixed at 3.0 units
                const float visualLength = 3.0f;
                float halfAngle = states[i].OuterConeAngle;
                float radius = visualLength * tanf(halfAngle);
                cone->UpdateAngle(visualLength, radius);

                // 锥体颜色 = 聚光灯颜色
                // Cone color = spot light color
                cone->SetColor(states[i].Color.x, states[i].Color.y, states[i].Color.z, 1.0f);
                cone->SetRotationSpeed(0.0f, 0.0f, 0.0f);
            }
            coneIdx++;
        }
    }
}
