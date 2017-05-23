/* 
 * SZARP: SCADA software 
 *
 * Copyright (C) 
 * 2011 - Jakub Kotur <qba@newterm.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
 /** 
 * @file sampledmn.cc
 * @brief Daemon for remote logging users activity.
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-15
 */
/*
 @description_start
 @class 4

 @devices None -- always outputs 1
 @devices.pl ¿adne -- jedynie wypluwa 1
 @config None
 @config.pl ¿adna

 @description_end
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <liblog.h>

#include "base_daemon.h"

class SampleDaemon {
public:
	using interface = interface_wrapper<IPCProvider, EventProvider, SignalProvider>;

	SampleDaemon(interface* if_provider): m_if(if_provider) {
		m_if->call(&SignalProvider::InitSignals);

		std::function<void()> poll_cycle =
		[this]() {
			Read();
			m_if->call(&IPCProvider::Publish);
		};

		//m_if->call(&EventProvider::RegisterTimerCallback, poll_cycle, -1, -1); // Default arguments dont work :(((
		(*m_if)[&EventProvider::RegisterTimerCallback](poll_cycle, 4, 500); // To use defaults we need to specify another function within the interface

		m_if->call(&EventProvider::PollForever);
	}

	void Read()
	{
		//for( size_t i=0 ; i < m_if->get<IPCProvider>()->GetParamsCount() ; ++i )
		(*m_if)(&IPCProvider::SetVal<int16_t>, size_t(0), int16_t(1)); // this one needs casted arguments

		// Make the interface a reference to remove the * or make the interface_wrapper a base class
		(*m_if)[&IPCProvider::SetVal<int16_t>](1, 2);

		// Or even
		auto SetVal =(*m_if)[&IPCProvider::SetVal<int16_t>];
		SetVal(2, 3);
		SetVal(3, sz4::getTimeNow<sz4::second_time_t>());
	}

private:
	interface *m_if;

};

int main(int argc, const char *argv[])
{
	DaemonConfig* cfg = new DaemonConfig("sampledmn");
	if(cfg->Load(&argc, const_cast<char**>(argv))) throw std::runtime_error("Could not initialize configuration");
	
	SampleDaemon dmn(SampleDaemon::interface::concrete_creator<IPCHandlerWrapper, LibeventWrapper, SignalProvider>::create(cfg));
}
