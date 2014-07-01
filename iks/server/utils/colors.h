#ifndef __UTILS_COLORS_H__
#define __UTILS_COLORS_H__

#include <string>
#include <vector>
#include <unordered_map>

class Colors {
public:
	/**
	 * Colors table used by draw3 in params with undefined color
	 */
	static const std::vector<std::string> draw3_defaults;

	/**
	 * Table translates colors from names to hex codes
	 */
	static const std::unordered_map<std::string,std::string> name_to_hex;
};

#endif /* __UTILS_COLORS_H__ */

