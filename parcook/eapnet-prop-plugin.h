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

#ifndef __EAPNET_PROP_PLUGIN_H__
#define __EAPNET_PROP_PLUGIN_H__

#define MAX_DATA 64 /* maximum number of devices on RS-485 line */

typedef struct {
       	char EAPCode[14]; /* EAP meter id code - also RS485 identifier */
       	int Ratio; /* energy transmission ratio */
} tEAPParams;


class EAPNetDataInterface {
 public:
	int m_params_count;	/**< size of params array */

	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	 EAPNetDataInterface(int params, int _Single) {}
	 virtual ~EAPNetDataInterface() {}

	/**
	 * Function oppening serial port special for Pollustat E Heat Meter
	 * @param line Path to device np "/dev/ttyS0"
	 * @param return file descriptor
	 */
	virtual int OpenLine(const char *line, int BaudRate) = 0;

	/**
	 * Send query and receive response, copy it to data.
	 */
	virtual void Query(int fd, short *data) = 0;
	virtual int ParseCommandLine(int argc, char *argv[]) = 0;
};

/** Typedefs for create/destroy functions imported using dlsym. 
 * Based on ideas from http://www.faqs.org/docs/Linux-mini/C++-dlopen.html */
typedef EAPNetDataInterface *EAPNetDataCreate_t(int params, int single);
typedef void EAPNetDataDestroy_t(EAPNetDataInterface *);

#endif	/* __EPANET_PROP_PLUGINS_H__ */
