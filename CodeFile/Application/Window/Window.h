/**
 * @file Window.h
 * @brief 窗口管理类 / Window management class
 *
 * 封装 Win32 窗口创建与管理，包含窗口类注册、窗口过程、
 * 键盘/鼠标输入、DX11/DX12 图形设备、以及自定义异常类型。
 * 支持普通窗口和弹出窗口（启动画面）两种模式。
 *
 * Encapsulates Win32 window creation and management, including window class
 * registration, window procedure, keyboard/mouse input, DX11/DX12 graphics
 * devices, and custom exception types. Supports both normal window and
 * popup window (splash screen) modes.
 */

#pragma once
#include <Windows.h>
#include <memory>
#include <iostream>
#include "../../../Time/CodeFile/Time/Time.h"
#include "../../Exception/Exception.h"
#include "../../Graphics/Graphics.h"
#include "../Keyboard/Keyboard.h"
#include "../Mouse/Mouse.h"
#include "../../../resource.h"
#include "../../Graphics/Camera/Camera.h"

namespace YingLong
{
    // 前置声明，避免循环依赖 / Forward declaration to avoid circular dependency
    class DX12Renderer;

	/**
	 * @brief 窗口类 / Window class
	 *
	 * 管理 Win32 窗口的创建、销毁、消息处理。
	 * 包含 Graphics 图形设备、Keyboard/Mouse 输入设备、Camera 相机，
	 * 以及可选的 DX12Renderer（DX12 渲染模式）。
	 *
	 * Manages Win32 window creation, destruction, and message handling.
	 * Contains Graphics device, Keyboard/Mouse input devices, Camera,
	 * and an optional DX12Renderer (DX12 rendering mode).
	 */
	class Window
	{
	public:
		/**
		 * @brief 窗口异常基类 / Window exception base class
		 *
		 * 提供 HRESULT 错误码转字符串的静态工具函数。
		 * Provides a static utility function to translate HRESULT error codes to strings.
		 */
		class WinException : public Exception
		{
		using Exception::Exception;
		public:
			/**
			 * @brief 将 HRESULT 错误码翻译为人类可读的字符串
			 *        Translate an HRESULT error code to a human-readable string
			 * @param HRESULTWindowException 错误码 / Error code
			 * @return std::string 错误描述字符串 / Error description string
			 */
			static std::string TranslateErrorCode(HRESULT HRESULTWindowException) noexcept;
		};

		/**
		 * @brief HRESULT 窗口异常 / HRESULT window exception
		 *
		 * 携带 HRESULT 错误码的窗口异常，提供错误码获取和错误描述。
		 * Window exception carrying an HRESULT error code, providing error code
		 * retrieval and error description.
		 */
		class HRESULTException : public Exception
		{
			using Exception::Exception;
		public:
			/**
			 * @brief 构造 HRESULT 异常 / Construct HRESULT exception
			 * @param line 行号 / Line number
			 * @param file 文件名 / File name
			 * @param HRESULTException 错误码 / Error code
			 */
			HRESULTException(int line, const char* file, HRESULT HRESULTException) noexcept;

			/// @return 异常描述（含错误码和错误字符串）/ Exception description with error code and string
			const char* what() const noexcept override;

			/// @return 异常类型名称 / Exception type name
			const char* GetType() const noexcept override;

			/// @return HRESULT 错误码 / HRESULT error code
			HRESULT GetErrorCode() const noexcept;

			/// @return 错误码的字符串描述 / String description of the error code
			std::string GetErrorString() const noexcept;

		private:
			HRESULT HRESULTWindowException;  ///< 存储的错误码 / Stored error code
		};

		/**
		 * @brief 无图形设备异常 / No graphics device exception
		 *
		 * 当尝试访问不存在的 Graphics 设备时抛出。
		 * Thrown when attempting to access a non-existent Graphics device.
		 */
		class NoGfxException : public Exception
		{
			using Exception::Exception;
		public:
			/// @return 异常类型名称 / Exception type name
			const char* GetType() const noexcept override;
		};

	private:
		/**
		 * @brief 窗口类（WNDCLASSEX 封装）/ Window class (WNDCLASSEX wrapper)
		 *
		 * 封装 Win32 窗口类的注册与注销。每个 Window 对象持有一个 WindowClass。
		 * 普通窗口和弹出窗口使用不同的窗口过程。
		 *
		 * Encapsulates registration and unregistration of a Win32 window class.
		 * Each Window object holds one WindowClass. Normal windows and popup
		 * windows use different window procedures.
		 */
		class WindowClass
		{
		public:
			WindowClass() = default;

			/**
			 * @brief 注册窗口类 / Register window class
			 * @param windowTitle 窗口标题（用作类名前缀）/ Window title (used as class name prefix)
			 * @param popup 是否为弹出窗口 / Whether this is a popup window
			 */
			WindowClass(const wchar_t* windowTitle, bool popup);

			WindowClass(const WindowClass& ) = default;

			/// @brief 注销窗口类 / Unregister window class
			~WindowClass();

			std::wstring pClassName;  ///< 窗口类名 / Window class name
			HINSTANCE hInstance;      ///< 应用程序实例句柄 / Application instance handle

			friend class Window;
		};

	public:
		/**
		 * @brief 构造窗口 / Construct window
		 * @param width 窗口宽度 / Window width
		 * @param height 窗口高度 / Window height
		 * @param Name 窗口标题 / Window title
		 * @param popup 是否为弹出窗口（无标题栏，用于启动画面）/
		 *        Whether it's a popup window (no title bar, for splash screen)
		 */
		Window(int width, int height, const wchar_t* Name, bool popup = false);

		/// @brief 析构窗口（调用 Destroy）/ Destruct window (calls Destroy)
		~Window() noexcept;

		Window(const Window&) = delete;
		WindowClass& operator = (const WindowClass&) = delete;

		/// @brief 显示窗口 / Show window
		void Display();

		/// @brief 销毁窗口 / Destroy window
		void Destroy();

		/**
		 * @brief 获取 Graphics 引用 / Get Graphics reference
		 * @return Graphics& 图形设备引用 / Graphics device reference
		 * @throws std::runtime_error 如果 pGraphics 为 null / if pGraphics is null
		 */
		Graphics& graphics();

		/// @brief 初始化 DX12 渲染器 / Initialize DX12 renderer
		void InitializeDX12();

		/// @brief 关闭 DX12 渲染器 / Shutdown DX12 renderer
		void ShutdownDX12();

		/// @return DX12 渲染器指针（可能为 null）/ DX12 renderer pointer (may be null)
		class DX12Renderer* GetDX12Renderer() const;

		/// @return 是否启用了 DX12 模式 / Whether DX12 mode is enabled
		bool IsDX12Enabled() const noexcept { return bUseDX12; }

		/**
		 * @brief 处理窗口消息（静态工具函数）/ Process window messages (static utility)
		 * @return 退出码（收到 WM_QUIT 时），否则返回空 optional
		 *         Exit code (when WM_QUIT received), empty optional otherwise
		 */
		static std::optional<int> ProcessMessages() noexcept;

		/// @return 窗口句柄 / Window handle
		HWND GetWindowHandle() const;

		Camera camera;  ///< 相机对象 / Camera object

	private:
		// ===== 窗口过程：普通窗口 / Window procedure: normal window =====
		static LRESULT CALLBACK HandleMessageSetup(HWND hWnd,
			UINT Message, WPARAM wParam, LPARAM lParam) noexcept;
		static LRESULT CALLBACK HandleMessageThunk(HWND hWnd,
			UINT Message, WPARAM wParam, LPARAM lParam) noexcept;
		LRESULT HandleMessage(HWND hWnd, UINT Message,
			WPARAM wParam, LPARAM lParam) noexcept;

		// ===== 窗口过程：弹出窗口 / Window procedure: popup window =====
		static LRESULT CALLBACK PopupHandleMessageSetup(HWND hWnd,
			UINT Message, WPARAM wParam, LPARAM lParam) noexcept;
		static LRESULT CALLBACK PopupHandleMessageThunk(HWND hWnd,
			UINT Message, WPARAM wParam, LPARAM lParam) noexcept;
		LRESULT PopupHandleMessage(HWND hWnd, UINT Message,
			WPARAM wParam, LPARAM lParam) noexcept;

		WindowClass WindowClassObject;  ///< 窗口类实例 / Window class instance
		int Width;                      ///< 窗口宽度 / Window width
		int Height;                     ///< 窗口高度 / Window height
		HWND hWnd;                      ///< 窗口句柄 / Window handle

		bool CursorEnabled = true;      ///< 光标是否启用（用于 FPS 风格相机）/
		                                ///< Whether cursor is enabled (for FPS-style camera)

	public:
		Keyboard keyboard;  ///< 键盘输入设备 / Keyboard input device
		Mouse mouse;        ///< 鼠标输入设备 / Mouse input device

		static Time time;    ///< 全局时间对象 / Global time object
		static Timer timer;  ///< 全局计时器 / Global timer

	private:
		/// @brief 将光标限制在窗口客户区内 / Confine cursor to window client area
		void ConfineCursor() noexcept;
		/// @brief 释放光标限制 / Free cursor confinement
		void FreeCursor() noexcept;
		/// @brief 显示光标 / Show cursor
		void ShowCursor() noexcept;
		/// @brief 隐藏光标 / Hide cursor
		void HideCursor() noexcept;

		std::unique_ptr<Graphics> pGraphics;  ///< DX11 图形设备 / DX11 graphics device

		// DX12 渲染器（DX11 的可选替代）/ DX12 Renderer (optional alternative to DX11)
		std::unique_ptr<class DX12Renderer> pDX12Renderer;
		bool bUseDX12;  ///< 是否使用 DX12 渲染 / Whether to use DX12 rendering

		bool IsDragging;  ///< 弹出窗口是否正在拖动 / Whether popup window is being dragged
	};
}

/// @brief 创建 HRESULT 窗口异常的宏 / Macro to create HRESULT window exception
#define WINDOW_EXCEPTION(hr) Window::HRESULTException(__LINE__, __FILE__, hr)

/// @brief 用 GetLastError() 创建窗口异常的宏 / Macro to create window exception with GetLastError()
#define WINDOW_LAST_EXCEPTION() Window::HRESULTException(__LINE__, __FILE__, GetLastError())
