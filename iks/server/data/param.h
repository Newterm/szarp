#ifndef __DATA_PARAM_H__
#define __DATA_PARAM_H__

#include <string>
#include <ostream>
#include <memory>

#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>

#include "utils/exception.h"

class Param {
public:
	Param( const std::string& parent );
	virtual ~Param();

	void from_params_xml( const boost::property_tree::ptree& var_ptree ) throw(xml_parse_error);

	void to_xml ( std::ostream& stream , bool pretty = false ) const;
	void to_json( std::ostream& stream , bool pretty = false ) const;

	std::string to_xml ( bool pretty = false ) const;
	std::string to_json( bool pretty = false ) const;

	const std::string& get_name() const
	{	return name; }

	const boost::property_tree::ptree& get_ptree() const
	{	return param_desc; }

	double get_value() const { return value; }
	void set_value( double v ) { value = v; }

private:
	std::string parent_tag;
	std::string name;

	double value;

	boost::property_tree::ptree param_desc;
};

typedef boost::signals2::signal<void (std::shared_ptr<const Param>)> sig_param;
typedef sig_param::slot_type sig_param_slot;

struct ParamRequest {
	std::shared_ptr<const Param> param;
	double value;
	std::string pin;
};

typedef boost::signals2::signal<void (std::shared_ptr<ParamRequest>)> sig_param_request;
typedef sig_param_request::slot_type sig_param_request_slot;

#endif /* __DATA_PARAM_H__ */

