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
 * Praterm 2005
 *
 * ipk2xml.h - IPK to XML conversion
 *
 * $Id$
 */
#ifndef __IPK2XML_H__
#define __IPK2XML_H__

#include <functional>

#include "szarp_config.h"

#include "param_tree_dynamic_data.h"

class PTTParamProxy;
class Szbase;

class AbstractNodesFilter {
public:
	virtual bool operator()(TParam* p) = 0;
};
/**
 * Class for reading params from params.xml 
 */
class IPKLoader {
public:
	/** @param _config_path path to params.xml file */
	IPKLoader(char *_config_file, PTTParamProxy *pttParamProxy, Szbase *szbase);

	virtual ~IPKLoader() { };
	/**
	 * Reads param table from IPK file and returns it as an XML tree.
	 * @return XML tree with params, NULL on error
	 */
	ParamDynamicTreeData get_dynamic_tree();

	xmlDocPtr GetXMLDoc(AbstractNodesFilter& nf, const time_t& time, const SZARP_PROBE_TYPE& pt);

	TSzarpConfig* GetSzarpConfig();
protected:

	char *config_file; /**<path to params.xml file */

	TSzarpConfig *szarpConfig;

	PTTParamProxy *m_pttParamProxy;

	Szbase *m_szbase;

	xmlDocPtr start_document();

	void end_document(xmlDocPtr doc, int all, int inbase);

	xmlDocPtr ipk2xml(); /**<transforms IPK configuration into XML document
			       conforming to http://www.praterm.com.pl/ISL/params.
			       DRAWDEFINABLE elements are omitted*/
	/**
	 * Insert new param to tree.
	 * @param node tree root
	 * @param param inserted param
	 * @param buildNodes create hierarchy nodes
	 * @param order additional attribute order in raport
	 * @param descr additional attribute description
 	*/
	xmlNodePtr insert_param(xmlNodePtr node, TParam *param, const xmlChar *value, bool buildNodes = true, double * order = NULL, const std::wstring descr = std::wstring());
	/**
	 * Insert new raport to tree.
	 * @param config IPK configuration
	 * @param node tree root
	 * @param raportTitle new raport title
 	*/
	void insert_raport(xmlNodePtr node, const std::wstring raportTitle, ParamDynamicTreeData &dynamic_tree);
	/**
	 * Creates and returns new node. If node with given name already exist, no new
	 * node is created, existed is returned.
	 * @param parent parent node
	 * @param name node name
	 * @param existing or newly created node
	 * @return created or already existing node, NULL on error
	 */
	xmlNodePtr create_node(xmlNodePtr parent, const std::wstring name);

	void insert_into_map(xmlNodePtr node, TParam *p, ParamDynamicTreeData &dynamic_tree);
};

#endif
