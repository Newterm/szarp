#include "vars.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>


namespace bf = boost::filesystem;

using boost::format;

Vars::Vars()
	: szb_wrapper(NULL) , initalized(false) 
{
}

Vars::~Vars()
{
	if( szb_wrapper ) delete szb_wrapper;
}

void Vars::from_szarp( const std::string& szarp_dir ) throw(file_not_found_error,xml_parse_error)
{
	if( initalized ) return;

	bf::path szdir(szarp_dir);
	bf::path pszbc = szdir / "config" / "params.xml";

	if( !bf::exists(szdir) || !bf::is_directory(szdir) )
		throw file_not_found_error( str( format("%s is not valid direcotry") % szarp_dir ) );

	if( !bf::exists(pszbc) )
		throw file_not_found_error( str( format("%s does not exists") % pszbc.string() ) );

	params.from_params_file( pszbc.string() );
	sets  .from_params_file( pszbc.string() );

	szb_wrapper = new SzbaseWrapper( szdir.filename()
#if BOOST_FILESYSTEM_VERSION == 3
	                                                 .string()
#endif
	                                                           );
}

void Vars::command_request( const std::string& cmd , const std::string& data ) const
{
	emit_command_received( cmd , data );
}

void Vars::response_received( const std::string& cmd , const std::string& data )
{
	emit_command_response_received( cmd , data );
}

