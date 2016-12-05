#ifndef __DATA_SETS_H__
#define __DATA_SETS_H__

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include "set.h"

#include "utils/signals.h"
#include "utils/exception.h"
#include "utils/iterator.h"

class Sets {
	struct SetEntry {
		typedef std::string first_type;

		std::string first;
		double order;
		Set::ptr set;
	};

	struct set_id {};
	struct set_name {};

public:
	typedef boost::multi_index::multi_index_container<
		SetEntry,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<set_id>,
				boost::multi_index::member<SetEntry,double,&SetEntry::order>
			>,
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<set_name>,
				boost::multi_index::member<SetEntry,std::string,&SetEntry::first>
			>
		>
	> SetsMap;

	using iterator = key_iterator<SetsMap>;

	void from_params_file( const std::string& path ) throw(xml_parse_error);

	iterator begin() const { return iterator(sets.cbegin()); }
	iterator end  () const { return iterator(sets.cend  ()); }

	bool has_set  ( const std::string& name ) const
	{	return boost::multi_index::get<set_name>(sets).count( name ); }

	std::shared_ptr<const Set>   get_set( const std::string& name ) const;
	std::shared_ptr<const Report> get_report( const std::string& name ) const;

	const SetsMap& list_reports() const { return reports; }

	void update_set( const Set& s , const std::string& old_name = "" );

	slot_connection on_set_updated( const sig_set_slot& slot ) const
	{	return emit_set_updated.connect( slot ); }

protected:
	void from_system_sets_xml( const boost::property_tree::ptree& sets_ptree ) throw(xml_parse_error);
	void from_params_xml( boost::property_tree::ptree& sets_ptree ) throw(xml_parse_error);


private:
	SetsMap   sets;
	SetsMap   reports;

	mutable sig_set emit_set_updated;

};

#endif /* end of include guard: __DATA_SETS_H__ */

