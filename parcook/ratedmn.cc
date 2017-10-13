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
  *
 * Logi l±duj± domy¶lnie w /opt/szarp/log/ratedmn.
 * W trybie Master zostala dodana konwersja FLOAT-> parametr MSB i FLOAT-> PARAMETR LSB
 */
/*
 @description_start
 @class 4
 @devices Daemon performs electric energy rate related calculations on existing SZARP parameters.
 @devices.pl Sterownik wykonuje obliczenia zwi±zane z taryfikowaniem energii elektrycznej na podstawie
 istniej±cych parametrów.
 @comment This daemon is obsolete, LUA parameters can be used to get the same effect.
 @comment.pl Ten demon jest obecnie niepotrzebny - ten sam efekt mo¿e byæ uzyskany za pomoc± parametrów w LUA.
 @description_end
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <termio.h>
#include <fcntl.h>

#include "conversion.h"

#include <libxml/tree.h>

#include "ipchandler.h"
#include "liblog.h"
#include "custom_assert.h"

#define N_OF_PARAMS 4
#define N_OF_SENDS 1

#define NO_VALUE -1
#define NO_ORDER -1
#define NO_PRICE -1
#define NO_CYCLES -1

#define IN_PERIOD 0x01
#define NOT_IN_PERIOD 0
#define NOT_CHECKED 0x02

//Tutaj sa bledy przy sprawdzaniu wejscia.

#define CHECK_OK 		 0
#define NO_FROM_HOUR 		-1
#define NO_TO_HOUR 		-2
#define NO_FROM_MIN 		-3
#define NO_TO_MIN 		-4
#define NO_FROM_DAY_OF_WEEK 	-5
#define NO_TO_DAY_OF_WEEK 	-6
#define NO_FROM_DAY_OF_MONTH 	-7
#define NO_TO_DAY_OF_MONTH 	-8
#define NO_FROM_MONTH 		-9
#define NO_TO_MONTH 		-10
#define NO_VAR_PRICE 		-11
#define NO_CONST_PRICE 		-12
#define NO_CYCLE 		-13

typedef struct list_of_periods{
	char FromHour;
	char ToHour;
	char FromMin;
	char ToMin;
	char FromDayOfWeek;
	char ToDayOfWeek;
	char FromDayOfMonth;
	char ToDayOfMonth;
	char FromMonth;
	char ToMonth;
	int VarPrice;
	int ConstPrice;
	char Cycle;
	//POLE DODATKOWE:
	int Order ; //Mówi w jakiej kolejno¶ci implementowaæ zmiany czasowe.
	struct list_of_periods* next;
} list_of_periods_t;

struct Prices{
	int VarPrice;
	int ConstPrice;
	char Cycle;
};


void CreateList(list_of_periods_t** result){
	*result = NULL;
}

void AddElement(
		char _FromHour,
		char _ToHour,
		char _FromMin,
		char _ToMin,
		char _FromDayOfWeek,
		char _ToDayOfWeek,
		char _FromDayOfMonth,
		char _ToDayOfMonth,
		char _FromMonth,
		char _ToMonth,
		int _VarPrice,
		int _ConstPrice,
		char _Cycle,
		list_of_periods_t** result){
		list_of_periods_t* periods;
		periods = (list_of_periods_t *) malloc (sizeof(list_of_periods_t));
		periods->FromHour = _FromHour;
		periods->ToHour = _ToHour;
		periods->FromMin = _FromMin;
		periods->ToMin = _ToMin ;
		periods->FromDayOfWeek = _FromDayOfWeek;
		periods->ToDayOfWeek = _ToDayOfWeek;
		periods->FromDayOfMonth = _FromDayOfMonth;
		periods->ToDayOfMonth = _ToDayOfMonth;
		periods->FromMonth = _FromMonth;
		periods->ToMonth = _ToMonth;
		periods->VarPrice = _VarPrice;
		periods->ConstPrice = _ConstPrice;
		periods->Order = NO_ORDER ;
		periods->Cycle = _Cycle ;
		//
		periods->next = *result;
		*result = periods;
}

int CheckData(
		char _FromHour,
		char _ToHour,
		char _FromMin,
		char _ToMin,
		char _FromDayOfWeek,
		char _ToDayOfWeek,
		char _FromDayOfMonth,
		char _ToDayOfMonth,
		char _FromMonth,
		char _ToMonth,
		int _VarPrice,
		int _ConstPrice,
		char _Cycle){

	if ((_FromHour==NO_VALUE) && (_ToHour!=NO_VALUE)){
		return (NO_FROM_HOUR);	
	}
	if ((_ToHour==NO_VALUE) && (_FromHour!=NO_VALUE)){
		return (NO_TO_HOUR);	
	}
	if ((_FromMin==NO_VALUE) && (_ToMin!=NO_VALUE)){
		return (NO_FROM_MIN);	
	}
	if ((_ToMin==NO_VALUE) && (_FromMin!=NO_VALUE)){
		return (NO_TO_MIN);	
	}
	if ((_FromDayOfWeek==NO_VALUE) && (_ToDayOfWeek!=NO_VALUE)){
		return (NO_FROM_DAY_OF_WEEK);	
	}
	if ((_ToDayOfWeek==NO_VALUE) && (_FromDayOfWeek!=NO_VALUE)){
		return (NO_TO_DAY_OF_WEEK);	
	}
	if ((_FromDayOfMonth==NO_VALUE) && (_ToDayOfMonth!=NO_VALUE)){
		return (NO_FROM_DAY_OF_MONTH);	
	}
	if ((_ToDayOfMonth==NO_VALUE) && (_FromDayOfMonth!=NO_VALUE)){
		return (NO_TO_DAY_OF_MONTH);	
	}
	if ((_FromMonth==NO_VALUE) && (_ToMonth!=NO_VALUE)){
		return (NO_FROM_MONTH);	
	}
	if ((_ToMonth==NO_VALUE) && (_FromMonth!=NO_VALUE)){
		return (NO_TO_MONTH);	
	}
	if ((_VarPrice==NO_VALUE)){
		return (NO_VAR_PRICE);	
	}
	if ((_ConstPrice==NO_VALUE)){
		return (NO_CONST_PRICE);	
	}
	if ((_ConstPrice==NO_CYCLE)){
		return (NO_CYCLE);	
	}
	return (CHECK_OK);	
}


int FindMaxOrder(list_of_periods_t* list){
	int MaxOrder=0;
	list_of_periods_t* periods;
	periods = list;
	/* Pimijamy ostatni element */
	if (periods){
		MaxOrder = periods->Order;
		periods = periods->next ;
	}
	while (periods){
		if (periods->Order > MaxOrder) MaxOrder = periods->Order;
		periods = periods->next ;
	}
	return (MaxOrder);
}

void PutOrder(list_of_periods_t* list){
	list_of_periods_t* periods;
	periods = list;
	int ActualOrder = NO_ORDER ;
	while ( periods ){
//			printf("periods->FromMonth %d periods->ToMonth %d periods->FromDayOfWeek %d periods->ToDayOfWeek %d\n", periods->FromMonth, periods->ToMonth, periods->FromDayOfWeek, periods->ToDayOfWeek );


		if ( (periods->FromMonth == NO_VALUE) && (periods->ToMonth == NO_VALUE) && (periods->FromDayOfMonth == NO_VALUE) && (periods->ToDayOfMonth == NO_VALUE)  && (periods->FromDayOfWeek == NO_VALUE) && (periods->ToDayOfWeek == NO_VALUE) ){
			ActualOrder++;
			//printf("jest etap 0\n");
			periods->Order = ActualOrder ;
		}else
		if ( (periods->FromMonth == NO_VALUE) && (periods->ToMonth == NO_VALUE) && (periods->FromDayOfMonth == NO_VALUE) && (periods->ToDayOfMonth == NO_VALUE) ){
			ActualOrder++;
			//printf("jest etap 1\n");
			periods->Order = ActualOrder ;
		}
		else

		if ( (periods->FromMonth == NO_VALUE) && (periods->ToMonth == NO_VALUE) ) {
			ActualOrder++;
			//printf("jest etap 2\n");
			periods->Order = ActualOrder ;
		}
		else
		if ( (periods->FromMonth != NO_VALUE) && (periods->ToMonth != NO_VALUE) ) {
			ActualOrder++;
			//printf("jest etap 3\n");
			periods->Order = ActualOrder ;
		}else
		if ( (periods->FromMonth == NO_VALUE) && (periods->ToMonth == NO_VALUE) && (periods->FromDayOfMonth == NO_VALUE) && (periods->ToDayOfMonth == NO_VALUE)  && (periods->FromDayOfWeek == NO_VALUE) && (periods->ToDayOfWeek == NO_VALUE) && (periods->FromHour == NO_VALUE) && (periods->ToHour == NO_VALUE) && (periods->FromMin == NO_VALUE) && (periods->ToMin == NO_VALUE)){
			ActualOrder++;
			//printf("jest etap 4\n");
			periods->Order = ActualOrder ;
		}
		periods = periods->next ;
	}
}

char IsInPeriod(list_of_periods *periods, tm *tm2)
{
	char tmp_result = NOT_IN_PERIOD ;
	char tmp_month = NOT_CHECKED ;
	char tmp_day_of_week = NOT_CHECKED ;
	char tmp_day_of_month = NOT_CHECKED ;
	char tmp_hour = NOT_CHECKED ;
	char tmp_min = NOT_CHECKED ;
	if ((periods->FromMonth != NO_VALUE ) && (periods->ToMonth != NO_VALUE )){
		if ( ((tm2->tm_mon + 1) >= periods->FromMonth)  &&  ((tm2->tm_mon + 1) <= periods->ToMonth ) ) {
			tmp_month = IN_PERIOD ;
		}else
			tmp_month = NOT_IN_PERIOD ;
			
	}
	if ((periods->FromDayOfMonth != NO_VALUE ) && (periods->ToDayOfMonth != NO_VALUE )){
		if ( ((tm2->tm_mday) >= periods->FromDayOfMonth)  &&  ((tm2->tm_mday) <= periods->ToDayOfMonth ) ) {
			tmp_day_of_month = IN_PERIOD ;
		}else
			tmp_day_of_month = NOT_IN_PERIOD ;
	}	

	if ((periods->FromDayOfWeek != NO_VALUE ) && (periods->ToDayOfWeek != NO_VALUE )){
		if ( ((tm2->tm_wday) >= periods->FromDayOfWeek)  &&  ((tm2->tm_wday) <= periods->ToDayOfWeek ) ) {
			tmp_day_of_week = IN_PERIOD ;
		}else
			tmp_day_of_week = NOT_IN_PERIOD ;
	}	

	if ((periods->FromHour != NO_VALUE ) && (periods->ToHour != NO_VALUE )){
		if ( ((tm2->tm_hour) >= periods->FromHour)  &&  ((tm2->tm_hour) <= periods->ToHour ) ) {
			tmp_hour = IN_PERIOD ;
		}else
			tmp_hour = NOT_IN_PERIOD ;
			
	}	

	if ((periods->FromMin != NO_VALUE ) && (periods->ToMin != NO_VALUE )){
		if ( ((tm2->tm_min) >= periods->FromMin)  &&  ((tm2->tm_min) <= periods->ToMin ) ) {
			tmp_min = IN_PERIOD ;
		}else
			tmp_min = NOT_IN_PERIOD ;

	}	

//	printf (" wynik: %d %d %d %d %d\n", tmp_month , tmp_day_of_month , tmp_day_of_week , tmp_hour , tmp_min  );
	
	if (((tmp_month | tmp_day_of_month | tmp_day_of_week | tmp_hour | tmp_min ) != NOT_IN_PERIOD) &&  ((tmp_month | tmp_day_of_month | tmp_day_of_week | tmp_hour | tmp_min ) != NOT_CHECKED))
		tmp_result = IN_PERIOD;
	/* Przypadek, gdy wszystko jest na NO_VALUE */
	if ( (periods->FromMonth == NO_VALUE) && (periods->ToMonth == NO_VALUE) && (periods->FromDayOfMonth == NO_VALUE) && (periods->ToDayOfMonth == NO_VALUE)  && (periods->FromDayOfWeek == NO_VALUE) && (periods->ToDayOfWeek == NO_VALUE) && (periods->FromHour == NO_VALUE) && (periods->ToHour == NO_VALUE) && (periods->FromMin == NO_VALUE) && (periods->ToMin == NO_VALUE)){
		tmp_result = IN_PERIOD;
	}
	return (tmp_result);
}

Prices *CalculatePrice (list_of_periods_t* list, tm *tm2)
{
	int MaxOrder = 0;
	list_of_periods_t* periods;
	int tmp_VarPrice = NO_PRICE ;
	int tmp_ConstPrice = NO_PRICE ;
	int tmp_Cycle = NO_CYCLES ;
	Prices *tmp_Prices ;	
	MaxOrder=FindMaxOrder(list);
	while (MaxOrder!=NO_ORDER){
		periods = list;

		while (periods){
			if (periods->Order == MaxOrder){
				if (IsInPeriod(periods, tm2) == IN_PERIOD){
			printf("Jestem tutaj 2\n" );
					tmp_VarPrice = periods->VarPrice; 
					tmp_ConstPrice = periods->ConstPrice;
					tmp_Cycle = periods->Cycle;
				}
			}
		periods = periods->next ;
		}
		MaxOrder--;
	}
	//printf("tmp_VarPrice %d\n",tmp_VarPrice);
	tmp_Prices = (Prices *)malloc(sizeof(Prices));
	tmp_Prices->VarPrice = tmp_VarPrice ;
	tmp_Prices->ConstPrice = tmp_ConstPrice ;
	tmp_Prices->Cycle = tmp_Cycle ;
	return (tmp_Prices);
}



/**
 * Rate communication config.
 */
class           RateInfo {
      public:
	/** info about single parameter */
	int             m_params_count;
	int             m_sends_count;
	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	RateInfo(int params, int sends) {
		ASSERT(params >= 0);
		ASSERT(sends >= 0);
		m_params_count = params;
		m_sends_count = sends;
	}

	~RateInfo() {
	}
	/**
	 * Filling m_read structure SZARP_NO_DATA value
	 */
	void            SetNoData(IPCHandler * ipc);
	
	/**
	 * Filling current element m_read SZARP_NO_DATA value
	 */
	void            SetNoData(IPCHandler * ipc, int param);

	/**
	 * Parses 'param' and 'send' children of 'unit' element,
	 * fills in params and sends arrays.
	 * @param unit XML 'unit' element
	 * @param ipk pointer to daemon config object
	 * @return 0 in success, 1 on error
	 */
	int             parseParams(xmlNodePtr unit, DaemonConfig * cfg, list_of_periods_t** list);

	      private:
		    /**<Error counter to avoid overflow or HDD space by generated logs ;) */
};

int RateInfo::parseParams(xmlNodePtr unit, DaemonConfig * cfg, list_of_periods_t** list)
{
	char FromHour = NO_VALUE;
	char ToHour = NO_VALUE;
	char FromMin = NO_VALUE;
	char ToMin = NO_VALUE;
	char FromDayOfWeek = NO_VALUE;
	char ToDayOfWeek = NO_VALUE;
	char FromDayOfMonth = NO_VALUE;
	char ToDayOfMonth = NO_VALUE;
	char FromMonth = NO_VALUE;
	char ToMonth = NO_VALUE;
	int VarPrice = NO_VALUE;
	int ConstPrice = NO_VALUE;
	char Cycle = NO_VALUE;
	int CheckDataStatus;
	
	char           *str;
	xmlNodePtr node = NULL;
	ASSERT(unit !=NULL);
	for (unit = unit->children; unit; unit = unit->next) {
		if ((unit->ns &&
		    !strcmp((char *) unit->ns->href,
			    (char *) SC::S2U(IPK_NAMESPACE_STRING).c_str())) ||
		    !strcmp((char *) unit->name, "unit"))
			break;
	}
		
	for (node = unit->children; node; node = node->next) {
		if (node->ns == NULL)
			continue;
		if (strcmp((char *) node->ns->href, IPKEXTRA_NAMESPACE_STRING) !=
		    0)
			continue;
		if (strcmp((char *) node->name, "period"))
			continue;

			if (!strcmp((char *) node->name, "period")){
				str=NULL;
				str = (char *) xmlGetNsProp(node, BAD_CAST("from_month"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					FromMonth = atoi(str) ;
					free (str);
				}

				str=NULL;
				str = (char *) xmlGetNsProp(node, BAD_CAST("to_month"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					ToMonth = atoi(str) ;
					free (str);
				}
				str=NULL;

				str = (char *) xmlGetNsProp(node, BAD_CAST("from_day_of_month"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					FromDayOfMonth = atoi(str) ;
					free (str);
				}

				str=NULL;

				str = (char *) xmlGetNsProp(node, BAD_CAST("to_day_of_month"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					ToDayOfMonth = atoi(str) ;
					free (str);
				}
				str=NULL;
	
				str = (char *) xmlGetNsProp(node, BAD_CAST("from_day_of_week"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					FromDayOfWeek = atoi(str) ;
					free (str);
				}

				str=NULL;

				str = (char *) xmlGetNsProp(node, BAD_CAST("to_day_of_week"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					ToDayOfWeek = atoi(str) ;
					free (str);
				}

				str=NULL;
				
				str = (char *) xmlGetNsProp(node, BAD_CAST("from_hour"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					FromHour = atoi(str) ;
					free (str);
				}

				str=NULL;

				str = (char *) xmlGetNsProp(node, BAD_CAST("to_hour"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					ToHour = atoi(str) ;
					free (str);
				}
	
				str=NULL;

				str = (char *) xmlGetNsProp(node, BAD_CAST("from_min"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					FromMin = atoi(str) ;
					free (str);
				}

				str=NULL;

				str = (char *) xmlGetNsProp(node, BAD_CAST("to_min"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					ToMin = atoi(str) ;
					free (str);
				}
				str=NULL;
				
				str = (char *) xmlGetNsProp(node, BAD_CAST("var_price"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					VarPrice = atoi(str) ;
					free (str);
				}

				str=NULL;
				str = (char *) xmlGetNsProp(node, BAD_CAST("const_price"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					ConstPrice = atoi(str) ;
					free (str);
				}

				str=NULL;
				str = (char *) xmlGetNsProp(node, BAD_CAST("cycle"),
					    BAD_CAST
				(IPKEXTRA_NAMESPACE_STRING));
		
				if (str!=NULL){
					Cycle = atoi(str) ;
					free (str);
				}
				
		}
			
		CheckDataStatus=CheckData(
			FromHour,
			ToHour,
			FromMin,
			ToMin,
			FromDayOfWeek,
			ToDayOfWeek,
			FromDayOfMonth,
			ToDayOfMonth,
			FromMonth,
			ToMonth,
			VarPrice,
			ConstPrice,
			Cycle			
				);
		if (CheckDataStatus!=CHECK_OK){
          		sz_log(1,
                    	"attribute ratedmn:error (%d) in configuration (period section is invalid. May be From..-To... pair is invalid or ...price, cycle are not found), line %ld",
                    	CheckDataStatus,	
			xmlGetLineNo(node));
			return (CheckDataStatus) ;	
		}
	
		AddElement(
			FromHour,
			ToHour,
			FromMin,
			ToMin,
			FromDayOfWeek,
			ToDayOfWeek,
			FromDayOfMonth,
			ToDayOfMonth,
			FromMonth,
			ToMonth,
			VarPrice,
			ConstPrice,
			Cycle,
			list
			);
	


		
	}

	return CHECK_OK;
}

void RateInfo::SetNoData(IPCHandler * ipc)
{
	int i;

	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;

}

void RateInfo::SetNoData(IPCHandler * ipc, int param)
{
	ipc->m_read[param] = SZARP_NO_DATA;
}

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	RateInfo     *rateinfo;
	IPCHandler     *ipc;
	list_of_periods *list;
	Prices *pp;
	time_t lt;
	struct tm *mytm;
	short  OldValue;
	short TotalPrice;
	CreateList(&list);
	
	cfg = new DaemonConfig("ratedmn");


	if (cfg->Load(&argc, argv))
		return 1;

	rateinfo = new RateInfo(cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount(),
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetSendParamsCount());


	if (rateinfo->parseParams(cfg->GetXMLDevice(), cfg, &list) != CHECK_OK){
		delete (rateinfo);
		return 1;
	}
	PutOrder(list); 	
	
	if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1))
		fprintf(stderr,"DEBUG[]: DAEMON STARTED\n");

	if (cfg->GetSingle()||cfg->GetDiagno()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n\
params out %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), rateinfo->m_params_count, rateinfo->m_sends_count);
	}	
	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	
	if (ipc->m_params_count!=N_OF_PARAMS){
	sz_log(1,
                    	"ratedmn:error in configuration (param section is invalid. number of parameters is invalid - shoud be %d is %d), ",
                    	N_OF_PARAMS,	
			ipc->m_params_count);
		return 1;
	}

	if (ipc->m_sends_count!=N_OF_SENDS){
	sz_log(1,
                    	"ratedmn:error in configuration (send section is invalid. number of sends is invalid - shoud be %d is %d), ",
                    	N_OF_SENDS,	
			ipc->m_sends_count);
		return 1;

		
	}	


	OldValue = SZARP_NO_DATA ;
	rateinfo->SetNoData(ipc);
	while (true) {
		ipc->GoParcook();

		ipc->GoSender();
				lt = time(NULL);
		     		mytm = localtime(&lt);
				pp=CalculatePrice(list, mytm);
				ipc->m_read[0]=(short)pp->Cycle;
				ipc->m_read[1]=(short)pp->ConstPrice;
				ipc->m_read[2]=(short)pp->VarPrice;
				TotalPrice=SZARP_NO_DATA;
				if (ipc->m_send[0] == SZARP_NO_DATA) ipc->m_read[3] = SZARP_NO_DATA ;
					
		if (ipc->m_send[0]!=OldValue){
				OldValue=ipc->m_send[0]	;

				if (ipc->m_send[0]==SZARP_NO_DATA) TotalPrice = SZARP_NO_DATA ;
				 else
				 TotalPrice = (short)pp->ConstPrice +  ipc->m_send[0]  * (short)pp->VarPrice;  
				 ipc->m_read[3]=(short)TotalPrice;
				 
				
		}
				 if (cfg->GetSingle()) {
				 	fprintf(stderr,"New Value: ");
				 	fprintf(stderr,"Cycle: %d ConstPrice: %d VarPrice: %d TotalPrice: %d\n ", (short)pp->Cycle, (short)pp->ConstPrice, (short)pp->VarPrice, TotalPrice);
				 }
				 free	 (pp);
				 usleep(1000*1000);
	}
}
