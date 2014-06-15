#include "probe_type.h"

#include "utils/exception.h"

ProbeType::ProbeType( ProbeType::Type pt , unsigned len )
	: pt(pt) , len(len)
{
}

ProbeType::ProbeType( const std::string& probe_string )
	: len(0)
{
		 if( probe_string == "10s" )
		pt = Type::LIVE;
	else if( probe_string == "10m" )
		pt = Type::M10;
	else if( probe_string == "1h" )
		pt = Type::HOUR;
	else if( probe_string == "8h" )
		pt = Type::H8;
	else if( probe_string == "1d" )
		pt = Type::DAY;
	else if( probe_string == "1w" )
		pt = Type::WEEK;
	else if( probe_string == "1M" )
		pt = Type::MONTH;
	else if( probe_string == "1Y" )
		pt = Type::YEAR;
	else
		/* TODO: Parse custom strings. (30/05/2014 08:26, jkotur) */
		throw parse_error("Cannot parse string to ProbeType: " + probe_string );
}

std::string ProbeType::to_string() const
{
	if( pt == Type::LIVE )
		return "10s";
	if( pt == Type::M10 )
		return "10m";
	if( pt == Type::HOUR )
		return "1h";
	if( pt == Type::H8 )
		return "8h";
	if( pt == Type::DAY )
		return "1d";
	if( pt == Type::WEEK )
		return "1w";
	if( pt == Type::MONTH )
		return "1M";
	if( pt == Type::YEAR )
		return "1Y";
	return "";
}

bool operator==( const ProbeType& a , const ProbeType& b )
{
	return a.pt == b.pt
	    && ( a.pt != ProbeType::Type::CUSTOM || a.len == b.len );
}

bool operator<( const ProbeType& a , const ProbeType& b )
{
	if( a.pt != ProbeType::Type::CUSTOM )
		return unsigned(a.pt) < unsigned(b.pt);

	/* TODO: Compare PT_CUSTOM probes (30/05/2014 09:36, jkotur) */
	return false;
}

