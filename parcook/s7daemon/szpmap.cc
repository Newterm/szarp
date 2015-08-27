#include "szpmap.h"

SzParamMap::SzParam SzParamMap::ConfigureParamFromXml( unsigned long int idx, xmlNodePtr& node ) 
{	
	sz_log(10, "SzParamMap::ConfigureParamFromXml");

	/* val_type */
	xmlChar* prop = xmlGetNsProp(node, BAD_CAST("val_type"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(0, "No attribute val_type given for param in line %ld",
			       	xmlGetLineNo(node));
		return SzParam();
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
		return SzParam();
	}
	int address = boost::lexical_cast<int>((char*)prop);
	xmlFree(prop);

	return SzParam(address, val_type, val_op, 0);
}

bool SzParamMap::ConfigureParamFromXml( unsigned long int idx, TSendParam* s, xmlNodePtr& node ) 
{	
	sz_log(10, "SzParamMap::ConfigureParamFromXml(TSendParam)");

	SzParam szp = ConfigureParamFromXml(idx, node);
	if (!szp.isValid()) return false;

	/* extra:prec is required for sending floats */
	xmlChar* prop = xmlGetNsProp(node, BAD_CAST("prec"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(0, "No attribute prec (required for float) given for param in line %ld",
			       	xmlGetLineNo(node));

		if (szp.isFloat()) return false;

	} else {
		szp.setPrec(boost::lexical_cast<int>((char*)prop));
		xmlFree(prop);
	}

	_params[idx] = szp;
	_send_params[idx] = s;

	return true;
}

bool SzParamMap::ConfigureParamFromXml( unsigned long int idx, TParam* p, xmlNodePtr& node ) 
{	
	sz_log(10, "SzParamMap::ConfigureParamFromXml(TParam)");

	SzParam szp = ConfigureParamFromXml(idx, node);
	if (!szp.isValid()) return false;
	
	szp.setPrec(p->GetPrec());

	_params[idx] = szp;
	return true;
}

