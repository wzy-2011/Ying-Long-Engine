#pragma once

#include <memory>
#include <vector>
#include "DX12Core.h"
#include "RenderTargetDX12.h"
#include "DepthStencilDX12.h"
#include "DX12PipelineState.h"
#include "DX12Drawable.h"
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

    private:
        static constexpr int FRAME_COUNT = 2;

        std::unique_ptr<DX12Core> pCore;
        std::unique_ptr<RenderTargetDX12> pRenderTargets[FRAME_COUNT];
        std::unique_ptr<RenderTargetDX12> pSceneRenderTarget;
        std::unique_ptr<DepthStencilDX12> pDepthStencil;
        std::unique_ptr<DepthStencilDX12> pSceneDepthStencil;
        std::unique_ptr<ImGuiDX12> pImGui;

        Camera* pCamera;

        int Width;
        int Height;
        int SceneWidth;
        int SceneHeight;

        float ClearColor[4];

        bool bInitialized;
        bool bInFrame;
        bool bInImGuiFrame;
        HWND hWnd;

        bool bNeedsResize;
        int PendingWidth;
        int PendingHeight;

        bool bNeedsSceneResize;
        int PendingSceneWidth;
        int PendingSceneHeight;

        void ExecuteResize();
        void ExecuteSceneResize();
    };
}
