#pragma once

#include <memory>
#include <vector>
#include "DX12Core.h"
#include "RenderTargetDX12.h"
#include "DepthStencilDX12.h"
#include "DX12PipelineState.h"
#include "DX12Drawable.h"
#include "DX12Primitives.h"
#include "GBuffer.h"
#include "ImGuiDX12.h"
#include "../Camera/Camera.h"

namespace YingLong
{
    class DX12Renderer
    {
    public:
        DX12Renderer();
        ~DX12Renderer();

        void Initialize(HWND hWnd, int width, int height);
        void Shutdown();

        void BeginFrame(const float clearColor[4] = nullptr);
        void BeginImGuiFrame();
        void EndImGuiFrame();
        void EndFrame();

        void Draw(DX12Drawable& drawable);
        void Draw(const std::vector<DX12Drawable*>& drawables);

        void SetCamera(Camera* camera);
        Camera* GetCamera() const noexcept { return pCamera; }

        DX12Core* GetCore() const noexcept { return pCore.get(); }
        RenderTargetDX12* GetRenderTarget() const noexcept;
        DepthStencilDX12* GetDepthStencil() const noexcept { return pDepthStencil.get(); }
        ImGuiDX12* GetImGui() const noexcept { return pImGui.get(); }
        GBuffer* GetGBuffer() const noexcept { return pGBuffer.get(); }

        void Resize(int width, int height);
        int GetWidth() const noexcept { return Width; }
        int GetHeight() const noexcept { return Height; }
        bool IsInitialized() const noexcept { return bInitialized; }

        void SetClearColor(const float color[4]);
        const float* GetClearColor() const noexcept { return ClearColor; }

        void WaitForGPU();

        void BeginSceneRender(const float clearColor[4] = nullptr);
        void EndSceneRender();
        D3D12_GPU_DESCRIPTOR_HANDLE GetSceneSRVHandle() const noexcept;
        int GetSceneWidth() const noexcept { return SceneWidth; }
        int GetSceneHeight() const noexcept { return SceneHeight; }
        void UpdateSceneSize(int width, int height);

        /// Enable/disable deferred rendering for subsequent BeginSceneRender/EndSceneRender calls.
        /// When enabled, BeginSceneRender performs the Geometry Pass (writes to G-Buffer),
        /// and EndSceneRender performs the Lighting Pass (reads G-Buffer, writes lit result to scene RT).
        /// When disabled, uses traditional forward rendering.
        void SetUseDeferredRendering(bool enabled) noexcept { bUseDeferredRendering = enabled; }
        bool IsDeferredRenderingEnabled() const noexcept { return bUseDeferredRendering; }

        /// Set light count and camera position data for the deferred rendering Lighting Pass.
        /// The data is uploaded to an internal constant buffer bound to root parameter 0 (b0)
        /// during ExecuteLightingPass(). When deferred rendering is enabled, this should be
        /// called once per frame before EndSceneRender() to provide camera position and light
        /// counts to the Lighting Pass pixel shader.
        /// @param data Light count constant buffer data (camera position + light counts)
        void SetLightCountData(const DX12LightCountCB& data);

    private:
        static constexpr int FRAME_COUNT = 2;

        std::unique_ptr<DX12Core> pCore;
        std::unique_ptr<RenderTargetDX12> pRenderTargets[FRAME_COUNT];
        std::unique_ptr<RenderTargetDX12> pSceneRenderTarget;
        std::unique_ptr<DepthStencilDX12> pDepthStencil;
        std::unique_ptr<DepthStencilDX12> pSceneDepthStencil;
        std::unique_ptr<GBuffer> pGBuffer;
        std::unique_ptr<ImGuiDX12> pImGui;

        // Light count constant buffer used by the deferred rendering Lighting Pass.
        // Lazily created on first SetLightCountData() call.
        std::unique_ptr<ConstantBufferDX12<DX12LightCountCB>> pLightCountBuffer;

        Camera* pCamera;

        int Width;
        int Height;
        int SceneWidth;
        int SceneHeight;

        float ClearColor[4];

        bool bInitialized;
        bool bInFrame;
        bool bInImGuiFrame;
        bool bInGeometryPass;
        bool bUseDeferredRendering;
        HWND hWnd;

        bool bNeedsResize;
        int PendingWidth;
        int PendingHeight;

        bool bNeedsSceneResize;
        int PendingSceneWidth;
        int PendingSceneHeight;

        void ExecuteResize();
        void ExecuteSceneResize();

        // Deferred rendering internal helpers
        void BeginGeometryPass(const float clearColor[4]);
        void EndGeometryPass();
        void ExecuteLightingPass();
    };
}
