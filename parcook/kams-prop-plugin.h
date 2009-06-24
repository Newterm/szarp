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


#ifndef __KAMS_PROP_PLUGIN_H__
#define __KAMS_PROP_PLUGIN_H__

#define NUMBER_OF_VALS 16

/**
 * Reads data from Kamstrup 601 Heatmeter.
 * @param linedev name of serial device to read from
 * @param vals buffer of 16 values to fill with data
 * @param single set to non-zero for debug output
 */
extern "C" void KamstrupReadData(const char *linedev, short *vals, int single);

typedef void KamstrupReadData_t(const char *, short *, int);

#endif
