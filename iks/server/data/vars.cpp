#include "vars.h"

#include <chrono>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "global_service.h"

#include "utils/exception.h"

namespace bf = boost::filesystem;

using boost::format;

Vars::Vars()
	: szb_wrapper(NULL) , initalized(false) 
	, timeout(GlobalService::get_service())
{
}

Vars::~Vars()
{
	if( szb_wrapper ) delete szb_wrapper;
}

void Vars::from_szarp( const std::string& szarp_base ) throw(file_not_found_error,xml_parse_error)
{
	if( initalized ) return;

	bf::path szdir = bf::path(SzbaseWrapper::get_dir()) / szarp_base;
	bf::path pszbc = szdir / "config" / "params.xml";

	if( !bf::exists(szdir) || !bf::is_directory(szdir) )
		throw file_not_found_error( str( format("%s is not valid base") % szarp_base ) );

	if( !bf::exists(pszbc) )
		throw file_not_found_error( str( format("%s does not exists") % pszbc.string() ) );

	params.from_params_file( pszbc.string() );
	sets  .from_params_file( pszbc.string() );

	szb_wrapper = new SzbaseWrapper( szarp_base );
}

void Vars::set_szarp_prober_server( const std::string& address , unsigned port )
{
	if( szb_wrapper ) {
		szb_wrapper->set_prober_address( address , port );

		/* FIXME: Check latest values even without probes server connection
		 * (22/05/2014 20:53, jkotur) */
		check_szarp_values();
	}
}

void Vars::command_request( const std::string& cmd , const std::string& data ) const
{
	emit_command_received( cmd , data );
}

void Vars::response_received( const std::string& cmd , const std::string& data )
{
	emit_command_response_received( cmd , data );
}

void Vars::check_szarp_values()
{
	using std::chrono::system_clock;
	using namespace boost::posix_time;

	time_t t = system_clock::to_time_t(system_clock::now());
	t = SzbaseWrapper::round( t , PT_SEC10 );

	try {
		for( auto itr=params.begin() ; itr!=params.end() ; ++itr )
			params.param_value_changed(
					*itr ,
					szb_wrapper->get_avg( *itr , t , PT_SEC10 ) );
	} catch( szbase_error& e ) {
		/* TODO: Better error handling (22/05/2014 20:54, jkotur) */
		std::cerr << "Szbase error: " << e.what() << std::endl;
	}

	t = SzbaseWrapper::next(t,PT_SEC10,1);

	timeout.expires_at( from_time_t(t) + seconds(1));
	timeout.async_wait( std::bind(&Vars::check_szarp_values,this) );
}

