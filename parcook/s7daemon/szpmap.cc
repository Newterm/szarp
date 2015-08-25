#include "szpmap.h"

bool SzParamMap::ConfigureParamFromXml( unsigned long int idx, TParam* p, xmlNodePtr& node ) 
{	
	sz_log(10, "SzParamMap::ConfigureParamFromXml");

	/* val_type */
	xmlChar* prop = xmlGetNsProp(node, BAD_CAST("val_type"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(0, "No attribute val_type given for param in line %ld",
			       	xmlGetLineNo(node));
		return false;
	}
	std::string val_type = std::string((char*)prop);
	xmlFree(prop);

	/* val_op */
	std::string val_op("");
	prop = xmlGetNsProp(node, BAD_CAST("val_op"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(2, "No attribute val_op given for param in line %ld",
			       	xmlGetLineNo(node));
	} else {
		val_op = std::string((char*)prop);
		xmlFree(prop);
	}

	/* address */
	prop = xmlGetNsProp(node, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(0, "No attribute address given for param in line %ld",
			       	xmlGetLineNo(node));
		return false;
	}
	int address = boost::lexical_cast<int>((char*)prop);
	xmlFree(prop);

	_params[idx] = SzParam(address, val_type, val_op, p->GetPrec());
	return true;
}

