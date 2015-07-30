/* 
  SZARP: SCADA software 

*/
/*
 * XML utils
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Miscellaneous libxml2 related functions.
 *
 * $Id$
 */

#include "xmlutils.h"

#ifndef NO_XML

#include <libxml/parser.h>

#ifdef LIBXML_XPATH_ENABLED

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <assert.h>

#include "liblog.h"

xmlNodePtr uxmlXPathGetNode(const xmlChar *xpath_expr,
		xmlXPathContextPtr xpath_ctx)
{
	xmlXPathObjectPtr xpath_obj;
	xmlNodePtr node;

	assert (xpath_expr != NULL);
	assert (xpath_ctx != NULL);
	
	xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
	assert (xpath_obj != NULL);

	if (!xpath_obj->nodesetval || (xpath_obj->nodesetval->nodeNr != 1)) {
		sz_log(1, "Error parsing XML - one '%s' expected, found %d",
				(char *)xpath_expr, 
					xpath_obj->nodesetval ? 
					xpath_obj->nodesetval->nodeNr :
					0);
		xmlXPathFreeObject(xpath_obj);
		return NULL;
	}
	
	node = xpath_obj->nodesetval->nodeTab[0];
	
	if (!node) {
		sz_log(1, "Error parsing XML - '%s' is NULL",
				xpath_expr);
		return NULL;
	}

	xmlXPathFreeObject(xpath_obj);
	return node;
}
		

xmlChar *uxmlXPathGetProp(const xmlChar *xpath_expr, 
		xmlXPathContextPtr xpath_ctx)
{
	xmlXPathObjectPtr xpath_obj;
	xmlNodePtr node;
	xmlChar *ret;

	assert (xpath_expr != NULL);
	assert (xpath_ctx != NULL);
	
	xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
	assert (xpath_obj != NULL);

	if (!xpath_obj->nodesetval || (xpath_obj->nodesetval->nodeNr != 1)) {
		sz_log(1, "Error parsing XML - one '%s' expected, found %d",
				(char *)xpath_expr, 
					xpath_obj->nodesetval ? 
					xpath_obj->nodesetval->nodeNr :
					0);
		xmlXPathFreeObject(xpath_obj);
		return NULL;
	}
	
	node = xpath_obj->nodesetval->nodeTab[0]->children;
	if (!node || !node->content) {
		sz_log(1, "Error parsing XML - '%s' has no content",
				xpath_expr);
		return NULL;
	}

	ret = xmlStrdup(node->content);

	xmlXPathFreeObject(xpath_obj);
	return ret;
}

std::string xmlToString(xmlDocPtr doc)
{
	xmlChar *s;
	int size;
	xmlDocDumpMemory(doc, &s, &size);
	if (s == NULL)
		throw std::bad_alloc();
	std::string out;
	try {
		out = (char *)s;
	} catch (...) {
		xmlFree(s);
		throw;
	}
	xmlFree(s);
	return out;
}

#endif /* LIBXML_XPATH_ENABLED */

#endif /* not NO_XML */

