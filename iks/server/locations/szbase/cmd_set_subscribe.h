#ifndef __SERVER_CMD_SET_SUBSCRIBE_H__
#define __SERVER_CMD_SET_SUBSCRIBE_H__

#include <boost/algorithm/string.hpp>

#include "locations/command.h"

class SetSubscribeRcv : public Command {
public:
	SetSubscribeRcv( Vars& vars , SzbaseProt& prot )
		: vars(vars) , prot(prot)
	{
		set_next( std::bind(&SetSubscribeRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~SetSubscribeRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		namespace ba = boost::algorithm;

		std::string set_name;
		std::string set_hash;

		try {
			auto r = find_quotation( line );
			set_name.assign(r.begin(),r.end());

			auto s = *r.end() == '\"' ? std::next(r.end()) : r.end();
			auto i = boost::make_iterator_range( s , line.end() );
			auto t = ba::find_token( i ,
					!ba::is_any_of(" \t") ,
					ba::token_compress_on );
			set_hash.assign( t.begin() , t.end() );

		} catch( parse_error& e ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		if( set_name.empty() ) {
			/** set current set to none */
			prot.set_current_set();
			apply();
			return;
		}

		auto s = vars.get_sets().get_set( set_name );

		if( !s ) {
			fail( ErrorCodes::unknown_set );
			return;
		}

		size_t hash;
		try {
			hash = boost::lexical_cast<size_t>( set_hash );
		} catch( const boost::bad_lexical_cast& ) {
			fail( ErrorCodes::invalid_set_hash );
			return;
		}

		if( hash != s->get_hash() ) {
			fail( ErrorCodes::invalid_set_hash );
			return;
		}

		/** subscribe to set */
		prot.set_current_set( s );
		apply();
	}

protected:
	Vars& vars;
	SzbaseProt& prot;

};

#endif /* end of include guard: __SERVER_CMD_SET_SUBSCRIBE_H__ */

