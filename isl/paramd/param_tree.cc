/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/*
 * Pawe³ Pa³ucha 2002
 *
 * param_tree.cc - XML params tree
 *
 * $Id$
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>

#include "svg_generator.h"
#include "param_tree.h"
#include "tree_processor.h"
#include "utf8.h"
#include "tokens.h"
#include "conversion.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <queue>

#define SZ_REPORTS_NS_URI "http://www.praterm.com.pl/SZARP/reports"
#define SZ_LIST_NS_URI "http://www.praterm.com.pl/SZARP/list"
#define SZ_CONTROL_NS_URI "http://www.praterm.com.pl/SZARP/control"

namespace {

const char* control_raport_attributes[] = 
	{ "formula", "value_check", "value_min", "value_max", "data_period", "precision", "alarm_type", "alarm_group", "alt_name" };


void copy_control_related_attributes(xmlNodePtr src, xmlNodePtr dst, xmlNsPtr ns) { 
	for (size_t i = 0; i < sizeof(control_raport_attributes) / sizeof(control_raport_attributes[0]); ++i) {
		xmlChar* prop = xmlGetNsProp(src, BAD_CAST control_raport_attributes[i], BAD_CAST SZ_CONTROL_NS_URI);
		if (prop) {
			xmlSetNsProp(dst, ns, BAD_CAST control_raport_attributes[i], prop);
			xmlFree(prop);
		}
	}
}

}


/**
 * Tree update check method. Checks and performs param values update.
 */
void ParamTree::check_update()
{
	int t;
	
	t = time(NULL);
	if (t - last_update < update_freq) {
		return;
	}
	update();
	last_update = time(NULL);
}

/**
 * Updates information about param values.
 */
void ParamTree::update(void)
{

	std::for_each(dynamicTree.updateTriggers.begin(), dynamicTree.updateTriggers.end(), &callMyArgument<UpdateTrigger> );

	for (xmlNodePtr node = tree->children; node; node = node->next)
		update_node(node);
}

/**
 * Recursive update of param values in node.
 * @param node node to update
 */
void ParamTree::update_node(xmlNodePtr node)
{
	if (!node)
		return;
	if (strcmp((char *)node->name, "param")) {
		if (!testNodeLastUsedTime(node)) {

			std::queue<xmlNodePtr> visitq;

			visitq.push(node);

			while (!visitq.empty()) {
				xmlNodePtr tmp = visitq.front();
				visitq.pop();
				for (tmp = tmp->children; tmp; tmp = tmp->next)
					visitq.push(tmp);
				dynamicTree.set_map.erase(tmp);
				dynamicTree.get_map.erase(tmp);
			}

			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return;
		}
		for (node = node->children; node; node = node->next)
			update_node(node);
	} else {
		boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> vls = dynamicTree.get_map[node]();

		xmlSetProp(node, (xmlChar *)"value", SC::S2U(vls.get<0>()).c_str());
		xmlSetProp(node, (xmlChar *)"value_minute", SC::S2U(vls.get<1>()).c_str());
		xmlSetProp(node, (xmlChar *)"value_10minutes", SC::S2U(vls.get<2>()).c_str());
		xmlSetProp(node, (xmlChar *)"value_hour", SC::S2U(vls.get<3>()).c_str());
	}
}

/**
 * Returns information from tree in XML format.
 * @param uri path to requested resource
 * @param code HTTP return code, can be set to 203 (Not Found) for example,
 * 200 means OK
 * @return dynamically allocated buffer with response content
 */
char *ParamTree::processXML(ParsedURI *uri, int *code)
{
	check_update();
	
	*code = 200;
	if (uri->nodes == NULL)
		return treeProcessor->dumpDocXML(tree);
	else {
		/* find node */
		xmlNodePtr node;
		node = parse_uri(tree->children, uri, code);
		return treeProcessor->processXML(node, code, uri->attribute);
	}
}

char *ParamTree::processReports()
{
	xmlOutputBufferPtr out_buf;
	char *xmlbuffer;
	
	xmlNodePtr node;
	node = find_node(tree->children->children, L"Reports");

	out_buf = xmlAllocOutputBuffer(NULL);

	xmlOutputBufferWriteString(out_buf, "<?xml version=\"1.0\"  encoding=\"UTF-8\"?>\n");

	xmlOutputBufferWriteString(out_buf, "<list:reports xmlns:rap=\"");
	xmlOutputBufferWriteString(out_buf, SZ_REPORTS_NS_URI);
	xmlOutputBufferWriteString(out_buf, "\" xmlns:list=\"");
	xmlOutputBufferWriteString(out_buf, SZ_LIST_NS_URI);
	xmlOutputBufferWriteString(out_buf, "\"");

	if (node != NULL) {
		xmlOutputBufferWriteString(out_buf, " source=\"");
		xmlOutputBufferWriteString(out_buf, (char *) SC::S2U(dynamicTree.szarpConfig->GetTitle()).c_str());
		xmlOutputBufferWriteString(out_buf, "\">\n");

		for (node = node->children; node; node = node->next) {
			xmlOutputBufferWriteString(out_buf, "<list:raport name=\"");
			xmlChar* prop = xmlGetProp(node, (xmlChar *)"name");
			xmlOutputBufferWriteString(out_buf, (char *) prop);
			xmlFree(prop);
			xmlOutputBufferWriteString(out_buf, "\" />\n");
		}
	} else {
		xmlOutputBufferWriteString(out_buf, ">\n");
	}

	xmlOutputBufferWriteString(out_buf, "</list:reports>\n");

	xmlOutputBufferFlush(out_buf);

#ifdef  LIBXML2_NEW_BUFFER
	xmlBufPtr _buf = out_buf->conv != NULL ? out_buf->conv : out_buf->buffer;
	xmlbuffer = (char *)xmlStrndup(xmlBufContent(_buf), xmlBufUse(_buf));
#else
	if (out_buf->conv != NULL)
		xmlbuffer = (char *)xmlStrndup(
				out_buf->conv->content, 
				out_buf->conv->use);
	else 
		xmlbuffer = (char *)xmlStrndup(
				out_buf->buffer->content, 
				out_buf->buffer->use);
#endif // LIBXML2_NEW_BUFFER
	xmlOutputBufferClose(out_buf);

	return xmlbuffer;
}

std::wstring ParamTree::processControlRaport(char *request, int *code) {
	xmlDocPtr doc = xmlReadMemory(request, strlen(request), NULL, NULL, 0);

	if (doc == NULL || doc->children == NULL || doc->children->children == NULL) {
		*code = 400;
		return L"No params";
	}
	return setControlRaport(doc, code);
}
	
std::wstring ParamTree::setControlRaport(xmlDocPtr doc, int *code) {
	std::wstring error_string;
	xmlChar* _title;

	xmlNodePtr control_raport_node = find_node(tree->children->children, L"control_raport");

	if (control_raport_node == NULL) {
		xmlNewNs(xmlDocGetRootElement(tree), 
				BAD_CAST SZ_CONTROL_NS_URI,	
				BAD_CAST "kon");

		control_raport_node = xmlNewChild(tree->children, NULL, (const xmlChar*)"node", NULL);
		xmlSetProp(control_raport_node, (const xmlChar*)"name", (const xmlChar*)"control_raport");
	}

	std::vector<xmlNodePtr> new_nodes, old_nodes;

	xmlNsPtr cns = xmlSearchNsByHref(tree, control_raport_node, BAD_CAST SZ_CONTROL_NS_URI); 
	assert(cns);

	xmlNodePtr node;
	int order = 1;
	for (node = doc->children->children; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;
		xmlChar *_name= xmlGetProp(node, (const xmlChar *)"name");

		if (_name == NULL)
			goto error;
		std::wstring name(SC::U2S(_name));
		xmlFree(_name);

		std::vector<std::wstring> tokens(3);
		boost::split(tokens, name, boost::is_any_of(L":") );

		if (!tokens.size()) {
			*code = 400;
			error_string = L"Invalid param name";
			goto error;
		}

		xmlNodePtr _node = tree->children;
		for (std::vector<std::wstring>::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
			_node = find_node(_node->children, *tok_iter);
			if (_node == NULL) {
				*code = 400;
				error_string = L"One of params doesn't exist";
				goto error;
			}
		}

		xmlNodePtr new_node = xmlNewNode(NULL, (xmlChar*)"param");
		copyParamToNode(_node, new_node, order++);

		copy_control_related_attributes(node, new_node, cns);
		new_nodes.push_back(new_node);

	}

	for (xmlNodePtr child = control_raport_node->children; child; child = child->next)
		old_nodes.push_back(child);
	
	for (size_t i = 0; i < old_nodes.size(); i++) {
		xmlUnlinkNode(old_nodes[i]);
		xmlFreeNode(old_nodes[i]);
	}

	for (size_t i = 0; i < new_nodes.size(); i++)
		xmlAddChild(control_raport_node, new_nodes[i]);

	_title = xmlGetProp(doc->children, BAD_CAST "title");
	if (_title == NULL) {
		std::wstringstream wss;
		wss << time(NULL);
		_title = xmlStrdup(SC::S2U(wss.str()).c_str());
	}
	xmlSetProp(control_raport_node, BAD_CAST "title", _title);
	xmlFree(_title);

	*code = 200;
	xmlFreeDoc(doc);
	return L"OK";

error:
	for (size_t i = 0; i < new_nodes.size(); i++)
		xmlFreeNode(new_nodes[i]);
	*code = 400;
	xmlFreeDoc(doc);
	return error_string;

}

char *ParamTree::processReport(char * raport, int *code, bool custom)
{
	xmlNodePtr node;
	*code = 200;
	
	check_update();

	if (raport == NULL || strlen(raport) == 0) {
		*code = 404;
		return strdup("400 Bad request");
	}

	if (custom)
		node = find_node(tree->children->children, L"custom");
	else
		node = find_node(tree->children->children, L"Reports");

	if (node == NULL) {
		*code = 404;
		return strdup("404 No reports");
	}

	node = find_node(node->children, SC::U2S((unsigned char*)raport));

	if (node == NULL) {
		*code = 404;
		return strdup("404 No raport");
	}
	if (custom)
		updateNodeLastUsedTime(node);

	return processReportParams(node);
}

char *ParamTree::processControlRaportRequest(int *code) {
	xmlNodePtr node;
	*code = 200;
	
	check_update();

	node = find_node(tree->children->children, L"control_raport");
	if (node == NULL) {
		*code = 404;
		return strdup("404 No control report");
	}

	*code = 200;
	return processReportParams(node, false, true);
}

char *ParamTree::processReportParams(xmlNodePtr raport, bool listing, bool control)
{
	check_update();

	xmlOutputBufferPtr out_buf;
	char *xmlbuffer;
	xmlChar * prop;
	
	xmlNodePtr node = raport;

	out_buf = xmlAllocOutputBuffer(NULL);

	if (!listing)
		xmlOutputBufferWriteString(out_buf, "<?xml version=\"1.0\"  encoding=\"UTF-8\"?>\n");

	xmlOutputBufferWriteString(out_buf, "<list:parameters xmlns:list=\"");
	xmlOutputBufferWriteString(out_buf, SZ_LIST_NS_URI);
	xmlOutputBufferWriteString(out_buf, "\" xmlns:rap=\"");
	xmlOutputBufferWriteString(out_buf, SZ_REPORTS_NS_URI);
	xmlOutputBufferWriteString(out_buf, "\" xmlns:kon=\"");
	xmlOutputBufferWriteString(out_buf, SZ_CONTROL_NS_URI);
	xmlOutputBufferWriteString(out_buf, "\" rap:title=\"");
	prop = xmlGetProp(node, (xmlChar *)"name");
	xmlOutputBufferWriteString(out_buf, (char *) prop);
	xmlFree(prop);
	if (!listing) {
		xmlOutputBufferWriteString(out_buf, "\" source=\"");
		xmlOutputBufferWriteString(out_buf, (char *) SC::S2U(dynamicTree.szarpConfig->GetTitle()).c_str());
	}

	xmlOutputBufferWriteString(out_buf, "\">\n");

	if (node != NULL) {

		for (node = node->children; node; node = node->next) {

			xmlOutputBufferWriteString(out_buf, "<list:param name=\"");
			prop = xmlGetProp(node, (xmlChar *)"full_name");
			xmlOutputBufferWriteString(out_buf, (char *) prop);
			xmlFree(prop);

			xmlOutputBufferWriteString(out_buf, "\" rap:short_name=\"");
			prop = xmlGetProp(node, (xmlChar *)"short_name");
			xmlOutputBufferWriteString(out_buf, (char *) prop);
			xmlFree(prop);			

			xmlOutputBufferWriteString(out_buf, "\" rap:order=\"");
			prop = xmlGetProp(node, (xmlChar *)"order");
			xmlOutputBufferWriteString(out_buf, (char *) prop);
			xmlFree(prop);

			if (!listing) {
				xmlOutputBufferWriteString(out_buf, "\" rap:unit=\"");
				prop = xmlGetProp(node, (xmlChar *)"unit");
				xmlOutputBufferWriteString(out_buf, (char *) prop);
				xmlFree(prop);
	
				xmlOutputBufferWriteString(out_buf, "\" rap:prec=\"");
				prop = xmlGetProp(node, (xmlChar *)"prec");
				xmlOutputBufferWriteString(out_buf, (char *) prop);
				xmlFree(prop);
	
				xmlOutputBufferWriteString(out_buf, "\" rap:value=\"");
				prop = xmlGetProp(node, (xmlChar *)"value");
				xmlOutputBufferWriteString(out_buf, (char *) prop);
				xmlFree(prop);
	
				xmlOutputBufferWriteString(out_buf, "\" rap:description=\"");
				prop = xmlGetProp(node, (xmlChar *)"description");
				xmlOutputBufferWriteString(out_buf, (char *) prop);
				xmlFree(prop);
			}

			if (control) {
				for (size_t i = 0; i < sizeof(control_raport_attributes) / sizeof(control_raport_attributes[0]); ++i) {
					xmlChar* prop = xmlGetNsProp(node, BAD_CAST control_raport_attributes[i], BAD_CAST SZ_CONTROL_NS_URI);
					if (prop) {
						xmlOutputBufferWriteString(out_buf, "\" kon:");
						xmlOutputBufferWriteString(out_buf, (char*) control_raport_attributes[i]);
						xmlOutputBufferWriteString(out_buf, "=\"");
						xmlOutputBufferWriteString(out_buf, (char*)prop);
						xmlFree(prop);
					}
				}
			}

			xmlOutputBufferWriteString(out_buf, "\" />\n");
		}
	}
	else {
		xmlOutputBufferWriteString(out_buf, ">\n");
	}

	xmlOutputBufferWriteString(out_buf, "</list:parameters>\n");

	xmlOutputBufferFlush(out_buf);

#ifdef  LIBXML2_NEW_BUFFER
	xmlBufPtr _buf = out_buf->conv != NULL ? out_buf->conv : out_buf->buffer;
	xmlbuffer = (char *)xmlStrndup(xmlBufContent(_buf), xmlBufUse(_buf));
#else
	if (out_buf->conv != NULL)
		xmlbuffer = (char *)xmlStrndup(
				out_buf->conv->content, 
				out_buf->conv->use);
	else 
		xmlbuffer = (char *)xmlStrndup(
				out_buf->buffer->content, 
				out_buf->buffer->use);
#endif // LIBXML2_NEW_BUFFER

	xmlOutputBufferClose(out_buf);

	return xmlbuffer;
}

std::wstring ParamTree::processCustomRaportRequest(char *request, int *code) {

	*code = 200;
	
	check_update();

	xmlDocPtr doc = xmlReadMemory(request, strlen(request), NULL, NULL, 0);

	if (doc == NULL || doc->children == NULL || doc->children->children == NULL) {
		*code = 400;
		return L"No params";
	}

	xmlNodePtr node = find_node(tree->children->children, L"custom");

	if (node == NULL) {
		node = xmlNewChild(tree->children, NULL, (const xmlChar*)"node", NULL);
		xmlSetProp(node, (const xmlChar*)"name", (const xmlChar*)"custom");
	}

	xmlNodePtr raport;
	std::wstring raportName;
	int start = customRaportCounter;

	do {
		raportName = boost::lexical_cast<std::wstring>(customRaportCounter++);
		
		raport = find_node(node, raportName);

		if (start == customRaportCounter) {
			*code = 503;
			return L"No more space for new custom raport.";
		}

	} while (raport != NULL);

	raport = xmlNewChild(node, NULL, (xmlChar*)"node", NULL);
	xmlSetProp(raport, (xmlChar*)"name", SC::S2U(raportName).c_str());
	updateNodeLastUsedTime(raport);

	int order = 0;
	for (node = doc->children->children; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE) {
			xmlChar *prop = xmlGetProp(node, (const xmlChar *)"name");

			if (prop) {
				std::wstring prop_string(SC::U2S(prop));
				xmlFree(prop);

				xmlNodePtr n;

				std::vector<std::wstring> tokens(3);
				boost::split(tokens, prop_string, boost::is_any_of(L":") );

				if (!tokens.size())
					goto error;

				n = tree->children;
				for (std::vector<std::wstring>::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
					n = find_node(n->children, *tok_iter);
					if(n == NULL) {
						xmlUnlinkNode(raport);
						xmlFreeNode(raport);
						*code = 400;
						return L"One of params doesn't exist";
					}
				}

				xmlNodePtr copy = xmlNewChild(raport, NULL, (xmlChar*)"param", NULL);
	
				copyParamToNode(n, copy, order++);
	
			} else {
				goto error;
			}
		}
	}

	return raportName;

error:
	xmlUnlinkNode(raport);
	xmlFreeNode(raport);
	*code = 400;
	return L"Error while processing";

}

void ParamTree::updateNodeLastUsedTime(xmlNodePtr node) {
	std::wstring raportTime = boost::lexical_cast<std::wstring>(time(NULL));
	xmlSetProp(node, (xmlChar*)"lastUpdateTime", SC::S2U(raportTime).c_str());
}

bool ParamTree::testNodeLastUsedTime(xmlNodePtr node) {
	xmlChar* prop;
	prop = xmlGetProp(node, (xmlChar*)"lastUpdateTime");
	if (prop != NULL) {
		long nodeTime = atol((char *)prop);
		xmlFree(prop);
		return time(NULL) - nodeTime < 600;
	}
	return true;
}

void ParamTree::copyParamToNode(xmlNodePtr src, xmlNodePtr dst, int order) {
	xmlChar *prop;
	prop = xmlGetProp(src, (xmlChar *)"name");
	xmlSetProp(dst, (xmlChar*)"name", prop);
	xmlFree(prop);
	prop = xmlGetProp(src, (xmlChar *)"full_name");
	xmlSetProp(dst, (xmlChar*)"full_name", prop);
	xmlFree(prop);
	prop = xmlGetProp(src, (xmlChar *)"short_name");
	xmlSetProp(dst, (xmlChar*)"short_name", prop);
	xmlFree(prop);
	prop = xmlGetProp(src, (xmlChar *)"unit");
	xmlSetProp(dst, (xmlChar*)"unit", prop);
	xmlFree(prop);
	prop = xmlGetProp(src, (xmlChar *)"ipc_ind");
	xmlSetProp(dst, (xmlChar*)"ipc_ind", prop);
	xmlFree(prop);
	prop = xmlGetProp(src, (xmlChar *)"prec");
	xmlSetProp(dst, (xmlChar*)"prec", prop);
	xmlFree(prop);
	prop = xmlGetProp(src, (xmlChar *)"value");
	xmlSetProp(dst, (xmlChar*)"value", prop);
	xmlFree(prop);

	char order_str[10];
	sprintf(order_str, "%d", order);
	xmlSetProp(dst, (xmlChar*)"order", (xmlChar*)order_str);

	for (xmlNodePtr n = src->children; n; n = n->next) {
		xmlNodePtr tmp = xmlNewChild(dst, NULL, n->name, NULL);
		prop = xmlGetProp(n, (xmlChar *)"ipc_ind");
		xmlSetProp(tmp, (xmlChar*)"ipc_ind", prop);
		xmlFree(prop);
	}
	dynamicTree.get_map[dst] = dynamicTree.get_map[src];
	dynamicTree.set_map[dst] = dynamicTree.set_map[src];
}

/**
 * Returns information from tree in HTML format.
 * @param uri path to requested resource
 * @param code HTTP return code, can be set to 203 (Not Found) for example,
 * 200 means OK
 * @return dynamically allocated buffer with response content
 */
char *ParamTree::processHTML(ParsedURI *uri, int *code)
{
	xmlNodePtr node;
	check_update();
	
	*code = 200;
	if (uri->nodes == NULL) {
		node = tree->children;
	} else {
		node = parse_uri(tree->children, uri, code);
	}

	return treeProcessor->processHTML(node, code, uri->last_slash);
}

/**
 * Sets value of param.
 * @param uri path to requested resource with "put" option containing new param
 * value
 * @return 0 on success, -1 on error
 */
int ParamTree::set(ParsedURI *uri)
{
	xmlNodePtr node;
	int ret = -1;
	int code;

	if (uri->nodes == NULL) {
		return ret;
	} else {
		/* find node */
		node = parse_uri(tree->children, uri, &code);
		if (!node) 
			return ret;
		if (strcmp((char *)node->name, "param"))
			return ret;

		return dynamicTree.set_map[node](SC::A2S(uri->getOption("put")));

	}

}


ParamTree::~ParamTree(void)
{
//	pthread_mutex_destroy(&m);
//	pthread_cond_destroy(&c_r);
//	pthread_cond_destroy(&c_u);
}

/**
 * Writes node information to buffer.
 * @param buf output buffer
 * @param node described node, must be of type "node" or "params" (not "param")
 * @param last_slash 1 if '/' was a last char of uri, else 0; needed for proper
 * links constructing
 */
void ParamTree::writeNode(xmlOutputBufferPtr buf, xmlNodePtr node,
	int last_slash)
{
	xmlNodePtr n;
	char *c;
	
#define xWS xmlOutputBufferWriteString
#define xGP (char *)xmlGetProp
#define xF xmlOutputBufferFlush
	
	xWS(buf, "<p><table>\n"); 

	if (strcmp((char *)node->name, "params")) {
		xWS(buf, "<tr><td></td><td></td>\n<td><a href=\"");
		if (!last_slash) {
	 		c = (char *)xmlGetProp(node, (xmlChar *)"name");
			utf2a((xmlChar *)c);
			xWS(buf, c);
			xmlFree(c);
			xWS(buf, "/.."); 
		} else {
			xWS(buf, ".."); 
		}
		xWS(buf, (char *)SC::S2U(SC::A2S(("\"> .. (poziom wy¿ej) </a></td></tr>\n"))).c_str());
	}

	for (n = node->children; n; n = n->next) {
		

		xWS(buf, "<tr><td>");
		if (!(strcmp((char *)n->name, "param"))) {
			c = xGP(n, (xmlChar *)"short_name");
			xWS(buf, c);
			xmlFree(c);
			xWS(buf, "</td>\n<td> ");
			c = xGP(n, (xmlChar *)"value");
			xWS(buf, c);
			if (strcmp(c, "unknown")) {
				xmlFree(c);
				c = xGP(n, (xmlChar *)"unit");
				if (strcmp(c, "-")) {
					xWS(buf, " ");
					xWS(buf, c);
				}
			}
			xmlFree(c);
//			xWS(buf, ")");
//			xF(buf);
		} else {
			xWS(buf, "</td>\n<td>");
		}

		xWS(buf, "</td><td><a href=\"");
		if (!last_slash) {
	 		c = (char *)xmlGetProp(node, (xmlChar *)"name");
			utf2a((xmlChar *)c);
			xWS(buf, c); 
			xmlFree(c);
			xWS(buf, "/");
			
		}
		c = xGP(n, (xmlChar *)"name");
		utf2a((xmlChar *)c);
		xWS(buf, c); 
		xmlFree(c);
		xWS(buf, "\">"); 
		
		c = xGP(n, (xmlChar *)"name");
		xWS(buf, c);
		xmlFree(c);
		xWS(buf, "</a></td></tr>\n");
	}
	xWS(buf, "</table></p>\n"); 
	xF(buf);
} 

/**
 * Write table with param information to buffer.
 * @param buf output buffer
 * @param node described node, must be of type "param"
 * @param last_slash 1 if '/' was a last char of uri, else 0; needed for proper
 * links constructing
 */
void ParamTree::writeParam(xmlOutputBufferPtr buf, xmlNodePtr node,
	int last_slash)
{
	char *c;

	xWS(buf, "<p><li><a href=\"");
	if (!last_slash) {
	 	c = (char *)xmlGetProp(node, (xmlChar *)"name");
		utf2a((xmlChar *)c);
		xWS(buf, c);
		xWS(buf, "/.."); 
		xmlFree(c);
	} else {
		xWS(buf, "..");
	}
	 xWS(buf, (char *)SC::S2U(SC::A2S("\"> .. (poziom wy¿ej) </a></li></p>")).c_str());
	
	xWS(buf, (char *)SC::S2U(SC::A2S(
			"<p><table><tr><td>Pe³na nazwa</td>\n<td>")).c_str()); 
	c = xGP(node, (xmlChar *)"full_name");
	xWS(buf, c);
	xmlFree(c);
	xWS(buf, (char *)SC::S2U(SC::A2S("</td></tr><tr><td>Nazwa skrócona</td><td>")).c_str()); 
	c = xGP(node, (xmlChar *)"short_name");
	xWS(buf, c);
	xmlFree(c);
	xWS(buf, "</td></tr><tr><td>Jednostka</td><td>"); 
	c = xGP(node, (xmlChar *)"unit");
	xWS(buf, c);
	xmlFree(c);
	xWS(buf, (char *)SC::S2U(SC::A2S("</td></tr><tr><td>Warto¶æ</td><td>")).c_str());
	c = xGP(node, (xmlChar *)"value");
	xWS(buf, c);
	xmlFree(c);
	xWS(buf, "</td></tr></table></p>");
	xmlOutputBufferFlush(buf);
#undef xWS
#undef xGP
}

void ParamTree::saveControlRaport(const char* path) {

	xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr root = xmlNewDocNode(doc, NULL, BAD_CAST "control_raport", NULL);
	xmlDocSetRootElement(doc, root);
	xmlNsPtr cns = xmlNewNs(root, BAD_CAST SZ_CONTROL_NS_URI, BAD_CAST "kon");

	xmlNodePtr control_raport_node = find_node(tree->children->children, L"control_raport");
	if (control_raport_node == NULL)
		return;

	for (xmlNodePtr node = control_raport_node->children; node; node = node->next) {
		xmlChar* name = xmlGetProp(node, BAD_CAST "full_name");
		if (name == NULL)
			continue;

		xmlNodePtr n = xmlNewNode(NULL, BAD_CAST "param");

		xmlSetProp(n, BAD_CAST "name", name);
		xmlFree(name);

		copy_control_related_attributes(node, n, cns);

		xmlAddChild(root, n);
	}

	xmlSaveFormatFile(path, doc, 1);
	xmlFreeDoc(doc);
}

void ParamTree::loadControlRaport(const char* path) {

	xmlDocPtr doc = xmlParseFile(path);
	if (doc == NULL)
		return;

	int code;
	setControlRaport(doc, &code);
}


namespace {

class NodesFilter : public AbstractNodesFilter {
	ParsedURI::NodeList *nlist;
public:
	NodesFilter(ParsedURI::NodeList *_nlist) : nlist(_nlist) {}
	virtual bool operator()(TParam *p) {
		if (nlist == NULL)
			return true;

		for (ParsedURI::NodeList *node = nlist; node; node = node->next)
			if (!cmpURI((unsigned char*)node->name, SC::S2U(p->GetName()).c_str()))
				return true;

		return false;
	}
};

}

void ContentHandler::handle_request_for_historical_data(HTTPRequest *req, HTTPResponse *res) {
	char *eptr;
	time_t time = strtoul(req->uri->getOption("time"), &eptr, 10);

	SZARP_PROBE_TYPE probe_type = PT_MIN10;
	int custom_length = 0;

	const char* period_type_string = req->uri->getOption("period");
	if (!strcmp(period_type_string, "month"))
		probe_type = PT_MONTH;
	else if (!strcmp(period_type_string, "week"))
		probe_type = PT_WEEK;
	else if (!strcmp(period_type_string, "day"))
		probe_type = PT_DAY;
	else if (!strcmp(period_type_string, "hour8"))
		probe_type = PT_HOUR8;
	else if (!strcmp(period_type_string, "hour"))
		probe_type = PT_HOUR;
	else if (!strcmp(period_type_string, "min10"))
		probe_type = PT_MIN10;
	else if (!strcmp(period_type_string, "custom")) {
		probe_type = PT_CUSTOM;
		custom_length = atoi(req->uri->getOption("custom_length"));
	}


	NodesFilter filter(req->uri->nodes);
	xmlDocPtr doc = il->GetXMLDoc(filter, time, probe_type);
	if (req->uri->nodes == 0) {
		res->content = tp->dumpDocXML(doc);
	} else {
		res->content = tp->processXML(doc->children, &(res->code), req->uri->attribute);
	}
	xmlFreeDoc(doc);
}

void ContentHandler::handle_request_for_svg_graph(HTTPRequest *req, HTTPResponse *res) {
	xmlDocPtr request_doc = xmlReadMemory(req->content, strlen(req->content), NULL, NULL, 0);
	if (request_doc == NULL) {
		res->code = 400;
		res->content = strdup("Invalid query - unparseable xml document");
		return;
	}

	SVGGenerator sgg(Szbase::GetObject(), request_doc);
	res->content = (char*)sgg.GenerateSVG(&res->code);
}

/**
 * Creates response.
 * @param req client's request
 * @param res created response, this object must exist, method fills in it's
 * attributes
 * @param put is setting param value allowed?
 */
void ContentHandler::create(HTTPRequest *req, HTTPResponse *res)
{
	time_t t;
	char *c;
	
	res->code = 200;
	time(&t);
	res->date = (char *) xmlStrdup(BAD_CAST ctime(&t));
	res->date[strlen(res->date)-1] = 0;
	t += pt->getUpdateFreq();
	res->exp_date = (char *) xmlStrdup(BAD_CAST ctime(&t));
	res->exp_date[strlen(res->exp_date)-1] = 0;
	
	c = req->uri->getOption("output");

	if (req->uri->getOption("time") && req->uri->getOption("period")) {
		handle_request_for_historical_data(req, res);
		if (c == NULL || !strcmp(c, "xml")) 
			res->content_type= (char *) xmlStrdup(BAD_CAST 
				"text/xml; charset=utf-8");
		else
			res->content_type = (char *) xmlStrdup(BAD_CAST 
				"text/plain; charset=utf-8");
	} else if (req->uri->getOption("svg")) {
		handle_request_for_svg_graph(req, res);
		if (c == NULL || !strcmp(c, "xml")) 
			res->content_type= (char *) xmlStrdup(BAD_CAST 
				"text/xml; charset=utf-8");
		else
			res->content_type = (char *) xmlStrdup(BAD_CAST 
				"text/plain; charset=utf-8");
	} else if (req->uri->nodes != NULL 
			&& req->uri->nodes->next == NULL
			&& (!strcmp("xreports", req->uri->nodes->name)
			|| (!strcmp("custom", req->uri->nodes->name) && req->uri->getOption("title") != NULL))) {

		if (req->uri->getOption("title") == NULL) {
			res->content_type = (char *) xmlStrdup(BAD_CAST "text/xml; charset=utf-8");
			res->code = 200;
			res->content = pt->processReports();
		} else {
			char * response = pt->processReport(req->uri->getOption("title"), &(res->code), req->uri->nodes->name[0] == 'c');
			if(res->code == 200)
				res->content_type = (char *) xmlStrdup(BAD_CAST "text/xml; charset=utf-8");
			else
				res->content_type = (char *) xmlStrdup(BAD_CAST "text/plain; charset=utf-8");
			res->content = response;
		}
	} else if (req->uri->nodes != NULL && req->uri->nodes->next != NULL &&
			req->uri->nodes->next->next == NULL && !strcmp("custom", req->uri->nodes->name) &&
			!strcmp("add", req->uri->nodes->next->name)) {

			res->content_type = (char *) xmlStrdup(BAD_CAST "text/text; charset=utf-8");
			res->content = (char *) xmlStrdup(SC::S2U(pt->processCustomRaportRequest(req->content, &res->code)).c_str());

	} else if (req->uri->nodes != NULL && !strcmp(req->uri->nodes->name, "control_raport")) {
		res->content_type = (char *) xmlStrdup(BAD_CAST "text/xml; charset=utf-8");
		if (req->uri->getOption("set")) {
			res->content = (char*) xmlStrdup(SC::S2U(pt->processControlRaport(req->content, &res->code)).c_str());	
			if (crp)
				pt->saveControlRaport(crp);
		} else
			res->content = (char*) pt->processControlRaportRequest(&res->code);	
	} else if (req->uri->nodes != NULL && req->uri->nodes->next == NULL && !strcmp("params.xml", req->uri->nodes->name)) {
		res->content_type = (char *) xmlStrdup(BAD_CAST "text/xml; charset=utf-8");
		res->code = 200;

		std::ostringstream ipks;
		std::ifstream ipkf(cf);
		ipks << ipkf.rdbuf();
		res->content = strdup(ipks.str().c_str());
	} else {
		if (req->uri->attribute) {
			if ((c != NULL) && strcmp(c, "xml")) 
				res->content_type = 
					(char *) xmlStrdup(BAD_CAST "text/plain; charset=utf-8");
			else
				res->content_type = 
					(char *) xmlStrdup(BAD_CAST "text/xml; charset=utf-8");
			res->content = pt->processXML(req->uri, &(res->code));
		} else {
			if ((c == NULL) || (!strcmp(c, "html"))) {
				res->content_type= (char *) xmlStrdup(BAD_CAST 
						"text/html; charset=utf-8");
				res->content = pt->processHTML(req->uri, &(res->code));
			} else {
				if (!strcmp(c, "xml")) 
					res->content_type= (char *) xmlStrdup(BAD_CAST 
						"text/xml; charset=utf-8");
				else
					res->content_type = (char *) xmlStrdup(BAD_CAST 
						"text/plain; charset=utf-8");
				res->content = pt->processXML(req->uri, &(res->code));
			}
		}
	}
	res->content_length = strlen(res->content);
}

/**
 * Set value of param to value given by "put" option.
 * @param req HTTP request
 * @return 0 on success, -1 on error
 */
int ContentHandler::set(HTTPRequest *req)
{
	return pt->set(req->uri);
}

void ContentHandler::configure(ConfigLoader *cloader, const char* sect)
{
	if (!strcmp(cloader->getStringSect(sect, "control_raport_path", "no"), "no"))
		return;

	crp = strdup(cloader->getStringSect(sect, "control_raport_path", "no"));
	pt->loadControlRaport(crp);

}

ContentHandler::~ContentHandler() {
	free(crp);
}

xmlNodePtr find_node(xmlNodePtr first, const std::wstring name) {
	xmlChar *prop;

	std::basic_string<unsigned char> uname = SC::S2U(name);
	for (; first; first = first->next) {
		prop = xmlGetProp(first, (xmlChar *)"name");
		int i = cmpURI(prop, uname.c_str());
		xmlFree(prop);
		if (i == 0)
			break;
	}
	return first;
}

xmlNodePtr parse_uri(xmlNodePtr node, ParsedURI *uri, int *code) {
	ParsedURI::NodeList *n;

	*code = 404;
	if (node == NULL)
		return NULL;
	for (n = uri->nodes; n; n = n->next) {
		node = find_node(node->children, SC::U2S((unsigned char*)n->name));
		if (node == NULL) 
			break;
	}
	if (node == NULL) {
		return NULL;
	}
	*code = 200;
	return node;
}

