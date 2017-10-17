#include "config_info.h"
#include "szarp_config.h"

int TAttribHolder::parseXML(xmlNodePtr node) {
	for (xmlAttrPtr at = node->properties; at; at = at->next) {
		xmlChar* value = xmlNodeListGetString(node->doc, at->children, 1);
		xmlNs* ns = at->ns;
		if (ns) {
			if (SC::U2A(ns->href) == TAttribHolder::extra_ns_href) {
				storeAttribute(std::string("extra:")+SC::U2L(at->name), SC::U2L(value));
			} else {
				storeAttribute(SC::U2L(ns->prefix)+std::string(":")+SC::U2L(at->name), SC::U2L(value));
			}
		} else {
			storeAttribute(SC::U2L(at->name), SC::U2L(value));
		}

		xmlFree(value);
	}

	return processAttributes();
}

int TAttribHolder::parseXML(xmlTextReaderPtr reader) {
	return TAttribHolder::parseXML(xmlTextReaderCurrentNode(reader));
}

int TAttribHolder::parseXML(const boost::property_tree::wptree& tree) {
	auto attrs = tree.get_child(L"<xmlattr>");
	for (const auto& attr: attrs) {
		auto attr_name = SC::S2L(attr.first);
		size_t sep_pos = attr_name.find_first_of(":");
		if (sep_pos != std::string::npos) {
			auto ns_prefix = attr_name.substr(0, sep_pos);
			if (ns_prefix == "extra") {}
			else if (ns_prefix == "modbus" || ns_prefix == "iec" || ns_prefix == "k601" || ns_prefix ==  "exec" || ns_prefix == "mbus") {
				//deprecated prefixes
				attr_name = "extra"+attr_name.substr(sep_pos);
			}
		}

		storeAttribute(attr_name, SC::S2L(attr.second.data()));
	}

	return processAttributes();
}

template <>
std::string TAttribHolder::getAttribute(std::string attr_name) const {
	auto attr = attrs.find(attr_name);
	if (attr != attrs.end()) {
		return attr->second;
	}

	throw std::runtime_error("Attribute "+attr_name+" not present! Cannot continue!");
}


template <>
const std::string TAttribHolder::getAttribute(std::string attr_name) const {
	return getAttribute<std::string>(attr_name);
}


template <>
char* TAttribHolder::getAttribute(std::string attr_name) const {
	return strdup(getAttribute<std::string>(attr_name).c_str());
}


template <>
const char* TAttribHolder::getAttribute(std::string attr_name) const {
	return getAttribute<std::string>(attr_name).c_str();
}
