#ifndef __REMOTES_CMD_CONNECT_REMOTE_H__
#define __REMOTES_CMD_CONNECT_REMOTE_H__

#include "locations/command.h"

class CmdConnectRemoteSnd : public Command {
public:
	CmdConnectRemoteSnd( const std::string& tag ) : tag(tag) {} 

	virtual ~CmdConnectRemoteSnd() {}

	virtual to_send send_str()
	{	return to_send( tag ); }

	virtual bool single_shot()
	{	return true; }

protected:
	std::string tag;
};


#endif /* end of include guard: __REMOTES_CMD_CONNECT_REMOTE_H__ */

