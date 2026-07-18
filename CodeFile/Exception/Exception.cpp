#include "Exception.h"
#include "../Application/Window/Window.h"

YingLong::Exception::Exception(int line,
	const char* file) noexcept : line(line), file(file)
{

}

//printing error information at wnd
const char* YingLong::Exception::what() const noexcept
{
	std::ostringstream oss_throw_exception;
	oss_throw_exception << GetType() << std::endl
		<< GetOriginString();
	what_buffer = oss_throw_exception.str();
	return what_buffer.c_str();
}

const char* YingLong::Exception::GetType() const noexcept
{
	//the name of the wnd that throw exceptions 
	return "���������쳣";
}

//get wrong line
int YingLong::Exception::GetLine() const noexcept
{
	return line;
}

//get wrong file
const std::string& YingLong::Exception::GetFile() const noexcept
{
	return file;
}

//get exceptions' origin
std::string YingLong::Exception::GetOriginString() const noexcept
{
	std::ostringstream oss_get_origin;
	oss_get_origin << "�����ļ�Ŀ¼��"
		<< file << std::endl
		<< "����������" << line;
	return oss_get_origin.str();
}
