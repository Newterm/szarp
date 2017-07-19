/* 
 * SZARP: SCADA software 
 *
 * Copyright (C) 
 * 2011 - Jakub Kotur <qba@newterm.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
/** 
 * @file basedmn.h
 * @brief Base class for line daemons.
 * 
 * Contains basic functions that may be useful.
 *
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-16
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/lexical_cast.hpp>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <snap7.h>

#include <map>
#include <set>

#include "conversion.h"
#include "liblog.h"
#include "xmlutils.h"
#include "custom_assert.h"
#include "base_daemon.h"

class s7map_key
{
public:
	enum Type { SHORT, INTEGER, FLOAT };

	s7map_key(s7map_key::Type vt, int addr)
		: vtype(vt), address(addr) {} ;

	int get_address() const { return address; };
	int end_address() const
	{
		switch (vtype) {
			case SHORT:
				return address + 1;
			case INTEGER:
			case FLOAT:
				return address + 3;
		};
		return -1;
	}

	bool operator<(const s7map_key& other) const
	{
		return (end_address() < other.address);
	}

	bool operator>(const s7map_key& other) const
	{
		return ( address > other.end_address() );
	}

	bool operator==( const s7map_key& other ) const
	{
		return (address == other.address && vtype == other.vtype);
	}


	bool operator!=( const s7map_key& other) const
	{
		return vtype != other.vtype || address != other.address;
	}

	enum Type vtype;
	int address;
};

class s7map_val
{
public:
	s7map_val() : lsw_ind(-1), msw_ind(-1), lsw_param(NULL), msw_param(NULL) {};
	

	void set_msw_ind(int ind) { msw_ind = ind; };
	void set_lsw_ind(int ind) { lsw_ind = ind; };

	int lsw_ind;
	int msw_ind;

	TParam* lsw_param;
	TParam* msw_param;
};


class Snap7Daemon : public BaseDaemon
{
public:
	typedef std::map<s7map_key, s7map_val> s7_db_map;
	typedef std::pair<s7map_key, s7map_val> s7_db_pair;

	Snap7Daemon() : BaseDaemon("snap7dmn") {
		s7client = Cli_Create();
	};
	virtual ~Snap7Daemon() {};

	/** 
	 * @brief Reads data from device
	 */
	virtual int Read();
protected:
	virtual int ParseConfig(DaemonConfig * cfg);

	int ConfigureParam(int param_ind, xmlNodePtr node, TSzarpConfig* ipk, TParam* p);

	int ReadDB(int db, s7_db_map& ops);
	int DBVal(const s7map_key& vkey, s7map_val& vval, int start, char* data);

	S7Object s7client;
	std::string address;
	int rack;
	int slot;

	std::map<int, s7_db_map> m_params_map;

	char buffer[512];
};

int Snap7Daemon::ParseConfig(DaemonConfig * cfg)
{
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(m_cfg->GetXMLDoc());
	xmlNodePtr node = m_cfg->GetXMLDevice();
	xp_ctx->node = node;

	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	if (-1 == ret) {
		sz_log(0, "Cannot register namespace %s", SC::S2U(IPK_NAMESPACE_STRING).c_str());
		return 1;
	}

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (-1 == ret) {
		sz_log(0, "Cannot register namespace %s", IPKEXTRA_NAMESPACE_STRING);
		return 1;
	}

	xmlChar* prop = xmlGetNsProp(node, BAD_CAST("address"), BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (prop == NULL) {
		sz_log(0, "No attribute address given in line %ld", xmlGetLineNo(node));
		return 1;
	}
	address = (char*)prop;
	xmlFree(prop);	

	rack = 0;
	prop = xmlGetNsProp(node, BAD_CAST("rack"), BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (prop == NULL) {
		sz_log(2, "No attribute rack given, assuming default %d (line %ld)", rack, xmlGetLineNo(node));
	}
	else {
		rack = boost::lexical_cast<int>(prop);
		xmlFree(prop);	
	}

	slot = 2;
	prop = xmlGetNsProp(node, BAD_CAST("slot"), BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (prop == NULL) {
		sz_log(2, "No attribute slot given, assuming default %d (line %ld)", slot, xmlGetLineNo(node));
	}
	else {
		slot = boost::lexical_cast<int>(prop);
		xmlFree(prop);	
	}

	sz_log(9, "Configured with address: %s, rack: %d, slot: %d", address.c_str(), rack, slot);

	TUnit* u = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit();
	TParam *p = u->GetFirstParam();
	for (int i = 1; i <= u->GetParamsCount(); i++) {
		char *expr;
		ret = asprintf(&expr, ".//ipk:param[position()=%d]", i);
		if (-1 == ret) {
			sz_log(0, "Cannot allocate XPath expression for param number %d", i);
			return 1;
		}
		xmlNodePtr pnode = uxmlXPathGetNode(BAD_CAST expr, xp_ctx, false);
		ASSERT(pnode);
		free(expr);

		if (ConfigureParam(i, pnode, u->GetSzarpConfig(), p))
			return 1;

		p = p->GetNext();
	}

	return 0;
}

int Snap7Daemon::ConfigureParam(int ind, xmlNodePtr node, TSzarpConfig* ipk, TParam* p)
{
	xmlChar* prop = xmlGetNsProp(node, BAD_CAST("db"), BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (NULL == prop) {
		sz_log(0, "No attribute db given for param: %s in line %ld",
				SC::S2U(p->GetName()).c_str(),
			       	xmlGetLineNo(node));
		return 1;
	}
	int db = boost::lexical_cast<int>((char*)prop);
	xmlFree(prop);

	s7_db_map& vmap = m_params_map[db];

	prop = xmlGetNsProp(node, BAD_CAST("address"), BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (NULL == prop) {
		sz_log(0, "No attribute address given for param: %s in line %ld",
				SC::S2U(p->GetName()).c_str(),
			       	xmlGetLineNo(node));
		return 1;
	}
	int address = boost::lexical_cast<int>((char*)prop);
	xmlFree(prop);	

	prop = xmlGetNsProp(node, BAD_CAST("val_type"), BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (NULL == prop) {
		sz_log(0, "No attribute val_type given for param: %s in line %ld",
				SC::S2U(p->GetName()).c_str(),
			       	xmlGetLineNo(node));
		return 1;
	}
	std::string val_type = (char*)prop;
	xmlFree(prop);

	std::string val_op;
	if (val_type != "short") {
		prop = xmlGetNsProp(node, BAD_CAST("val_op"), BAD_CAST IPKEXTRA_NAMESPACE_STRING);
		if (NULL == prop) {
			sz_log(0, "No attribute val_op given for param: %s in line %ld",
					SC::S2U(p->GetName()).c_str(),
					xmlGetLineNo(node));
			return 1;
		}
		val_op = (char*)prop;
		xmlFree(prop);
	}

	sz_log(9, "ConfigureParam param %s db: %d, address: %d, val_type: %s",
		       	SC::S2U(p->GetName()).c_str(),
			db,
			address,
			val_type.c_str());

	if (val_type == "short") {
		s7map_key nkey(s7map_key::SHORT, address);
		auto it = vmap.find( nkey ) ;
		if (it == vmap.end()) {
			s7map_val& mval = vmap[nkey];
			mval.set_lsw_ind(ind - 1);
			mval.lsw_param = p;
		}
		else {
			sz_log(0, "Overlaping mapping for db: %d, addr: %d wth: addr: %d type: %d",
				db, address, it->first.address, it->first.vtype);
			return 1;
		}
	}
	else {
		s7map_key::Type vt;
	       	if (val_type == "integer") {
			vt = s7map_key::INTEGER;
		}
		else if (val_type == "float") {
			vt = s7map_key::FLOAT;
		}
		else {
			sz_log(1, "Unrecognized val_type");
			return 1;
		}

		s7map_key nkey(vt, address);
		auto iret = vmap.find( nkey );
		if (iret != vmap.end() && iret->first != nkey) {
			sz_log(0, "ERROR: overlaping mappings in configuration");
			return 1;
		}

		s7map_val& mval = vmap[nkey];

		if (val_op == "LSW") {
			mval.set_lsw_ind(ind - 1);
			mval.lsw_param = p;
		}
		else if (val_op == "MSW" ) {
			mval.set_msw_ind(ind - 1);
			mval.msw_param = p;
		}
		else {
			sz_log(0, "Wrong argument for attributte val_op");
			return 1;
		}
	}

	return 0;
}

int Snap7Daemon::DBVal(const s7map_key& vkey, s7map_val& vval, int start, char* data)
{
	short* psv;
	int* piv;
	float* pfv;
	float fv;
	switch(vkey.vtype) {
		case s7map_key::SHORT:
			psv = reinterpret_cast<short*>(&data[vkey.address - start]);
			if (vval.lsw_ind != -1)
				Set(vval.lsw_ind, *psv);
			return 0;
		case s7map_key::INTEGER:
			piv = reinterpret_cast<int*>(&data[vkey.address - start]);
			if (vval.lsw_ind != -1) {
				Set(vval.lsw_ind, *piv & 0xFFFF);
			}
			if (vval.msw_ind != -1) {
				Set(vval.lsw_ind, ((*piv) >> 16) & 0xFFFF);
			}
			return 0;
		case s7map_key::FLOAT:
			pfv = reinterpret_cast<float*>(&data[vkey.address - start]);
			if (vval.lsw_param)
				fv = (*pfv) * pow10(vval.lsw_param->GetPrec());
			else {
				sz_log(0, "ERROR: no param configured for value");
				return 1;
			}
			piv = reinterpret_cast<int*>(&fv);
			if (vval.lsw_ind != -1) {
				Set(vval.lsw_ind, *piv & 0xFFFF);
			}
			if (vval.msw_ind != -1) {
				Set(vval.lsw_ind, ((*piv) >> 16) & 0xFFFF);
			}
			return 0;
		default:
			return 1;
	}
}

int Snap7Daemon::ReadDB(int db, s7_db_map& ops)
{
	int start_address = -1;
	int end_address = -1;

	for (size_t i = 0; i < 256; i++) {
		buffer[i] = 0;
	}

	for (auto it = ops.begin(); it != ops.end(); it++) {
		if (start_address == -1 || it->first.get_address() < start_address)
			start_address = it->first.get_address();

		if (end_address == 0 || end_address < it->first.end_address())
			end_address = it->first.end_address();
	}



	if (Cli_DBRead(s7client, db, start_address, end_address - start_address, (void*)buffer)) {
		sz_log(2, "Cli_DBRead returned error");
		// TODO error
		return 1;
	}


	for (auto it = ops.begin(); it != ops.end(); it++) {
		int r = DBVal(it->first, it->second, start_address, buffer);
		if (r)
			return r;
	}


	return 0;
}

int Snap7Daemon::Read()
{
	int ret;
	int connected = 0;
	for (auto it = m_params_map.begin(); it != m_params_map.end(); it++) {
		if (!Cli_GetConnected(s7client, &connected) || connected) {
			ret = Cli_ConnectTo(s7client, address.c_str(), rack, slot);
			if (ret) {
				sz_log(2, "Cannot connect to: %s", address.c_str());
				return 1;
			}
		}

		ret = ReadDB(it->first, it->second);
		if (ret) {
			return 1;
		}
	}


	return 0;
}

int main(int argc, const char *argv[])
{
	Snap7Daemon dmn;

	sz_log(1, "Starting %s", dmn.Name());

	if( dmn.Init( argc , argv ) ) {
		sz_log(0,"Cannot start %s daemon", dmn.Name());
		return 1;
	}

	sz_log(8, "Initialization done");

	for(;;)
	{
		dmn.Read();
		dmn.Transfer();
		dmn.Wait();
	}

	return 0;
}
