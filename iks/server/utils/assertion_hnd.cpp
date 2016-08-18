#include "utils/exception.h"

/**
 * This is a fix for asserting:
 *
 * assert_fail ("!p.empty() && \"Empty path not allowed for put_child.\"",
 * "/usr/include/boost/property_tree/detail/ptree_implementation.hpp", line=886
 *
 * (and potentially all other BOOST_ASSERTS) at runtime.
 *
 * Instead, we shall throw exceptions each time a BOOST_ASSERT
 * or BOOST_ASSERT_MSG (TODO) fails.
 * http://www.boost.org/doc/libs/1_55_0/libs/utility/assert.html 
 * (check definition of handler in .cpp file)
 *
 * This will affect all files compiled with BOOST_ENABLE_ASSERT_HANDLER defined
 */

namespace boost
{
	void assertion_failed(char const * expr, char const * function, char const * file, long line)
	{
		throw assertion_error(std::string("BOOST_ASSERT failed: assertion=") + expr + ", function="
			+ function + ", file=" + file + ", line=" + std::to_string(line));
	}
}
