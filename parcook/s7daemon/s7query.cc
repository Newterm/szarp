#include "s7query.h"

#include <sstream>
#include <iomanip>

#include "liblog.h"

unsigned int S7Query::typeSize()
{
	sz_log(10, "S7Query::typeSize");

	switch (_w_len) {
		case 0x01: return 1;
		case 0x02: return 1;
		case 0x04: return 2;
		/** We ask for real in two parts */
		case 0x08: return 2; 

		default: return 1;
	}
}

void S7Query::appendId( int id )
{
	sz_log(10, "S7Query::appendId");
	_ids.push_back(id);
}

void S7Query::merge( S7Query& query ) 
{
	sz_log(10, "S7Query::merge");
	auto qit = _ids.end();
	_ids.insert(qit, query._ids.begin(), query._ids.end());
	_amount += query._amount;
}
	
bool S7Query::isValid() 
{ 
	sz_log(10, "S7Query::isValid");
	return ((_area != -1) && (_start != -1 ) && (_amount != -1 ) && (_w_len != -1) &&
		(!((_area == 0x84) && (_db_num == -1))));
}	

bool S7Query::ask( S7Object& client ) 
{
	sz_log(10, "S7Query::ask");

	if(hasData()) _data.clear();
	_data.resize(_amount * typeSize());
				
	int ret = Cli_ReadArea(client,_area,_db_num,_start,_amount,_w_len,_data.data());
	if( ret != 0 ) {
		sz_log(1, "Query area:%d,db:%d,start:%d,amount:%d,w_len:%d FAILED",
				_area,_db_num,_start,_amount,_w_len);
		return false;
	}
	sz_log(3, "Query area:%d,db:%d,start:%d,amount:%d,w_len:%d SUCCESS",
				_area,_db_num,_start,_amount,_w_len);
	return true;
}
		
void S7Query::dump() 
{
	sz_log(10, "S7Query::dump");
	std::ostringstream os;

	os << "idx:( ";
	for (auto id = _ids.begin(); id != _ids.end(); id++) {
		os << *id << " ";
	}
	os << ") hex:[ ";
	if (!hasData()) { 
		os << "NO_DATA ]";
	} else {
		for (unsigned int i = 0; i < (_amount * typeSize()); i++) {
			os <<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')
				<< static_cast<unsigned>(_data[i]) << " ";
		}
		os << "]";
	}
	sz_log(1, "%s", os.str().c_str());
}

int S7Query::nextAddress()
{
	sz_log(10, "S7Query::nextAddress");
	return (_start + (_amount * typeSize())); 
}
