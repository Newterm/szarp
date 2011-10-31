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
/*
 * draw3
 * SZARP
*/ 

#include "classes.h"
#include "delqitem.h"

std::deque<DelQueueItem*> DelQueueItem::m_del_queue;

DelQueueItem::~DelQueueItem() {}

void DelQueueItem::SubmitToDelQueue(DelQueueItem* item) {
	m_del_queue.push_back(item);
}

void DelQueueItem::CleanDelQueue() {
	while (m_del_queue.size() > 0) {
		delete m_del_queue.front();
		m_del_queue.pop_front();
	}
}
