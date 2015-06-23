#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <exception>
#include <stdio.h>
#include <stdarg.h>

/** size of buffer for exception message */
#define EXT_BUFFER_SIZE	256

/** std::exception subclass, with additional message set with SetMsg method */
class MsgException: public std::exception {
public:
	/** Buffer for messages. */
	char buffer[EXT_BUFFER_SIZE];
	/** Method called to get exception info. */
	const char* what() const noexcept override
	{
		return buffer;
	}
	MsgException() { buffer[0] = 0; }
	/** Set message printed after throw. */
	void SetMsg(const char * format, ...) noexcept
	{
		va_list fmt_args;
		va_start(fmt_args, format);
		vsnprintf(buffer, EXT_BUFFER_SIZE - 1, format, fmt_args);
		va_end(fmt_args);
		buffer[EXT_BUFFER_SIZE - 1] = 0;
	}
};

#endif // __EXCEPTION_H__
