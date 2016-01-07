#ifndef IKS_CMD_ID_H
#define IKS_CMD_ID_H

typedef int IksCmdId;

namespace iks_client_error {

enum error {
	not_connected_to_peer = 1,
	invalid_server_response
};

}

namespace detail {

class iks_client_category : public boost::system::error_category
{
public:
	const char* name() const BOOST_SYSTEM_NOEXCEPT;

	std::string message(int value) const;
};

}

const boost::system::error_category& get_iks_client_category();

inline boost::system::error_code make_error_code(iks_client_error::error e)
{
	return boost::system::error_code( static_cast<int>(e) , get_iks_client_category() );
}


enum IksCmdStatus {
	cmd_done,
	cmd_cont
};

typedef std::function<IksCmdStatus( const boost::system::error_code& , const std::string& , std::string & )> IksCmdCallback;

#endif
/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
