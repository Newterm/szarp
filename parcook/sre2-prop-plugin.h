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

#ifndef __SRE2_PROP_PLUGIN_H__
#define __SRE2_PROP_PLUGIN_H__

#define NO_PARITY 0
#define EVEN 1
#define ODD 2
#define BCD_ERROR -1
#define MAX_CHANNELS 8
typedef struct {
	signed char t_hour;
	signed char t_min;
	signed char t_sec;
} ttime;

typedef struct {
	unsigned int t_en;
	unsigned long t_sec;
} tenergy;

class MyTime {
 public:
	MyTime() {
	};
	ttime CreateTime(signed char _t_hour, signed char _t_min,
			 signed char _t_sec);
	unsigned long Time2Sec(ttime time);
	unsigned long TimeDiffSec(ttime time_1, ttime time_2);
	unsigned long DiffSec(unsigned long time_1, unsigned long time_2);

	signed char CheckTimeError(ttime time);
};


class DataPower:private MyTime {
 public:
	DataPower(unsigned short _probes);
	void UpdateEnergy(unsigned int eng, ttime time);
	short ActualPower;
 private:
	unsigned short probes;
	tenergy *energy;
	unsigned short pointer;
 public:
	~DataPower() {
		if (energy != NULL)
			free(energy);
	}
};


/** Initialize DataPower objects array suitable for passing
 * to GetSRE2Data function. */
extern "C" DataPower **InitSRE2Data();

/**
 * Get data from SRE2 concentrator.
 * @param fd opened file descriptor of serial port
 * @param data buffer to fill with values on return
 * @param serial_no serial number of concentrator from configuraion
 * @param dps array of initialized DataPower objects to update
 * @param energy_factor energy factor parameter from configuraion
 * @param single 1 for verbose ouput, 0 otherwise
 */
extern "C" void GetSRE2Data(int fd, short* data, int serial_no, DataPower ** dps, int energy_factor, int single);

/** Convert serial number. */
extern "C" int GetSerialNum(long d);

typedef DataPower** InitSRE2Data_t();
typedef void GetSRE2Data_t(int, short*, int, DataPower**, int, int);
typedef int GetSerialNum_t(long);

#endif /* __SRE2_PROP_PLUGIN_H__ */
