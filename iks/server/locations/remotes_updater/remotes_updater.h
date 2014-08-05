#ifndef __REMOTES_UPDATER_H__
#define __REMOTES_UPDATER_H__

#include <memory>

#include "locations/protocol.h"

#include "net/tcp_client.h"
#include "locations/locations_list.h"
#include "locations/protocol_location.h"

class RemotesUpdater : public Protocol , public std::enable_shared_from_this<RemotesUpdater> {
public:
	typedef std::shared_ptr<RemotesUpdater> ptr;

	RemotesUpdater(
			const std::string& name , const std::string& draw_name ,
			const std::string& address , unsigned port ,
			LocationsList& locs );
	virtual ~RemotesUpdater();

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

	void connect();

protected:
	void get_remotes();
	void rm_remotes();

	void reconnect( const boost::system::error_code& e );

private:
	std::shared_ptr<ProtocolLocation> protocol_mgr;
	std::shared_ptr<TcpClient> remote;

	LocationsList& locs;

	std::string name;
	std::string draw_name;

	std::string address;
	unsigned port;

	boost::asio::deadline_timer timeout;
};

#endif /* __REMOTES_UPDATER_H__ */

