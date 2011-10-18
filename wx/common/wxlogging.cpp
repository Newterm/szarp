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
 */
/** 
 * @file wxlogging.cpp
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.3
 * @date 2011-10-11
 */

#include "wxlogging.h"

#include <exception>
#include <typeinfo>
#include <iostream>

#include <boost/asio.hpp>

#include <wx/log.h>

using boost::asio::ip::udp;

void strset( std::string& out , const char* in )
{
	size_t l = strlen(in);
	out.resize(l);
	out.replace(0,l,in);
}

std::string UDPLogger::appname("szarp");
std::string UDPLogger::address("localhost");
std::string UDPLogger::port   ("7777");

void UDPLogger::HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event)
{
	if( event.m_callbackUserData
	 && typeid(*event.m_callbackUserData) == typeid(UDPLogger::Data) )
		LogEvent(reinterpret_cast<UDPLogger::Data*>(event.m_callbackUserData)->get());
}

void UDPLogger::LogEvent( const char * msg )
{
	boost::asio::io_service  io_service;
	udp::socket   s( io_service, udp::endpoint(udp::v4(), 0) );
	udp::resolver resolver( io_service );
	udp::resolver::query query( udp::v4() , UDPLogger::address , UDPLogger::port );

	char buf[4096];
	int len = snprintf(buf,4096,"%s:%s",appname.c_str(),msg);

	try {
		udp::resolver::iterator iterator = resolver.resolve(query);
		s.send_to(boost::asio::buffer(buf, len), *iterator);
	} catch( std::exception& e ) {
		wxLogWarning(_T("UDPLogger::send_param exception: %s"),wxString(e.what(),wxConvUTF8).wc_str());
	}
}

