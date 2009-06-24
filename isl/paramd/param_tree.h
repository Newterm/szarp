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

#ifndef __PARAM_TREE_H__
#define __PARAM_TREE_H__

#include <libxml/parser.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "config_load.h"
#include "ptt2xml.h"
#include "ipk2xml.h"
#include "server.h"
#include "uri.h"
#include "tree_processor.h"

#include "param_tree_dynamic_data.h"

/**
 * Find a node with given name among given node's brothers. String comparison
 * is made with encoding conversion - xml tree nodes are expected to be UTF-8
 * coded, while name is expected to be UTF-8 or pure ASCII coded. When
 * it's pure ASCII, 'o' is equal to 'ó' and so on. Also, '_' is equal to space.
 * @param first first node to check, then it's brothers are checked
 * @param name name of node
 * @return node found or NULL
 */
xmlNodePtr find_node(xmlNodePtr node, std::wstring name);

/**
 * Parses URI from client's request, returns corresponding XML tree node.
 * @param uri request URI
 * @param code pointer to HTTP return code, when node is node found, code is 
 * set to 404 (Not Found)
 * @return node from tree corresponding to given uri, NULL when not found
 */
xmlNodePtr parse_uri(xmlNodePtr node, ParsedURI *uri, int *code);



/**
 * Representation of params tree. Class also includes methods for printing
 * tree nodes in XML or HTML.
 */
class ParamTree {
	public:
		
		/**
		 * @param _tree parsed params XML tree
		 * @param cl configuration informations
		 */
		ParamTree(ParamDynamicTreeData & data, const ConfigLoader *cl, TreeProcessor *tp) :
			dynamicTree(data),
			tree(data.tree),
			last_update(0),
			update_freq(cl->getInt("update_freq", 10)),
			customRaportCounter(0),
			treeProcessor(tp)
		{ };
		void saveControlRaport(const char *path);
		void loadControlRaport(const char *path);
		~ParamTree();
		char *processHTML(ParsedURI *uri, int *code);
		char *processXML(ParsedURI *uri, int *code);
		char *processReports();
		char *processReport(char * raport, int *code, bool custom = false);
		char *processReportParams(xmlNodePtr raport, bool listing = false, bool control = false);
		char *processControlRaportRequest(int *code);
		std::wstring processCustomRaportRequest(char *request, int *code);
		std::wstring processControlRaport(char *request, int *code);
		std::wstring setControlRaport(xmlDocPtr doc, int *code);
		int set(ParsedURI *uri);
		/**
		 * @return update_freq attribute
		 */
		int getUpdateFreq(void) {
			return update_freq;
		}
	protected:

		void check_update(void);
		void update(void);
		void update_node(xmlNodePtr node);
		void writeNode(xmlOutputBufferPtr buf, xmlNodePtr node,
				int last_slash);
		void writeParam(xmlOutputBufferPtr buf, xmlNodePtr node,
				int last_slash);
		void copyParamToNode(xmlNodePtr src, xmlNodePtr dst, int order);

		ParamDynamicTreeData& dynamicTree;
		xmlDocPtr tree;
		int last_update;	/**< time of last tree update 
					  (in seconds since EPOCH) */
		int update_freq;	/**< update frequency in seconds */
		char *html_header;	/**< html output header */
		char *html_footer;	/**< html output footer */
		int customRaportCounter;
		TreeProcessor* treeProcessor;
		static void updateNodeLastUsedTime(xmlNodePtr node);
		static bool testNodeLastUsedTime(xmlNodePtr node);
};

/**
 * Response creator.
 */
class ContentHandler : public AbstractContentHandler {
	public :
		/**
		 * @param _pt params tree pointer
		 */
		ContentHandler(ParamTree *_pt, TreeProcessor *_tp, IPKLoader *_il, const char* _cf) :
			pt(_pt),
			tp(_tp),
			il(_il),
			cf(_cf),
			crp(NULL)
		{ };
		virtual void create(HTTPRequest *req, HTTPResponse *res); 
		virtual int set(HTTPRequest *req);
		virtual void configure(ConfigLoader *conf, const char* sect);
		virtual ~ContentHandler();
	private :
		void handle_request_for_historical_data(HTTPRequest *req, HTTPResponse *response);
		void handle_request_for_svg_graph(HTTPRequest *req, HTTPResponse *response);
		ParamTree *pt;	/**< params tree pointer */
		TreeProcessor *tp;
		IPKLoader *il;
		const char *cf;
		char *crp;
};

#endif

