/**
 * @file DX12ShaderCompiler.h
 * @brief DX12 着色器编译器头文件 / DX12 Shader Compiler Header
 *
 * 本文件定义了 DX12ShaderCompiler 类，提供从 HLSL 源文件
 * 编译着色器字节码的静态方法。支持顶点着色器和像素着色器的编译，
 * 并提供详细的错误信息输出。
 *
 * This file defines the DX12ShaderCompiler class, providing static methods
 * for compiling shader bytecode from HLSL source files. Supports vertex shader
 * and pixel shader compilation, with detailed error message output.
 */

#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <windows.h>
#include "../../Debug/DX12Log.h"

namespace YingLong
{
    /**
     * @brief DX12 着色器编译器类 / DX12 Shader Compiler Class
     *
     * DX12ShaderCompiler 类提供了一组静态方法，用于从 HLSL 源文件
     * 编译 D3D12 着色器字节码。主要功能包括：
     * - 通用着色器编译（指定入口点和着色器模型）
     * - 顶点着色器编译（vs_5_0，入口点 main）
     * - 像素着色器编译（ps_5_0，入口点 main）
     * - 编译错误信息的详细日志输出
     *
     * 编译使用 D3DCompileFromFile 函数，启用调试信息并跳过优化，
     * 便于开发调试。
     *
     * The DX12ShaderCompiler class provides a set of static methods for compiling
     * D3D12 shader bytecode from HLSL source files. Main features include:
     * - Generic shader compilation (specify entry point and shader model)
     * - Vertex shader compilation (vs_5_0, entry point main)
     * - Pixel shader compilation (ps_5_0, entry point main)
     * - Detailed log output of compilation errors
     *
     * Compilation uses the D3DCompileFromFile function with debug info enabled
     * and optimization skipped for development and debugging.
     */
    class DX12ShaderCompiler
    {
    public:
        /**
         * @brief 编译着色器 / Compile shader
         *
         * 从 HLSL 文件编译着色器字节码，可指定入口点和着色器模型。
         * 编译时启用调试信息并跳过优化，便于调试。
         *
         * Compiles shader bytecode from an HLSL file, with specifiable entry point
         * and shader model. Compilation includes debug info and skips optimization
         * for debugging purposes.
         *
         * @param filePath 着色器文件路径（宽字符串）/ Shader file path (wide string)
         * @param entryPoint 着色器入口点函数名 / Shader entry point function name
         * @param shaderModel 着色器模型（如 "vs_5_0"、"ps_5_0"）/ Shader model (e.g. "vs_5_0", "ps_5_0")
         * @return 编译后的着色器字节码 / Compiled shader bytecode
         * @throws std::runtime_error 如果文件不存在或编译失败 / If file not found or compilation fails
         */
        static std::vector<uint8_t> CompileShader(
            const std::wstring& filePath,
            const std::string& entryPoint,
            const std::string& shaderModel)
        {
            Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;      ///< 错误信息 Blob / Error info blob
            Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;     ///< 着色器字节码 Blob / Shader bytecode blob

            // 将宽路径转换为窄字符串用于日志
            // Convert wide path to narrow for logging
            std::string filePathNarrow(filePath.begin(), filePath.end());
            DX12Log(("[DX12ShaderCompiler] Compiling shader: " + filePathNarrow + "\n").c_str());

            // 检查文件是否存在
            // Check if file exists
            if (GetFileAttributesW(filePath.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                DX12LogError(("[DX12ShaderCompiler] Shader file not found: " + filePathNarrow + "\n").c_str());
                throw std::runtime_error("Shader file not found");
            }

            // 调用 D3DCompileFromFile 编译着色器
            // Call D3DCompileFromFile to compile shader
            HRESULT hr = D3DCompileFromFile(
                filePath.c_str(),                                    ///< 着色器文件路径 / Shader file path
                nullptr,                                             ///< 宏定义（无）/ Macro definitions (none)
                D3D_COMPILE_STANDARD_FILE_INCLUDE,                   ///< 标准文件包含 / Standard file include
                entryPoint.c_str(),                                  ///< 入口点函数 / Entry point function
                shaderModel.c_str(),                                 ///< 着色器模型 / Shader model
                D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    ///< 编译标志：调试+跳过优化 / Compile flags: debug + skip optimization
                0,                                                   ///< 效果标志（无）/ Effect flags (none)
                shaderBlob.GetAddressOf(),                           ///< 输出着色器 Blob / Output shader blob
                errorBlob.GetAddressOf()                             ///< 输出错误 Blob / Output error blob
            );

            // 检查编译结果
            // Check compilation result
            if (FAILED(hr))
            {
                std::string errorMsg = "Failed to compile shader: ";
                // 如果有错误 Blob，读取错误信息
                // If error blob exists, read error message
                if (errorBlob)
                {
                    errorMsg += std::string((const char*)errorBlob->GetBufferPointer());
                }
                else
                {
                    errorMsg += "Unknown error (hr = " + std::to_string(hr) + ")";
                }
                DX12LogError((errorMsg + "\n").c_str());
                throw std::runtime_error(errorMsg);
            }

            DX12LogSuccess("[DX12ShaderCompiler] Shader compiled successfully\n");

            // 将着色器 Blob 转换为字节向量
            // Convert shader blob to byte vector
            return std::vector<uint8_t>(
                (uint8_t*)shaderBlob->GetBufferPointer(),
                (uint8_t*)shaderBlob->GetBufferPointer() + shaderBlob->GetBufferSize()
            );
        }

        /**
         * @brief 编译顶点着色器 / Compile vertex shader
         *
         * 使用默认入口点 "main" 和着色器模型 "vs_5_0" 编译顶点着色器。
         * Compiles a vertex shader using default entry point "main" and shader model "vs_5_0".
         *
         * @param filePath 着色器文件路径（宽字符串）/ Shader file path (wide string)
         * @return 编译后的顶点着色器字节码 / Compiled vertex shader bytecode
         * @throws std::runtime_error 如果文件不存在或编译失败 / If file not found or compilation fails
         */
        static std::vector<uint8_t> CompileVertexShader(const std::wstring& filePath)
        {
            return CompileShader(filePath, "main", "vs_5_0");
        }

        /**
         * @brief 编译像素着色器 / Compile pixel shader
         *
         * 使用默认入口点 "main" 和着色器模型 "ps_5_0" 编译像素着色器。
         * Compiles a pixel shader using default entry point "main" and shader model "ps_5_0".
         *
         * @param filePath 着色器文件路径（宽字符串）/ Shader file path (wide string)
         * @return 编译后的像素着色器字节码 / Compiled pixel shader bytecode
         * @throws std::runtime_error 如果文件不存在或编译失败 / If file not found or compilation fails
         */
        static std::vector<uint8_t> CompilePixelShader(const std::wstring& filePath)
        {
            return CompileShader(filePath, "main", "ps_5_0");
        }

        /**
         * @brief 编译计算着色器 / Compile compute shader
         *
         * 使用默认入口点 "main" 和着色器模型 "cs_5_0" 编译计算着色器。
         * Compiles a compute shader using default entry point "main" and shader model "cs_5_0".
         *
         * @param filePath 着色器文件路径（宽字符串）/ Shader file path (wide string)
         * @return 编译后的计算着色器字节码 / Compiled compute shader bytecode
         * @throws std::runtime_error 如果文件不存在或编译失败 / If file not found or compilation fails
         */
        static std::vector<uint8_t> CompileComputeShader(const std::wstring& filePath)
        {
            return CompileShader(filePath, "main", "cs_5_0");
        }
    };
}
