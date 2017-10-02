#include "config_info.h"
#include "szarp_config.h"

int TAttribHolder::parseXML(xmlNodePtr node) {
	for (xmlAttrPtr at = node->properties; at; at = at->next) {
		xmlChar* value = xmlNodeListGetString(node->doc, at->children, 1);
		xmlNs* ns = at->ns;
		if (ns) {
			if (SC::U2A(ns->href) == TAttribHolder::extra_ns_href) {
				storeAttribute(std::string("extra:")+SC::U2A(at->name), SC::U2A(value));
			} else {
				storeAttribute(SC::U2A(ns->prefix)+std::string(":")+SC::U2A(at->name), SC::U2A(value));
			}
		} else {
			storeAttribute(SC::U2A(at->name), SC::U2A(value));
		}

		xmlFree(value);
	}

	return processAttributes();
}

int TAttribHolder::parseXML(xmlTextReaderPtr reader) {
	return parseXML(xmlTextReaderCurrentNode(reader));
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
