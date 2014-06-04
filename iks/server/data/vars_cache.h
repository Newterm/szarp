#ifndef __DATA_VARS_CACHE_H__
#define __DATA_VARS_CACHE_H__

#include <memory>
#include <string>
#include <unordered_map>

#include "vars.h"

class VarsCache {
public:
	Vars& get_szarp( const std::string& base );

protected:

	std::unordered_map<std::string,std::unique_ptr<Vars>> szarp_vars;
};

#endif /** __DATA_VARS_CACHE_H__ */

