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
 * ptt2xml.c - IPK to XML conversion.
 *
 * $Id$
 */

#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libpar.h>
#include <liblog.h>

#include "ipk2xml.h"
#include "utf8.h"
#include "tokens.h"
#include "conversion.h"

#include "szarp_config.h"

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "param_tree_dynamic_data.h"

/**
 * ISL namespace string.
 */
#define	ISL_NAMESPACE_STRING	"http://www.praterm.com.pl/ISL/params"

/** character converting macro */
#define X 	(xmlChar *)

IPKLoader::IPKLoader(char *_config_file, PTTParamProxy *pttParamProxy, Szbase *szbase) : config_file(_config_file), m_pttParamProxy(pttParamProxy), m_szbase(szbase) { 
	szarpConfig = IPKContainer::GetObject()->GetConfig(SC::L2S(_config_file));
}

xmlDocPtr IPKLoader::start_document() {
	if (szarpConfig == NULL)
		return NULL;

	xmlDocPtr doc = xmlNewDoc(X"1.0"); 
	doc->children = xmlNewDocNode(doc, NULL, X"params", NULL);
	xmlNewNs(doc->children, X(ISL_NAMESPACE_STRING), NULL);

	return doc;

}

void IPKLoader::end_document(xmlDocPtr doc, int all, int inbase) {
	assert(doc);

	char buffer[20];
	snprintf(buffer, 19, "%d", all);
	xmlSetProp(doc->children, X"all", X(buffer));
	snprintf(buffer, 19, "%d", inbase);
	xmlSetProp(doc->children, X"inbase", X(buffer));
}

ParamDynamicTreeData IPKLoader::get_dynamic_tree()
{

	ParamDynamicTreeData ret;
	ret.tree = NULL;

	ret.szarpConfig = szarpConfig;

	if (szarpConfig == NULL)
		return ret;

	xmlDocPtr doc = start_document();

	int all = 0, inbase = 0;
	
	for (TParam* p = szarpConfig->GetFirstParam(); p; p = szarpConfig->GetNextParam(p)) {
		if (p->GetFormulaType() == TParam::DEFINABLE) {
			try {
				p->PrepareDefinable();
			} catch( TCheckException& e ) {
				sz_log(0,"Invalid definable formula %s",SC::S2A(p->GetName()).c_str());
			}
		}
		if (p->IsInBase())
			inbase++;
		all++;

		xmlNodePtr n = insert_param(doc->children, p, X"unknown");

		insert_into_map(n, p, ret);

	}

	for (std::wstring rapt = szarpConfig->GetFirstRaportTitle(); !rapt.empty(); rapt = szarpConfig->GetNextRaportTitle(rapt)) {
		insert_raport(doc->children, rapt, ret);
	}

	end_document(doc, all, inbase);

	ret.tree = doc;
	ret.updateTriggers.push_back(PTTProxyUpdater(m_pttParamProxy));

	return ret;
}

void IPKLoader::insert_raport(xmlNodePtr node, const std::wstring raportTitle, ParamDynamicTreeData &dynamic_tree)
{
	xmlNodePtr n;
	n = node;

	n = create_node(n, L"Reports");

	xmlSetProp(n, X"configTitle", SC::S2U(szarpConfig->GetTitle()).c_str());

	n = create_node(n, raportTitle);

	for (TRaport* rap = szarpConfig->GetFirstRaportItem(raportTitle); rap; rap = szarpConfig->GetNextRaportItem(rap)) {
		double order = rap->GetOrder();
		xmlNodePtr pn = insert_param(n, rap->GetParam(), X"unknown", false, &order, rap->GetDescr());
		insert_into_map(pn, rap->GetParam(), dynamic_tree);
	}

}

void IPKLoader::insert_into_map(xmlNodePtr node, TParam *p, ParamDynamicTreeData &dynamic_tree) {
	switch (p->GetType()) {

		case TParam::P_REAL:
			dynamic_tree.get_map[node] = PTTParamValueGetter(m_pttParamProxy, p->GetIpcInd(), p->GetPrec());
			dynamic_tree.set_map[node] = PTTParamValueSetter(m_pttParamProxy, p->GetIpcInd(), p->GetPrec());
			break;

		case TParam::DEFINABLE:
		case TParam::P_LUA:
			dynamic_tree.get_map[node] = SzbaseParamValueGetter(m_szbase, p);
			dynamic_tree.set_map[node] = DummyHandler();
			break;

		case TParam::P_COMBINED:
			dynamic_tree.get_map[node] = PTTParamCombinedValueGetter(m_pttParamProxy, p->GetFormulaCache()[1]->GetIpcInd(), p->GetFormulaCache()[0]->GetIpcInd(), p->GetPrec());
			dynamic_tree.set_map[node] = DummyHandler();
			break;

		default:
			assert(false);
	}

}


xmlNodePtr IPKLoader::insert_param(xmlNodePtr node, TParam *p, const xmlChar* value, bool buildTree, double * order, const std::wstring descr)
{
	std::wstring buffer;
	unsigned int i;
	xmlNodePtr n = node;

	std::vector<std::wstring> tokens(3);
	boost::split( tokens, p->GetName(), boost::is_any_of(L":") );

	if (tokens.size() == 0)
		return n;
	if (buildTree) {
		for (i = 0; i < tokens.size() - 1; i++) {
			n = create_node(n, tokens[i]);
		}
	}
	n = xmlNewChild(n, NULL, X"param", NULL); 
	xmlSetProp(n, X"name", SC::S2U(tokens.back()).c_str());
	xmlSetProp(n, X"full_name", SC::S2U(p->GetName()).c_str());
	if(!p->GetShortName().empty())
		xmlSetProp(n, X"short_name", SC::S2U(p->GetShortName()).c_str());
	else
		xmlSetProp(n, X"short_name", X"");
	if(!p->GetUnit().empty())
		xmlSetProp(n, X"unit", SC::S2U(p->GetUnit()).c_str());
	else
		xmlSetProp(n, X"unit", X"");
	buffer = boost::lexical_cast<std::wstring>(p->GetIpcInd());
	xmlSetProp(n, X"ipc_ind", SC::S2U(buffer).c_str());
	buffer = boost::lexical_cast<std::wstring>(p->GetPrec());
	xmlSetProp(n, X"prec", SC::S2U(buffer).c_str());
	xmlSetProp(n, X"value", value);
	if (p->GetType() == TParam::P_COMBINED) {
		xmlNodePtr tmp = xmlNewChild(n, NULL, X"msw", NULL);
		buffer = boost::lexical_cast<std::wstring>(p->GetFormulaCache()[0]->GetIpcInd());
		xmlSetProp(tmp, X"ipc_ind", SC::S2U(buffer).c_str());
		tmp = xmlNewChild(n, NULL, X"lsw", NULL);
		buffer = boost::lexical_cast<std::wstring>(p->GetFormulaCache()[1]->GetIpcInd());
		xmlSetProp(tmp, X"ipc_ind", SC::S2U(buffer).c_str());
	}
	if (order) {
		buffer = boost::lexical_cast<std::wstring>(*order);
		xmlSetProp(n, X"order", SC::S2U(buffer).c_str());
	}
	if (!descr.empty()) {
		xmlSetProp(n, X"description", SC::S2U(descr).c_str());
	}

	return n;

}

xmlNodePtr IPKLoader::create_node(xmlNodePtr parent, const std::wstring name)
{
	xmlNodePtr n;
	xmlChar *str = NULL;
	
	for (n = parent->children; n; n = n->next) {
		str = xmlGetProp(n, X"name");
		if (!str) {
			sz_log(0, "TREE not properly built");
			return NULL;
		};
		if (!strcmp((char *)str, (char *)(SC::S2U(name).c_str()))) {
			xmlFree(str);
			break;
		}
		xmlFree(str);
	}
	if (!n) {
		n = xmlNewChild(parent, NULL, X"node", NULL);
		xmlSetProp(n, X"name", SC::S2U(name).c_str());
	}
	return n;
}

xmlDocPtr IPKLoader::GetXMLDoc(AbstractNodesFilter &nf, const time_t& time, const SZARP_PROBE_TYPE& pt) {
	if (szarpConfig == NULL)
		return NULL;

	xmlDocPtr doc = start_document();

	int pcount = 0;
	
	for (TParam* p = szarpConfig->GetFirstParam(); p; p = szarpConfig->GetNextParam(p)) {
		if (!p->IsInBase())
			continue;
		if (!nf(p))
			continue;
		if (p->GetFormulaType() == TParam::DEFINABLE) {
			try {
				p->PrepareDefinable();
			} catch( TCheckException& e ) {
				sz_log(0,"Invalid definable formula %s",SC::S2A(p->GetName()).c_str());
			}
		}
		pcount++;

		bool fixed;
		bool ok = true;
		std::wstring err;
		double v = m_szbase->GetValue(p->GetGlobalName(), time, pt, 0, &fixed, ok, err);
		if (!ok || std::isnan(v))
			insert_param(doc->children, p, X"unknown");
		else
			insert_param(doc->children, p, SC::S2U(boost::lexical_cast<std::wstring>(v)).c_str());

	}

	end_document(doc, pcount, pcount);

	return doc;

}

TSzarpConfig* IPKLoader::GetSzarpConfig() {
	return szarpConfig;
}

#undef X
#undef U
