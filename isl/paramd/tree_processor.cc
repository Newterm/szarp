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
 * Pawel Palucha 2002
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
#include <boost/lexical_cast.hpp>

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

char *TreeProcessor::dumpDocXML(xmlDocPtr doc) {
	char *xmlbuffer;
	int size;
	xmlDocDumpFormatMemory(doc, (xmlChar **)(&xmlbuffer), &size, 1);
	return xmlbuffer;
}

/**
 * Returns information from tree in XML format.
 * @param uri path to requested resource
 * @param code HTTP return code, can be set to 203 (Not Found) for example,
 * 200 means OK
 * @return dynamically allocated buffer with response content
 */
char *TreeProcessor::processXML(xmlNodePtr node, int *code, const char* attribute)
{
	char *xmlbuffer;

	*code = 200;
	if (!node) {
		return strdup("");
	}

	/* check if it's request for attrribute */
	xmlOutputBufferPtr out_buf = xmlAllocOutputBuffer(NULL);
	if (out_buf == NULL) {
		sz_log(0, "cannot allocate xml output buffer");
		*code = 500;
		return NULL;
	}
		
	if (attribute) {
		/* Request for attribute */
		if(!strcmp(attribute, "v_u")) {
			xmlbuffer = NULL;
			char *value = (char *)xmlGetProp(node, (const xmlChar *)
				"value");
			if (value != NULL) {
				char *unit = (char *)xmlGetProp(node, (const xmlChar *) "unit");
				if (unit != NULL) {
					xmlbuffer = (char*)malloc(sizeof(char) * (strlen(value) + strlen(unit) + 2));
					xmlbuffer[0] = '\0';
					strcat(xmlbuffer, value);
					strcat(xmlbuffer, " ");
					strcat(xmlbuffer, unit);
					xmlFree(unit);
				}
					
				xmlFree(value);
			}
		} else
			xmlbuffer = (char *)xmlGetProp(node, (xmlChar *) attribute);

		if (xmlbuffer == 0) {
			xmlbuffer = (char*) xmlStrdup(BAD_CAST "404 Attribute Not Found");
			*code = 404;
		}
		xmlOutputBufferWriteString(out_buf, 
				"<?xml version=\"1.0\"?>\n");
		xmlOutputBufferWriteString(out_buf, 
				"<params\n\
\txmlns=\"http://www.praterm.com.pl/ISL/params\">\
<attribute name=\"");
		xmlOutputBufferWriteString(out_buf, attribute);
		xmlOutputBufferWriteString(out_buf, "\">");
		xmlOutputBufferWriteString(out_buf, xmlbuffer);
		xmlOutputBufferWriteString(out_buf, 
				"</attribute></params>\n"
				);
		xmlFree(xmlbuffer);
	} else {
		xmlOutputBufferWriteString(out_buf, 
				"<?xml version=\"1.0\"?>\n");
		xmlOutputBufferWriteString(out_buf, 
				"<params\n\
\txmlns=\"http://www.praterm.com.pl/ISL/params\">\n");
		xmlNodeDumpOutput(out_buf, node->doc, node, 0, 1, NULL);
		xmlOutputBufferWriteString(out_buf, 
				"</params>\n");
	}
		
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

/**
 * Returns information from tree in HTML format.
 * @param uri path to requested resource
 * @param code HTTP return code, can be set to 203 (Not Found) for example,
 * 200 means OK
 * @return dynamically allocated buffer with response content
 */
char *TreeProcessor::processHTML(xmlNodePtr node, int *code, int last_slash)
{
	char *xmlbuffer;
	char *c;
	time_t t;

	xmlOutputBufferPtr out_buf = xmlAllocOutputBuffer(NULL);
	if (out_buf == NULL) {
		sz_log(0, "cannot allocate xml output buffer");
		*code = 500;
		return NULL;
	}

	*code = 200;
	xmlOutputBufferWriteString(out_buf, html_header);

	if (node == NULL)
		xmlOutputBufferWriteString(out_buf, (char *)
				SC::S2U(SC::A2S("<p><b>Wezel nie znaleziony!</b></p>")).c_str());
	else {
		if (strcmp((char *)node->name, "params")) {
			xmlOutputBufferWriteString(out_buf, (char *)
				SC::S2U(SC::A2S("<p>Wezel <b>")).c_str());
			c = (char *)xmlGetProp(node, (xmlChar *)"name");
			xmlOutputBufferWriteString(out_buf, c);
			xmlFree(c);
		}
		xmlOutputBufferWriteString(out_buf, "</b></p><p>Data: ");
		t = time(NULL);
		xmlOutputBufferWriteString(out_buf, ctime(&t));
		xmlOutputBufferWriteString(out_buf, "</p>");
		
		if (!strcmp((char *)node->name, "node") ||
				(!strcmp((char *)node->name, "params"))) {
			writeNode(out_buf, node, last_slash);
		} else {
			writeParam(out_buf, node, last_slash);
		}
				
	}
	xmlOutputBufferWriteString(out_buf, html_footer);

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


TreeProcessor::~TreeProcessor(void)
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
void TreeProcessor::writeNode(xmlOutputBufferPtr buf, xmlNodePtr node,
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
		xWS(buf, (char *)SC::S2U(SC::A2S(("\"> .. (poziom wyzej) </a></td></tr>\n"))).c_str());
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
void TreeProcessor::writeParam(xmlOutputBufferPtr buf, xmlNodePtr node,
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
	 xWS(buf, (char *)SC::S2U(SC::A2S("\"> .. (poziom wyzej) </a></li></p>")).c_str());
	
	xWS(buf, (char *)SC::S2U(SC::A2S(
			"<p><table><tr><td>Pelna nazwa</td>\n<td>")).c_str());
	c = xGP(node, (xmlChar *)"full_name");
	xWS(buf, c);
	xmlFree(c);
	xWS(buf, (char *)SC::S2U(SC::A2S("</td></tr><tr><td>Nazwa skrocona</td><td>")).c_str());
	c = xGP(node, (xmlChar *)"short_name");
	xWS(buf, c);
	xmlFree(c);
	xWS(buf, "</td></tr><tr><td>Jednostka</td><td>"); 
	c = xGP(node, (xmlChar *)"unit");
	xWS(buf, c);
	xmlFree(c);
	xWS(buf, (char *)SC::S2U(SC::A2S("</td></tr><tr><td>Wartosc</td><td>")).c_str());
	c = xGP(node, (xmlChar *)"value");
	xWS(buf, c);
	xmlFree(c);
	xWS(buf, "</td></tr></table></p>");
	xmlOutputBufferFlush(buf);
#undef xWS
#undef xGP
}


