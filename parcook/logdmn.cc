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
 * @file logdmn.cc
 * @brief Daemon for remote logging users activity.
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-15
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <exception>

#include <liblog.h>
#include <conversion.h>
#include <szarp_config.h>

#include <map>

#include "base_daemon.h"
#include "log_prefix.h"

#define BUF_LEN 1024

/** 
 * @brief Parcook daemon to log activity.
 */
class LogDaemon : public BaseDaemon {
public:
	/** 
	 * @brief Constructs empty sharp daemon.
	 */
	LogDaemon() : BaseDaemon("logdmn") , sock(-1)
	{
	}

	/** 
	 * @brief Cleanup after logdmn.
	 */
	virtual ~LogDaemon()
	{
	}

	/** 
	 * @brief Reads incoming messages and writes them to memory.
	 *
	 * This method hangs and wait for incoming message. Based on hash received
	 * messages are searched in multimap where its index in parcook shared is
	 * written. If found parcook memory is incremented.
	 * 
	 * @return -1 in case of error 0 otherwise.
	 */
	virtual int Read()
	{
		char msg [BUF_LEN];
		ssize_t recsize;

		recsize = recvfrom(sock, (void *)msg, BUF_LEN, 0,
		                  (struct sockaddr *)&sa, &fromlen);
		if (recsize < 0) {
			if( errno != EINTR ) 
				sz_log( 0, "Reciving data failed: %s",strerror(errno));
			return -1;
		}

		std::wstring pname( recsize, 0 );
//                std::string smsg( msg , recsize );

		pname.insert( 0 , LOG_PREFIX L":" );
		std::copy( msg , msg+recsize ,
			 pname.begin() + LOG_PREFIX_LEN + 1 ) ;

		sz_log(10,"size: %d + 1 + %d", LOG_PREFIX_LEN , recsize );
		sz_log(10,"size: %d",pname.length());
		sz_log(10,"datagram: %.*s", (int)recsize, msg);
		sz_log(10,"param: %ls",pname.c_str());

		int k = -1;
		unsigned long h = hash_sdbm(pname);
		sz_log(10,"hash: %lu",h);
		for( params_map::iterator i = log_params.find(h) ;
		     i != log_params.end() && i->first == h ;
		     ++i )
		{
			sz_log(10,"%ls <=> %ls",pname.c_str(),i->second.first.c_str());
			if( pname == i->second.first ) {
				sz_log(10,"found matching parameter");
				k = i->second.second;
				break;
			}
		}

		sz_log(10,"Setting %d at %d",At(k)+1,k);
		if( k >= 0 ) Set( k , At( k ) + 1 );

		return 0;
	}

protected:

	virtual int ParseConfig(DaemonConfig * cfg)
	{
		return InitParams( cfg->GetDevice() )
		    || InitSocket( 7777u );
	}

	/** 
	 * @brief Constructs multimap with parameters in given device.
	 * 
	 * @param d device tag with parameters to log
	 * 
	 * @return 0 on success 2 otherwise
	 */
	virtual int InitParams( TDevice* d )
	{
		if( !d ) { sz_log(10,"No device found"); return 2; }
		TRadio* r = d->GetFirstRadio();
		if( !r ) { sz_log(10,"No radio found"); return 2; }
		TUnit * u = r->GetFirstUnit ();
		if( !u ) { sz_log(10,"No unit found"); return 2; }
		TParam* p = u->GetFirstParam();
		if( !p ) { sz_log(10,"No param found"); return 2; }

		for( int i=0 ; p ; p = p->GetNext() , ++i )
			log_params.insert(
				params_pair(
					hash_sdbm(p->GetName()),
					params_value( 
						p->GetName(),
						i ) ) );

		return 0;
	}

	/** 
	 * @brief Sets UDP socket on specified port
	 * 
	 * @param port port number
	 * 
	 * @return 0 on success 3 otherwise
	 */
	virtual int InitSocket( unsigned int port )
	{
		sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if( sock == -1 ) {
			sz_log(0,"Create socket failed: %s",strerror(errno));
			return 3;
		}

		memset(&sa, 0, sizeof sa);
		sa.sin_family = AF_INET;
		inet_pton( AF_INET, "0.0.0.0", &(sa.sin_addr.s_addr) );
		sa.sin_port = htons(port);
		fromlen = sizeof(sa);

		if( bind(sock,(struct sockaddr *)&sa, sizeof(sa))
		    == -1 ) {
			sz_log(0,"Bind failed: %s",strerror(errno));
			close(sock);
			return 3;
		} 

		return 0;
	}

	/** 
	 * @brief Simple and fast hashing function to fast look up trough multimap.
	 * 
	 * @param str wide string that should be hashed
	 * 
	 * @return hash of this string
	 */
	unsigned long hash_sdbm( const std::wstring& str )
	{
		unsigned long hash = 0;
		int c;

		for( unsigned int i=0 ; i<str.length() ; ++i )
		{
			c = str[i];
			hash = c + (hash << 6) + (hash << 16) - hash;
		}

		return hash;
	}

	/** @brief Parameter name and its position in parcook memory */
	typedef std::pair<const std::wstring&,int> params_value;
	/** @brief Multimap with hashed params strings as keys */
	typedef std::multimap<unsigned long,params_value> params_map;
	/** @brief Key-value pair to params_map */
	typedef std::pair<unsigned long,params_value> params_pair;

	int sock;
	struct sockaddr_in sa; 
	socklen_t fromlen;

	params_map log_params;
};

void on_alarm( int )
{
	sz_log(10,"Cought alarm");
}

int main(int argc, const char *argv[])
{
	try{ 
		LogDaemon dmn;

		TSzarpConfig sz_cfg;

		sz_cfg.CreateLogParams();

		sz_log(2, "Initing %s",dmn.Name());

		if( dmn.Init( argc , argv , &sz_cfg , 1 ) ) {
			sz_log(0,"Cannot start %s daemon",dmn.Name());
			return 1;
		}

		struct sigaction act;
		memset(&act,0,sizeof(act));
		act.sa_handler = on_alarm;
		sigaction(SIGALRM,&act,NULL);

		sz_log(2, "Starting %s",dmn.Name());

		dmn.Clear();
		alarm(600);

		for(;;)
		{
			sz_log(10,"Looping!");
			if( dmn.Read() == -1 && errno == EINTR ) {
				sz_log(10,"Transfering data");
				dmn.Transfer();
				dmn.Clear();
				alarm(600);
			}
		}

		sz_log(10,"Closing logdmn");
	} catch( std::exception& e ) {
		sz_log(0,"Dying after exception: %s",e.what());
	}

	return 0;
}

