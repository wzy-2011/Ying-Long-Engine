#include "imgui_impl_dx12.h"
#include "imgui_internal.h"
#include <d3d12.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

// Static state
static ID3D12Device*                g_pd3dDevice = nullptr;
static ID3D12GraphicsCommandList*   g_pd3dCommandList = nullptr;
static D3D12_CPU_DESCRIPTOR_HANDLE  g_hCpuDescriptorHandle = {};
static D3D12_GPU_DESCRIPTOR_HANDLE  g_hGpuDescriptorHandle = {};
static int                          g_NumFramesInFlight = 3;
static int                          g_CurrentFrameIndex = 0;

// Shader blobs
static ID3DBlob*                    g_pVertexShaderBlob = nullptr;
static ID3DBlob*                    g_pPixelShaderBlob = nullptr;

// ImGui DX12 vertex shader HLSL
static const char* ImGuiVertexShaderHLSL = R"(
cbuffer vertexBuffer : register(b0)
{
    float4x4 ProjectionMatrix;
};
struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};
struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
}
)";

// ImGui DX12 pixel shader HLSL
static const char* ImGuiPixelShaderHLSL = R"(
struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};
SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);
float4 main(PS_INPUT input) : SV_Target
{
    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
    return out_col;
}
)";

bool ImGui_ImplDX12_Init(ID3D12Device* device, int num_frames_in_flight, DXGI_FORMAT rtv_format, ID3D12DescriptorHeap* srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE uav_gpu_desc_handle)
{
    g_pd3dDevice = device;
    g_NumFramesInFlight = num_frames_in_flight;
    g_hGpuDescriptorHandle = srv_gpu_desc_handle;

    return ImGui_ImplDX12_CreateDeviceObjects();
}

void ImGui_ImplDX12_Shutdown()
{
    ImGui_ImplDX12_InvalidateDeviceObjects();
    g_pd3dDevice = nullptr;
}

void ImGui_ImplDX12_NewFrame()
{
    if (!g_pd3dDevice)
        return;

    g_CurrentFrameIndex = (g_CurrentFrameIndex + 1) % g_NumFramesInFlight;
}

void ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* command_list)
{
    if (!command_list || !draw_data || draw_data->CmdListsCount == 0)
        return;

    g_pd3dCommandList = command_list;
}

void ImGui_ImplDX12_GetShaders(const void** out_vertex_shader, size_t* out_vertex_shader_size, const void** out_pixel_shader, size_t* out_pixel_shader_size)
{
    if (g_pVertexShaderBlob)
    {
        *out_vertex_shader = g_pVertexShaderBlob->GetBufferPointer();
        *out_vertex_shader_size = g_pVertexShaderBlob->GetBufferSize();
    }
    else
    {
        *out_vertex_shader = nullptr;
        *out_vertex_shader_size = 0;
    }

    if (g_pPixelShaderBlob)
    {
        *out_pixel_shader = g_pPixelShaderBlob->GetBufferPointer();
        *out_pixel_shader_size = g_pPixelShaderBlob->GetBufferSize();
    }
    else
    {
        *out_pixel_shader = nullptr;
        *out_pixel_shader_size = 0;
    }
}

void ImGui_ImplDX12_InvalidateDeviceObjects()
{
    if (g_pVertexShaderBlob)
    {
        g_pVertexShaderBlob->Release();
        g_pVertexShaderBlob = nullptr;
    }
    if (g_pPixelShaderBlob)
    {
        g_pPixelShaderBlob->Release();
        g_pPixelShaderBlob = nullptr;
    }
}

bool ImGui_ImplDX12_CreateDeviceObjects()
{
    if (!g_pd3dDevice)
        return false;

    // Compile vertex shader
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompile(
        ImGuiVertexShaderHLSL,
        strlen(ImGuiVertexShaderHLSL),
        "ImGuiVS",
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        0,
        0,
        &g_pVertexShaderBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return false;
    }

    // Compile pixel shader
    hr = D3DCompile(
        ImGuiPixelShaderHLSL,
        strlen(ImGuiPixelShaderHLSL),
        "ImGuiPS",
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        0,
        0,
        &g_pPixelShaderBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        ImGui_ImplDX12_InvalidateDeviceObjects();
        return false;
    }

    return true;
}