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
 * param_tree.h - XML params tree
 *
 * $Id$
 */

#ifndef __TREE_PROCESSOR_H__
#define __TREE_PROCESSOR_H__

#include <libxml/parser.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "config_load.h"
#include "ptt2xml.h"
#include "server.h"
#include "uri.h"

#include "param_tree_dynamic_data.h"

/**
 * Representation of params tree. Class also includes methods for printing
 * tree nodes in XML or HTML.
 */
class TreeProcessor {
public:
	
	/**
	 * @param _tree parsed params XML tree
	 * @param cl configuration informations
	 */
	TreeProcessor(const ConfigLoader *cl) :
		html_header(strdup(cl->getString("html_header",
			"<html><head>\
	                <title>Paramd output</title></head><body>"))),
		html_footer(strdup(cl->getString("html_footer",
				"<body></html>")))
	{ };
	~TreeProcessor();
	char *dumpDocXML(xmlDocPtr doc);
	char *processHTML(xmlNodePtr node, int *code, int last_slash);
	char *processXML(xmlNodePtr node, int *code, const char *attribute);
protected:
	void writeNode(xmlOutputBufferPtr buf, xmlNodePtr node, int last_slash);
	void writeParam(xmlOutputBufferPtr buf, xmlNodePtr node, int last_slash);
	char *html_header;	/**< html output header */
	char *html_footer;	/**< html output footer */
};

#endif

