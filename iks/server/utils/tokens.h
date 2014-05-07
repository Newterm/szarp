#ifndef __UTILS_TOKENS_H__
#define __UTILS_TOKENS_H__

#include <boost/range.hpp>
#include <boost/algorithm/string.hpp>

#include "utils/exception.h"

template<class RangeT>
boost::iterator_range< typename boost::range_iterator<const RangeT>::type >
	find_quotation( const RangeT& range )
{
	if( *boost::begin(range) != '\"' )
		return boost::algorithm::find_token(
				range ,
				!boost::algorithm::is_any_of(" \t") ,
				boost::algorithm::token_compress_on );

	auto i = std::next( boost::begin(range) );

	while( i != boost::end(range) && *i != '\"' )
		std::advance( i , 1 );

	if( i == boost::end(range) )
		throw parse_error("Cannot find terminating \"");

	return boost::make_iterator_range( std::next(boost::begin(range)) , i );
}

template<class RangeT>
boost::iterator_range< typename boost::range_iterator<const RangeT>::type >
	find_after_quotation( const RangeT& range )
{
	char delim = *boost::begin(range) != '\"' ? ' ' : '\"';

	auto i = boost::begin(range);

	if( i != boost::end(range) )
		std::advance( i , 1 );

	while( i != boost::end(range) && *i != delim )
		std::advance( i , 1 );

	/** skip leading white characters and " */
	while( i != boost::end(range) && (*i == delim || *i == ' ') )
		std::advance( i , 1 );

	return boost::make_iterator_range( i , boost::end(range) );
}

#endif /* end of include guard: __UTILS_TOKENS_H__ */

