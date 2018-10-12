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
 * draw3
 * SZARP

 *
 * $Id$
 *
 * Creating new defined window
 */

#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>

#include <wx/filedlg.h>
#include <wx/colordlg.h>
#include <wx/gdicmn.h>
#include <wx/dynarray.h>

#include <libxml/tree.h>
#include <libxml/relaxng.h>
#include "xmlutils.h"

#include <wx/config.h>

#include "cconv.h"
#include "szarp_config.h"

#include "ids.h"
#include "classes.h"

#include "dcolors.h"
#include "drawapp.h"
#include "drawobs.h"
#include "coobs.h"
#include "cfgmgr.h"
#include "defcfg.h"
#include "errfrm.h"

#include "defverifier.h"

#define FREE(x) if (x != NULL) free(x)

#define X (xmlChar*)

const wxChar* const DefinedDrawsSets::DEF_PREFIX  = _T("userD");

const wxChar* const DefinedDrawsSets::DEF_NAME = _("User defined");

DefinedDrawInfo::DefinedDrawInfo(DrawInfo *di, DefinedDrawsSets* ds)
:  DrawInfo(di->GetDraw(), di->GetParam())
{
	c = di->GetDrawColor();

	m_ds = ds;
	m_long_changed = false;
	m_short_changed = false;
	m_min_changed = false;
	m_max_changed = false;
	m_special_changed = false;
	m_scale_changed = false;
	m_min_scale_changed = false;
	m_max_scale_changed = false;
	m_col_changed = false;
	m_unit_changed = false;
	m_valid = true;

	m_base_draw = di->GetDraw()->GetWindow();
	m_param_name = di->GetParam()->GetParamName();
	m_base_prefix = di->GetParam()->GetBasePrefix();

}

DefinedDrawInfo::DefinedDrawInfo(DefinedDrawsSets* ds)
:  DrawInfo()
{
	m_ds = ds;
	m_long_changed = false;
	m_short_changed = false;
	m_min_changed = false;
	m_max_changed = false;
	m_scale_changed = false;
	m_min_scale_changed = false;
	m_max_scale_changed = false;
	m_col_changed = false;
	m_special_changed = false;
	m_unit_changed = false;
	m_valid = true;
}


DefinedDrawInfo::DefinedDrawInfo(EkrnDefDraw &edd, DefinedDrawsSets* ds) :
	DrawInfo(edd.di->GetDraw(), edd.di->GetParam())

{
	m_ds = ds;
	m_long_changed = false;
	m_short_changed = false;
	m_min_changed = false;
	m_max_changed = false;
	m_scale_changed = false;
	m_min_scale_changed = false;
	m_max_scale_changed = false;
	m_col_changed = false;
	m_special_changed = false;
	m_unit_changed = false;
	m_valid = true;

	m_base_draw = edd.di->GetDraw()->GetWindow();
	m_param_name = edd.di->GetParam()->GetParamName();
	m_base_prefix = edd.di->GetParam()->GetBasePrefix();

	SetDrawColor(edd.color);
}

DefinedDrawInfo::DefinedDrawInfo(wxString draw_name,
			wxString short_name,
			wxColour color,
			double min,
			double max,
			TDraw::SPECIAL_TYPES special,
			wxString unit,
			DefinedParam *dp,
			DefinedDrawsSets *ds) {

	m_ds = ds;

	m_name = draw_name;
	m_short_name = short_name;
	c = color;
	m_max = max;
	m_min = min;
	m_sp = special;
	m_unit = unit;

	m_long_changed = true;
	m_short_changed = true;
	m_min_changed = true;
	m_max_changed = true;
	m_scale_changed = false;
	m_min_scale_changed = false;
	m_max_scale_changed = false;
	m_col_changed = true;
	m_special_changed = true;
	m_unit_changed = true;
	m_valid = true;

	d = NULL;
	p = dp;

	m_param_name = dp->GetParamName();
	m_base_prefix = dp->GetBasePrefix();

}

void DefinedParam::SetParamName(wxString pn) {
	m_param_name = pn;
}

void DefinedParam::CreateParam() {

	m_param = new TParam(NULL, NULL, L"", FormulaType::LUA_VA, ParamType::LUA);

	m_param->SetName(m_param_name.wc_str());
	m_param->SetPrec(m_prec);
	m_param->SetTimeType(TParam::NANOSECOND);
	m_param->SetFormulaType(m_type);
	m_param->SetLuaScript(SC::S2U(m_formula).c_str());
	m_param->SetLuaStartDateTime(m_start_time);
}

void DefinedDrawInfo::SetScale(int scale) {
	if (scale == GetScale())
		return;

	m_ds->SetModified();

	m_scale_changed = true;
	m_scale = scale;
}

int DefinedDrawInfo::GetScale() {
	if (m_scale_changed)
		return m_scale;

	if (d == NULL)
		return 0;
	else
		return DrawInfo::GetScale();
}

void DefinedDrawInfo::SetScaleMin(double min_scale) {
	if (GetScaleMin() == min_scale)
		return;
	
	m_ds->SetModified();

	m_min_scale_changed = true;	
	m_min_scale = min_scale;
}

double DefinedDrawInfo::GetScaleMin() {
	if (m_min_scale_changed)
		return m_min_scale;
	else if (d)
		return DrawInfo::GetScaleMin();
	else
		return 0;
}

int DefinedDrawInfo::GetPrec() {
	if (IsValid() == false)
		return 0;

	return DrawInfo::GetPrec();
}

void DefinedDrawInfo::SetScaleMax(double max_scale) {
	if (GetScaleMax() == max_scale)
		return;
	
	m_ds->SetModified();

	m_max_scale_changed = true;	
	m_max_scale = max_scale;
}

double DefinedDrawInfo::GetScaleMax() {
	if (m_max_scale_changed)
		return m_max_scale;
	else if (d)
		return DrawInfo::GetScaleMax();
	else
		return 0;
}

wxString DefinedDrawInfo::GetParamName() const {
	return m_param_name;
}

void DefinedDrawInfo::SetSetName(wxString window) {
	m_set_name = window;
}

wxString DefinedDrawInfo::GetSetName() {
	return m_set_name;
}

void DefinedDrawInfo::SetParamName(wxString param_name) {
	m_param_name = param_name;
}

wxString DefinedDrawInfo::GetName() {
	if (IsValid() == false)
		return wxString();

	if (m_long_changed)
		return m_name;
	else
		return DrawInfo::GetName();
}

wxString DefinedDrawInfo::GetShortName() {
	if (IsValid() == false)
		return wxString();

	if (m_short_changed)
		return m_short_name;
	else
		return DrawInfo::GetShortName();
}

TDraw::SPECIAL_TYPES DefinedDrawInfo::GetSpecial() {
	if (IsValid() == false)
		return TDraw::NONE;

	if (m_special_changed)
		return m_sp;
	else if (d)
		return DrawInfo::GetSpecial();
	else
		return TDraw::NONE;
}

void DefinedDrawInfo::SetSpecial(TDraw::SPECIAL_TYPES special) {
	if (GetSpecial() == special) 
		return;

	m_ds->SetModified();

	m_special_changed = true;
	m_sp = special;
}

void DefinedDrawInfo::SetDrawColor(wxColour color) {
	if (color == GetDrawColor())
		return;

	m_ds->SetModified();

	m_col_changed = true;
	c = color;
}

double DefinedDrawInfo::GetMin() {
	if (IsValid() == false)
		return -2;

	if (m_min_changed)
		return m_min;
	else
		return DrawInfo::GetMin();
}

void DefinedDrawInfo::SetMax(double max) {
	if (GetMax() == max)
		return;

	m_ds->SetModified();

	m_max = max;
	m_max_changed = true;

}

void DefinedDrawInfo::SetMin(double min) {
	if (GetMin() == min)
		return;

	m_ds->SetModified();

	m_min = min;
	m_min_changed = true;

}

void DefinedDrawInfo::SetDraw(TDraw *_d) {
	d = _d;
}

void DefinedDrawInfo::SetParam(DrawParam *_p) {
	p = _p;
	SetParamName(_p->GetParamName());
	if (TParam::IsHourSumUnit(_p->GetUnit().wc_str())) {
		m_special_changed = true;
		m_sp = TDraw::HOURSUM;
	}
}

void DefinedDrawInfo::SetDrawName(wxString name) {
	if (GetName() == name)
		return;

	m_ds->SetModified();

	m_name = name;
	m_long_changed = true;

}

void DefinedDrawInfo::SetShortName(wxString name) {
	if (GetShortName() == name)
		return;

	m_ds->SetModified();

	m_short_name = name;
	m_short_changed = true;

}

double DefinedDrawInfo::GetMax() {
	if (IsValid() == false)
		return -1;

	if (m_max_changed)
		return m_max;
	else
		return DrawInfo::GetMax();
}

wxString DefinedDrawInfo::GetUnit() {
	if (IsValid() == false)
		return wxString();

	if (m_unit_changed)
		return m_unit;
	else
		return DrawInfo::GetUnit();
}

void DefinedDrawInfo::SetUnit(wxString unit) {
	if (GetUnit() == unit)
		return;

	m_unit = unit;
	m_unit_changed = true;
}

xmlNodePtr DefinedDrawInfo::GenerateXML(xmlDocPtr doc) {

	xmlNodePtr param = xmlNewDocNode(doc, NULL, X "param", NULL);

	xmlSetProp(param, X "name",
		   SC::S2U(m_param_name).c_str());

	xmlSetProp(param, X "source",
		   SC::S2U(m_base_prefix).c_str());

	xmlSetProp(param, X "draw",
		   SC::S2U(m_base_draw).c_str());

	if (m_special_changed) {
		if (m_sp == TDraw::HOURSUM) {
			xmlSetProp(param, X "hoursum", X "true");
		} else {
			xmlSetProp(param, X "hoursum", X "false");
		}
	}

	xmlNodePtr draw = xmlNewChild(param, NULL, X "draw", NULL);
	
	bool any = false;	// anythhing changed

	// setting attributes only if value changed

	if (m_short_changed) {
		xmlSetProp(draw, X "short",
			   SC::S2U(m_short_name).c_str());
		any = true;
	}

	if (m_long_changed) {
		xmlSetProp(draw, X "title",
			   SC::S2U(m_name).c_str());
		any = true;
	}

	if (m_col_changed) {
		xmlSetProp(draw, X "color",
			   SC::S2U(c.GetAsString(wxC2S_HTML_SYNTAX)).c_str());
		any = true;
	}

	if (m_min_changed) {
		xmlSetProp(draw, X "min", (const xmlChar*) ((std::stringstream&)(std::ostringstream() << m_min)).str().c_str());
		any = true;
	}

	if (m_max_changed) {
		xmlSetProp(draw, X "max", (const xmlChar*) ((std::stringstream&)(std::ostringstream() << m_max)).str().c_str());
		any = true;
	}

	if (m_unit_changed) {
		xmlSetProp(draw, X "unit", SC::S2U(m_unit).c_str());
		any = true;
	}

	if (m_scale_changed) {
		xmlSetProp(draw, X "scale",(const xmlChar*) ((std::stringstream&)(std::stringstream() << m_scale)).str().c_str()); 
		any = true;
	}

	if (m_min_scale_changed) {
		xmlSetProp(draw, X "min_scale",(const xmlChar*) ((std::stringstream&)(std::stringstream() << m_min_scale)).str().c_str()); 
		any = true;
	}

	if (m_max_scale_changed) {
		xmlSetProp(draw, X "max_scale",(const xmlChar*) ((std::stringstream&)(std::stringstream() << m_max_scale)).str().c_str()); 
		any = true;
	}

	if (!any)	// no need for 'draw' element
	{
		xmlUnlinkNode(draw);
		xmlFreeNode(draw);
	}

	return param;
}

bool DefinedParam::IsValid() const {
	return DrawParam::IsValid();
}

bool DefinedDrawInfo::IsValid() const {
	return m_valid;
}

void DefinedDrawInfo::SetValid(bool valid) {
	m_valid = valid;
}

xmlNodePtr DefinedParam::GenerateXML(xmlDocPtr doc) {
	xmlNodePtr f = xmlNewDocNode(doc, NULL, X "param", NULL);

	xmlChar *c =xmlStrdup(SC::S2U(m_formula).c_str());
	xmlNodePtr cd = xmlNewCDataBlock(f->doc, c, strlen((char*)c));
	xmlFree(c);

	xmlAddChild(f, cd);

	xmlSetProp(f, X "base", SC::S2U(m_base_prefix).c_str());

	xmlSetProp(f, X "name", SC::S2U(m_param_name).c_str());

	xmlSetProp(f, X "unit", SC::S2U(m_unit).c_str());

	char buffer[10];

	snprintf(buffer, 9, "%d", m_prec);
	xmlSetProp(f, X "prec", X buffer);

	if (m_type == FormulaType::LUA_VA)
		xmlSetProp(f, X "type", X "va");
	else
		xmlSetProp(f, X "type", X "av");

	xmlSetProp(f, X "unit", SC::S2U(m_unit).c_str());
		
	xmlSetProp(f, X "start_date", (const xmlChar*)((std::stringstream&)(std::stringstream() << m_start_time)).str().c_str());

	if (m_network_param) {
		xmlSetProp(f, X "network_param", X"yes");
		xmlSetProp(f, X "mod_time", (const xmlChar*) ((std::stringstream&)(std::ostringstream() << m_modification_time)).str().c_str());
	}

	return f;
		
}

DefinedParam::DefinedParam(wxString base_prefix,
				wxString param_name,
				wxString unit,
				wxString formula,
				int prec,
				FormulaType type,
				time_t start_time,
				bool network_param) {

	m_base_prefix = base_prefix;
	m_formula = formula;
	m_param_name = param_name;
	m_unit = unit;
	m_prec = prec;
	m_type = type;
	m_start_time = start_time;
	m_network_param = network_param;

}

wxString DefinedParam::GetParamName() const {
	return m_param_name;
}

wxString DefinedDrawInfo::GetBasePrefix() const {
	return m_base_prefix;
}

wxString DefinedDrawInfo::GetBaseDraw() {
	return m_base_draw;
}

wxString DefinedDrawInfo::GetBaseParam() {
	return m_param_name;
}

DefinedDrawsSets* DefinedDrawInfo::GetDrawsSets() {
	return m_ds;
}

wxString DefinedDrawInfo::GetValueStr(const double &val, const wxString& no_data_str) {
	if (IsValid())
		return DrawInfo::GetValueStr(val, no_data_str);
	else
		return no_data_str;
}


void DefinedDrawInfo::ParseXML(xmlNodePtr node) {
	xmlChar* _source = xmlGetProp(node, X "source");
	if (_source == NULL) {
		throw IllFormedParamException(_("Invalid source attribute"));
	}

	m_base_prefix = SC::U2S(_source);
	xmlFree(_source);

	xmlChar* _draw = xmlGetProp(node, X "draw");
	if (_draw) {
		m_base_draw = SC::U2S(_draw);
		xmlFree(_draw);
	}

	xmlChar* _name = xmlGetProp(node, X "name");
	if (_name == NULL) {
		throw IllFormedParamException(_("Invalid name attribute"));
	}

	m_param_name = SC::U2S(_name);
	xmlFree(_name);

	xmlChar* _hoursum = xmlGetProp(node, X "hoursum");
	if (_hoursum) {
		wxString hoursum = SC::U2S(_hoursum);
		xmlFree(_hoursum);

		if (hoursum == _T("true"))
			m_sp = TDraw::HOURSUM;
		else
			m_sp = TDraw::NONE;

		m_special_changed = true;
	} else
		m_special_changed = false;


	xmlNodePtr d = node->xmlChildrenNode;
	for (; d != NULL; d = d->next)
		if (!xmlStrcmp(d->name, X"draw"))
			ParseDrawElement(d);
}

void DefinedDrawInfo::ParseXMLRPCValue(XMLRPC_VALUE v) {
	XMLRPC_VALUE d = XMLRPC_VectorRewind(v);
	m_base_prefix = SC::U2S((const unsigned char*) XMLRPC_GetValueString(d));

	d = XMLRPC_VectorNext(v);
	m_param_name = SC::U2S((const unsigned char*) XMLRPC_GetValueString(d));

	d = XMLRPC_VectorNext(v);
	m_base_draw = SC::U2S((const unsigned char*) XMLRPC_GetValueString(d));

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_string == XMLRPC_GetValueTypeEasy(d)) {
		m_long_changed = true;
		m_name = SC::U2S((const unsigned char*) XMLRPC_GetValueString(d));
	}

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_string == XMLRPC_GetValueTypeEasy(d)) {
		m_short_changed = true;
		m_short_name = SC::U2S((const unsigned char*) XMLRPC_GetValueString(d));
	}

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_int == XMLRPC_GetValueTypeEasy(d)) {
		m_special_changed = true;
		if (XMLRPC_GetValueInt(d)) 
			m_sp = TDraw::HOURSUM;
		else
			m_sp = TDraw::NONE;
	}

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_string == XMLRPC_GetValueTypeEasy(d)) {
		c = wxColour(SC::U2S((const unsigned char*) XMLRPC_GetValueString(d)));
		m_col_changed = true;
	}

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_double == XMLRPC_GetValueTypeEasy(d)) {
		m_min_changed = true;
		m_min = XMLRPC_GetValueDouble(d);
	}

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_double == XMLRPC_GetValueTypeEasy(d)) {
		m_max_changed = true;
		m_max = XMLRPC_GetValueDouble(d);
	}

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_int == XMLRPC_GetValueTypeEasy(d)) {
		m_scale_changed = true;
		m_scale = XMLRPC_GetValueInt(d);
	}

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_double == XMLRPC_GetValueTypeEasy(d)) {
		m_min_scale_changed = true;
		m_min_scale = XMLRPC_GetValueDouble(d);
	}

	d = XMLRPC_VectorNext(v);
	if (xmlrpc_type_double == XMLRPC_GetValueTypeEasy(d)) {
		m_max_scale_changed = true;
		m_max_scale = XMLRPC_GetValueDouble(d);
	}

}


void DefinedParam::ParseXML(xmlNodePtr d) {
	xmlChar *_script = xmlNodeListGetString(d->doc, d->children, 1);
	if (_script == NULL) {
		throw IllFormedParamException(_("Invalid script"));
	}

	m_formula = SC::U2S(_script);

	xmlChar* _base_prefix = xmlGetProp(d, BAD_CAST "base");
	if (_base_prefix == NULL) {
		throw IllFormedParamException(_("Invalid base attribute"));
	}

	m_base_prefix = SC::U2S(_base_prefix);
	xmlFree(_base_prefix);

	xmlChar* _param_name = xmlGetProp(d, BAD_CAST "name");
	if (_param_name == NULL) {
		throw IllFormedParamException(_("Invalid name attribute"));
	}

	m_param_name = SC::U2S(_param_name);
	xmlFree(_param_name);

	xmlChar *prec = xmlGetProp(d, BAD_CAST "prec");
	if (prec == NULL) {
		throw IllFormedParamException(_("Invalid precision attribute"));
	}

	m_prec = wcstol(SC::U2S(prec).c_str(), NULL, 10);
	xmlFree(prec);

	xmlChar* _network_param = xmlGetProp(d, X "network_param");
	if (_network_param && !xmlStrcmp(_network_param, X"yes")) {
		m_network_param = true;

		xmlChar* _modification_time = xmlGetProp(d, X "modification_time");	
		if (_modification_time)
			std::stringstream((const char*)_modification_time) >> m_modification_time;
		else
			m_modification_time = 0;

		xmlFree(_modification_time);
	} else {
		m_network_param = false;
	}
	xmlFree(_network_param);

	xmlChar *type = xmlGetProp(d, BAD_CAST "type");
	if (type && !xmlStrcmp(type, BAD_CAST "av"))
		m_type = FormulaType::LUA_AV;
	else
		m_type = FormulaType::LUA_VA;
	xmlFree(type);

	xmlChar *unit = xmlGetProp(d, BAD_CAST "unit");
	if (unit)
		m_unit = SC::U2S(unit);
	xmlFree(unit);
	
	time_t start_date;
	xmlChar *a_start_date = xmlGetProp(d, X "start_date");
	if (a_start_date != NULL) {
		if (wxString(SC::U2S(a_start_date)).ToLong(&start_date))
			m_start_time = start_date;
		xmlFree(a_start_date);
	}
	
	CreateParam();

}

void DefinedParam::ParseXMLRPCValue(XMLRPC_VALUE p) {
	XMLRPC_VALUE v = XMLRPC_VectorRewind(p);
	m_param_name = SC::U2S((const unsigned char*) XMLRPC_GetValueString(v));

	v = XMLRPC_VectorNext(p);
	m_base_prefix = SC::U2S((const unsigned char*) XMLRPC_GetValueString(v));

	v = XMLRPC_VectorNext(p);
	m_formula = SC::U2S((const unsigned char*) XMLRPC_GetValueString(v));

	v = XMLRPC_VectorNext(p);
	std::wstring sftype = SC::U2S((const unsigned char*) XMLRPC_GetValueString(v));
	if (sftype == L"av") 
		m_type = FormulaType::LUA_AV;
	else
		m_type = FormulaType::LUA_VA;
	
	v = XMLRPC_VectorNext(p);
	m_unit = SC::U2S((const unsigned char*) XMLRPC_GetValueString(v));

	v = XMLRPC_VectorNext(p);
	if (XMLRPC_GetValueTypeEasy(v) == xmlrpc_type_datetime)
		m_start_time = XMLRPC_GetValueDateTime(v);
	else
		m_start_time = -1;

	v = XMLRPC_VectorNext(p);
	m_prec = XMLRPC_GetValueInt(v);

	m_network_param = true;

	v = XMLRPC_VectorNext(p);
	m_modification_time = XMLRPC_GetValueDateTime(v);

	CreateParam();
}

int DefinedParam::GetPrec() {
	return m_prec;
}

void DefinedDrawInfo::SetColorFromBaseDrawInfo(const wxColour& col) {
	if (m_col_changed == false)
		c = col;
}

void DefinedDrawInfo::ParseDrawElement(xmlNodePtr d) {
	xmlChar *a_title = xmlGetProp(d, X "title");
	if (a_title != NULL) {
		m_name = SC::U2S(a_title);
		xmlFree(a_title);

		m_long_changed = true;
	}

	xmlChar *a_short = xmlGetProp(d, X "short");
	if (a_short != NULL) {
		m_short_name = SC::U2S(a_short);
		xmlFree(a_short);

		m_short_changed = true;
	}	

	xmlChar *a_color = xmlGetProp(d, X "color");
	if (a_color != NULL) {
		wxString cstr = SC::U2S(a_color);
		c = wxColor(cstr);
		xmlFree(a_color);

		m_col_changed = true;
	}
	
	xmlChar *a_max = xmlGetProp(d, X "max");
	if (a_max != NULL) {
		std::wstringstream ss;

		ss << SC::U2S(a_max);
		ss >> m_max;

		xmlFree(a_max);

		m_max_changed = true;
	}

	xmlChar *a_min = xmlGetProp(d, X "min");
	if (a_min != NULL) {
		std::wstringstream ss;

		ss << SC::U2S(a_min);
		ss >> m_min;

		xmlFree(a_min);

		m_min_changed = true;
	}

	xmlChar *a_scale = xmlGetProp(d, X "scale");
	if (a_scale != NULL) {
		std::wstringstream ss;

		ss << SC::U2S(a_scale);
		ss >> m_scale;

		xmlFree(a_scale);

		m_scale_changed = true;
	}

	xmlChar *a_min_scale = xmlGetProp(d, X "min_scale");
	if (a_min_scale != NULL) {
		std::wstringstream ss;

		ss << SC::U2S(a_min_scale);
		ss >> m_min_scale;

		xmlFree(a_min_scale);

		m_min_scale_changed = true;
	}

	xmlChar *a_max_scale = xmlGetProp(d, X "max_scale");
	if (a_max_scale != NULL) {
		std::wstringstream ss;

		ss << SC::U2S(a_max_scale);
		ss >> m_max_scale;

		xmlFree(a_max_scale);

		m_max_scale_changed = true;
	}

	xmlChar *a_unit = xmlGetProp(d, X "unit");
	if (a_unit != NULL) {
		m_unit = SC::U2S(a_unit);

		xmlFree(a_unit);

		m_unit_changed = true;
	}

}

wxString DefinedParam::GetFormula() {
	return m_formula;
}

FormulaType DefinedParam::GetFormulaType() {
	return m_type;
}

void DefinedParam::SetStartTime(time_t start_time) {
	m_start_time = start_time;
}

time_t DefinedParam::GetStartTime() {
	return m_start_time;
}

void DefinedParam::SetPrec(int prec) {
	m_prec = prec;	
}

void DefinedDrawInfo::SetBasePrefix(wxString prefix) {
	m_base_prefix = prefix;
}

void DefinedParam::SetFormula(wxString formula) {
	m_formula = formula;
}

void DefinedParam::SetFormulaType(FormulaType type) {
	m_type = type;
}

void DefinedParam::SetUnit(wxString unit) {
	m_unit = unit;
}

wxString DefinedParam::GetUnit() {
	return m_unit;
}

wxString DefinedParam::GetBasePrefix() const {
	return m_base_prefix;
}

bool DefinedParam::IsNetworkParam() const {
	return m_network_param;
}

time_t DefinedParam::GetModificationTime() const {
	return m_modification_time;
}

DefinedDrawSet::DefinedDrawSet(DrawsSets* parent,
		DefinedDrawSet *ds) : DrawSet(parent) {

	delete m_draws;
	m_draws = ds->m_draws;

	m_ds = ds->m_ds;
	m_copies = ds->m_copies;
	m_sets_nr = ds->m_sets_nr;
	m_name = ds->m_name;
	m_prior = ds->m_prior;
	m_number = ds->m_number;
	m_network_set = ds->m_network_set;
	m_user_name = ds->m_user_name;
	m_modification_time = ds->m_modification_time;

	m_copies->push_back(this);

	m_temporary = false;
	m_copy = true;
}

DefinedDrawSet::DefinedDrawSet(DefinedDrawsSets *ds, bool network) : DrawSet(ds), m_network_set(network) {
	m_ds = ds;
	m_temporary = false;
	m_copies = new std::vector<DefinedDrawSet*>(1, this);
	m_copy = false;
}

std::vector<DefinedDrawSet*>* DefinedDrawSet::GetCopies() {
	return m_copies;
}

bool DefinedDrawSet::IsNetworkSet() const {
	return m_network_set;
}

wxString DefinedDrawSet::GetUserName() const {
	return m_user_name;
}

void DefinedDrawSet::SetUserName(wxString user_name) {
	m_user_name = user_name;	
}

time_t DefinedDrawSet::GetModificationTime() const {
	return m_modification_time;
}

void DefinedDrawSet::SetModificationTime(time_t modification_time) {
	m_modification_time = modification_time;
}

double DefinedDrawSet::GetPrior() {
	return m_prior;
}

void DefinedDrawSet::SetPrior(double prior) {
	for (std::vector<DefinedDrawSet*>::iterator i = m_copies->begin();
			i != m_copies->end();
			i++)
		(*i)->m_prior = prior;
}

DefinedDrawSet::DefinedDrawSet(DefinedDrawsSets *ds, wxString title, std::vector<DefinedDrawInfo::EkrnDefDraw>& v)  : DrawSet(ds) {
	m_copies = new std::vector<DefinedDrawSet*>(1, this);
	m_copy = false;
	m_ds = ds;
	m_temporary = false;
	m_network_set = false;

	for (size_t i = 0; i < v.size(); ++i) {
		DefinedDrawInfo::EkrnDefDraw& ed = v[i];

		DefinedDrawInfo* ddi = new DefinedDrawInfo(ed, ds);
		ddi->SetSetName(title);

		m_draws->push_back(ddi);
		m_sets_nr[ddi->GetBasePrefix()]++;
	}

	SetName(title);
}

DefinedDrawSet::~DefinedDrawSet()
{
	if (!m_copy) {
		delete m_copies;
		m_copies = NULL;
	} else {
		if (m_copies) {
			std::vector<DefinedDrawSet*>::iterator i = std::remove(m_copies->begin(), m_copies->end(), this);
			m_copies->erase(i, m_copies->end());
		}
		//this would be deleted by parent destructor, but since each copy share the same array
		//obviously we want this to be deleted only once
		m_draws = NULL;
	}
}

xmlNodePtr DefinedDrawSet::GenerateXML(xmlDocPtr doc)
{
	if (m_draws->size() == 0 || this->IsTemporary())
		return NULL;

	xmlNodePtr set = xmlNewDocNode(doc, NULL, X "window", NULL);

	xmlSetProp(set, X "title", SC::S2U(m_name).c_str());

	for (size_t i = 0; i < m_draws->size(); i++) {
		DefinedDrawInfo *di = dynamic_cast<DefinedDrawInfo*>(m_draws->at(i));
		xmlAddChild(set, di->GenerateXML(doc));
	}

	if (m_network_set) {
		xmlSetProp(set, X "network_set", X"yes");
		xmlSetProp(set, X "mod_time", (const xmlChar*) ((std::stringstream&)(std::ostringstream() << m_modification_time)).str().c_str());
		xmlSetProp(set, X "user_name", SC::S2U(m_user_name).c_str());
	}

	return set;

}

namespace {

struct ColorCompare {
	bool operator()(const wxColour& c1, const wxColour& c2) const {
		return (c1.Red() << 16) + (c1.Green() << 8) + c1.Blue() 
			< (c2.Red() << 16) + (c2.Green() << 8) + c2.Blue();
	}
};


typedef std::map<wxColour, bool, ColorCompare> ColorMap;

}

void DefinedDrawSet::GetColor(DefinedDrawInfo *ddi) {
	wxColour color = ddi->GetDrawColor();
	ddi->SetDrawColor(color);
}

void DefinedDrawSet::ParseXML(xmlNodePtr node)
{
	xmlChar *_title = xmlGetProp(node, X "title");
	if (_title == NULL) {
		throw IllFormedParamException(_("Invalid title attribute"));
	}

	SetName(SC::U2S(_title));
	xmlFree(_title);

	xmlChar* _network_set = xmlGetProp(node, X "network_set");
	if (_network_set && !xmlStrcmp(_network_set, X "yes"))
		m_network_set = true;
	xmlFree(_network_set);

	if (m_network_set) {
		xmlChar* _modification_time = xmlGetProp(node, X "modification_time");
		if (_modification_time)
			std::wstringstream(SC::U2S(_modification_time)) >> m_modification_time;
		else
			m_modification_time = 0;
		xmlFree(_modification_time);

		xmlChar* _user_name = xmlGetProp(node, X "user_name");
		if (_user_name)
			m_user_name = SC::U2S(_user_name);
		xmlFree(_user_name);
	}

	for (xmlNodePtr p = node->xmlChildrenNode; p != NULL; p = p->next) {
		if ((xmlStrcmp(p->name, X "param")))
			continue;
		
		std::unique_ptr<DefinedDrawInfo> ddi(new DefinedDrawInfo(m_ds));
		ddi->ParseXML(p);
		Add(ddi.release());
	}

}

void DefinedDrawSet::Swap(int a, int b)
{
	DrawInfo *tmp = m_draws->at(a);
	m_draws->at(a) = m_draws->at(b);
	m_draws->at(b) = tmp;
}

void DefinedDrawSet::Remove(DefinedDrawInfo* di) {
	size_t idx;
	for (idx = 0; idx < m_draws->size(); idx++)
		if (m_draws->at(idx) == di)
			break;

	if (idx == m_draws->size())
		return;

	Remove(idx);
}

void DefinedDrawSet::Remove(int idx)
{
	wxString op = m_draws->at(idx)->GetBasePrefix();

	for (std::vector<DefinedDrawSet*>::iterator i = m_copies->begin();
			i != m_copies->end();
			i++) {
		(*i)->m_sets_nr[op]--;
		if ((*i)->m_sets_nr[op] <= 0)
			(*i)->m_sets_nr.erase(op);
	}

	delete m_draws->at(idx);

	DrawInfoArray::iterator i = m_draws->begin();
	std::advance(i, idx);
	m_draws->erase(i);
}

DefinedDrawInfo *DefinedDrawSet::GetDraw(int idx)
{
	return dynamic_cast<DefinedDrawInfo*>(m_draws->at(idx));
}

void DefinedDrawSet::Replace(int idx, DefinedDrawInfo * ndi)
{
	DefinedDrawInfo* odi = dynamic_cast<DefinedDrawInfo*>(m_draws->at(idx));
	wxString op = odi->GetBasePrefix();

	for (std::vector<DefinedDrawSet*>::iterator i = m_copies->begin();
			i != m_copies->end();
			i++) {
		(*i)->m_sets_nr[odi->GetBasePrefix()]--;
		(*i)->m_sets_nr[ndi->GetBasePrefix()]++;
		if ((*i)->m_sets_nr[op] <= 0)
			(*i)->m_sets_nr.erase(op);
	}

	delete odi;
	m_draws->at(idx) = ndi;
}

namespace {

bool color_clash(DrawInfoArray& draws, DefinedDrawInfo *di) {
	for (DrawInfoArray::iterator i = draws.begin(); i != draws.end(); i++)
		if ((*i)->GetDrawColor() == di->GetDrawColor())
			return true;
	return false;
}


std::set<double> get_draw_hues(DrawInfoArray& draws) {
	std::set<double> r;
	for (DrawInfoArray::iterator i = draws.begin(); i != draws.end(); i++) {
		wxColor c = (*i)->GetDrawColor();
		wxImage::HSVValue hsv = wxImage::RGBtoHSV(wxImage::RGBValue(c.Red(), c.Green(), c.Blue()));
		r.insert(hsv.hue);
	}
	return r;
}

double index2hue(size_t index) {
	unsigned divisor = 1;
	double ret = 0;
	while (index) {
		if (index & 1)
			ret += (.5 / divisor);
		divisor <<= 1;
		index >>= 1;
	}
	return ret;
}

bool sufficiently_distant(std::set<double>& hues, double val, double allowed_distance) {
	double v1;
	double v2;

	std::set<double>::iterator i = hues.lower_bound(val);

	if (i == hues.end()) {
		v1 = *(--i);
		v2 = *(hues.begin());
	} else if (i == hues.begin()) {
		v1 = *i;
		v2 = *(--hues.end());
	} else {
		v1 = *i;
		v2 = *(--i);
	}

	if (fabs(v1 - val) < allowed_distance || fabs(1 - (v1 - val)) < allowed_distance)
		return false;	

	if (fabs(v2 - val) < allowed_distance || fabs(1 - (v2 - val)) < allowed_distance)
		return false;	
	
	return true;
}

void make_color_unique(DrawInfoArray& draws, DefinedDrawInfo* di, size_t& color_index) {
	if (!color_clash(draws, di))
		return;

	double allowed_distance = .5 / draws.size();

	if (allowed_distance < 1. / 360)
		return;

	std::set<double> hues = get_draw_hues(draws);

	double hue = index2hue(color_index);
	while (!sufficiently_distant(hues, hue, allowed_distance))
		hue = index2hue(++color_index);

	wxImage::HSVValue hsv(hue, 1, 1);
	wxImage::RGBValue rgb = wxImage::HSVtoRGB(hsv);

	di->SetDrawColor(wxColor(rgb.red, rgb.green, rgb.blue));

}

}

void DefinedDrawSet::Add(const std::vector<DrawInfo*>& draws, bool color_unique) {

	size_t color_index = 0;

	for (std::vector<DrawInfo*>::const_iterator i = draws.begin(); i != draws.end(); i++) {
		if (DefinedDrawInfo* d = dynamic_cast<DefinedDrawInfo*>(*i)) {
			DefinedDrawInfo* ddi = new DefinedDrawInfo(*d);
			Add(ddi);
		} else {
			DefinedDrawInfo* ddi = new DefinedDrawInfo(*i, m_ds);
			if (color_unique) 
				make_color_unique(*m_draws, ddi, color_index);
			Add(ddi);
		}
	}
}

void DefinedDrawSet::Add(DefinedDrawInfo *ddi) {
	m_draws->push_back(ddi);
	ddi->SetSetName(m_name);

	GetColor(ddi);

	for (std::vector<DefinedDrawSet*>::iterator i = m_copies->begin();
			i != m_copies->end();
			i++)
		(*i)->m_sets_nr[ddi->GetBasePrefix()]++;
}

SetsNrHash& DefinedDrawSet::GetPrefixes()
{
	return m_sets_nr;
}

void DefinedDrawSet::Clear()
{
	for (std::vector<DefinedDrawSet*>::iterator i = m_copies->begin();
			i != m_copies->end();
			i++) {
		(*i)->m_name = wxString();
		(*i)->m_prior = -1.0;
		(*i)->m_number = -1;
		(*i)->m_sets_nr.clear();
	}
	
	for (size_t i = 0; i < m_draws->size(); i++)
		delete m_draws->at(i);
	m_draws->resize(0);

}

void DefinedDrawSet::SetName(const wxString& name)
{
	if (GetName() == name)
		return;
	if (!m_copy)
		for (size_t i = 0; i < m_draws->size(); i++) {
			DefinedDrawInfo* dif = dynamic_cast<DefinedDrawInfo*>(m_draws->at(i));
			dif->SetSetName(name);
		}

	DrawSet::SetName(name);
	for (std::vector<DefinedDrawSet*>::iterator i = m_copies->begin();
			i != m_copies->end();
			i++)
		(*i)->SetName(name);
	
	if (!m_copy)
		m_ds->SetModified();
}

DefinedDrawSet* DefinedDrawSet::MakeShallowCopy(DrawsSets* parent) {
	return new DefinedDrawSet(parent, this);
}

DefinedDrawSet* DefinedDrawSet::MakeDeepCopy() {
	DefinedDrawSet *ret = new DefinedDrawSet(m_ds, m_network_set);
	ret->m_sets_nr = m_sets_nr;
	ret->m_network_set = m_network_set;
	ret->m_user_name = m_user_name;
	ret->m_modification_time = m_modification_time;
	ret->m_prior = m_prior;

	DrawInfoArray *ds = ret->GetDraws();

	for (size_t i = 0; i < m_draws->size(); i++) {
		DefinedDrawInfo& ddi = dynamic_cast<DefinedDrawInfo&>(*m_draws->at(i));
		DefinedDrawInfo* ndi = new DefinedDrawInfo(ddi);

		ds->push_back(ndi);
	}

	ret->SetName(GetName());
	ret->m_prior = m_prior;

	return ret;
}

bool DefinedDrawSet::RefersToPrefix(wxString prefix) {
	return m_sets_nr.find(prefix) != m_sets_nr.end();
}

std::set<wxString> DefinedDrawSet::GetUnresolvedPrefixes() {
	std::set<wxString> r;

	for (size_t j = 0; j < m_draws->size(); ++j) {
		DefinedDrawInfo* dw = dynamic_cast<DefinedDrawInfo*>(m_draws->at(j));
		assert(dw);

		if (dw->GetParam() == NULL)
			r.insert(dw->GetBasePrefix());
	}

	return r;
}

DefinedParam* DefinedDrawSet::LookupDefinedParam(wxString prefix, wxString param_name, const std::vector<DefinedParam*>& defined_params) {
	std::vector<DefinedParam*>::const_iterator i;
	for (i = defined_params.begin();
			i != defined_params.end();
			i++) {
		
		DefinedParam *p = *i;
		if (p->GetBasePrefix() == prefix && 
				param_name == p->GetParamName())
			return p;
	}
	return NULL;
}

void DefinedDrawSet::SyncWithAllPrefixes() {

	for (SetsNrHash::iterator i = GetPrefixes().begin();
			i != GetPrefixes().end();
			i++)
		SyncWithPrefix(i->first);
}

void DefinedDrawSet::SyncWithPrefix(wxString prefix) {
	SyncWithPrefix(prefix, m_ds->GetDefinedParams());
}

void DefinedDrawSet::SyncWithPrefix(wxString prefix, const std::vector<DefinedParam*>& defined_params) {
	IPKConfig *bc = dynamic_cast<IPKConfig*>(m_ds->GetParentManager()->GetConfigByPrefix(prefix));

	std::vector<std::pair<DefinedDrawInfo*,int>> invalid_params;
	for (size_t j = 0; j < m_draws->size(); ++j) {
		DefinedDrawInfo* dw = dynamic_cast<DefinedDrawInfo*>(m_draws->at(j));

		if (dw->GetBasePrefix() != prefix)
			continue;

		dw->SetValid(false);

		if (dw->GetBaseDraw().IsEmpty()) {
			DefinedParam* tp = LookupDefinedParam(dw->GetBasePrefix(), dw->GetParamName(), defined_params);
			if (tp != NULL)  {
				dw->SetParam(tp);
				dw->SetValid(true);
			} else {
				continue;
			}
		}

		if (bc->GetRawDrawsSets().find(dw->GetBaseDraw()) 
				== bc->GetRawDrawsSets().end())
			continue;

		DrawSet* bds = bc->GetRawDrawsSets()[dw->GetBaseDraw()];
		for (size_t k = 0; k < bds->GetDraws()->size(); ++k) {
			DrawInfo* di = bds->GetDraws()->at(k);
			if (di->GetParamName() == dw->GetBaseParam()) {
				dw->SetDraw(di->GetDraw());
				dw->SetParam(di->GetParam());
				dw->SetColorFromBaseDrawInfo(di->GetDrawColor());
				dw->SetValid(true);
				break;
			}
		}

		if (!dw->IsValid()){
			invalid_params.push_back(std::make_pair(dw, j));
		}

	}

	for( const auto& p : invalid_params )
	{
			wxString msg = _("Param \"") + p.first->GetBaseParam()
				+ _("\" is not present in draw set \"") + p.first->GetBaseDraw() + wxT("\".\n")
				+ _("Do you want to remove it from your set?");
			if( wxMessageBox(msg, _("Operation failed."),
						wxYES_NO | wxICON_ERROR, wxGetApp().GetTopWindow()) == wxYES )
				m_draws->erase(m_draws->begin() + p.second);
	}
}

DefinedDrawsSets::DefinedDrawsSets(ConfigManager *cfg) : DrawsSets(cfg), m_prior(DrawSet::defined_draws_prior_start - 1), m_modified(false), m_params_attached(false) {
	cfg->RegisterConfigObserver(this);
}

DrawSetsHash& DefinedDrawsSets::GetDrawsSets(bool all) {
	return drawsSets;
}

DrawSetsHash& DefinedDrawsSets::GetRawDrawsSets() {
	return drawsSets;
}

DefinedDrawsSets::~DefinedDrawsSets() {

	for (DrawSetsHash::iterator i = drawsSets.begin(); i != drawsSets.end(); ++i) {
		DefinedDrawSet *dset = dynamic_cast<DefinedDrawSet*>(i->second);

		if (dset != NULL) {
			if (dset->IsTemporary()) {
				delete dset;
			}
		}
	}
}

wxString DefinedDrawsSets::GetID() {
	return wxGetTranslation(DEF_NAME);
}

wxString DefinedDrawsSets::GetPrefix() const {
	return DEF_PREFIX;
}

bool DefinedDrawsSets::IsModified() {
	return m_modified;
}

void DefinedDrawsSets::SetModified(bool modified) {
	m_modified = modified;
}

void DefinedDrawsSets::LoadSets(wxString path) {
	std::vector<DefinedDrawSet*> draw_sets;
	std::vector<DefinedParam*> defined_params;
	LoadSets(path, draw_sets, defined_params);
	for (size_t i = 0; i < draw_sets.size(); i++) {
		DefinedDrawSet *ds = draw_sets[i];
		drawsSets[ds->GetName()] = ds;
		if (ds->IsNetworkSet())
			for (SetsNrHash::iterator i = ds->GetPrefixes().begin(); i != ds->GetPrefixes().end(); i++)
				m_network_param_and_set_prefixes.insert(i->first);
	}
	for (size_t i = 0; i < defined_params.size(); i++) {
		DefinedParam* param = defined_params[i];
		param->setUserDefined(true);
		definedParams.push_back(param);
		if (param->IsNetworkParam())
			m_network_param_and_set_prefixes.insert(param->GetBasePrefix());
	}
}

/* As of C++11, in C++14 we can do this with a lambda in LoadSets function:
auto parse = [this](const xmlChar *xpath_expr, std::function<*DType()> new_dp, std::vector<DType*>& ret_vec, std::vector<DType*>& err_vec) {...} */
template <class DType = DefinedParam>
void parse(const xmlChar *xpath_expr, std::function<DType*()> new_dp,
           std::vector<DType*>& ret_vec, std::vector<DType*>& err_vec,
           std::vector<DefinedParam*>& def_params,
           DefinedVerifier& verify, xmlXPathContextPtr xpath_ctx)
{
	auto node_set = uxmlXPathGetNodes(xpath_expr, xpath_ctx);
	if (node_set == NULL) return;

	for (int i = 0; i < node_set->nodeNr; ++i) {
		xmlNodePtr node = node_set->nodeTab[i];
		DType* dp = new_dp();

		try {
			dp->ParseXML(node);

			verify(*dp, def_params);
			ret_vec.push_back(dp);

		} catch (const SzException& e) {
			err_vec.push_back(dp);

			wxString msg = _("Failed to import defined draw sets from configuration file.\n") + wxString::FromUTF8(e.what());
			wxMessageBox(msg, _("Operation failed."), wxOK | wxICON_ERROR, wxGetApp().GetTopWindow());
		}
	}
}

void DefinedDrawsSets::LoadSets(wxString path, std::vector<DefinedDrawSet*>& draw_sets, std::vector<DefinedParam*>& defined_params) {
	DefinedVerifier verify{m_cfgmgr};
	
	std::unique_ptr<szXmlDoc> root = szXmlDoc::getDoc(SC::S2A(path).c_str());
	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(&(**root));

	xmlXPathRegisterNs(xpath_ctx, BAD_CAST "draw3", BAD_CAST "http://www.praterm.com.pl/SZARP/draw3");

	parse<DefinedParam>(BAD_CAST "/draw3:windows/draw3:param", [](){ return new DefinedParam(); }, defined_params, importedInvalidDefinedParams, defined_params, verify, xpath_ctx);

	auto create_defined_set = [this](){
		auto defined_set = new DefinedDrawSet(this, false);
		defined_set->SetPrior(this->m_prior--);
		return defined_set;
	};

	parse<DefinedDrawSet>(BAD_CAST "/draw3:windows/draw3:window", create_defined_set, draw_sets, importedInvalidDrawsSets, defined_params, verify, xpath_ctx);

	/* if (draw_sets.size() == 0) {
		throw IllFormedSetException(wxString(_("Set is empty"))); // Is it really an error?
	} */
}

bool DefinedDrawsSets::SaveSets(wxString path, const std::vector<DefinedDrawSet*>& ds, const std::vector<DefinedParam*>& dp) {
	xmlDocPtr doc = xmlNewDoc(X "1.0");
	doc->children = xmlNewDocNode(doc, NULL, X "windows", NULL);
	xmlNewNs(doc->children, X "http://www.praterm.com.pl/SZARP/draw3", NULL);

	for (std::vector<DefinedDrawSet*>::const_iterator i = ds.begin(); i != ds.end(); i++)
		xmlAddChild(doc->children, (*i)->GenerateXML(doc));
	for (std::vector<DefinedParam*>::const_iterator i = dp.begin(); i != dp.end(); i++)
		xmlAddChild(doc->children, (*i)->GenerateXML(doc));

	bool ret = true;
	if (xmlSaveFormatFileEnc(SC::S2A(path).c_str(), doc, "UTF-8", 1) == -1)
		ret = false;
	xmlFreeDoc(doc);
	return ret;

}

void DefinedDrawsSets::SaveSets(wxString path) {

	if (m_modified == false)
		return;

	std::vector<DefinedDrawSet*> dsv = importedInvalidDrawsSets;

	SortedSetsArray ssa = GetSortedDrawSetsNames();
	for (size_t i = 0; i < ssa.GetCount(); i++) {
		DefinedDrawSet *ds = dynamic_cast<DefinedDrawSet*>(ssa.Item(i));
		assert(ds);
		dsv.push_back(ds);
	}

	std::vector<DefinedParam*> ddp = definedParams;
	ddp.insert(ddp.end(), importedInvalidDefinedParams.begin(), importedInvalidDefinedParams.end());

	if (SaveSets(path, dsv, ddp) == false)
		wxMessageBox(_("Failed to save file with users sets."),
			     _("Operation failed."), wxOK | wxICON_ERROR,
			     wxGetApp().GetTopWindow());
}

void DefinedDrawsSets::AddDrawsToConfig(wxString prefix, std::set<wxString> *configs_to_load) {
	std::set<wxString> modified;
	std::set<wxString> removed;
	std::set<wxString> missing_prefixes;

	for (DrawSetsHash::iterator i = GetRawDrawsSets().begin();
			i != GetRawDrawsSets().end();
			i++) {
		assert(i->second);

		std::vector<unsigned> tr;

		DefinedDrawSet* df = dynamic_cast<DefinedDrawSet*>(i->second);
		if (df->RefersToPrefix(prefix) == false)
			continue;

		std::set<wxString> ur = df->GetUnresolvedPrefixes();
		missing_prefixes.insert(ur.begin(), ur.end());

		df->SyncWithPrefix(prefix, definedParams);
		if( df->IsEmpty() )
			drawsSets.erase(i);
	}

	if (configs_to_load)
		*configs_to_load = missing_prefixes;

	if (modified.size() == 0 && removed.size() == 0)
		return;

	SetModified();

	for (std::set<wxString>::iterator i = removed.begin();
			i != removed.end();
			i++) {
		DrawSet *ds = drawsSets[*i];
		drawsSets.erase(*i);
		delete ds;
	}

	for (std::set<wxString>::iterator i = modified.begin();
			i != modified.end();
			i++) {
		DefinedDrawSet *dds = dynamic_cast<DefinedDrawSet*>(drawsSets[*i]);
		for (SetsNrHash::iterator j = dds->GetPrefixes().begin();
				j != dds->GetPrefixes().end();
				j++) {
			if (j->first == prefix)
				continue;
			m_cfgmgr->NotifySetModified(j->first, *i, drawsSets[*i]);
		}
		m_cfgmgr->NotifySetModified(DEF_PREFIX, *i, drawsSets[*i]);
	}

}


void DefinedDrawsSets::ConfigurationWasLoaded(wxString prefix,
		std::set<wxString> *configs_to_load) {
	AddDrawsToConfig(prefix, configs_to_load);
}

void DefinedDrawsSets::RemoveAllRelatedToPrefix(wxString prefix) {

	wxLogInfo(_T("Removing all draws related to prefix %s"), prefix.c_str());

	std::set<wxString> modified;
	std::set<wxString> removed;

	for (DrawSetsHash::iterator i = GetRawDrawsSets().begin();
			i != GetRawDrawsSets().end();
			i++) {
		assert(i->second);

		std::vector<unsigned> tr;

		DefinedDrawSet* df = dynamic_cast<DefinedDrawSet*>(i->second);
		if (df->GetPrefixes().find(prefix) == df->GetPrefixes().end())
			continue;

		DrawInfoArray *dia = df->GetDraws();
		for (size_t j = 0; j < dia->size(); ++j) {
			DefinedDrawInfo* dw = dynamic_cast<DefinedDrawInfo*>(dia->at(j));
			if (dw->GetBasePrefix() == prefix) {
				ErrorFrame::NotifyError(wxString::Format(_("Draw %s removed from set %s, it relates to non-existent configuration"), dw->GetBaseDraw().c_str(), i->second->GetName().c_str()));
				tr.push_back(j);
			}
		}

		if (tr.size() == 0)
			continue;

		if (tr.size() == dia->size())
			removed.insert(i->first);
		else {
			for (size_t j = 0; j < tr.size(); j++)
				df->Remove(tr[j] - j);
			modified.insert(i->first);
		}
	}

	if (modified.size() || removed.size())
		SetModified();

	for (std::set<wxString>::iterator i = removed.begin();
			i != removed.end();
			i++) {
		DrawSet *s = drawsSets[*i];
		ErrorFrame::NotifyError(wxString::Format(_("Defined set %s was removed because all draws in this set were removed"), s->GetName().c_str()));
		drawsSets.erase(*i);
		m_cfgmgr->NotifySetRemoved(DEF_PREFIX, *i);
		delete s;
	}

	for (std::set<wxString>::iterator i = modified.begin();
			i != modified.end();
			i++) {
		DefinedDrawSet *dds = dynamic_cast<DefinedDrawSet*>(drawsSets[*i]);
		for (SetsNrHash::iterator j = dds->GetPrefixes().begin();
				j != dds->GetPrefixes().end();
				j++)
			m_cfgmgr->NotifySetModified(j->first, *i, drawsSets[*i]);
		m_cfgmgr->NotifySetModified(DEF_PREFIX, *i, drawsSets[*i]);
	}

}

bool DefinedDrawsSets::AddSet(DefinedDrawSet *s) {
	if (drawsSets.find(s->GetName()) != drawsSets.end()) {
		return SubstituteSet(s->GetName(), s);
	}


	drawsSets[s->GetName()] = s;
	s->SetPrior(m_prior--);

	SetsNrHash& snh = s->GetPrefixes();

	for (SetsNrHash::iterator i = snh.begin();
			i != snh.end();
			i++) {
		m_network_param_and_set_prefixes.insert(i->first);

		IPKConfig *c = dynamic_cast<IPKConfig*>(m_cfgmgr->GetConfigByPrefix(i->first, false));
		if (c == NULL)
			continue;

		DefinedDrawSet* nc = s->MakeShallowCopy(c);
		c->AddUserSet(nc);

		m_cfgmgr->NotifySetAdded(i->first, s->GetName(), nc);
	}

	m_cfgmgr->NotifySetAdded(DEF_PREFIX, s->GetName(), s);

	return true;

}

bool DefinedDrawsSets::RemoveSet(wxString name) {
	if (drawsSets.find(name) == drawsSets.end())
		return false;

	std::vector<DrawSet*> copies_to_delete;
	DefinedDrawSet *s = dynamic_cast<DefinedDrawSet*>(drawsSets[name]);
	assert(s);

	drawsSets.erase(name);

	for (SetsNrHash::iterator i = s->GetPrefixes().begin();
			i != s->GetPrefixes().end();
			i++) {
		DrawsSets *ds = m_cfgmgr->GetConfigByPrefix(i->first);
		assert(ds);
		
		copies_to_delete.push_back(ds->GetRawDrawsSets()[name]);
		ds->RemoveUserSet(name);

		m_cfgmgr->NotifySetRemoved(i->first, name);

	}

	m_cfgmgr->NotifySetRemoved(DEF_PREFIX, name);

	for (std::vector<DrawSet*>::iterator i = copies_to_delete.begin();
			i != copies_to_delete.end();
			i++)
		delete *i;
	delete s;

	return true;
}

void DefinedDrawsSets::AttachDefined() {
}

void DefinedDrawsSets::RemoveDrawFromSet(DefinedDrawInfo *di) {
	DefinedDrawSet* ds = dynamic_cast<DefinedDrawSet*>(drawsSets[di->GetSetName()]);
	assert(ds);

	DrawSet* ods = NULL;

	wxString prefix = di->GetBasePrefix();

	ds->Remove(di);

	if (ds->GetPrefixes().find(prefix) == ds->GetPrefixes().end()) {
		DrawsSets *c = m_cfgmgr->GetConfigByPrefix(prefix);
		assert(c);

		ods = c->GetRawDrawsSets()[ds->GetName()];
		c->RemoveUserSet(ds->GetName());
		m_cfgmgr->NotifySetRemoved(prefix, ds->GetName());
	}
	delete ods;

	std::vector<DefinedDrawSet*>* c = ds->GetCopies();
	for (std::vector<DefinedDrawSet*>::iterator i = c->begin(); i != c->end(); i++)
		m_cfgmgr->NotifySetModified((*i)->GetDrawsSets()->GetPrefix(), (*i)->GetName(), *i);

}

bool DefinedDrawsSets::SubstituteSet(wxString name, DefinedDrawSet *s) {
	DefinedDrawSet* ods = dynamic_cast<DefinedDrawSet*>(drawsSets[name]);
	assert(ods);

	if (ods->IsNetworkSet() && s->IsNetworkSet() && ods->GetModificationTime() == s->GetModificationTime()) {
		delete s;
		return false;
	}

	s->SetPrior(ods->GetPrior());

	std::set<wxString> added_prefixes;
	std::set<wxString> removed_prefixes;

	std::vector<DrawSet*> copies_to_delete;

	SetsNrHash &osh = ods->GetPrefixes();
	SetsNrHash &nsh = s->GetPrefixes();

	for (SetsNrHash::iterator i = osh.begin();
			i != osh.end();
			i++)
		if (nsh.find(i->first) == nsh.end())
			removed_prefixes.insert(i->first);

	for (SetsNrHash::iterator i = nsh.begin();
			i != nsh.end();
			i++)
		if (osh.find(i->first) == osh.end())
			added_prefixes.insert(i->first);

	for (std::set<wxString>::iterator i = removed_prefixes.begin();
			i != removed_prefixes.end();
			i++) {
		DrawsSets *c = m_cfgmgr->GetConfigByPrefix(*i);
		assert(c);

		copies_to_delete.push_back(c->GetRawDrawsSets()[ods->GetName()]);
		c->RemoveUserSet(ods->GetName());
		m_cfgmgr->NotifySetRemoved(*i, ods->GetName());
	}

	for (std::set<wxString>::iterator i = added_prefixes.begin();
			i != added_prefixes.end();
			i++) {
		DrawsSets *c = m_cfgmgr->GetConfigByPrefix(*i);
		assert(c);

		DefinedDrawSet *nc = s->MakeShallowCopy(c);
		c->AddUserSet(nc);
		m_cfgmgr->NotifySetAdded(*i, ods->GetName(), nc);
	}

	if (name != s->GetName()) {
		for (SetsNrHash::iterator i = nsh.begin();
				i != nsh.end();
				i++) {
			if (added_prefixes.find(i->first) != added_prefixes.end())
				continue;

			DrawsSets *c = m_cfgmgr->GetConfigByPrefix(i->first);
			assert(c);

			copies_to_delete.push_back(c->GetRawDrawsSets()[ods->GetName()]);
			DefinedDrawSet *nc = s->MakeShallowCopy(c);
			c->RenameUserSet(ods->GetName(), nc);
			m_cfgmgr->NotifySetRenamed(i->first, ods->GetName(), s->GetName(), nc);
			//wxLogError(_T("%s %s %d"), ods->GetName().c_str(), s->GetName().c_str(), count);
		}
		drawsSets.erase(ods->GetName());
		drawsSets[s->GetName()] = s;

		m_cfgmgr->NotifySetRenamed(DEF_PREFIX, ods->GetName(), s->GetName(), s);
	} else {
		for (SetsNrHash::iterator i = nsh.begin();
				i != nsh.end();
				i++) {
			if (added_prefixes.find(i->first) != added_prefixes.end())
				continue;

			DrawsSets *c = m_cfgmgr->GetConfigByPrefix(i->first);
			assert(c);

			copies_to_delete.push_back(c->GetRawDrawsSets()[ods->GetName()]);

			DefinedDrawSet *nc = s->MakeShallowCopy(c);
			c->SubstituteUserSet(ods->GetName(), nc);

			m_cfgmgr->NotifySetModified(i->first, name, nc);
		}
		drawsSets.erase(ods->GetName());
		drawsSets[s->GetName()] = s;
		m_cfgmgr->NotifySetModified(DEF_PREFIX, name, s);
	}

	for (std::vector<DrawSet*>::iterator i = copies_to_delete.begin();
			i != copies_to_delete.end();
			i++)
		delete *i;

	delete ods;

	return true;
}

namespace EkrnDefConv {
	typedef std::pair<wxString, std::vector<DefinedDrawInfo::EkrnDefDraw> > window_def;
	std::vector<window_def> get_windows(DrawsSets *cfg, wxString path);
}

namespace {
	wxString ConvertName(const wxString& p) {
		wxString r;
		for (size_t i = 0; i < p.Length(); ++i) {
			wxChar c = p[i];
			if ((c >= L'0' && c <= L'9')
					|| (c >= L'A' && c <= L'Z')
					|| (c >= L'a' && c <= L'z')) {
				r += c;
			} else 
				r += L'_';
		}

		return r;

	}

	bool CompareParamNames(const wxString &p1, const wxString &p2) {
		return ConvertName(p1) == ConvertName(p2);
	}

}


wxString DefinedDrawsSets::GetNameForParam(const wxString& prefix, const wxString& proposed_name) {
	bool done = false;
	wxString result = proposed_name;
	int v = 0;

	do {
		std::vector<DefinedParam*>::iterator i = definedParams.begin();

		for (;i != definedParams.end();
				i++) {
			if ((*i)->GetBasePrefix() != prefix)
				continue;

			if (CompareParamNames((*i)->GetParamName().AfterLast(L':') , result)) {
				result = proposed_name;
				result << _T("_"); 
				result << ++v;
				break;
			}
		}

		done = (i == definedParams.end());

	} while (!done);

	return result;
}

std::vector<DefinedParam*>& DefinedDrawsSets::GetDefinedParams() {
	return definedParams;
}

void DefinedDrawsSets::AddDefinedParam(DefinedParam *dp) {
	definedParams.push_back(dp);
}

bool DefinedDrawsSets::RemoveParam(DefinedParam *dp) {
	bool deleted = false;

	std::vector<DefinedParam*> n;
	for (size_t i = 0; i < definedParams.size(); i++)
		if (definedParams[i] != dp) 
			n.push_back(definedParams[i]);
		else
			deleted = true;
	definedParams = n;
	
	return deleted;
}

const std::set<wxString>& DefinedDrawsSets::GetNetworkParamAndSetsPrefixes() const {
	return m_network_param_and_set_prefixes;
}

void DefinedDrawsSets::LoadDraw2File() {

	wxString path = wxFileSelector(_("Choose file to import"), wxGetHomeDir(), _T(""), _T(".def"), _T("DEF(*.def)|*.def"));
	if (path.IsEmpty())
		return;

	size_t sp = path.rfind(_T("ekrn"));
	size_t ep = path.rfind(_T(".def"));

	wxString prefix;

	if (sp == wxString::npos || ep == wxString::npos) {
		prefix = wxGetTextFromUser(
				_("Unable to perform automatic detection of configuration's prefix for this file. Please enter prefix of this configuration"),
			       	_("Unable to detect prefix")
				);
	} else
		prefix = path.SubString(sp + 4, ep - 1);

	if (prefix.IsEmpty())
		return;

	DrawsSets *pcfg = m_cfgmgr->GetConfigByPrefix(prefix);
	if (pcfg == NULL) {
		wxMessageBox(wxString::Format(_("Failed to load configuration of prefix %s. Unable to import draws."), prefix.c_str()), _("Error!"), wxICON_ERROR, 
				wxGetApp().GetTopWindow());
		return;
	}

	std::vector<EkrnDefConv::window_def> ws = EkrnDefConv::get_windows(pcfg, path);
	if (ws.size() == 0) {
		wxMessageBox(_("Failed to import draws from file."), _("Error!"),
				wxICON_ERROR, wxGetApp().GetTopWindow());
		return;
	}

	for (size_t i = 0; i < ws.size(); i++) {
		EkrnDefConv::window_def& def = ws[i];
		def.first = _T("*") + prefix + _T(" ") + def.first;

		DefinedDrawSet *dw = new DefinedDrawSet(this, def.first, def.second);
		if (dw->GetDraws()->size() == 0) {
			delete dw;
			continue;
		}

		AddSet(dw);
	}

	//SetModified();

	wxMessageBox(_("Draws imported successfully."), _("Ok!"), wxOK,
			wxGetApp().GetTopWindow());
}


namespace EkrnDefConv {

	struct AxisInfo {
		double maxval;
		double minval;
		double maxrozsz;
		double minrozsz;
		int procrosz;
	};

#if 0
	std::vector<window_def> get_windows(DrawsSets *cfg, wxString path);
#endif

	void skip_to(std::istream &file, std::string value) {
		std::string line;
		do 
			std::getline(file, line);
		while (line.find(value) == std::string::npos);
	}

	DrawInfo *get_draw_info(DrawsSets *cfg, const std::wstring& _ipkname, const AxisInfo& ai) {

		wxString ipkname = _ipkname;

		for (DrawSetsHash::iterator i = cfg->GetRawDrawsSets().begin(); i != cfg->GetRawDrawsSets().end(); ++i) {
			DrawInfoArray* draws = i->second->GetDraws();

			for (size_t j = 0; j < draws->size(); j++) {
				DrawInfo* di = (*draws)[j];

				DrawParam *p = di->GetParam();
				TDraw *d = di->GetDraw();

				if (ipkname == p->GetParamName()
						&& d->GetMax() == ai.maxval
						&& d->GetMin() == ai.minval
						&& (d->GetScale() == 0 || 
							(d->GetScale() == ai.procrosz
							 && d->GetScaleMin() == ai.minrozsz
							 && d->GetScaleMax() == ai.maxrozsz))) 
					return di;

			}

		}

		return NULL;
	}

	window_def get_window(DrawsSets *cfg, std::istream &file) {
		std::vector<DefinedDrawInfo::EkrnDefDraw> result;

		skip_to(file, "Title");
		std::string title;
		std::getline(file, title);	

		skip_to(file, "NumberOfAxes");
		int number_of_axes;
		file >> number_of_axes;

		std::vector<AxisInfo> ais;

		for (int i = 0; i < number_of_axes; i++) {
			AxisInfo ai;

			skip_to(file, "Maxval");
			file >> ai.maxval;

			skip_to(file, "Minval");
			file >> ai.minval;

			skip_to(file, "Maxrozsz");
			file >> ai.maxrozsz;

			skip_to(file, "Minrozsz");
			file >> ai.minrozsz;

			skip_to(file, "Procrozsz");
			file >> ai.procrosz;

			ais.push_back(ai);
		}

		skip_to(file, "NumberOfDraw");
		int number_of_draws;
		file >> number_of_draws;

		for (int i = 0; i < number_of_draws; i++) {
			skip_to(file, "Color");
			int color_idx;
			file >> color_idx;

			skip_to(file, "IPKName");
			std::string ipk_name;
			std::getline(file, ipk_name);

			skip_to(file, "Axisnr");
			int axis_nr;
			file >> axis_nr;

			DrawInfo* di = get_draw_info(cfg, SC::L2S(ipk_name), ais.at(axis_nr));
			if (di == NULL)
				continue;

			wxColour color = DrawDefaultColors::MakeColor(color_idx - 1);
			DefinedDrawInfo::EkrnDefDraw e;
			e.di = di;
			e.color = color;

			result.push_back(e);
		}

		return std::make_pair(wxString(SC::L2S(title.substr(1), true)), result);

	}

	std::vector<window_def> get_windows(DrawsSets *cfg, wxString path) {
		std::ifstream fs(SC::S2A(path).c_str());

		fs.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
		try {
			std::vector<window_def> r;

			int number_of_windows;
			fs >> number_of_windows;

			for (int i = 0; i < number_of_windows; i++)
				r.push_back(get_window(cfg, fs));

			return r;
		} catch (std::ios_base::failure) {
			return std::vector<window_def>();
		}
	}

}

