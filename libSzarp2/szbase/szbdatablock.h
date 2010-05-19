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
#ifndef __SZBDATABLOCK_H__
#define __SZBDATABLOCK_H__

#include <config.h>

#include <limits.h>
#include <time.h>
#include <assert.h>

#include "szbdefines.h"
#include "szarp_config.h"
#include "szbfile.h"
#include "szbdate.h"

#define IS_INITIALIZED if(this->initialized) throw std::wstring("Datablock not initialized properly");
#define NOT_INITIALIZED {this->initialized = false; return;}
#define DATABLOCK_CREATION_LOG_LEVEL 8
#define DATABLOCK_REFRESH_LOG_LEVEL 8
#define DATABLOCK_CACHE_ACTIONS_LOG_LEVEL 8
#define SEARCH_DATA_LOG_LEVEL 8
#define MAX_PROBES 4500

#endif

