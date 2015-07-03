
#ifndef __COMPAT_H__
#define __COMPAT_H__

/**
 * @file compat.h Compatibility with older/non-standard compilers/libs
 */

#if defined(__arm__)
	/* compatibility with strange g++ versions on ARM */

	// C++0x keywords introduced with g++ 4.7:
	#define override
	#define final
	#define noexcept throw()
	#define nullptr NULL
#endif

#endif
