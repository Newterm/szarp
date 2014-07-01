#ifndef __WELCOME_CMD_UPDATE_REMOTES_H__
#define __WELCOME_CMD_UPDATE_REMOTES_H__

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "global_service.h"
#include "locations/command.h"

#include "locations/locations_list.h"

class CmdAddRemoteSnd : public Command {
public:
	CmdAddRemoteSnd( const std::string& tag ) : tag(tag) {} 

	virtual ~CmdAddRemoteSnd() {}

	virtual to_send send_str()
	{	return to_send( tag ); }

	virtual bool single_shot()
	{	return true; }

protected:
	std::string tag;
};

class CmdRemoveRemoteSnd : public Command {
public:
	CmdRemoveRemoteSnd( const std::string& tag ) : tag(tag) {} 

	virtual ~CmdRemoveRemoteSnd() {}

	virtual to_send send_str()
	{	return to_send( tag ); }

	virtual bool single_shot()
	{	return true; }

protected:
	std::string tag;
};

#endif /* end of include guard: __WELCOME_CMD_UPDATE_REMOTES_H__ */

