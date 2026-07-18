/** @file Drawable.h
 *  @brief 可绘制对象基类（已弃用） - Drawable base class (deprecated)
 *
 *  包含旧版 DX11 可绘制对象的抽象基类定义。
 *  新代码应使用 Graphics/DX12/ 目录中的 DX12Drawable。
 *
 *  Contains the abstract base class definition for legacy DX11 drawable objects.
 *  New code should use DX12Drawable in the Graphics/DX12/ directory.
 */
#pragma once

// =============================================================================
// DEPRECATED: This file contains the old DX11 Drawable implementation.
// New code should use DX12Drawable in the Graphics/DX12/ directory.
// =============================================================================
// 已弃用：此文件包含旧版 DX11 Drawable 实现。
// 新代码应使用 Graphics/DX12/ 目录中的 DX12Drawable。
// =============================================================================

#pragma message("WARNING: Drawable.h is deprecated. Use DX12Drawable instead.")

#include <cassert>
#include <typeinfo>
#include <memory>
#include <vector>
#include <fstream>
#include <DirectXMath.h>
#include "../Bindable/Bindable.h"
#include "../Bindable/IndexBuffer.h"
#include "../AABB/AABB.h"
#include "../../ImGui/CodeFile/ImGui/imgui.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace YingLong
{
	class Bindable;
	class Graphics;

	/** @brief 可绘制对象抽象基类（已弃用）
	 *  Abstract base class for drawable objects (deprecated)
	 *
	 *  所有 DX11 可绘制对象的抽象基类，定义了绘制、更新、变换等基本接口。
	 *  维护实例绑定和静态绑定两套绑定系统，并管理索引缓冲区。
	 *  新代码请使用 DX12Drawable。
	 *
	 *  Abstract base class for all DX11 drawable objects, defining basic interfaces
	 *  for drawing, updating, and transformation. Maintains two binding systems:
	 *  instance binds and static binds, and manages the index buffer.
	 *  Use DX12Drawable for new code.
	 *
	 *  @deprecated 请使用 DX12Drawable 替代 / Use DX12Drawable instead
	 */
	class Drawable [[deprecated("Use DX12Drawable instead")]]
	{
		template<class T>
		friend class DrawableBase;

	public:
		/** @brief 默认构造函数
		 *  Default constructor
		 */
		Drawable() = default;

		/** @brief 拷贝构造函数
		 *  Copy constructor
		 *
		 *  @param 要拷贝的可绘制对象 / The drawable to copy from
		 */
		Drawable(const Drawable&);

		/** @brief 获取变换矩阵（纯虚函数）
		 *  Get transformation matrix (pure virtual)
		 *
		 *  返回该可绘制对象的世界变换矩阵，用于着色器中的顶点变换。
		 *  Returns the world transformation matrix for this drawable, used for
		 *  vertex transformation in the shader.
		 *
		 *  @return DirectX 变换矩阵 / DirectX transformation matrix
		 */
		virtual DirectX::XMMATRIX GetTransformXM() const noexcept = 0;

		/** @brief 绘制可绘制对象
		 *  Draw the drawable object
		 *
		 *  依次绑定实例级绑定和静态级绑定，然后执行索引绘制。
		 *  Binds instance-level and static-level bindings sequentially, then
		 *  performs indexed drawing.
		 *
		 *  @param graphics 图形设备对象引用 / Graphics device object reference
		 */
		void Draw(Graphics& graphics) const noexcept;

		/** @brief 更新可绘制对象状态（纯虚函数）
		 *  Update drawable object state (pure virtual)
		 *
		 *  每帧调用，用于更新动画、位置、旋转等状态。
		 *  Called every frame to update animation, position, rotation, etc.
		 *
		 *  @param dt 时间增量（秒） / Time delta in seconds
		 *  @param aspect 宽高比 / Aspect ratio
		 */
		virtual void Update(float dt, float aspect) noexcept = 0;

		/** @brief 添加实例级绑定对象
		 *  Add an instance-level bindable object
		 *
		 *  将可绑定对象添加到实例绑定列表中。
		 *  注意：索引缓冲区必须使用 AddIndexBuffer 单独添加。
		 *
		 *  Adds a bindable object to the instance binding list.
		 *  Note: Index buffers must be added separately using AddIndexBuffer.
		 *
		 *  @param bind 要添加的可绑定对象的唯一指针 / Unique pointer to the bindable to add
		 */
		void AddBind(std::unique_ptr<Bindable> bind) noexcept;

		/** @brief 添加实例级索引缓冲区
		 *  Add an instance-level index buffer
		 *
		 *  将索引缓冲区添加到实例绑定列表中，并保存其指针以便绘制时使用。
		 *  Adds the index buffer to the instance binding list and saves its
		 *  pointer for use during drawing.
		 *
		 *  @param IndexBufferObject 索引缓冲区对象的唯一指针 / Unique pointer to the index buffer object
		 */
		void AddIndexBuffer(std::unique_ptr<class IndexBuffer> IndexBufferObject) noexcept;

		/** @brief 虚析构函数
		 *  Virtual destructor
		 */
		virtual ~Drawable() = default;

	private:
		/** @brief 获取静态绑定列表的常量引用（纯虚函数）
		 *  Get const reference to static binds list (pure virtual)
		 *
		 *  由派生类实现，返回该类型的静态绑定列表。
		 *  Implemented by derived classes to return the static binds list
		 *  for this type.
		 *
		 *  @return 静态绑定列表的常量引用 / Const reference to static binds list
		 */
		virtual const std::vector<std::unique_ptr<Bindable>>&
			GetStaticBinds() const noexcept = 0;

		const class IndexBuffer* pIndexBuffer = nullptr; ///< 索引缓冲区指针 / Index buffer pointer
		std::vector<std::unique_ptr<Bindable>> binds;     ///< 实例级绑定列表 / Instance-level binds list
	};
}
