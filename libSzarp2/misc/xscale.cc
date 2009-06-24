/* 
  SZARP: SCADA software 

*/
/*

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 * Workaround for Moxa UC7410/7420 glibc bug.
 * File bits/atomicity.cc from Moxa version of glibc (included in Moxa cross-compiling
 * environment for UC7410/7420) version 3.3.2-6 is buggy - it contains initialization
 * of 'extern' variable - so linker gets error about mutliplt definition of variable
 * _gnu_cxx:_Atomic_add_nutex. The workaround is to remove initialization from
 * atomicity.h and add following code for one-time definition of variable.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ARM_ATOMICITY_BUG

#include <bits/atomicity.h>
using namespace __gnu_cxx;
__gthread_mutex_t __gnu_cxx::_Atomic_add_mutex = __GTHREAD_MUTEX_INIT;

#endif
