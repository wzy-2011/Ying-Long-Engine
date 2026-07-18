/**
 * @file Sampler.cpp
 * @brief DX11 采样器状态实现文件 / DX11 sampler state implementation file
 *
 * 实现 Sampler 类的构造、初始化和绑定功能。
 * Implements Sampler class construction, initialization, and binding functionality.
 */

#include "Sampler.h"

namespace YingLong
{
	Sampler::Sampler()
	{

	}

	Sampler::Sampler(Graphics& graphics)
	{
		this->InitializeSampler(graphics);
	}

	Sampler::Sampler(const Sampler& other)
	{
		
	}

	void Sampler::InitializeSampler(Graphics& graphics)
	{
		D3D11_SAMPLER_DESC SamplerDesc = {};
		// U 方向使用环绕纹理寻址模式
		// Use wrap texture addressing mode in U direction
		SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		// V 方向使用环绕纹理寻址模式
		// Use wrap texture addressing mode in V direction
		SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		// W 方向使用环绕纹理寻址模式
		// Use wrap texture addressing mode in W direction
		SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		// 使用线性过滤（缩小、放大和 mipmap 均使用线性插值）
		// Use linear filtering (linear interpolation for min, mag, and mip)
		SamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR;

		// 创建采样器状态对象
		// Create sampler state object
		GetDevice(&graphics)->CreateSamplerState(&SamplerDesc, this->SamplerState.GetAddressOf());
	}

	void Sampler::Bind(Graphics& graphics) noexcept
	{
		// 将采样器绑定到像素着色器的第 0 个采样槽，数量为 1
		// Bind sampler to pixel shader sampler slot 0, count is 1
		GetDevicContext(&graphics)->PSSetSamplers(0, 1, this->SamplerState.GetAddressOf());
	}
}
