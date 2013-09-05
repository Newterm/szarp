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

#ifndef __LIBLOG_IMPL_ASYSLOG_H__
#define __LIBLOG_IMPL_ASYSLOG_H__
/** Implementation using libevent-ased synchronous syslog library*/
void* sz_log_asyslog_init(int level, const char* logname, SZ_LIBLOG_FACILITY facility, void *context);

void sz_log_asyslog_close(void* data); 

void sz_log_asyslog_vlog(void* data, int level, const char * msg_format, va_list fmt_args);
#endif
