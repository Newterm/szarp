/* Custom assert to prevent gcc warning output with -Wunused-but-set-variable flag */

#ifndef __CUSTOM_ASSERT_H
#define __CUSTOM_ASSERT_H

#ifdef NDEBUG
#define ASSERT(expr) (void)sizeof(expr)
#else
#include <assert.h>
#define ASSERT(x) assert(x)
#endif

#endif /* __CUSTOM_ASSERT_H */
