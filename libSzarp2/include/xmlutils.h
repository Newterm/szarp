/* 
  SZARP: SCADA software 

*/
/*
 * XML utils.
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Miscellaneous libxml2 related functions.
 *
 * $Id$
 */

#ifndef __XMLUTILS_H__
#define __XMLUTILS_H__

#ifndef NO_XML

#include <libxml/parser.h>

#ifdef LIBXML_XPATH_ENABLED

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <string>

/** Returns pointer to XML node described by XPath expresion.
 * @param xpath_expr expresion describing node (exactly one!)
 * @param xpath_ctx libxml2 xpath context
 * @return pointer to node found, NULL if not found of more than 1 found
 */
xmlNodePtr uxmlXPathGetNode(const xmlChar *xpath_expr, 
		xmlXPathContextPtr xpath_ctx,
		bool log_write = true);

/** Get content of attribute described by XPath expresion.
 * @param xpath_expr expresion describing attribute
 * @param xpath_ctx libxml2 xpath context
 * @return attribute content, NULL if not found
 */
xmlChar *uxmlXPathGetProp(const xmlChar *xpath_expr, 
		xmlXPathContextPtr xpath_ctx,
		bool log_write = true);

/** Dump XML document to string
 * (may contain non-printable chars as TAB, FORM FEED)
 */
std::string xmlToString(xmlDocPtr doc);

#endif /* LIBXML_XPATH_ENABLED */

#endif /* not NO_XML */

#endif /* __XMLUTILS_H__ */
