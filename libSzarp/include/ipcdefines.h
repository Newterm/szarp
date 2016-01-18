/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/* $Id$

 */

#ifndef __IPC_DEFINES_H__
#define __IPC_DEFINES_H__

#include "szdefines.h"

#define SEM_PARCOOK 1

#define SEM_PROBE 0
#define SEM_MINUTE 2 
#define SEM_MIN10 4
#define SEM_HOUR 6
//#define SEM_DAY 8	/* not used anymore */
#define SEM_ALERT 10
#define SEM_PROBES_BUF 12
#define SEM_LINE 14

#define SHM_PROBE 1
#define SHM_MINUTE 2
#define SHM_MIN10 3
#define SHM_HOUR 4
//#define SHM_DAY 5	/* not used anymore */
//#define SHM_PTT 6	/* not used anymore */
#define SHM_ALERT 7
#define SHM_PROBES_BUF 8

#define NO_ALERT 0              /* brak przekroczenia zakresu */
#define ALERT1   1              /* przekroczenie stopnia wa¿no¶ci 1 */
#define ALERT2   2              /* przekroczenie stopnia wa¿no¶ci 2 */
#define ALERT3   3              /* przekroczenie stopnia wa¿no¶ci 3 */
#define ALERT4   4              /* przekroczenie stopnia wa¿no¶ci 4 */
#define ALERT5   5              /* przekroczenie stopnia wa¿no¶ci 5 */

#define FULL_NAME_LEN	161
#define ALT_NAME_LEN	24
#define SHORT_NAME_LEN	8

#define SHM_PROBES_BUF_POS_INDEX 0
#define SHM_PROBES_BUF_CNT_INDEX 1
#define SHM_PROBES_BUF_DATA_OFF  2


#endif // __IPC_DEFINES_H__
