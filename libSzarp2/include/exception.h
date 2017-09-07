#ifndef __SZEXCEPTION_H
#define __SZEXCEPTION_H

#include <cstring>
#include <stdexcept>
#include "compiler.h"

/**
 * SZ_INHERIT_CONSTR(cls, base) - inherit constructors from SzException (or its
 *                                derivatives).
 *
 * This macro is somewhat old-fashioned C-style solution for shortening
 * a definitions of new exceptions that derive from SzException. But that's the
 * only way to do it before C++11 standard! Hopefully in the future this can be
 * removed and all calls could be replaced with C++11 "using" declaration.
 */
#if GCC_VERSION >= 40800
#define SZ_INHERIT_CONSTR(cls, base) using base::base;	// only C++11
#else
#define SZ_INHERIT_CONSTR(cls, base) \
public: \
	explicit cls (const std::string& what_arg) \
		: base(what_arg) { } \
	explicit cls (const char* what_arg) \
		: base(what_arg) { } \
	explicit cls (int errnum, const std::string& what_arg) \
		: base(errno_what(errnum, what_arg)) { }
#endif

/**
 * class SzException - base class for all SZARP-specific exceptions.
 *
 * Use SZ_INHERIT_CONSTR() macro to inherit SzException's constructors.
 *
 * Example:
 *
 *  class ExampleError : public SzException {
 *  	SZ_INHERIT_CONSTR(ExampleError, SzException)
 *  };
 *
 *  class NewExampleError : public ExampleError {
 *  	SZ_INHERIT_CONSTR(NewExampleError, ExampleError)
 *  };
 *
 */
class SzException : public std::runtime_error
{
#if GCC_VERSION >= 40800
	using std::runtime_error::runtime_error;
#else
public:
	explicit SzException (const std::string& what_arg)
		: std::runtime_error(what_arg) { }
	explicit SzException (const char* what_arg)
		: std::runtime_error(what_arg) { }
#endif
public:
	explicit SzException (int errnum, const std::string& what_arg)
		: std::runtime_error(errno_what(errnum, what_arg)) { }

protected:
	static std::string errno_what (int errnum, const std::string& what_arg) {
		return what_arg + ", errno " + std::to_string(errnum)
			+ " (" + strerror(errnum) + ")";
	}
};

#endif /* __SZEXCEPTION_H */
