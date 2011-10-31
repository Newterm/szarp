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

enum UDPLogger::states UDPLogger::state = UNINIT;

std::string UDPLogger::appname("szarp");
std::string UDPLogger::address("localhost");
std::string UDPLogger::port   ("7777");

boost::asio::io_service* UDPLogger::io_service = NULL;
udp::socket*             UDPLogger::s          = NULL;
udp::resolver*           UDPLogger::resolver   = NULL;
udp::resolver::query*    UDPLogger::query      = NULL;

udp::resolver::iterator  UDPLogger::resolver_results;

void UDPLogger::HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event)
{
	if( event.m_callbackUserData
	 && typeid(*event.m_callbackUserData) == typeid(UDPLogger::Data) )
		LogEvent(reinterpret_cast<UDPLogger::Data*>(event.m_callbackUserData)->get());
}

void UDPLogger::LogEvent( const char * msg )
{
	if( state == BROKEN || (state == UNINIT && ResolveAdress()) ) return;

	char buf[4096];
	int len = snprintf(buf,4096,"%s:%s",appname.c_str(),msg);

	try {
		s->send_to(boost::asio::buffer(buf, len), *resolver_results);
	} catch( std::exception& e ) {
		wxLogWarning(_T("UDPLogger::LogEvent exception: %s"),wxString(e.what(),wxConvUTF8).wc_str());
	}
}

int UDPLogger::ResolveAdress()
{
	if( io_service ) delete io_service;
	if( s ) delete s;
	if( resolver ) delete resolver;
	if( query ) delete query;
	
	io_service = new boost::asio::io_service();
	s = new udp::socket( *io_service, udp::endpoint(udp::v4(), 0) );
	resolver = new udp::resolver( *io_service );
	query = new udp::resolver::query( udp::v4() , UDPLogger::address , UDPLogger::port );

	try {
		resolver_results = resolver->resolve(*query);
	} catch( std::exception& e ) {
		wxLogWarning(_T("UDPLogger::ResolveAdress exception: %s"),wxString(e.what(),wxConvUTF8).wc_str());
		state = BROKEN;
		return -1;
	}
	state = WORKING;
	return 0;
}

