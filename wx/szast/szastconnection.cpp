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

#include "szastconnection.h"
#include "pscgui.h"

SzastConnection::SzastConnection(wxEvtHandler *receiver) : 
	m_receiver(receiver), m_document(NULL), m_mutex(), m_semaphore(0, 1), m_finish_flag(false)
{}

void SzastConnection::Query(xmlDocPtr ptr) {
	wxMutexLocker lock(m_mutex);
	m_document = ptr;
	m_semaphore.Post();
}

void SzastConnection::Finish() {
	wxMutexLocker lock(m_mutex);
	m_finish_flag = true;
	m_semaphore.Post();
}

void* SzastConnection::Entry() {
	do {
		xmlDocPtr request = NULL;
		if (m_semaphore.Wait() == wxSEMA_NO_ERROR) {
			wxMutexLocker lock(m_mutex);
			if (m_finish_flag)
				return NULL;

			request = m_document;
		}

		if (request == NULL) 
			return NULL;

		NullDaemonStoppersFactory factory;
		xmlDocPtr response = PPSET::handle_command(request, factory);
		xmlFreeDoc(request);

		PSetDResponse event(true, response);
		wxPostEvent(m_receiver, event);

	} while (true);

	return NULL;
}
