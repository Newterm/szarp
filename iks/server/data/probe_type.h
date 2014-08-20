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
		LIVE   = PT_SEC10  ,
		M10    = PT_MIN10  ,
		HOUR   = PT_HOUR   ,
		H8     = PT_HOUR8  ,
		DAY    = PT_DAY    ,
		WEEK   = PT_WEEK   ,
		MONTH  = PT_MONTH  ,
		YEAR   = PT_YEAR   ,
		CUSTOM = PT_CUSTOM ,
		MAX
	};

	ProbeType( Type pt = Type::LIVE , unsigned len = 0 );
	ProbeType( const std::string& id );

	Type get_type() const { return pt; }

	SZARP_PROBE_TYPE get_szarp_pt() const { return (SZARP_PROBE_TYPE)pt; }
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


