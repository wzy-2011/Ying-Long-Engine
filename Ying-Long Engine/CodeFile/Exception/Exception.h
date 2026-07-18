/**
 * @file Exception.h
 * @brief 引擎异常基类 / Engine exception base class
 *
 * 定义 YingLong::Exception 基类，继承自 std::exception，
 * 提供行号、文件名、异常类型等信息的统一异常框架。
 * 所有引擎自定义异常都应继承此类。
 *
 * Defines the YingLong::Exception base class, inheriting from std::exception,
 * providing a unified exception framework with line number, file name,
 * exception type, etc. All engine custom exceptions should inherit from this class.
 */

#pragma once
#include <exception>
#include <string>
#include <sstream>

namespace YingLong
{
	/**
	 * @brief 引擎异常基类 / Engine exception base class
	 *
	 * 存储异常发生的行号和文件名，提供统一的 what() 接口。
	 * 子类通过重写 GetType() 来标识具体异常类型。
	 *
	 * Stores the line number and file name where the exception occurred,
	 * providing a unified what() interface. Subclasses override GetType()
	 * to identify specific exception types.
	 */
	class Exception : public std::exception
	{
	public:
		/**
		 * @brief 构造异常 / Construct exception
		 * @param line 异常发生的行号 / Line number where exception occurred
		 * @param file 异常发生的文件名 / File name where exception occurred
		 */
		Exception(int line, const char* file) noexcept;

		/**
		 * @brief 获取异常描述字符串 / Get exception description string
		 *
		 * 格式：异常类型 + 来源信息（文件 + 行号）
		 * Format: exception type + origin info (file + line)
		 *
		 * @return const char* 异常描述 C 字符串 / Exception description C-string
		 */
		virtual const char* what() const noexcept override;

		/**
		 * @brief 获取异常类型名称 / Get exception type name
		 * @return const char* 类型名称 / Type name
		 */
		virtual const char* GetType() const noexcept;

		/// @return 异常发生的行号 / Line number where exception occurred
		int GetLine() const noexcept;

		/// @return 异常发生的文件名 / File name where exception occurred
		const std::string& GetFile() const noexcept;

		/**
		 * @brief 获取异常来源的格式化字符串 / Get formatted string of exception origin
		 *
		 * 格式：[文件路径] + [行号]
		 * Format: [file path] + [line number]
		 *
		 * @return std::string 来源描述 / Origin description
		 */
		std::string GetOriginString() const noexcept;

	private:
		int line;           ///< 异常行号 / Exception line number
		std::string file;   ///< 异常文件名 / Exception file name

	protected:
		mutable std::string what_buffer;  ///< 用于存储 what() 返回的字符串（可变，因为 what 是 const）
		                                  ///< Buffer for what() return string (mutable because what is const)
	};
}
