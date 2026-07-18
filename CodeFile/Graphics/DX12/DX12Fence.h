/**
 * @file DX12Fence.h
 * @brief DX12 围栏头文件 / DX12 Fence Header
 *
 * 本文件定义了 DX12Fence 类，封装了 D3D12 围栏（Fence）的功能，
 * 用于 CPU 和 GPU 之间的同步操作。
 *
 * This file defines the DX12Fence class, which encapsulates D3D12 fence
 * functionality for synchronization between CPU and GPU.
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace YingLong
{
    /**
     * @brief DX12 围栏类 / DX12 Fence Class
     *
     * DX12Fence 类封装了 D3D12 围栏对象，用于 CPU-GPU 同步。
     * 提供信号（Signal）、等待（Wait）、增量（Increment）等操作，
     * 支持查询 GPU 已完成的围栏值。
     *
     * The DX12Fence class encapsulates the D3D12 fence object for CPU-GPU synchronization.
     * Provides Signal, Wait, Increment operations, and supports querying
     * the GPU-completed fence value.
     */
    class DX12Fence
    {
    public:
        /**
         * @brief 构造函数 / Constructor
         *
         * 创建 D3D12 围栏对象和事件句柄。
         * Creates the D3D12 fence object and event handle.
         *
         * @param device D3D12 设备指针 / D3D12 device pointer
         * @param commandQueue 命令队列指针 / Command queue pointer
         */
        DX12Fence(ID3D12Device* device, ID3D12CommandQueue* commandQueue);

        /**
         * @brief 析构函数 / Destructor
         *
         * 释放围栏资源和事件句柄。
         * Releases fence resources and event handle.
         */
        ~DX12Fence();

        /**
         * @brief 用指定值信号围栏 / Signal the fence with current value
         *
         * 在命令队列上添加一个信号命令，当 GPU 执行到该命令时，
         * 会将围栏值设置为指定值。
         *
         * Adds a signal command on the command queue. When the GPU executes
         * this command, it will set the fence value to the specified value.
         *
         * @param value 要设置的围栏值 / Fence value to set
         */
        void Signal(UINT64 value);

        /**
         * @brief 等待指定的围栏值 / Wait for a specific fence value
         *
         * 阻塞 CPU 直到 GPU 完成指定的围栏值，或超时。
         * Blocks the CPU until the GPU completes the specified fence value, or times out.
         *
         * @param value 要等待的围栏值 / Fence value to wait for
         * @param timeoutMs 超时时间（毫秒）/ Timeout in milliseconds
         */
        void Wait(UINT64 value, DWORD timeoutMs = 5000);

        /**
         * @brief 获取当前围栏值（CPU端）/ Get current fence value (CPU side)
         * @return 当前围栏值 / Current fence value
         */
        UINT64 GetCurrentValue() const noexcept { return CurrentFenceValue; }

        /**
         * @brief 获取 GPU 已完成的围栏值 / Get completed value from GPU
         * @return GPU 已完成的围栏值 / GPU completed fence value
         */
        UINT64 GetCompletedValue() const noexcept;

        /**
         * @brief 递增围栏值并返回新值 / Increment fence value and return new value
         * @return 递增后的围栏值 / Incremented fence value
         */
        UINT64 Increment();

        /**
         * @brief 获取围栏对象 / Get the fence object
         * @return ID3D12Fence 指针 / ID3D12Fence pointer
         */
        ID3D12Fence* GetFence() const noexcept { return pFence.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> pFence;    ///< D3D12 围栏对象 / D3D12 fence object
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue;  ///< 关联的命令队列 / Associated command queue
        HANDLE FenceEvent;                                ///< 等待事件句柄 / Wait event handle
        UINT64 CurrentFenceValue;                         ///< 当前围栏值（CPU端跟踪）/ Current fence value (CPU tracked)
    };
}
