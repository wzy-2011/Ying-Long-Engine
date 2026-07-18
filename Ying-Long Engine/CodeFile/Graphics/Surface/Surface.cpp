/**
 * @file Surface.cpp
 * @brief 图像表面类实现 / Image surface class implementation
 *
 * 实现图像表面的加载、保存、像素操作等功能。
 * Implements image surface loading, saving, pixel manipulation, and other features.
 */
#include "Surface.h"

namespace YingLong
{
	Surface::Surface()
	{
		// 初始化空的ScratchImage / Initialize empty ScratchImage
		this->SurfaceImage = DirectX::ScratchImage();
	}

	Surface::Surface(unsigned int width, unsigned int height)
	{
		// 初始化指定尺寸的表面 / Initialize surface with specified size
		this->InitializeSurface(width, height);
	}

	Surface::Surface(std::string ImageFileName)
	{
		// 从文件加载表面 / Load surface from file
		this->InitializeSurface(ImageFileName);
	}

	void Surface::InitializeSurface(unsigned int width, unsigned int height)
	{
		// 初始化2D纹理，格式为R8G8B8A8_UNORM
		// Initialize 2D texture with format R8G8B8A8_UNORM
		HRESULT hr = this->SurfaceImage.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM,
			width, height, 1, 1);
		if (FAILED(hr))
		{
			assert("Failed to InitializeSurface()!");
		}
	}

	void Surface::InitializeSurface(std::string ImageFileName)
	{
		// 初始化COM（WIC需要）/ Initialize COM (needed for WIC)
		HRESULT hr = CoInitialize(NULL);
		if (FAILED(hr))
		{
			assert("CoInitialize() Failed!");
		}

		// 转换为宽字符串（WIC需要）/ Convert to wide string (needed for WIC)
		std::wstring WidePath = std::wstring(ImageFileName.begin(),
			ImageFileName.end());
		// 从WIC文件加载图像（忽略sRGB）/ Load image from WIC file (ignore sRGB)
		hr = DirectX::LoadFromWICFile(WidePath.c_str(),
			WIC_FLAGS_IGNORE_SRGB,
			NULL, this->SurfaceImage);
		if (FAILED(hr))
		{
			// 加载失败时创建1x1的占位图像（深灰色）
			// Create 1x1 placeholder image on load failure (dark gray)
			this->InitializeSurface(1, 1);
			this->ClearSurface(Color(51, 76, 76));
		}

		// 如果格式不是R8G8B8A8_UNORM，则转换格式
		// If format is not R8G8B8A8_UNORM, convert format
		if (this->SurfaceImage.GetImage(0, 0, 0)->format
			!= DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			ScratchImage ConvertedImage;
			hr = DirectX::Convert(*this->SurfaceImage.GetImage(0, 0, 0),
				DXGI_FORMAT_R8G8B8A8_UNORM,
				TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT,
				ConvertedImage);
			if (FAILED(hr))
			{
				assert("Failed to convert image to correct format!");
			}

			// 移动转换后的图像 / Move converted image
			this->SurfaceImage = std::move(ConvertedImage);
		}
	}

	void Surface::ClearSurface(Color FillValue)
	{
		// 获取图像数据 / Get image data
		auto& ImageData = *this->SurfaceImage.GetImage(0, 0, 0);
		// 逐行填充 / Fill row by row
		for (int y = 0; y < this->GetSurfaceHeight(); y++)
		{
			// 获取当前行起始指针 / Get current row start pointer
			auto RowStart = reinterpret_cast<Color*>(ImageData.pixels
				+ ImageData.rowPitch * y);
			// 填充整行 / Fill entire row
			std::fill(RowStart, RowStart + this->GetSurfaceWidth(),
				FillValue);
		}
	}

	void Surface::ReplacePixel(unsigned int x, unsigned int y,
		Color ReplaceColor)
	{
		// 获取图像数据 / Get image data
		auto& ImageData = *this->SurfaceImage.GetImage(0, 0, 0);
		// 计算像素位置并替换 / Calculate pixel position and replace
		reinterpret_cast<Color*>(&ImageData.pixels[y
			* ImageData.rowPitch])[x]
			= ReplaceColor;
	}

	void Surface::SaveSurface(std::string SavePath)
	{
		// 根据文件扩展名获取编解码器ID的lambda
		// Lambda to get codec ID based on file extension
		const auto GetCodecId = [](std::string SavePath)
			{
				const std::filesystem::path FilePath = SavePath;
				if (FilePath.extension().string() == ".png")
				{
					return WIC_CODEC_PNG;
				}
				else if (FilePath.extension().string() == ".jpg")
				{
					return WIC_CODEC_JPEG;
				}
				else if (FilePath.extension().string() == ".bmp")
				{
					return WIC_CODEC_BMP;
				}
				else
				{
					return (WICCodecs)0;
				}
			};

		// 保存到WIC文件 / Save to WIC file
		HRESULT hr = DirectX::SaveToWICFile(
			*this->SurfaceImage.GetImage(0, 0, 0), WIC_FLAGS_NONE,
			GetWICCodec(GetCodecId(SavePath)),
			std::wstring(SavePath.begin(), SavePath.end()).c_str());
		if (FAILED(hr))
		{
			assert("Failed to save image to file!");
		}
	}

	const Color& Surface::GetPixel(unsigned int x,
		unsigned int y) const noexcept
	{
		// 获取图像数据 / Get image data
		auto& ImageData = *this->SurfaceImage.GetImage(0, 0, 0);
		// 返回指定位置的像素引用 / Return pixel reference at specified position
		return reinterpret_cast<Color*>(&ImageData.pixels[y
			* ImageData.rowPitch])[x];
	}

	const int Surface::GetSurfaceWidth() const noexcept
	{
		// 从元数据获取宽度 / Get width from metadata
		return (int)this->SurfaceImage.GetMetadata().width;
	}

	const int Surface::GetSurfaceHeight() const noexcept
	{
		// 从元数据获取高度 / Get height from metadata
		return (int)this->SurfaceImage.GetMetadata().height;
	}

	const int Surface::GetBytePitch() const noexcept
	{
		// 获取行字节间距 / Get row byte pitch
		return (int)this->SurfaceImage.GetImage(0, 0, 0)->rowPitch;
	}

	Color* Surface::GetBufferData() const noexcept
	{
		// 获取像素缓冲区指针 / Get pixel buffer pointer
		return reinterpret_cast<Color*>(this->SurfaceImage.GetPixels());
	}

	Surface::Surface(DirectX::ScratchImage scratch) noexcept
		:
		scratch(std::move(scratch))
	{
		// 移动构造ScratchImage / Move construct ScratchImage
	}

	std::shared_ptr<Surface> Surface::CreateNewSurface(unsigned int width,
		unsigned int height)
	{
		// 静态工厂方法：创建空表面 / Static factory method: create empty surface
		return std::make_shared<Surface>(width, height);
	}

	std::shared_ptr<Surface> Surface::CreateNewSurface(
		std::string ImageFileName)
	{
		// 静态工厂方法：从文件创建表面 / Static factory method: create surface from file
		return std::make_shared<Surface>(ImageFileName);
	}
}
