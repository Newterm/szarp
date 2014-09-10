#ifndef __CONFIG_CONTAINER_H__
#define __CONFIG_CONTAINER_H__

#include "data/config.h"

/** Class for storing config, added to decompose config storage
 * from Protocol, as WelcomeProtocol needs to host Config */
class ConfigContainer {
public:
	ConfigContainer( Config& config )
		: config(config)
	{}

	virtual ~ConfigContainer()
	{}

	Config& get_config()
	{
		return config;
	}

protected:
	Config & config;

};

#endif /* end of include guard: __CONFIG_CONTAINER_H__ */

