/**
 * @file RenderTarget.cpp
 * @brief 渲染目标类实现（已弃用）
 *        Render target class implementation (deprecated)
 *
 * 实现DirectX 11渲染目标的创建、绑定和管理功能。
 * Implements DirectX 11 render target creation, binding, and management.
 *
 * @deprecated 已弃用，请使用 RenderTargetDX12 替代
 *             / Deprecated, use RenderTargetDX12 instead
 */
#include "RenderTarget.h"
#include "../Graphics.h"

namespace YingLong
{
	RenderTarget::RenderTarget()
	{
		// 默认构造 / Default constructor
	}

	void RenderTarget::InitializeRenderTarget(
		RenderTargetType type, Graphics& graphics, int width, int height)
	{
		if (type == RenderTargetType::DE_RTTYPE_WINOUTPUT)
		{
			// 窗口输出模式：从交换链获取后台缓冲区
			// Window output mode: get back buffer from swap chain
			WRL::ComPtr<ID3D11Resource> pBackBaffer;
			graphics.pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource),
				reinterpret_cast <void**> (pBackBaffer.GetAddressOf()));
			// 创建渲染目标视图 / Create render target view
			graphics.pDevice->CreateRenderTargetView(
				pBackBaffer.Get(),
				nullptr,
				this->pRenderTargetView.GetAddressOf()
			);
		}
		else if (type == RenderTargetType::DE_RTTYPE_TEXTUREOUTPUT)
		{
			// 纹理输出模式：创建纹理作为渲染目标
			// Texture output mode: create texture as render target
			if (width <= 0 || height <= 0)
			{
				assert("You fucked up the size of this render target!");
			}

			// 描述渲染目标纹理 / Describe render target texture
			D3D11_TEXTURE2D_DESC RenderTargetTextureDesc;
			ZeroMemory(&RenderTargetTextureDesc, sizeof(D3D11_TEXTURE2D_DESC));
			RenderTargetTextureDesc.Width = width;
			RenderTargetTextureDesc.Height = height;
			RenderTargetTextureDesc.MipLevels = 1;
			RenderTargetTextureDesc.ArraySize = 1;
			RenderTargetTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			RenderTargetTextureDesc.SampleDesc.Count = 1;
			RenderTargetTextureDesc.SampleDesc.Quality = 0;
			RenderTargetTextureDesc.Usage = D3D11_USAGE_DEFAULT;
			// 同时作为渲染目标和着色器资源绑定
			// Bind as both render target and shader resource
			RenderTargetTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET
				| D3D11_BIND_SHADER_RESOURCE;

			// 创建渲染目标纹理 / Create render target texture
			WRL::ComPtr<ID3D11Texture2D> RenderTargetTexture;
			HRESULT hr = graphics.pDevice->CreateTexture2D(&RenderTargetTextureDesc,
				NULL, RenderTargetTexture.GetAddressOf());
			if (FAILED(hr))
			{
				assert("Cannot create texture for render target!");
			}

			// 创建着色器资源视图（用于采样渲染结果）
			// Create shader resource view (for sampling render result)
			graphics.pDevice->CreateShaderResourceView(RenderTargetTexture.Get(),
				nullptr, this->pRenderTargetResource.GetAddressOf());
			if (FAILED(hr))
			{
				assert("Cannot create resource for render target!");
			}

			// 创建渲染目标视图（用于写入渲染结果）
			// Create render target view (for writing render results)
			graphics.pDevice->CreateRenderTargetView(RenderTargetTexture.Get(),
				NULL, this->pRenderTargetView.GetAddressOf());
			if (FAILED(hr))
			{
				assert("Cannot create render target!");
			}
		}
	}

	void RenderTarget::BindRenderTarget(Graphics& graphics,
		WRL::ComPtr<ID3D11DepthStencilView> dsv)
	{
		// 设置渲染目标和深度模板视图到输出合并阶段
		// Set render target and depth stencil view to output merger stage
		graphics.pDeviceContext->OMSetRenderTargets(1, 
			this->pRenderTargetView.GetAddressOf(), dsv.Get());
		// 清除渲染目标缓冲区（使用指定的清除颜色）
		// Clear render target buffer (using specified clear color)
		graphics.pDeviceContext->ClearRenderTargetView(this->pRenderTargetView.Get(),
			graphics.color);
	}

	WRL::ComPtr<ID3D11RenderTargetView> RenderTarget::GetRenderTargetView()
	{
		return this->pRenderTargetView;
	}

	WRL::ComPtr<ID3D11ShaderResourceView> RenderTarget::GetRenderTargetResource()
	{
		return this->pRenderTargetResource;
	}
}
