/**
 * @file Mouse.h
 * @brief 鼠标输入类 / Mouse input class
 *
 * 封装 Windows 鼠标输入，提供位置查询、按键状态查询、鼠标事件队列。
 * 由 Window 类在窗口过程中调用 OnMouseMove/OnLeftPressed 等函数来更新状态。
 *
 * Encapsulates Windows mouse input, providing position query, button state
 * query, and mouse event queue. Updated by the Window class in the window
 * procedure via OnMouseMove/OnLeftPressed etc.
 */

#pragma once
#include <queue>
#include "../Keyboard/Keyboard.h"

namespace YingLong
{
	/**
	 * @brief 鼠标输入管理器 / Mouse input manager
	 *
	 * 维护鼠标位置、左右键状态，以及鼠标事件队列（移动、按键、滚轮）。
	 * 事件队列有大小限制（默认 16），超出时丢弃最旧的事件。
	 *
	 * Maintains mouse position, left/right button states, and mouse event
	 * queue (move, button, wheel). Event queue has a size limit (default 16),
	 * dropping oldest events when exceeded.
	 */
	class Mouse
	{
		friend class Window;  ///< Window 类需要调用私有输入处理函数 / Window class needs to call private input handlers

	public:
		/**
		 * @brief 鼠标事件 / Mouse event
		 *
		 * 表示单个鼠标事件，包含事件类型、按键状态、鼠标位置。
		 * Represents a single mouse event, containing event type, button state,
		 * and mouse position.
		 */
		class Event
		{
		public:
			/**
			 * @brief 鼠标事件类型枚举 / Mouse event type enum
			 */
			enum class Type
			{
				LPress,     ///< 左键按下 / Left button pressed
				LRelease,   ///< 左键释放 / Left button released
				RPress,     ///< 右键按下 / Right button pressed
				RRelease,   ///< 右键释放 / Right button released
				WheelUp,    ///< 滚轮向上 / Wheel up
				WheelDown,  ///< 滚轮向下 / Wheel down
				Move,       ///< 鼠标移动 / Mouse moved
				Invalid     ///< 无效事件（空队列时返回）/ Invalid event (returned when queue is empty)
			};

		private:
			Type type;               ///< 事件类型 / Event type
			bool leftIsPressed;      ///< 左键是否按下 / Whether left button is pressed
			bool rightIsPressed;     ///< 右键是否按下 / Whether right button is pressed
			int x, y;                ///< 鼠标坐标（客户区）/ Mouse position (client area)

		public:
			/**
			 * @brief 默认构造（无效事件）/ Default constructor (invalid event)
			 */
			Event() noexcept :
				type(Type::Invalid),
				leftIsPressed(false),
				rightIsPressed(false),
				x(0),
				y(0)
			{
			}

			/**
			 * @brief 从父 Mouse 对象构造事件（快照当前状态）
			 *        Construct event from parent Mouse object (snapshots current state)
			 * @param type 事件类型 / Event type
			 * @param parent 父 Mouse 对象引用 / Parent Mouse object reference
			 */
			Event(Type type, const Mouse& parent) noexcept :
				type(type),
				leftIsPressed(parent.LeftIsPressed()),
				rightIsPressed(parent.RightIsPressed()),
				x(parent.GetPosX()),
				y(parent.GetPosY())
			{
			}

			/// @return 事件是否有效 / Whether the event is valid
			bool IsValid() const noexcept
			{
				return type != Type::Invalid;
			}

			/// @return 事件类型 / Event type
			Type GetType() const noexcept
			{
				return type;
			}

			/// @return 鼠标位置（x, y）/ Mouse position (x, y)
			std::pair <int, int> GetPos() const noexcept
			{
				return { x, y };
			}

			/// @return 鼠标 X 坐标 / Mouse X coordinate
			int GetPosX() const noexcept
			{
				return x;
			}

			/// @return 鼠标 Y 坐标 / Mouse Y coordinate
			int GetPosY() const noexcept
			{
				return y;
			}

			/// @return 左键是否按下 / Whether left button is pressed
			bool LeftIsPressed() const noexcept
			{
				return leftIsPressed;
			}

			/// @return 右键是否按下 / Whether right button is pressed
			bool RightIsPressed() const noexcept
			{
				return rightIsPressed;
			}
		};

	public:
		Mouse() = default;

		Mouse(const Mouse&) = delete;
		Mouse& operator = (const Mouse&) = delete;

		/// @return 鼠标位置（x, y）/ Mouse position (x, y)
		std::pair <int, int> GetPos() const noexcept;

		/// @return 鼠标 X 坐标 / Mouse X coordinate
		int GetPosX() const noexcept;

		/// @return 鼠标 Y 坐标 / Mouse Y coordinate
		int GetPosY() const noexcept;

		/// @return 左键是否按下 / Whether left button is pressed
		bool LeftIsPressed() const noexcept;

		/// @return 右键是否按下 / Whether right button is pressed
		bool RightIsPressed() const noexcept;

		/**
		 * @brief 从事件队列读取一个事件（弹出）/ Read one event from event queue (pops)
		 * @return 事件（空队列返回无效事件）/ Event (invalid event if queue empty)
		 */
		Mouse::Event Read() noexcept;

		/// @return 事件队列是否为空 / Whether event queue is empty
		bool IsEmpty() const noexcept
		{
			return buffer.empty();
		}

		/// @brief 清空事件队列 / Flush event queue
		void Flush() noexcept;

	private:
		/// @brief 鼠标移动时由 Window 调用 / Called by Window when mouse moves
		void OnMouseMove(int x, int y) noexcept;
		/// @brief 左键按下时由 Window 调用 / Called by Window when left button is pressed
		void OnLeftPressed(int x, int y) noexcept;
		/// @brief 左键释放时由 Window 调用 / Called by Window when left button is released
		void OnLeftRelease(int x, int y) noexcept;
		/// @brief 右键按下时由 Window 调用 / Called by Window when right button is pressed
		void OnRightPressed(int x, int y) noexcept;
		/// @brief 右键释放时由 Window 调用 / Called by Window when right button is released
		void OnRightRelease(int x, int y) noexcept;
		/// @brief 滚轮向上时由 Window 调用 / Called by Window when wheel scrolls up
		void OnWheelUp(int x, int y) noexcept;
		/// @brief 滚轮向下时由 Window 调用 / Called by Window when wheel scrolls down
		void OnWheelDown(int x, int y) noexcept;
		/// @brief 修剪事件队列到最大容量 / Trim event queue to max capacity
		void TrimBuffer() noexcept;

		static constexpr unsigned int BufferSize = 16u;  ///< 事件队列最大容量 / Max event queue capacity
		int x, y;                                         ///< 鼠标当前坐标 / Current mouse position
		bool leftIsPressed = false;                       ///< 左键状态 / Left button state
		bool rightIsPressed = false;                      ///< 右键状态 / Right button state
		std::queue <Event> buffer;                        ///< 鼠标事件队列 / Mouse event queue
	};
}
