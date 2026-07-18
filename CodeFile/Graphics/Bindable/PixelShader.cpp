/**
 * @file PixelShader.cpp
 * @brief DX11 像素着色器实现文件 / DX11 pixel shader implementation file
 *
 * 实现 PixelShader 类的构造、初始化、绑定和字节码获取功能。
 * Implements PixelShader class construction, initialization, binding,
 * and bytecode retrieval functionality.
 */

#include "PixelShader.h"
#include <d3dcompiler.h>

namespace YingLong 
{
	PixelShader::PixelShader(Graphics& graphics, const std::string& filePath)
	{
		this->Initialize(graphics, filePath);
	}

	void PixelShader::Initialize(Graphics& graphics, const std::string& FilePath)
	{
		WRL::ComPtr<ID3D10Blob> ErrorBuffer;
		// 使用 D3DCompileFromFile 从文件编译像素着色器
		// 定义 DX11_RENDERER 宏，使 Lighting.hlsli 使用 cbuffer 路径而非 StructuredBuffer
		// Compile pixel shader from file using D3DCompileFromFile
		// Define DX11_RENDERER macro so Lighting.hlsli uses cbuffer path instead of StructuredBuffer
		D3D_SHADER_MACRO macros[] = { "DX11_RENDERER", "1", nullptr, nullptr };
		HRESULT hr = D3DCompileFromFile(std::wstring(FilePath.begin(), FilePath.end()).c_str(),
			macros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", 0, 0,
			this->pPixelShaderBuffer.GetAddressOf(), ErrorBuffer.GetAddressOf());
		if (FAILED(hr))
		{
			// 编译失败时在控制台和消息框中显示错误信息
			// Display error message in console and message box on compilation failure
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
			std::cout << "Failed to read Pixel shader from file : '" << FilePath << "' !" << std::endl << "Detailed : " << (LPCSTR)ErrorBuffer->GetBufferPointer();
			
			MessageBoxA(nullptr, ("Failed to read Pixel shader from file : '" +
				FilePath + "' !\n    Detailed : " + (LPCSTR)ErrorBuffer->GetBufferPointer()).c_str(), "ERROR", MB_ICONERROR);
		}

		// 使用编译好的字节码创建 D3D11 像素着色器对象
		// Create D3D11 pixel shader object using compiled bytecode
		hr = GetDevice(&graphics)->CreatePixelShader(
			this->pPixelShaderBuffer->GetBufferPointer(),
			this->pPixelShaderBuffer->GetBufferSize(),
			nullptr, this->pPixelShaderObject.GetAddressOf());
		if (FAILED(hr))
		{
			// 创建失败时在控制台和消息框中显示错误信息
			// Display error message in console and message box on creation failure
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
			std::cout << "Failed to create Pixel shader from file : '" << FilePath << "' !";

			MessageBoxA(nullptr, ("Failed to create Pixel shader from file : '" +
				FilePath + "' !").c_str(), "ERROR", MB_ICONERROR);
		}
	}

	void PixelShader::Bind(Graphics& graphics) noexcept
	{
		// 将像素着色器绑定到像素着色器阶段，不使用类实例和接口
		// Bind pixel shader to pixel shader stage, no class instances or interfaces
		GetDevicContext(&graphics)->PSSetShader(this->pPixelShaderObject.Get(), nullptr, 0);
	}

	ID3D10Blob* PixelShader::GetBytecode()
	{
		return this->pPixelShaderBuffer.Get();
	}
}
