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

#include "config.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <zmq.hpp>

#include "szarp_config.h"

#include "sz4/time.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"
#include "sz4/live_cache.h"
#include "sz4/base.h"
#include "sz4/base_templ.h"
#include "sz4/defs.h"

namespace sz4 {

template class base_templ<base_types>;

}
