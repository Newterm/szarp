#ifndef __COMPILER_H
#define __COMPILER_H

/* GCC */
#ifdef __GNUC__

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#endif  /* GCC_VERSION */

#endif  /* __GNUC__ */
/* End of GCC */

#endif  /* __COMPILER_H */
