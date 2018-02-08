#ifndef CONFIGINFO__H__
#define CONFIGINFO__H__

#include <string>
#include <map>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include "conversion.h"
#include <sstream>
#include <boost/optional.hpp>
#include "argsmgr.h"

namespace pt = boost::property_tree;

struct IPCInfo {
	std::string GetParcookPath() const { return parcook_path; }
	std::string GetLinexPath() const { return linex_path; }

	std::string parcook_path;
	std::string linex_path;
};

class TAttribHolder {
public:
	virtual ~TAttribHolder() {}

	// throws
	template <typename RT = std::string>
	RT getAttribute(std::string attr_name) const {
		auto opt = getOptAttribute<RT>(attr_name);
		if (opt) return *opt;

		throw std::runtime_error("Attribute "+attr_name+" not present! Cannot continue!");
	}

	template <typename RT>
	RT getAttribute(std::string attr_name, const RT& fallback) const {
		return getOptAttribute<RT>(attr_name).get_value_or(fallback);
	}

	template <typename RT>
	boost::optional<RT> getOptAttribute(std::string attr_name) const {
		try {
			auto attr = attrs.find(attr_name);
			if (attr != attrs.end()) {
				return cast_val<RT>(attr->second);
			}
		} catch(...) {}

		return boost::none;
	}

	bool hasAttribute(std::string attr_name) const {
		return attrs.find(attr_name) != attrs.end();
	}

	std::vector<std::pair<std::string, std::string>> getAllAttributes() const {
		std::vector<std::pair<std::string, std::string>> ret;
		for (auto it: attrs) {
			ret.push_back(std::make_pair(it.first, it.second));
		}
		return ret;
	}

	virtual int parseXML(xmlNodePtr node);
	virtual int parseXML(xmlTextReaderPtr reader);
	virtual int parseXML(const boost::property_tree::ptree&);

	virtual int processAttributes() { return 0; }

protected:
	template <typename T>
	void storeAttribute(std::string attr_name, T attr_val) {
		std::stringstream ss;
		ss << attr_val;
		attrs[attr_name] = ss.str();
	}

private:
	std::map<std::string, std::string> attrs;

	static constexpr auto extra_ns_href = "http://www.praterm.com.pl/SZARP/ipk-extra";
};

template <>
std::string TAttribHolder::getAttribute(std::string attr_name) const ;

template <>
const std::string TAttribHolder::getAttribute(std::string attr_name) const ;

template <>
char* TAttribHolder::getAttribute(std::string attr_name) const ;

template <>
const char* TAttribHolder::getAttribute(std::string attr_name) const ;

template <typename T>
class TNodeList {
	T* next{ nullptr };

public:
	virtual ~TNodeList() {
		if (next) delete next;
	}

	// Appends at end and returns added element
	virtual T* Append(T* t) {
		if (next == nullptr) {
			SetNext(t);
		} else {
			T* r = next;
			while (r->next)
				r = r->next;
			r->SetNext(t);
		}

		return t;
	}

	virtual T* GetNext() const {
		return next;
	}

	virtual void SetNext(T* t) {
		next = t;
	}
};


#endif
