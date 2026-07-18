#pragma once

#include "imgui.h"
#include <d3d12.h>

IMGUI_IMPL_API bool     ImGui_ImplDX12_Init(ID3D12Device* device, int num_frames_in_flight, DXGI_FORMAT rtv_format, ID3D12DescriptorHeap* srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE uav_gpu_desc_handle);
IMGUI_IMPL_API void     ImGui_ImplDX12_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplDX12_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* command_list);
IMGUI_IMPL_API void     ImGui_ImplDX12_GetShaders(const void** out_vertex_shader, size_t* out_vertex_shader_size, const void** out_pixel_shader, size_t* out_pixel_shader_size);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_IMPL_API void     ImGui_ImplDX12_InvalidateDeviceObjects();
IMGUI_IMPL_API bool     ImGui_ImplDX12_CreateDeviceObjects();
