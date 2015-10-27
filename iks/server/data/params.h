#ifndef __DATA_PARAMS_H__
#define __DATA_PARAMS_H__

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

#include "param.h"
#include "szbase_wrapper.h"

#include "utils/exception.h"
#include "utils/iterator.h"
#include "utils/signals.h"

typedef boost::signals2::signal<void (std::shared_ptr<const Param>)> sig_param;
typedef sig_param::slot_type sig_param_slot;

typedef boost::signals2::signal<void (std::shared_ptr<const Param>,double,ProbeType)> sig_param_value;
typedef sig_param_value::slot_type sig_param_value_slot;

class Params {
	typedef std::unordered_map<std::string,std::shared_ptr<Param>> ParamsMap;

public:
	class iterator : public key_iterator<ParamsMap> {
		friend class Params;
	public:
		iterator( const iterator& itr )
			: key_iterator<ParamsMap>(itr.itr) {}
		iterator( const ParamsMap::const_iterator& itr )
			: key_iterator<ParamsMap>(itr) {}
	};

	Params();

	void from_params_file( const std::string& path ) throw(xml_parse_error);

	iterator begin() const { return iterator(params.cbegin()); }
	iterator end  () const { return iterator(params.cend  ()); }

	iterator find( const std::string& name ) const
	{	return iterator(params.find(name)); }

	bool has_param( const std::string& name ) const
	{	return params.count(name); }

	std::shared_ptr<const Param> get_param( const std::string& name ) const;

	void param_changed( iterator itr );
	void param_changed( const std::string& name );

	void param_value_changed( iterator itr , double value , ProbeType pt );
	void param_value_changed( const std::string& name , double value , ProbeType pt );
	void request_param_value( const std::string& name ,
	                          double value ,
							  const std::string& pin = "" ) const;

	slot_connection on_param_changed( const sig_param_slot& slot ) const
	{	return emit_changed.connect( slot ); }
	slot_connection on_param_value_changed( const sig_param_value_slot& slot ) const
	{	return emit_value_changed.connect( slot ); }
	slot_connection on_request_param_value( const sig_param_request_slot& slot ) const
	{	return emit_request_value.connect( slot ); }

protected:
	void from_vars_xml( const boost::property_tree::ptree& vars_doc ) throw(xml_parse_error);
	void from_params_xml( boost::property_tree::ptree& params_doc ) throw(xml_parse_error);

private:
	ParamsMap params;

	mutable sig_param_value emit_value_changed;
	mutable sig_param emit_changed;
	mutable sig_param_request emit_request_value;
};

#endif /* end of include guard: __DATA_PARAMS_H__ */

