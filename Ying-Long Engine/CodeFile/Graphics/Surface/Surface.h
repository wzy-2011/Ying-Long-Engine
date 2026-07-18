/**
 * @file Surface.h
 * @brief 图像表面类 / Image surface class
 *
 * 提供2D图像表面的表示和操作，使用DirectXTex库处理图像加载、保存和像素操作。
 * Provides 2D image surface representation and manipulation, uses DirectXTex
 * library for image loading, saving, and pixel operations.
 */
#pragma once
#include <assert.h>
#include <memory>
#include <Windows.h>
#include <string>
#include <xstring>
#include "../../../Setup/DirectXTex/Includes/DirectXTex.h"
#include <filesystem>
#include <dxgiformat.h>

using namespace DirectX;

namespace YingLong
{

	/**
	 * @brief 颜色类 / Color class
	 *
	 * 表示32位ARGB颜色，支持单个dword存储。
	 * Represents a 32-bit ARGB color, stored in a single dword.
	 */
	class Color
	{
	public:
		unsigned int dword;   ///< 颜色数据（ARGB格式）/ Color data (ARGB format)

		/**
		 * @brief 默认构造函数（黑色）/ Default constructor (black)
		 */
		constexpr Color() noexcept : dword() {}

		/**
		 * @brief 拷贝构造函数 / Copy constructor
		 *
		 * @param col 另一个Color对象 / Another Color object
		 */
		constexpr Color(const Color& col) noexcept : dword(col.dword) {}

		/**
		 * @brief 从dword构造颜色 / Construct color from dword
		 *
		 * @param dw 颜色dword值 / Color dword value
		 */
		constexpr Color(unsigned int dw) noexcept : dword(dw) {}

		/**
		 * @brief 从分量构造颜色 / Construct color from components
		 *
		 * @param x Alpha分量 / Alpha component
		 * @param r Red分量 / Red component
		 * @param g Green分量 / Green component
		 * @param b Blue分量 / Blue component
		 */
		constexpr Color(unsigned char x, unsigned char r,
			unsigned char g, unsigned char b) :
			dword((x << 24u) | (r << 16u) | (g << 8u) | b) {
		}

		/**
		 * @brief 从RGB分量构造颜色（Alpha默认为255）
		 *        Construct color from RGB components (Alpha defaults to 255)
		 *
		 * @param r Red分量 / Red component
		 * @param g Green分量 / Green component
		 * @param b Blue分量 / Blue component
		 */
		constexpr Color(unsigned char r, unsigned char g,
			unsigned char b) :
			dword((255u << 24u) | (r << 16u) | (g << 8u) | b)
		{
		}

		/**
		 * @brief 从颜色和新的Alpha构造颜色
		 *        Construct color from color and new Alpha
		 *
		 * @param col 基础颜色 / Base color
		 * @param x 新的Alpha分量 / New Alpha component
		 */
		constexpr Color(Color col, unsigned char x) :
			dword((x << 24u) | col.dword)
		{
		}

		/**
		 * @brief 赋值运算符 / Assignment operator
		 *
		 * @param color 源颜色 / Source color
		 * @return Color& 自身引用 / Self reference
		 */
		Color& operator =(Color color)
		{
			dword = color.dword;
			return *this;
		}

		/**
		 * @brief 转换为UINT运算符 / Convert to UINT operator
		 *
		 * @return UINT 颜色dword值 / Color dword value
		 */
		operator UINT()
		{
			return this->dword;
		}

		/**
		 * @brief 获取X（Alpha）分量 / Get X (Alpha) component
		 *
		 * @return unsigned char Alpha分量值 / Alpha component value
		 */
		constexpr unsigned char GetX() const
		{
			return dword >> 24u;
		}

		/**
		 * @brief 获取Alpha分量（同GetX）/ Get Alpha component (same as GetX)
		 *
		 * @return unsigned char Alpha分量值 / Alpha component value
		 */
		constexpr unsigned char GetA() const
		{
			return GetX();
		}

		/**
		 * @brief 获取Red分量 / Get Red component
		 *
		 * @return unsigned char Red分量值 / Red component value
		 */
		constexpr unsigned char GetR() const
		{
			return (dword >> 16u) & 0xFFu;
		}

		/**
		 * @brief 获取Green分量 / Get Green component
		 *
		 * @return unsigned char Green分量值 / Green component value
		 */
		constexpr unsigned char GetG() const
		{
			return (dword >> 8u) & 0xFFu;
		}

		/**
		 * @brief 获取Blue分量 / Get Blue component
		 *
		 * @return unsigned char Blue分量值 / Blue component value
		 */
		constexpr unsigned char GetB() const
		{
			return dword & 0xFFu;
		}

		/**
		 * @brief 设置X（Alpha）分量 / Set X (Alpha) component
		 *
		 * @param x Alpha分量值 / Alpha component value
		 */
		void SetX(unsigned char x)
		{
			dword = (dword & 0xFFFFFFu) | (x << 24u);
		}

		/**
		 * @brief 设置Alpha分量（同SetX）/ Set Alpha component (same as SetX)
		 *
		 * @param a Alpha分量值 / Alpha component value
		 */
		void SetA(unsigned char a)
		{
			SetX(a);
		}

		/**
		 * @brief 设置Red分量 / Set Red component
		 *
		 * @param r Red分量值 / Red component value
		 */
		void SetR(unsigned char r)
		{
			dword = (dword & 0xFF00FFFFu) | (r << 16u);
		}

		/**
		 * @brief 设置Green分量 / Set Green component
		 *
		 * @param g Green分量值 / Green component value
		 */
		void SetG(unsigned char g)
		{
			dword = (dword & 0xFFFF00FFu) | (g << 8u);
		}

		/**
		 * @brief 设置Blue分量 / Set Blue component
		 *
		 * @param b Blue分量值 / Blue component value
		 */
		void SetB(unsigned char b)
		{
			dword = (dword & 0xFFFFFF00u) | b;
		}
	};

	/**
	 * @brief 图像表面类 / Image surface class
	 *
	 * 封装DirectXTex的ScratchImage，提供图像加载、保存、像素操作等功能。
	 * Wraps DirectXTex ScratchImage, provides image loading, saving,
	 * pixel manipulation, and other features.
	 */
	class Surface
	{
	public:
		/**
		 * @brief 默认构造函数 / Default constructor
		 */
		Surface();

		/**
		 * @brief 创建指定尺寸的空表面 / Create empty surface with specified size
		 *
		 * @param width 表面宽度 / Surface width
		 * @param height 表面高度 / Surface height
		 */
		Surface(unsigned int width, unsigned int height);

		/**
		 * @brief 从文件加载图像表面 / Load image surface from file
		 *
		 * @param ImageFileName 图像文件路径 / Image file path
		 */
		Surface(std::string ImageFileName);

		/**
		 * @brief 移动构造函数 / Move constructor
		 */
		Surface(Surface&&) = default;

		/**
		 * @brief 禁用拷贝构造 / Disabled copy constructor
		 */
		Surface(const Surface&) = delete;

		/**
		 * @brief 析构函数 / Destructor
		 */
		~Surface() = default;

		/**
		 * @brief 初始化指定尺寸的空表面 / Initialize empty surface with specified size
		 *
		 * @param width 表面宽度 / Surface width
		 * @param height 表面高度 / Surface height
		 */
		void InitializeSurface(unsigned int width, unsigned int height);

		/**
		 * @brief 从文件初始化图像表面 / Initialize image surface from file
		 *
		 * @param ImageFileName 图像文件路径 / Image file path
		 */
		void InitializeSurface(std::string ImageFileName);

		/**
		 * @brief 用指定颜色填充表面 / Fill surface with specified color
		 *
		 * @param fillValue 填充颜色 / Fill color
		 */
		void ClearSurface(Color fillValue);

		/**
		 * @brief 替换指定位置的像素 / Replace pixel at specified position
		 *
		 * @param x X坐标 / X coordinate
		 * @param y Y坐标 / Y coordinate
		 * @param ReplaceColor 替换颜色 / Replacement color
		 */
		void ReplacePixel(unsigned int x, unsigned int y, Color ReplaceColor);

		/**
		 * @brief 保存表面到文件 / Save surface to file
		 *
		 * 支持PNG、JPG、BMP格式（根据扩展名自动选择）
		 * Supports PNG, JPG, BMP formats (auto-selected based on extension)
		 *
		 * @param SavePath 保存路径 / Save path
		 */
		void SaveSurface(std::string SavePath);

		/**
		 * @brief 获取指定位置的像素（只读）
		 *        Get pixel at specified position (read-only)
		 *
		 * @param x X坐标 / X coordinate
		 * @param y Y坐标 / Y coordinate
		 * @return const Color& 像素颜色引用 / Pixel color reference
		 */
		const Color& GetPixel(unsigned int x, unsigned int y) const noexcept;

		/**
		 * @brief 获取表面宽度 / Get surface width
		 *
		 * @return const int 表面宽度 / Surface width
		 */
		const int GetSurfaceWidth() const noexcept;

		/**
		 * @brief 获取表面高度 / Get surface height
		 *
		 * @return const int 表面高度 / Surface height
		 */
		const int GetSurfaceHeight() const noexcept;

		/**
		 * @brief 获取行字节间距 / Get row byte pitch
		 *
		 * @return const int 行字节数 / Number of bytes per row
		 */
		const int GetBytePitch() const noexcept;

		/**
		 * @brief 获取缓冲区数据指针 / Get buffer data pointer
		 *
		 * @return Color* 像素缓冲区指针 / Pixel buffer pointer
		 */
		Color* GetBufferData() const noexcept;

		/**
		 * @brief 创建新的空表面（静态工厂方法）
		 *        Create new empty surface (static factory method)
		 *
		 * @param width 表面宽度 / Surface width
		 * @param height 表面高度 / Surface height
		 * @return std::shared_ptr<Surface> 表面智能指针 / Surface smart pointer
		 */
		static std::shared_ptr<Surface> CreateNewSurface(unsigned int width, unsigned int height);

		/**
		 * @brief 从文件创建新表面（静态工厂方法）
		 *        Create new surface from file (static factory method)
		 *
		 * @param ImageFileName 图像文件路径 / Image file path
		 * @return std::shared_ptr<Surface> 表面智能指针 / Surface smart pointer
		 */
		static std::shared_ptr<Surface> CreateNewSurface(std::string ImageFileName);

		/**
		 * @brief 从ScratchImage移动构造 / Move construct from ScratchImage
		 *
		 * @param scratch DirectXTex图像数据 / DirectXTex image data
		 */
		Surface(DirectX::ScratchImage scratch) noexcept;

	private:
		/**
		 * @brief 私有构造函数（带缓冲区）
		 *        Private constructor (with buffer)
		 *
		 * @param width 表面宽度 / Surface width
		 * @param height 表面高度 / Surface height
		 * @param pBuffer 颜色缓冲区 / Color buffer
		 */
		Surface(unsigned int width, unsigned int height, std::unique_ptr<Color[]> pBuffer);

		DirectX::ScratchImage SurfaceImage;  ///< 主图像数据 / Main image data
		DirectX::ScratchImage scratch;       ///< 备用图像数据 / Backup image data
	};
}
