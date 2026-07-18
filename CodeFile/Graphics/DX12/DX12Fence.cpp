/**
 * @file DX12Fence.cpp
 * @brief DX12 围栏实现文件 / DX12 Fence Implementation
 *
 * 本文件实现了 DX12Fence 类的所有方法，包括围栏创建、
 * 信号、等待和增量操作。
 *
 * This file implements all methods of the DX12Fence class, including
 * fence creation, signal, wait, and increment operations.
 */

#include "DX12Fence.h"
#include "../../Debug/DX12Log.h"
#include <stdexcept>

namespace YingLong
{
    /**
     * @brief 构造函数实现 / Constructor implementation
     *
     * 创建 D3D12 围栏和 Windows 事件对象用于同步等待。
     * Creates D3D12 fence and Windows event object for synchronization wait.
     *
     * @param device D3D12 设备指针 / D3D12 device pointer
     * @param commandQueue 命令队列指针 / Command queue pointer
     * @throws std::runtime_error 如果设备或队列为空，或创建失败
     *                             If device or queue is null, or creation fails
     */
    DX12Fence::DX12Fence(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
        : pCommandQueue(commandQueue)    ///< 保存命令队列指针 / Store command queue pointer
        , CurrentFenceValue(0)           ///< 初始围栏值为0 / Initial fence value is 0
    {
        // 验证设备指针
        // Validate device pointer
        if (!device)
        {
            throw std::runtime_error("Null device passed to DX12Fence constructor");
        }

        // 验证命令队列指针
        // Validate command queue pointer
        if (!commandQueue)
        {
            throw std::runtime_error("Null command queue passed to DX12Fence constructor");
        }

        // 创建 D3D12 围栏对象（初始值为0，无标志）
        // Create D3D12 fence object (initial value 0, no flags)
        HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
        if (FAILED(hr) || !pFence)
        {
            throw std::runtime_error("Failed to create fence");
        }

        // 创建 Windows 事件对象用于等待
        // Create Windows event object for waiting
        FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (FenceEvent == nullptr)
        {
            throw std::runtime_error("Failed to create fence event");
        }
    }

    /**
     * @brief 析构函数实现 / Destructor implementation
     *
     * 关闭事件句柄并释放围栏资源。
     * Closes event handle and releases fence resources.
     */
    DX12Fence::~DX12Fence()
    {
        // 关闭事件句柄
        // Close event handle
        if (FenceEvent)
        {
            CloseHandle(FenceEvent);
            FenceEvent = nullptr;
        }

        // 释放围栏和命令队列
        // Release fence and command queue
        pFence.Reset();
        pCommandQueue.Reset();
    }

    /**
     * @brief 信号围栏 / Signal the fence
     *
     * 在命令队列上添加信号命令，GPU 执行时设置围栏值。
     * Adds a signal command on the command queue, GPU sets fence value when executed.
     *
     * @param value 要设置的围栏值 / Fence value to set
     */
    void DX12Fence::Signal(UINT64 value)
    {
        if (!pFence || !pCommandQueue)
            return;

        // 在命令队列上添加围栏信号命令
        // Add fence signal command on command queue
        pCommandQueue->Signal(pFence.Get(), value);
    }

    /**
     * @brief 等待指定围栏值 / Wait for specified fence value
     *
     * 检查当前完成值，如果未完成则设置事件并等待。
     * Checks current completed value, sets event and waits if not completed.
     *
     * @param value 要等待的围栏值 / Fence value to wait for
     * @param timeoutMs 超时时间（毫秒）/ Timeout in milliseconds
     */
    void DX12Fence::Wait(UINT64 value, DWORD timeoutMs)
    {
        // 检查围栏对象是否存在
        // Check if fence object exists
        if (!pFence)
            return;

        // 如果 GPU 还未完成该值
        // If GPU hasn't completed this value yet
        if (pFence->GetCompletedValue() < value)
        {
            // 设置完成时触发的事件
            // Set event to trigger on completion
            pFence->SetEventOnCompletion(value, FenceEvent);

            // 等待事件触发或超时
            // Wait for event to trigger or timeout
            DWORD result = WaitForSingleObject(FenceEvent, timeoutMs);
            if (result == WAIT_TIMEOUT)
            {
                DX12LogError("[DX12Fence::Wait] Timeout waiting for GPU fence!\n");
            }
        }
    }

    /**
     * @brief 获取 GPU 已完成的围栏值 / Get GPU completed fence value
     * @return GPU 已完成的围栏值 / GPU completed fence value
     */
    UINT64 DX12Fence::GetCompletedValue() const noexcept
    {
        if (!pFence)
            return 0;
        return pFence->GetCompletedValue();
    }

    /**
     * @brief 递增围栏值 / Increment fence value
     *
     * 将 CPU 端跟踪的围栏值加1并返回。
     * Increments the CPU-tracked fence value by 1 and returns it.
     *
     * @return 递增后的围栏值 / Incremented fence value
     */
    UINT64 DX12Fence::Increment()
    {
        CurrentFenceValue++;
        return CurrentFenceValue;
    }
}
