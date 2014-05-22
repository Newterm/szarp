#ifndef __UTILS_CONFIG_H__
#define __UTILS_CONFIG_H__

#include <map>
#include <string>

typedef std::map<std::string,std::string> CfgPairs;

class CfgSections : public std::map<std::string,CfgPairs> {
public:
	CfgSections() {}
	virtual ~CfgSections() {}

	void from_flat( const CfgPairs& flat );
};

#endif /* __UTILS_CONFIG_H__ */


