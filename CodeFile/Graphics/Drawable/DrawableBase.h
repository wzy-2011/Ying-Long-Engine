/** @file DrawableBase.h
 *  @brief 可绘制对象基类模板 - Drawable base class template
 *
 *  提供静态绑定的模板基类，使用 CRTP 模式实现同类对象共享静态绑定资源。
 *  Provides a template base class with static bindings, using CRTP pattern
 *  to share static binding resources among objects of the same type.
 */
#pragma once
#include <assert.h>
#include <memory>
#include <vector>
#include "../Bindable/Bindable.h"
#include "../Bindable/IndexBuffer.h"
#include "../Drawable/Drawable.h"

namespace YingLong
{
	/** @brief 可绘制对象基类模板
	 *  Drawable base class template
	 *
	 *  使用奇异递归模板模式(CRTP)，为派生类提供静态绑定资源的共享机制。
	 *  同类的所有实例共享同一份静态绑定数据（如顶点着色器、像素着色器、输入布局等），
	 *  从而减少内存占用和状态切换开销。
	 *
	 *  Uses the Curiously Recurring Template Pattern (CRTP) to provide a shared
	 *  static binding resource mechanism for derived classes. All instances of
	 *  the same type share the same static binding data (e.g., vertex shader,
	 *  pixel shader, input layout, etc.), reducing memory footprint and state
	 *  switching overhead.
	 *
	 *  @tparam T 派生类类型 / Derived class type
	 */
	template<class T>
	class DrawableBase : public Drawable
	{
	public:
		/** @brief 检查静态绑定是否已初始化
		 *  Check if static bindings have been initialized
		 *
		 *  @return 如果静态绑定列表非空则返回 true / Returns true if static binds list is not empty
		 */
		bool IsStaticInitialized() const noexcept
		{
			return !StaticBinds.empty();
		}

		/** @brief 添加静态绑定对象
		 *  Add a static bindable object
		 *
		 *  将可绑定对象添加到静态绑定列表中，供该类型的所有实例共享。
		 *  注意：索引缓冲区必须使用 AddStaticIndexBuffer 单独添加。
		 *
		 *  Adds a bindable object to the static binding list, shared by all
		 *  instances of this type. Note: Index buffers must be added separately
		 *  using AddStaticIndexBuffer.
		 *
		 *  @param bind 要添加的可绑定对象的唯一指针 / Unique pointer to the bindable to add
		 */
		void AddStaticBind(std::unique_ptr<Bindable> bind) noexcept
		{
			// 确保不通过此方法添加索引缓冲区
			// Ensure index buffer is not added through this method
			assert("*Must* use AddIndexBuffer to bind index buffer!"
				&& typeid(*bind) != typeid(IndexBuffer));
			StaticBinds.push_back(std::move(bind));
		}

		/** @brief 添加静态索引缓冲区
		 *  Add a static index buffer
		 *
		 *  将索引缓冲区添加到静态绑定列表中，并保存其指针以便绘制时使用。
		 *  Adds the index buffer to the static binding list and saves its
		 *  pointer for use during drawing.
		 *
		 *  @param IndexBufferObject 索引缓冲区对象的唯一指针 / Unique pointer to the index buffer object
		 */
		void AddStaticIndexBuffer(std::unique_ptr<IndexBuffer> IndexBufferObject) noexcept
		{
			// 确保只添加一次索引缓冲区
			// Ensure index buffer is only added once
			assert(pIndexBuffer == nullptr);
			pIndexBuffer = IndexBufferObject.get();
			StaticBinds.push_back(std::move(IndexBufferObject));
		}

		/** @brief 从静态绑定中查找索引缓冲区
		 *  Find index buffer from static bindings
		 *
		 *  当静态绑定时已经初始化（由其他实例创建）时，遍历静态绑定列表
		 *  找到索引缓冲区并保存其指针。
		 *
		 *  When static bindings are already initialized (created by another
		 *  instance), traverse the static binding list to find the index
		 *  buffer and save its pointer.
		 */
		void FindIndexBufferFromStatic()
		{
			// 遍历静态绑定列表，查找索引缓冲区
			// Traverse static binds list to find index buffer
			for (auto& bindable : StaticBinds)
			{
				if (const auto p = dynamic_cast<IndexBuffer*>(bindable.get()))
				{
					pIndexBuffer = p;
				}
			}

			// 确保找到索引缓冲区
			// Ensure index buffer was found
			assert("No static index buffer found!");
		}

	private:
		/** @brief 获取静态绑定列表的常量引用
		 *  Get const reference to static binds list
		 *
		 *  实现 Drawable 基类的纯虚函数，返回静态绑定列表。
		 *  Implements the pure virtual function from Drawable base class,
		 *  returning the static binds list.
		 *
		 *  @return 静态绑定列表的常量引用 / Const reference to static binds list
		 */
		const std::vector<std::unique_ptr<Bindable>>& GetStaticBinds() const noexcept
		{
			return StaticBinds;
		}

		static std::vector<std::unique_ptr<Bindable>> StaticBinds; ///< 静态绑定列表，同类所有实例共享 / Static binds list, shared by all instances of same type
	};

	/// 静态成员变量定义 / Static member variable definition
	template<class T>
	std::vector<std::unique_ptr<Bindable>> DrawableBase<T>::StaticBinds;
}
