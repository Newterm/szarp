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
 * calcdmn
 * Pawe³ Kolega
 * demon nie odczytuj±cy danych z ¿adnych urz±dzeñ, a jedynie wyliczaj±cy warto¶æ chwilow± z warto¶ci sumarycznej np. Chwilowy przep³yw z przepp³ywu sumarycznego itd.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <assert.h>
#include <libxml/tree.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>


#define _IPCTOOLS_H_
#define _HELP_H_
#define _ICON_ICON_
#include "szarp.h"
#include "msgtypes.h"

#include "ipchandler.h"
#include "liblog.h"

#define DAEMON_ERROR 1

#define SND_NKE_LENGTH 5
#define REQ_UD2_LENGTH 5

#define GENERAL_ERROR -1
#define TIMEOUT_ERROR -2
#define PACKET_SIZE_ERROR -1

/* DEFINY do parametrow oblicznych */
#define C_DATA_TIMEOUT  600 /* sec */  

#define AVG_BUFFER 10

#define HOW_PARAMS 2
#define HOW_SENDS 2




//int AvgFlowBuffer[AVG_BUFFER];

class AvgBuffer {
	public:
	AvgBuffer(int size);
	~AvgBuffer();
	/**
	 * Buffer - point to avg buffer
	 * FillValue - initial value
	 * Size - size of buffer
	*/
	void InitAvgBuffer(int FillValue);
	/**
	 * Buffer - point to avg buffer
	 * Ptr - Pointer to buffer
	 * Size - size of buffer
	*/
	int CalcAvgBuffer(int Value); 

	private:
	int _size;
	int * _AvgBuffer;
	int _ptr;


};

	AvgBuffer::AvgBuffer(int size){
		_ptr = 0;
		_size = size;
		_AvgBuffer = (int *)malloc(_size * sizeof(int));
	}

	void AvgBuffer::InitAvgBuffer(int FillValue){
	int i;
	for (i=0;i<_size;i++)
		_AvgBuffer[i] = FillValue;
	}

int AvgBuffer::CalcAvgBuffer(int Value){
	int Div;
	int Sum;
	int i;
	Sum = 0;
	Div = 0;
	for (i=0;i<_size;i++){
		if (_AvgBuffer[i]!=SZARP_NO_DATA){ Div ++; Sum+=_AvgBuffer[i];}

	}
	_ptr = (_ptr % _size) + 1;
	_AvgBuffer[_ptr-1] = Value;
	if (Sum==0) return 0;
	if (Div==0) return SZARP_NO_DATA;
	return (Sum/Div);
}


AvgBuffer::~AvgBuffer(){
	free (_AvgBuffer);
}


class Calc : public AvgBuffer{
	public:
		int data_timeout ;
		Calc() : AvgBuffer (10) {data_timeout = 600;InitFlowBuffer=0;};
		Calc(int size) : AvgBuffer (size) {data_timeout = 600;InitFlowBuffer=0;};
		void SetTimeout(int timeout){data_timeout=timeout;};
		int Update(int Value);
		~Calc();
	private:
	int		CFlow ; /* Calculated Flow (from Sumaric flow) */
	int		NewFlow;
	int		OldFlow;
	time_t		NewFlowTime ;
	time_t		OldFlowTime ;
	char InitFlowBuffer ;
};



int Calc::Update(int Value){
		 NewFlow = Value;
		 NewFlowTime = time(NULL); 
		/* Inicjacja */
		if (OldFlow==0){
			OldFlow = NewFlow;
			CFlow = 0;
		}

		if (OldFlowTime==0){
			OldFlowTime = NewFlowTime;
		}

		if (OldFlowTime != NewFlowTime){
			if (NewFlowTime - OldFlowTime >=data_timeout){
				OldFlowTime = NewFlowTime;
				CFlow = 0 ;
			}
		}

		if (OldFlow!=NewFlow){
			if (NewFlowTime != OldFlowTime	)
				CFlow = ((NewFlow - OldFlow) * 3600)/(NewFlowTime - OldFlowTime) ;
				if (InitFlowBuffer==0){
					InitAvgBuffer(CFlow);
					InitFlowBuffer = 1;
				}else{
					CFlow = CalcAvgBuffer(CFlow);	
				}
				OldFlowTime = NewFlowTime;
				OldFlow = NewFlow;
				//InitFlowBuffer; //to bylo ale bylo bez sensu?
		}

		return CFlow;
}




/**
 * DaemonClass communication config.
 */
class           DaemonClass {
      public:
	/** info about single parameter */
		int m_params_count ;
		int m_sends_count ;
		int m_refresh;
		int m_timeout;
		int m_size;
	      
	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	DaemonClass(int params, int sends) {
		assert(params >= 0);
		assert(sends >= 0);

		m_params_count = params;
		m_sends_count = sends;
	}

		/**
	 * Search for 'modbus:mode' and 'modbus:id' attribute in 
	 * 'device' element, sets 'm_mode' and 'm_id' attributes;
	 * @param node XML 'device' element
	 * @return 0 on success, 1 on error
	 */
	int  parseDevice(xmlNodePtr node);

	~DaemonClass() {
	}



	/**
	 * Filling m_read structure SZARP_NO_DATA value
	 */
	void            SetNoData(IPCHandler * ipc);

	/**
	 * Function oppening serial port special for Infocal 5 Heat Meter
	 * @param Device Path to device np "/dev/ttyS0"
	 */

};


int DaemonClass::parseDevice(xmlNodePtr node)
{
	assert(node != NULL);
	char           *str;
	char           *tmp;
	
	str = (char *) xmlGetNsProp(node,
				    BAD_CAST("timeout"),
				    BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		    "attribute calcdmn:timeout not found in device element, line %ld",
		    xmlGetLineNo(node));
		free(str);
		return 1;
	}else{
		m_timeout = strtol(str, &tmp, 0);
		free (str);
	
	}

	str = (char *) xmlGetNsProp(node,
				    BAD_CAST("refresh"),
				    BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		    "attribute calcdmn:refresh not found in device element, line %ld",
		    xmlGetLineNo(node));
		free(str);
		return 1;
	}else{
		m_refresh = strtol(str, &tmp, 0);
		free (str);
	}
	str = (char *) xmlGetNsProp(node,
				    BAD_CAST("size"),
				    BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		    "attribute calcdmn:size not found in device element, line %ld",
		    xmlGetLineNo(node));
		free(str);
		return 1;
	}else{
		m_size = strtol(str, &tmp, 0);
		free (str);
	}



	return 0;
}




void DaemonClass::SetNoData(IPCHandler * ipc)
{
	int i;

	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;

}

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	DaemonClass     *calcinfo;
	IPCHandler     *ipc;
	Calc		*clc;
	int		MyData;	

	cfg = new DaemonConfig("calcdmn");

	if (cfg->Load(&argc, argv))
		return 1;
	calcinfo = new DaemonClass(cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount(),
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetSendParamsCount());


	if (calcinfo->m_params_count != HOW_PARAMS ){
		sz_log(0, "amount of params must be 2");
		delete calcinfo;
		return 1;

	}
	if (calcinfo->m_sends_count != HOW_SENDS ){
		sz_log(0, "amount of sends must be 2");
		delete calcinfo;
		return 1;
	}


	if (calcinfo->parseDevice(cfg->GetXMLDevice()))
	return 1;


	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), calcinfo->m_params_count);
	}


	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	clc = new Calc(calcinfo->m_size);
	clc->SetTimeout(calcinfo->m_timeout);

	if (cfg->GetSingle()) {
		printf("Average buffer size: %d [probes]\n",calcinfo->m_size);
		printf("Calculation timeout: (between probes): %d [s]\n",calcinfo->m_timeout);
		printf("Refresh cycle: %d [s]\n",calcinfo->m_refresh);
	}

	MyData = 0;
	calcinfo->SetNoData(ipc);
	sz_log(2, "starting main loop");
	while (true) {
		MyData = ipc->m_send[0] ; //LSB 
		MyData |= (ipc->m_send[1] << 16 ); //MSB 
		if (cfg->GetSingle()) {
			printf("ReadData: %d\n",MyData);
		}
		if(MyData != SZARP_NO_DATA)
			MyData = clc->Update(MyData);
		if (MyData ==SZARP_NO_DATA){
			ipc->m_read[0] = SZARP_NO_DATA;
			ipc->m_read[1] = SZARP_NO_DATA;
	
			if (cfg->GetSingle()) {
				printf("CalculatedData: SZARP_NO_DATA\n");
			}
		}else{
		if (cfg->GetSingle()) {
				printf("CalculatedData: %d\n",MyData);
			}

		ipc->m_read[0] = (MyData & 0x0000ffff) ; //LSB
		ipc->m_read[1] = (MyData & 0xffff0000) ; //MSB
		}
		
		ipc->GoParcook();
		ipc->GoSender();
		sleep(calcinfo->m_refresh);
	}
	delete calcinfo;
	return 0;
}

