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
 * hwkey - Hardware Key Generator
 * SZARP

 * Michal Blajerski <nameless@praterm.com.pl>
 *
 * $Id$
 */

#include "hwkey.h"
#include "string.h"
#include "stdio.h"

#ifdef __WXMSW__

#include "windows.h"
#include "iphlpapi.h"
#include "winsock2.h"

#else
#include "assert.h"
#include "sys/ioctl.h"
#include "sys/socket.h"
#include "sys/types.h"
#include "net/if.h"
#include "arpa/inet.h"

#endif



HardwareKey::HardwareKey() {
#ifndef __WXMSW__
	
	int sock;
	bool found = false;
	unsigned char *byte;
	struct ifreq ifr;
	struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;


	assert(0 < (sock = socket(AF_INET, SOCK_STREAM, 0)));
	
	sin->sin_family = AF_INET;
	for(int i = 0; i<16; i++) {
		char interface[5];
		sprintf(interface,"%s%d","eth",i);
		strcpy(ifr.ifr_name,interface);
		if (0 <= ioctl(sock, SIOCGIFHWADDR, &ifr)) {
			byte = (unsigned char *) &ifr.ifr_addr.sa_data;
			if((byte[0] + byte[1] + byte[2] + byte[3] + byte[4] + byte[5])>0) {
				sprintf(key,"%2.2x%2.2x%2.2x%2.2x%2.2x", (byte[1])^XOR, (byte[2])^XOR, (byte[3])^XOR, (byte[4])^XOR, (byte[5])^XOR);
				found = true;
				break;
			}	
		}
	}
	
	if(!found) 
		strcpy(key,SPECIAL_LINUX_KEY);
	


#else
/* Niech to bedzie pierwszy adapter z listy, bo ten z ktorym mozna sie polaczyc moze byc rozny przy starcie systemu */
	BOOL found = false;
//	DWORD c;
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	

/*	if(GetBestInterface(inet_addr("62.233.142.82"),&c) != NO_ERROR) {
		strcpy(key,SPECIAL_WINDOWS_KEY);
		return;
	}
*/		
		
	pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		  free(pAdapterInfo);
		    pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen); 
	}

	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
/*		pAdapter = pAdapterInfo;
		
		while (pAdapter) {
			if(c == pAdapter->Index && pAdapter->Type == MIB_IF_TYPE_ETHERNET) {
		
			       	ULONG   ulLen = 6;
				
			 	size_t i, j;
				PBYTE pbHexMac = (PBYTE) pAdapter->Address;
			
			        for (i = 1, j = 0; i < ulLen - 1; ++i) {
				        j += sprintf (key + j, "%02X", (pbHexMac[i])^XOR);
				}
				sprintf (key + j, "%02X", (pbHexMac[i])^XOR);
				found = true;	
			}	    

		    
			pAdapter=pAdapter->Next;
		}
		if(!found) {
*/		   pAdapter = pAdapterInfo;
		   while (pAdapter) {
			if(pAdapter->Type == MIB_IF_TYPE_ETHERNET) {
			       	ULONG   ulLen = 6;
				
			 	size_t i, j;
				PBYTE pbHexMac = (PBYTE) pAdapter->Address;

			        for (i = 1, j = 0; i < ulLen - 1; ++i) {
				        j += sprintf (key + j, "%02X", (pbHexMac[i])^XOR);
				}
				sprintf (key + j, "%02X", (pbHexMac[i])^XOR);
				found = true;	
				free(pAdapterInfo);
				break;
			}	    

		    
			pAdapter=pAdapter->Next;
		  }
	     //}
	}			    

	if(!found)
		strcpy(key,SPECIAL_WINDOWS_KEY);

	
	
#endif	
	
};


char * HardwareKey::GetKey() {
	return key;
};
