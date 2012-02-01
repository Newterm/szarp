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
 * $Id$
 * (C) 2009 Pawel Palucha
 */

#ifndef __PROP_PLUGINS_H__
#define __PROP_PLUGINS_H__

/** Class for communicating with Kamstrup 601 Heatmeter. */
class KMPSendInterface {
	public:
		KMPSendInterface() { };
		virtual ~KMPSendInterface() { };
		virtual void CreateQuery (unsigned char CID, unsigned char REMOTE_ADDR,
		    unsigned short REGISTER) = 0;
  		virtual void SendQuery (int fd) = 0;
};
/** Class for communicating with Kamstrup 601 Heatmeter. */
class KMPReceiveInterface {
public:
    virtual char GetREAD_OK() = 0;
    virtual char GetRESPONSE_OK() = 0;
    virtual char GetCONVERT_OK() = 0;
    KMPReceiveInterface() { } ;
    virtual ~KMPReceiveInterface() { } ;
    virtual void KMPPutResponse (unsigned char *_ReceiveBuffer, unsigned short _RSize) = 0;
    virtual unsigned short ReceiveSize () = 0;
    virtual unsigned short UnStuffedReceiveSize () = 0;
    virtual char ReceiveResponse (int fd, unsigned long dbc, unsigned char timeout) = 0;
    virtual void UnStuffResponse () = 0;
    virtual char CheckResponse () = 0;
    virtual double ReadFloatRegister (char *e_c) = 0;
};

/** Typedefs for create/destroy functions imported using dlsym. 
 * Based on ideas from http://www.faqs.org/docs/Linux-mini/C++-dlopen.html */
typedef KMPSendInterface* KMPSendCreate_t();
typedef void KMPSendDestroy_t(KMPSendInterface*);
typedef KMPReceiveInterface* KMPReceiveCreate_t();
typedef void KMPReceiveDestroy_t(KMPReceiveInterface*);

#endif /* __PROP_PLUGINS_H__ */
