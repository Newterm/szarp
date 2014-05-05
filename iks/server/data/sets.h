#ifndef __DATA_SETS_H__
#define __DATA_SETS_H__

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>

#include "set.h"

#include "utils/signals.h"
#include "utils/exception.h"
#include "utils/iterator.h"

class Sets {
	typedef std::unordered_map<std::string,std::shared_ptr<Set>> SetsMap;

public:
	using iterator = key_iterator<SetsMap>;

	void from_params_file( const std::string& path ) throw(xml_parse_error);

	void to_file( const std::string& path ) const;
	boost::property_tree::ptree get_xml_ptree() const;

	iterator begin() const { return iterator(sets.cbegin()); }
	iterator end  () const { return iterator(sets.cend  ()); }

	bool has_set  ( const std::string& name ) const
	{	return sets  .count(name); }

	std::shared_ptr<const Set>   get_set( const std::string& name ) const;

	void update_set( const Set& s , const std::string& old_name = "" );

	connection on_set_updated( const sig_set_slot& slot ) const
	{	return emit_set_updated.connect( slot ); }

protected:
	void from_system_sets_xml( const boost::property_tree::ptree& sets_ptree ) throw(xml_parse_error);
	void from_params_xml( boost::property_tree::ptree& sets_ptree ) throw(xml_parse_error);


private:
	SetsMap   sets;

	mutable sig_set emit_set_updated;

};

#endif /* end of include guard: __DATA_SETS_H__ */

