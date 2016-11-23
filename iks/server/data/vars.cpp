#include "vars.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "utils/exception.h"

namespace bf = boost::filesystem;

using boost::format;

Vars::Vars()
	: params_updater(params) , szb_wrapper(NULL) , initialized(false) 
{
}

Vars::~Vars()
{
	if( szb_wrapper ) delete szb_wrapper;
}

void Vars::from_szarp( const std::string& szarp_base )
{
	if( initialized ) return;

	bf::path szdir = bf::path(SzbaseWrapper::get_dir()) / szarp_base;
	bf::path pszbc = szdir / "config" / "params.xml";

	if( !bf::exists(szdir) || !bf::is_directory(szdir) )
		throw file_not_found_error( str( format("%s is not valid base") % szarp_base ) );

	if( !bf::exists(pszbc) )
		throw file_not_found_error( str( format("%s does not exists") % pszbc.string() ) );

	params.from_params_file( pszbc.string() );
	sets  .from_params_file( pszbc.string() );

	szb_wrapper = new SzbaseWrapper( szarp_base );
	params_updater.set_data_feeder( szb_wrapper );
}

void Vars::set_szarp_prober_server( const std::string& address , unsigned port )
{
	if( szb_wrapper )
		szb_wrapper->set_prober_address( address , port );
}

void Vars::command_request( const std::string& cmd , const std::string& data ) const
{
	emit_command_received( cmd , data );
}

void Vars::response_received( const std::string& cmd , const std::string& data )
{
	emit_command_response_received( cmd , data );
}

