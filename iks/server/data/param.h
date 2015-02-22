#ifndef __DATA_PARAM_H__
#define __DATA_PARAM_H__

#include <string>
#include <ostream>
#include <memory>

#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>

#include "utils/exception.h"

#include "data/probe_type.h"

class Param {
public:
	typedef std::shared_ptr<Param> ptr;
	typedef std::shared_ptr<const Param> const_ptr;

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

	bool is_summaric() const
	{	return summaric; }

	std::string get_summaric_unit() const
	{	return summaric_unit; }

	std::string get_draw_name() const
	{	return draw_name; }

	double get_value( ProbeType pt = ProbeType::Type::LIVE ) const;
	void set_value( double v , ProbeType pt );

private:
	bool get_summaric_from_xml() const;
	std::string get_summaric_unit_from_xml() const;

	static const std::set<std::string> summaric_units;
	std::string parent_tag;
	std::string name;
	std::string draw_name;
	bool summaric;
	std::string summaric_unit;

	std::map<ProbeType,double> values;

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

