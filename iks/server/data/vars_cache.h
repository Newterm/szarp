#ifndef __DATA_VARS_CACHE_H__
#define __DATA_VARS_CACHE_H__

#include <string>
#include <unordered_map>

#include "vars.h"

class VarsCache {
public:
	VarsCache();
	~VarsCache();

	Vars& get_szarp( const std::string& base );
protected:

	std::unordered_map<std::string,Vars*> szarp_vars;
};

#endif /** __DATA_VARS_CACHE_H__ */

