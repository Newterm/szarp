#ifndef __DATA_SET_H__
#define __DATA_SET_H__

#include <memory>
#include <string>
#include <unordered_set>
#include <cmath>

#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include "utils/iterator.h"


class Set {
	friend bool operator==( const Set& a , const Set& b );

	/**
	 * This struct has first member and its type to use key_iterator class
	 */
	struct ParamId {
		typedef std::string first_type;
		std::string first;
		double order;
		boost::property_tree::ptree desc;
	};

	struct param_id {};
	struct param_name {};

	typedef boost::multi_index::multi_index_container<
		ParamId,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<param_id>,
				boost::multi_index::member<ParamId,double,&ParamId::order>
			>,
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<param_name>,
				boost::multi_index::member<ParamId,std::string,&ParamId::first>
			>
		>
	> ParamsMap;

public:
	typedef std::shared_ptr<Set> ptr;
	typedef std::shared_ptr<const Set> const_ptr;

	typedef key_iterator<ParamsMap> const_iterator;

	Set();
	virtual ~Set();

	void from_xml ( const boost::property_tree::ptree& set_ptree );
	void from_json( const boost::property_tree::ptree& set_ptree );

	void to_xml ( std::ostream& stream , bool pretty = false ) const;
	void to_json( std::ostream& stream , bool pretty = false ) const;

	boost::property_tree::ptree get_json_ptree() const;
	boost::property_tree::ptree get_xml_ptree() const;
	std::string to_xml ( bool pretty = false ) const;
	std::string to_json( bool pretty = false ) const;

	bool has_order() const
	{	using std::isnan; return !isnan(order); }
	void set_order( double o )
	{	order = o; }
	const double get_order() const
	{	return order; }

	const std::string& get_name() const
	{	return name; }

	const size_t& get_hash() const
	{	return hash; }

	/* TODO: Better check if set is valid (20/03/2014 09:23, jkotur) */
	bool empty() const
	{	return name.empty(); }

	const_iterator begin() const { return cbegin(); }
	const_iterator   end() const { return   cend(); }

	const_iterator cbegin() const { return const_iterator(params.begin()); }
	const_iterator cend  () const { return const_iterator(params.  end()); }

	bool has_param( const std::string& name ) const
	{	return boost::multi_index::get<param_name>(params).count( name ); }

private:
	void update_hash();
	void upgrade_option( boost::property_tree::ptree& ptree , const std::string& prev , const std::string& curr );
	void convert_float ( boost::property_tree::ptree& ptree , const std::string& name );
	void convert_colour( boost::property_tree::ptree& ptree , const std::string& name );
	std::string convert_colour( const std::string& in );

	void convert_color_names_to_hex();
	void assign_color( ParamId& p , const std::string& color );
	void generate_colors_like_draw3();

	std::string name;
	std::size_t hash;
	double order;

	ParamsMap params;

	boost::property_tree::ptree set_desc;
};

bool operator==( const Set& a , const Set& b );
bool operator!=( const Set& a , const Set& b );

typedef boost::signals2::signal<void (const std::string&,std::shared_ptr<const Set>)> sig_set;
typedef sig_set::slot_type sig_set_slot;

#endif /* __DATA_SET_H__ */

