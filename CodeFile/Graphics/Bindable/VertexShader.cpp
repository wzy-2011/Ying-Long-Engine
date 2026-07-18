/**
 * @file VertexShader.cpp
 * @brief DX11 顶点着色器实现文件 / DX11 vertex shader implementation file
 *
 * 实现 VertexShader 类的构造、初始化、绑定和字节码获取功能。
 * Implements VertexShader class construction, initialization, binding,
 * and bytecode retrieval functionality.
 */

#include "VertexShader.h"
#include <d3dcompiler.h>

namespace YingLong
{
	VertexShader::VertexShader(Graphics& graphics, const std::string& FilePath)
	{
		this->Initialize(graphics, FilePath);
	}

	void VertexShader::Initialize(Graphics& graphics, const std::string& FilePath)
	{
		WRL::ComPtr<ID3D10Blob> ErrorBuffer;
		// 使用 D3DCompileFromFile 从文件编译顶点着色器
		// 入口点为 "main"，着色器模型为 vs_5_0
		// Compile vertex shader from file using D3DCompileFromFile
		// Entry point is "main", shader model is vs_5_0
		HRESULT hr = D3DCompileFromFile(std::wstring(FilePath.begin(), FilePath.end()).c_str(), 
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", 0, 0, 
			this->pVertexShaderBuffer.GetAddressOf(), ErrorBuffer.GetAddressOf());
		if (FAILED(hr))
		{
			// 编译失败时构造错误消息，包含编译错误详情
			// Construct error message on compilation failure, including compilation error details
			std::string errorMessage = "Failed to read vertex shader from file: '" + FilePath + "'!";
			if (ErrorBuffer)
			{
				errorMessage += "\nDetailed: " + std::string((LPCSTR)ErrorBuffer->GetBufferPointer());
			}
			MessageBoxA(nullptr, errorMessage.c_str(), "ERROR", MB_ICONERROR);
			// 抛出运行时异常
			// Throw runtime exception
			throw std::runtime_error(errorMessage);
		}

		// 使用编译好的字节码创建 D3D11 顶点着色器对象
		// Create D3D11 vertex shader object using compiled bytecode
		hr = GetDevice(&graphics)->CreateVertexShader(
			this->pVertexShaderBuffer->GetBufferPointer(),
			this->pVertexShaderBuffer->GetBufferSize(),
			nullptr, this->pVertexShaderObject.GetAddressOf());
		if (FAILED(hr))
		{
			// 创建失败时在消息框中显示错误信息
			// Display error message in message box on creation failure
			MessageBoxA(nullptr, ("Failed to create vertex shader from file : '" +
				FilePath + "' !").c_str(), "ERROR", MB_ICONERROR);

			return;
		}
	}

	void VertexShader::Bind(Graphics& graphics) noexcept
	{
		// 将顶点着色器绑定到顶点着色器阶段，不使用类实例和接口
		// Bind vertex shader to vertex shader stage, no class instances or interfaces
		GetDevicContext(&graphics)->VSSetShader(this->pVertexShaderObject.Get(), nullptr, 0);
	}

	ID3D10Blob* VertexShader::GetBytecode()
	{
		return this->pVertexShaderBuffer.Get();
	}
}
