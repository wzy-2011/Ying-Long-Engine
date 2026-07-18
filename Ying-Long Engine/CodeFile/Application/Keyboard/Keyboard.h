/**
 * @file Keyboard.h
 * @brief 键盘输入类 / Keyboard input class
 *
 * 封装 Windows 键盘输入，提供按键状态查询、按键事件队列、字符事件队列。
 * 由 Window 类在窗口过程中调用 OnKeyPressed/OnKeyReleased/OnChar 来更新状态。
 *
 * Encapsulates Windows keyboard input, providing key state query, key event
 * queue, and character event queue. Updated by the Window class in the window
 * procedure via OnKeyPressed/OnKeyReleased/OnChar.
 */

#pragma once
#include <queue>
#include <bitset>
#include <optional>

namespace YingLong
{
	/**
	 * @brief 键盘输入管理器 / Keyboard input manager
	 *
	 * 维护 256 个按键的当前状态（bitset），以及按键事件队列和字符事件队列。
	 * 事件队列有大小限制（默认 16），超出时会丢弃最旧的事件。
	 *
	 * Maintains the current state of 256 keys (bitset), as well as a key event
	 * queue and a character event queue. Event queues have a size limit (default
	 * 16), dropping the oldest events when exceeded.
	 */
	class Keyboard
	{
		friend class Window;  ///< Window 类需要调用私有输入处理函数 / Window class needs to call private input handlers

	public:
		/**
		 * @brief 键盘事件 / Keyboard event
		 *
		 * 表示单个按键的按下或释放事件，携带按键码。
		 * Represents a single key press or release event, carrying the key code.
		 */
		class Event
		{
		public:
			/**
			 * @brief 事件类型枚举 / Event type enum
			 */
			enum class Type
			{
				Press,    ///< 按键按下 / Key pressed
				Release,  ///< 按键释放 / Key released
			};

			unsigned char code;  ///< 按键虚拟码（0-255）/ Virtual key code (0-255)

			/**
			 * @brief 构造键盘事件 / Construct a keyboard event
			 * @param type 事件类型 / Event type
			 * @param code 按键码 / Key code
			 */
			Event(Type type, unsigned char code) noexcept
				:
				type(type),
				code(code)
			{
			}

			/// @return true 是按下事件 / true if press event
			bool IsPress() const noexcept
			{
				return type == Type::Press;
			}

			/// @return true 是释放事件 / true if release event
			bool IsRelease() const noexcept
			{
				return type == Type::Release;
			}

			/// @return 按键码 / Key code
			unsigned char GetCode() const noexcept
			{
				return code;
			}

		private:
			Type type;  ///< 事件类型 / Event type
		};

		Keyboard() = default;
		Keyboard(const Keyboard&) = delete;
		Keyboard& operator=(const Keyboard&) = delete;

		/**
		 * @brief 查询指定按键当前是否按下 / Query whether a specific key is currently pressed
		 * @param keycode 按键虚拟码 / Virtual key code
		 * @return true 按下 / pressed
		 * @return false 未按下 / not pressed
		 */
		bool KeyIsPressed(unsigned char keycode) const noexcept;

		/**
		 * @brief 从按键事件队列读取一个事件（弹出）/ Read one event from key event queue (pops)
		 * @return 事件（队列为空时返回空 optional）/ Event (empty optional if queue is empty)
		 */
		std::optional<Event> ReadKey() noexcept;

		/// @return 按键事件队列是否为空 / Whether key event queue is empty
		bool KeyIsEmpty() const noexcept;

		/// @brief 清空按键事件队列 / Flush key event queue
		void FlushKey() noexcept;

		/**
		 * @brief 从字符事件队列读取一个字符（弹出）/ Read one char from char event queue (pops)
		 * @return 字符（队列为空时返回空 optional）/ Char (empty optional if queue is empty)
		 */
		std::optional<char> ReadChar() noexcept;

		/// @return 字符事件队列是否为空 / Whether char event queue is empty
		bool CharIsEmpty() const noexcept;

		/// @brief 清空字符事件队列 / Flush char event queue
		void FlushChar() noexcept;

		/// @brief 清空所有队列（按键 + 字符）/ Flush all queues (key + char)
		void Flush() noexcept;

		/// @brief 启用按键自动重复 / Enable key autorepeat
		void EnableAutorepeat() noexcept;

		/// @brief 禁用按键自动重复 / Disable key autorepeat
		void DisableAutorepeat() noexcept;

		/// @return 自动重复是否启用 / Whether autorepeat is enabled
		bool AutorepeatIsEnabled() const noexcept;

	private:
		/// @brief 按键按下时由 Window 调用 / Called by Window when a key is pressed
		void OnKeyPressed(unsigned char keycode) noexcept;
		/// @brief 按键释放时由 Window 调用 / Called by Window when a key is released
		void OnKeyReleased(unsigned char keycode) noexcept;
		/// @brief 收到字符消息时由 Window 调用 / Called by Window when a char message is received
		void OnChar(char character) noexcept;
		/// @brief 窗口失去焦点时清空所有按键状态 / Clear all key states when window loses focus
		void ClearState() noexcept;

		/**
		 * @brief 修剪缓冲区，保持大小不超过 bufferSize
		 *        Trim buffer to keep size no greater than bufferSize
		 * @tparam T 缓冲区元素类型 / Buffer element type
		 * @param buffer 要修剪的队列 / Queue to trim
		 */
		template<typename T>
		static void TrimBuffer(std::queue<T>& buffer) noexcept;

		static constexpr unsigned int nKeys = 256u;     ///< 支持的按键总数 / Total number of supported keys
		static constexpr unsigned int bufferSize = 16u;  ///< 事件队列最大容量 / Max event queue capacity

		bool autorepeatEnabled = false;          ///< 是否启用自动重复 / Whether autorepeat is enabled
		std::bitset<nKeys> keystates;            ///< 按键状态位集 / Key state bitset
		std::queue<Event> keybuffer;             ///< 按键事件队列 / Key event queue
		std::queue<char> charbuffer;             ///< 字符事件队列 / Character event queue
	};
}
