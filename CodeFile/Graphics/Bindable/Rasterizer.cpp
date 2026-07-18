/**
 * @file Rasterizer.cpp
 * @brief DX11 光栅化状态实现 / DX11 rasterizer state implementation
 */

#include "Rasterizer.h"
#include "../Graphics.h"

namespace YingLong
{
	Rasterizer::Rasterizer(Graphics& graphics, D3D11_FILL_MODE fillMode, D3D11_CULL_MODE cullMode)
	{
		// 创建光栅化状态描述 / Create rasterizer state description
		D3D11_RASTERIZER_DESC rasterDesc = {};
		rasterDesc.FillMode = fillMode;
		rasterDesc.CullMode = cullMode;
		rasterDesc.FrontCounterClockwise = FALSE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.SlopeScaledDepthBias = 0.0f;
		rasterDesc.DepthClipEnable = TRUE;
		rasterDesc.ScissorEnable = FALSE;
		rasterDesc.MultisampleEnable = FALSE;
		rasterDesc.AntialiasedLineEnable = (fillMode == D3D11_FILL_WIREFRAME);

		GetDevice(&graphics)->CreateRasterizerState(&rasterDesc, &pRasterizerState);
	}

	void Rasterizer::Bind(Graphics& graphics) noexcept
	{
		GetDevicContext(&graphics)->RSSetState(pRasterizerState.Get());
	}
}
