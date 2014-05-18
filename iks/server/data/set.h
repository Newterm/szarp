#ifndef __DATA_SET_H__
#define __DATA_SET_H__

#include <string>
#include <unordered_set>

#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>

class Set {
	friend bool operator==( const Set& a , const Set& b );

	typedef std::unordered_set<std::string> ParamsMap;
public:
	typedef ParamsMap::iterator iterator;
	typedef ParamsMap::const_iterator const_iterator;

	Set();
	virtual ~Set();

	void from_xml ( const boost::property_tree::ptree& set_ptree );
	void from_json( const boost::property_tree::ptree& set_ptree );

	void to_xml ( std::ostream& stream , bool pretty = false ) const;
	void to_json( std::ostream& stream , bool pretty = false ) const;

	boost::property_tree::ptree get_xml_ptree() const;
	std::string to_xml ( bool pretty = false ) const;
	std::string to_json( bool pretty = false ) const;

	const std::string& get_name() const
	{	return name; }

	const size_t& get_hash() const
	{	return hash; }

	/* TODO: Better check if set is valid (20/03/2014 09:23, jkotur) */
	bool empty() const
	{	return name.empty(); }

	iterator begin() { return params.begin(); }
	iterator   end() { return params.  end(); }

	const_iterator begin() const { return params.begin(); }
	const_iterator   end() const { return params.  end(); }

	const_iterator cbegin() const { return params.begin(); }
	const_iterator cend  () const { return params.  end(); }

	bool has_param( const std::string& param_name ) const
	{	return params.count( param_name ); }

private:
	void update_hash();
	void convert_colour( boost::property_tree::ptree& ptree , const std::string& name );
	std::string convert_colour( const std::string& in );

	std::string name;
	std::size_t hash;

	ParamsMap params;

	boost::property_tree::ptree set_desc;
};

bool operator==( const Set& a , const Set& b );
bool operator!=( const Set& a , const Set& b );

typedef boost::signals2::signal<void (const std::string&,std::shared_ptr<const Set>)> sig_set;
typedef sig_set::slot_type sig_set_slot;

#endif /* __DATA_SET_H__ */

