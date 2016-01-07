#ifndef __DATA_PROBE_TYPE_H__
#define __DATA_PROBE_TYPE_H__

#include <functional>
#include <ctime>

#include <szbase/szbbase.h>


class ProbeType {
	friend bool operator<( const ProbeType& , const ProbeType& );
	friend bool operator==( const ProbeType& a , const ProbeType& b );

public:
	enum class Type : unsigned {
		LIVE   = 0 ,
		MS100  = PT_MSEC10  + 1 ,
		MS500  = PT_HALFSEC + 1 ,
		S      = PT_SEC     + 1 ,
		S10    = PT_SEC10   + 1 ,
		M10    = PT_MIN10   + 1 ,
		HOUR   = PT_HOUR    + 1 ,
		H8     = PT_HOUR8   + 1 ,
		DAY    = PT_DAY     + 1 ,
		WEEK   = PT_WEEK    + 1 ,
		MONTH  = PT_MONTH   + 1 ,
		YEAR   = PT_YEAR    + 1 ,
		CUSTOM = PT_CUSTOM  + 1 ,
		MAX
	};

	ProbeType( Type pt = Type::LIVE , unsigned len = 0 );
	ProbeType( SZARP_PROBE_TYPE pt  , unsigned len = 0 );
	ProbeType( const std::string& id );

	Type get_type() const { return pt; }

	SZARP_PROBE_TYPE get_szarp_pt() const
	{
		return pt == Type::LIVE ? PT_SEC10 : (SZARP_PROBE_TYPE)((unsigned)pt - 1);
	}

	unsigned get_len() const { return len; }

	std::string to_string() const;

	std::time_t get_time() const;

protected:
	Type pt;
	unsigned len;
};

bool operator==( const ProbeType& a , const ProbeType& b );
bool operator<( const ProbeType& a , const ProbeType& b );

#endif /* end of include guard: __DATA_PROBE_TYPE_H__ */


