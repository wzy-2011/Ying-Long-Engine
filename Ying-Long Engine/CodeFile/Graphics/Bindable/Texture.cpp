/**
 * @file Texture.cpp
 * @brief DX11 纹理实现文件 / DX11 texture implementation file
 *
 * 实现 Texture 类的构造、初始化和绑定功能。
 * Implements Texture class construction, initialization, and binding functionality.
 */

#include "Texture.h"

namespace YingLong
{
	Texture::Texture(Graphics& graphics, const Surface& surface, UINT index)
	{
		this->Initialize(graphics, surface, index);
	}

	Texture::Texture(const Texture& other)
	{
		// 共享 ComPtr 引用计数，实现纹理资源的浅拷贝
		// Share ComPtr reference count, enabling shallow copy of texture resources
		this->pTextureObject = other.pTextureObject;
		this->pTextureResource = other.pTextureResource;
	}

	void Texture::Initialize(Graphics& graphics,
		const Surface& surface, UINT index)
	{
		this->TextureIndex = index;

		D3D11_TEXTURE2D_DESC TextureDesc;
		ZeroMemory(&TextureDesc, sizeof(D3D11_TEXTURE2D_DESC));

		// 使用 R8G8B8A8_UNORM 格式（每个通道 8 位无符号归一化值）
		// Use R8G8B8A8_UNORM format (8-bit unsigned normalized value per channel)
		TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		TextureDesc.Width = surface.GetSurfaceWidth();
		TextureDesc.Height = surface.GetSurfaceHeight();
		// 不生成 mipmap 链
		// Do not generate mipmap chain
		TextureDesc.MipLevels = 1;
		// 单层纹理数组（普通 2D 纹理）
		// Single-layer texture array (ordinary 2D texture)
		TextureDesc.ArraySize = 1;
		// 多重采样级别 1（无多重采样）
		// Multisample level 1 (no multisampling)
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		// 默认使用方式，GPU 读写
		// Default usage, GPU read-write
		TextureDesc.Usage = D3D11_USAGE_DEFAULT;
		// 作为着色器资源绑定
		// Bind as shader resource
		TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		TextureDesc.CPUAccessFlags = 0;
		TextureDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA TextureResourceData;
		ZeroMemory(&TextureResourceData,
			sizeof(D3D11_SUBRESOURCE_DATA));
		// 设置纹理像素数据指针
		// Set texture pixel data pointer
		TextureResourceData.pSysMem = surface.GetBufferData();
		// 计算每行的字节跨度
		// Calculate byte stride per row
		TextureResourceData.SysMemPitch = surface.GetSurfaceWidth()
			* sizeof(Color);

		// 创建 2D 纹理资源
		// Create 2D texture resource
		HRESULT hr = GetDevice(&graphics)->CreateTexture2D(
			&TextureDesc,
			&TextureResourceData,
			this->pTextureObject.GetAddressOf());
		if (FAILED(hr))
		{
			assert("Failed to create texture!");
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc;
		ZeroMemory(&ShaderResourceViewDesc,
			sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		// 着色器资源视图格式与纹理格式一致
		// Shader resource view format matches texture format
		ShaderResourceViewDesc.Format = TextureDesc.Format;
		// 视图维度为 2D 纹理
		// View dimension is 2D texture
		ShaderResourceViewDesc.ViewDimension
			= D3D11_SRV_DIMENSION_TEXTURE2D;
		// mipmap 层级数为 1
		// Mipmap level count is 1
		ShaderResourceViewDesc.Texture2D.MipLevels = 1;
		// 最详细 mip 层级为 0
		// Most detailed mip level is 0
		ShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

		// 创建着色器资源视图，供着色器采样使用
		// Create shader resource view for shader sampling
		hr = GetDevice(&graphics)->CreateShaderResourceView(
			this->pTextureObject.Get(), &ShaderResourceViewDesc,
			this->pTextureResource.GetAddressOf());
		if (FAILED(hr))
		{
			assert("Failed to CreateShaderResourceView!");
		}
	}

	void Texture::Bind(Graphics& graphics) noexcept
	{
		// 将纹理资源视图绑定到像素着色器的指定纹理槽
		// Bind texture resource view to the specified pixel shader texture slot
		GetDevicContext(&graphics)->PSSetShaderResources(this->TextureIndex, 1,
			this->pTextureResource.GetAddressOf());
	}
}
