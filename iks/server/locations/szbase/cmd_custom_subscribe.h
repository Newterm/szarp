#ifndef __SERVER_CMD_CUSTOM_SUBSCRIBE_H__
#define __SERVER_CMD_CUSTOM_SUBSCRIBE_H__

#include "locations/command.h"

class CustomSubscribeRcv : public Command {
public:
	CustomSubscribeRcv( Vars& vars , SzbaseProt& prot )
		: vars(vars) , prot(prot)
	{
		set_next(std::bind(&CustomSubscribeRcv::parse_command,this,std::placeholders::_1));
	}

	virtual ~CustomSubscribeRcv()
	{
	}

	void parse_command( const std::string& data )
	{
		namespace bp = boost::property_tree;

		std::stringstream ss(data);
		bp::ptree json;

		try {
			bp::json_parser::read_json(ss,json);
		} catch(const bp::ptree_error& e) {
			fail(ErrorCodes::ill_formed);
			return;
		}
		
		std::string probe_type;
		if (json.count("type")) {
			probe_type = json.get<std::string>("type");
		} else {
			fail(ErrorCodes::ill_formed);
		}

		if (json.count("params")) {
			auto json_params = json.get_child("params");
			std::vector<std::string> params;
			for (auto jp = json_params.begin(); jp != json_params.end(); ++jp) {
				params.push_back((*jp).second.get<std::string>(""));
			}
			prot.subscribe_custom(ProbeType(probe_type), params);
			apply();
		} else {
			fail(ErrorCodes::ill_formed);
		}
	}

protected:
	Vars& vars;
	SzbaseProt& prot;

};

#endif /* end of include guard: __SERVER_CMD_CUSTOM_SUBSCRIBE_H__ */

