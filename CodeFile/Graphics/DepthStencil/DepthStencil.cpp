/**
 * @file DepthStencil.cpp
 * @brief 深度模板缓冲区类实现（已弃用）
 *        Depth stencil buffer class implementation (deprecated)
 *
 * 实现DirectX 11深度模板缓冲区的创建、绑定和管理功能。
 * Implements DirectX 11 depth stencil buffer creation, binding, and management.
 *
 * @deprecated 已弃用，请使用 DepthStencilDX12 替代
 *             / Deprecated, use DepthStencilDX12 instead
 */
#include "DepthStencil.h"
#include "../Graphics.h"

namespace YingLong
{
	DepthStencil::DepthStencil()
	{
		// 默认构造 / Default constructor
	}

	void DepthStencil::InitializeDepthStencil(Graphics& graphics, int width, int height)
	{
		// 1. 创建深度模板纹理缓冲区 / Create depth stencil texture buffer
		D3D11_TEXTURE2D_DESC depthBufferDesc;
		ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
		depthBufferDesc.Width = width;
		depthBufferDesc.Height = height;             // 与渲染目标高度一致 / Same as render target height
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.ArraySize = 1;
		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24位深度 + 8位模板 / 24-bit depth + 8-bit stencil
		depthBufferDesc.SampleDesc.Count = 1;              // 多重采样级别 / Multisample level
		depthBufferDesc.SampleDesc.Quality = 0;
		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;       // 默认用途，GPU读写 / Default usage, GPU read/write
		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 作为深度模板绑定 / Bind as depth stencil
		depthBufferDesc.CPUAccessFlags = 0;
		depthBufferDesc.MiscFlags = 0;

		HRESULT hr;
		// 2. 创建深度模板纹理 / Create depth stencil texture
		hr = graphics.pDevice->CreateTexture2D(&depthBufferDesc, nullptr, 
			this->DepthStencilBuffer.GetAddressOf());
		if (FAILED(hr))
		{
			assert("Fuck you");
		}

		// 3. 创建深度模板视图描述 / Create depth stencil view description
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
		depthStencilViewDesc.Format = depthBufferDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		// 4. 创建深度模板视图 / Create depth stencil view
		hr = graphics.pDevice->CreateDepthStencilView(this->DepthStencilBuffer.Get(), &depthStencilViewDesc, 
			this->DepthStencilView.GetAddressOf());
		if (FAILED(hr)) {
			// 创建失败 / Creation failed
			assert("Fuck you again!");
		}

		// 5. 创建深度模板状态描述 / Create depth stencil state description
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

		// 深度测试设置 / Depth test settings
		depthStencilDesc.DepthEnable = TRUE;                  // 启用深度测试 / Enable depth test
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 允许写入深度缓冲区 / Allow writing to depth buffer
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;   // 深度比较函数，小于时通过 / Depth comparison function, pass when less

		// 模板测试设置（默认禁用）/ Stencil test settings (disabled by default)
		depthStencilDesc.StencilEnable = FALSE;               // 禁用模板测试 / Disable stencil test
		depthStencilDesc.StencilReadMask = 0xFF;              // 模板读取掩码 / Stencil read mask
		depthStencilDesc.StencilWriteMask = 0xFF;             // 模板写入掩码 / Stencil write mask

		// 正面模板操作（StencilEnable为TRUE时生效）
		// Front face stencil operations (effective when StencilEnable is TRUE)
		depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// 背面模板操作（与正面相同）
		// Back face stencil operations (same as front face)
		depthStencilDesc.BackFace = depthStencilDesc.FrontFace;

		// 6. 创建深度模板状态 / Create depth stencil state
		hr = graphics.pDevice->CreateDepthStencilState(&depthStencilDesc, 
			this->DepthStencilState.GetAddressOf());
		if (FAILED(hr)) {
			// 创建失败 / Creation failed
			assert("Fuck you bitchass nigga");
		}
	}

	void DepthStencil::BindDepthStencil(Graphics& graphics)
	{
		// 设置深度模板状态到输出合并阶段
		// Set depth stencil state to output merger stage
		graphics.pDeviceContext->OMSetDepthStencilState(this->DepthStencilState.Get(), 0);
		// 清除深度模板缓冲区（深度值设为1.0，模板值设为0.0）
		// Clear depth stencil buffer (depth value set to 1.0, stencil value set to 0.0)
		graphics.pDeviceContext->ClearDepthStencilView(this->DepthStencilView.Get(),
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.0f);
	}

	WRL::ComPtr<ID3D11DepthStencilView> DepthStencil::GetDepthStencilView()
	{
		return this->DepthStencilView;
	}

	WRL::ComPtr<ID3D11DepthStencilState> DepthStencil::GetDepthStencilState()
	{
		return this->DepthStencilState;
	}
}
