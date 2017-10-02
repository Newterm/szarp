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
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'param' class implementation.
 *
 * $Id$
 */

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <string>
#include <dirent.h>
#include <assert.h>

#include <vector>
#include <sstream>
#include <iomanip>

#include "szarp_config.h"
#include "parcook_cfg.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "conversion.h"
#include "liblog.h"
#include "szdefines.h"
#include "szbase/szbdefines.h"
#include "szbase/szbname.h"

#ifdef MINGW32
#include "mingw32_missing.h"
#endif //mingw32

#define FREE(x)	if (x != NULL) free(x)

using namespace std;

bool TParam::IsHourSumUnit(const std::wstring& unit) {
	if (unit == L"MW" || unit == L"kW")
		return true;

	if (unit.size() > 2
			&& unit.substr(unit.size() - 2) == L"/h")
		return true;

	return false;
}

void
TParam::SetAutoBase()
{
    _baseInd = -1;
    _inbase = 1;
}

void
TParam::SetBaseInd(int index)
{
    _baseInd = index;
    _inbase = index >= 0;

    if (_inbase)
	_param_type = ParamType::REAL;
    else
	_param_type = ParamType::DEFINABLE;
}

const std::wstring& TParam::GetSzbaseName()
{
	if (_szbase_name.empty())
		_szbase_name = wchar2szb(GetName());
	return _szbase_name;
}

void
ParamFormulaInfo::SetFormulaType(FormulaType type)
{
    _ftype = type;

    if (FormulaType::DEFINABLE == _ftype)
	PrepareDefinable();
}

TDraw *
TParam::AddDraw(TDraw * draw)
{
    if (IsHourSumUnit(_unit) && (draw->GetSpecial() == TDraw::NONE))
	draw->SetSpecial(TDraw::HOURSUM);
    if (_draws == NULL) {
	_draws = draw;
	return draw;
    }
    return _draws->Append(draw);
}

int TParam::parseXML(xmlTextReaderPtr reader)
{
	if (TAttribHolder::parseXML(reader)) return 1;

	TValue* v = NULL;

	XMLWrapper xw(reader,true);

	const char* ignored_trees[] = { "doc", "editable", 0 };
	xw.SetIgnoredTrees(ignored_trees);

	if (_parentSzarpConfig)
		_parentSzarpConfig->AddParamToNamesCache(this);

	if (xw.IsEmptyTag())
		return 0;

	bool isFormula = false;

	if (!xw.NextTag())
		return 1;

	for (;;) {
		if (xw.IsTag("raport")) {
			if (xw.IsBeginTag()) {
				double o = -1.0;
				std::wstring strw_title, strw_desc;

				const char* need_attr_raport[] = { "title",0  };
				if (!xw.AreValidAttr(need_attr_raport)) {
					throw XMLWrapperException();
				}

				for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
					const xmlChar *attr = xw.GetAttr();

					try {
						if (xw.IsAttr("order")) {
							o = boost::lexical_cast<double>(attr);
						} else
						if (xw.IsAttr("title")) {
							strw_title = SC::U2S(attr);
						} else
						if (xw.IsAttr("description")) {
							strw_desc = SC::U2S(attr);
						}
					} catch (boost::bad_lexical_cast &) {
						xw.XMLErrorWrongAttrValue();
					}
				}

				AddRaport(strw_title,
					strw_desc,
					o);
			}
		} else
		if (xw.IsTag("value")) {
			if (xw.IsBeginTag()) {
				const char* need_attr_value[] = { "int", "name", 0 };
				if (!xw.AreValidAttr(need_attr_value)) {
					throw XMLWrapperException();
				}

				std::wstring wstr_name;
				int i = 0;

				for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
					const xmlChar *attr = xw.GetAttr();

					try {
						if (xw.IsAttr("int")) {
							i = boost::lexical_cast<int>(attr);
						} else
						if (xw.IsAttr("name")) {
							wstr_name = SC::U2S(attr);
						}
					} catch (boost::bad_lexical_cast &) {
						xw.XMLErrorWrongAttrValue();
					}
				}

				if (v == NULL)
					_values = v = new TValue(i, wstr_name, NULL);
				else
					v = v->Append(new TValue(i, wstr_name, NULL));
			}
		} else
		if (xw.IsTag("draw")) {
			if (xw.IsBeginTag()) {
				TDraw *d = TDraw::parseXML(reader);
			if (d != NULL)
				AddDraw(d);
			}
		} else
		if (xw.IsTag("define")) {
			if (xw.IsBeginTag()) {
				_param_type = ParamType::REAL;

				const char* need_attr_def[] = { "type", 0 };
				if (!xw.AreValidAttr(need_attr_def)) {
					throw XMLWrapperException();
				}

				bool request_lua_formula = false;
				for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
					const xmlChar *attr = xw.GetAttr();
					try {
						if (xw.IsAttr("type")) {
							if (!strcmp((char*)attr, "RPN")) {
								_ftype = FormulaType::RPN;
							}else 
							if (!strcmp((char*)attr, "DRAWDEFINABLE")) {
								_ftype = FormulaType::DEFINABLE;
								_param_type = ParamType::DEFINABLE;
							}
#ifndef NO_LUA
							else
							 if (!strcmp((char*)attr, "LUA")) {
								_param_type = ParamType::LUA;
								request_lua_formula = true;
							}
						} else // end "type"
						if (xw.IsAttr("lua_formula")) {
							if (!strcmp((char*)attr, "va"))
								_ftype = FormulaType::LUA_VA;
							else if (!strcmp((char*)attr, "av"))
								_ftype = FormulaType::LUA_AV;
							else if (!strcmp((char*)attr, "ipc"))
								_ftype = FormulaType::LUA_IPC;
							else {
								xw.XMLError("unknown value for 'lua_formula' attribute");
							}
						} else
						if (xw.IsAttr("lua_start_offset")) {
							_lua_start_offset = boost::lexical_cast<int>(attr);
						} else
						if (xw.IsAttr("lua_end_offset")) {
							_lua_end_offset = boost::lexical_cast<int>(attr);
						} else
						if (xw.IsAttr("lua_start_date_time")) {
							boost::posix_time::ptime star_date_time = boost::posix_time::not_a_date_time;
							try {
								star_date_time = boost::posix_time::time_from_string((char *)attr);
							} catch(std::exception e) {
								star_date_time = boost::posix_time::not_a_date_time;
							}

							if (star_date_time == boost::posix_time::not_a_date_time) {
								xw.XMLError("lua_start_date_time attribute has invalid value - expected format \"YYYY-MM-DD hh:mm\"");
							} else {
								struct tm t = to_tm(star_date_time);
								_lua_start_date_time = timegm(&t);
							}

#endif // NO_LUA
						} // end "lua_end_offset" | "type"
						else
						if (xw.IsAttr("formula")) {
						// workaround - take only one attr. "formula" when occur more than one <define>
							if (!isFormula) {
								_formula = SC::U2S(attr);
								isFormula = true;
							}
						} else {
							xw.XMLWarningNotKnownAttr();
						}
					} catch (boost::bad_lexical_cast &) {
						xw.XMLErrorWrongAttrValue();
					}
				} // end FORALLATTR
#ifndef NO_LUA
				if (request_lua_formula && !(_ftype == FormulaType::LUA_AV || _ftype == FormulaType::LUA_VA || _ftype == FormulaType::LUA_IPC)) {
					xw.XMLErrorAttr((const xmlChar *)"define", "lua_formula");
				}
#endif
			} // end "define"
		} else
		if (xw.IsTag("script")) {
			if (xw.IsBeginTag()) {
				_script = TScript::parseXML(reader);
			}
		} else
		if (xw.IsTag("param")) {
			break;
		}
		else {
			xw.XMLErrorNotKnownTag("param");
		}
		if (!xw.NextTag())
			return 1;
	}

	if (!hasAttribute("prec") && !_values) {
		xw.XMLError("Attribute 'prec' in 'param'");
	}

	return 0;
}

int TParam::processAttributes() {
	if (!hasAttribute("name")) {
		sz_log(0, "No attribute name in param! Cannot continue!");
		return 1;
	}

    _name = SC::A2S(getAttribute("name"));
	_shortName = SC::A2S(getAttribute<std::string>("short_name", " * "));
	_drawName = SC::A2S(getAttribute<std::string>("draw_name", " * "));
	_unit = SC::A2S(getAttribute<std::string>("unit", "-"));

	auto data_type_attr = getAttribute<std::string>("data_type", "short");
	if (data_type_attr == "short") {
		_data_type = SHORT;
	} else if (data_type_attr == "float") {
		_data_type = FLOAT;
	} else if (data_type_attr == "double") {
		_data_type = DOUBLE;
	} else if (data_type_attr == "int") {
		_data_type = INT;
	} else {
		sz_log(0, (std::string("Unknown data_type attribute in param ")+SC::S2A(GetName())).c_str());
		return 1;
	}
	
	auto time_type_attr = getAttribute<std::string>("time_type", "second");
	if (time_type_attr == "second") {
		_time_type = SECOND;
	} else if (time_type_attr == "nanosecond") {
		_time_type = NANOSECOND;
	} else {
		sz_log(0, (std::string("Unknown time_type attribute in param ")+SC::S2A(GetName())).c_str());
		return 1;
	}

	auto base_ind_attr = getAttribute<std::string>("base_ind", "auto");
	if (base_ind_attr == "auto") {
	    SetAutoBase();
	} else {
		SetBaseInd(getAttribute<int>("base_ind", -1));
	}

	return 0;
}

int
TParam::parseXML(xmlNodePtr node)
{
#define NOATR(p, n) \
{\
	sz_log(1, "XML parsing error: attribute '%s' in node '%s' not found (line %ld)", n, SC::U2A(p->name).c_str(), xmlGetLineNo(p)); \
	return 1; \
}
#define NEEDATR(p, n) \
	if (c) xmlFree(c); \
	c = xmlGetNoNsProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar*)

	if (TAttribHolder::parseXML(node)) return 1;

    if (_parentSzarpConfig)
		_parentSzarpConfig->AddParamToNamesCache(this);

    unsigned char *c = NULL;
    int i;
    xmlNodePtr ch;
    TValue *v = NULL;

    for (ch = node->children; ch; ch = ch->next) {
	if (ch->type != XML_ELEMENT_NODE) {
		continue;
	}
	if (!strcmp((char *) ch->name, "value")) {
	    NEEDATR(ch, "int");
	    i = atoi((char*)c);
	    NEEDATR(ch, "name");
	    if (v == NULL)
		_values = v = new TValue(i, SC::U2S(c), NULL);
	    else
		v = v->Append(new TValue(i, SC::U2S(c), NULL));
	    xmlFree(c);
	    c = NULL;
	}
	else if (!strcmp((char *) ch->name, "raport")) {
	    double o = -1.0;
	    c = xmlGetNoNsProp(ch, X "order");
	    if (c) {
		wstringstream ss;
		ss.imbue(locale("C"));
		ss << SC::U2S(c);
		ss >> o;
	    }
	    NEEDATR(ch, "title");
	    xmlChar* d = xmlGetNoNsProp(ch, X"description");

	    AddRaport(SC::U2S(c),
			d != NULL ? SC::U2S(d) : std::wstring(),
		    	o);

	    xmlFree(c);
	    xmlFree(d);
	    c = NULL;
	}
	else if (!strcmp((char *) ch->name, "draw")) {
	    TDraw *d = TDraw::parseXML(ch);
	    if (d != NULL)
		AddDraw(d);
	}
    }

    if (!hasAttribute("prec") && !_values) {
		sz_log(0, (std::string("No precision attribute in param") + SC::S2A(GetName())).c_str());
		return 1;
    }

    for (ch = node->children; ch; ch = ch->next)
	if (!strcmp((char *) ch->name, "define"))
	    break;

    if (ch == NULL) {
	if (_parentUnit)
	    return 0;
sz_log(1,
	    "XML file error: no definition for defined parameter found (line %ld)",
	    xmlGetLineNo(node));
	return 1;
    }

    _param_type = ParamType::REAL;

    NEEDATR(ch, "type");
    if (!strcmp((char*)c, "RPN")) {
	_ftype = FormulaType::RPN;
    }
    else if (!strcmp((char*)c, "DRAWDEFINABLE")) {
	_ftype = FormulaType::DEFINABLE;
	_param_type = ParamType::DEFINABLE;
    }
#ifndef NO_LUA
    else if (!strcmp((char*)c, "LUA")) {
	_param_type = ParamType::LUA;
	NEEDATR(ch, "lua_formula");
	if (!strcmp((char*)c, "va"))
		_ftype = FormulaType::LUA_VA;
	else if (!strcmp((char*)c, "av"))
		_ftype = FormulaType::LUA_AV;
	else if (!strcmp((char*)c, "ipc"))
		_ftype = FormulaType::LUA_IPC;
	else {
	    sz_log(1,
	    	"XML file error: unknown value for 'lua_formula' attribute (line %ld)",
	    	xmlGetLineNo(node));
	    return 1;
	}

	if (_ftype == FormulaType::LUA_VA || _ftype == FormulaType::LUA_AV) {
		xmlChar* offset = xmlGetNoNsProp(ch, X "lua_start_offset");
		if (offset) {
			_lua_start_offset = atoi((char*) offset);
			xmlFree(offset);
		}
		offset = xmlGetNoNsProp(ch, X "lua_end_offset");
		if (offset) {
			_lua_end_offset = atoi((char*) offset);
			xmlFree(offset);
		}


		xmlChar *start_dt = xmlGetNoNsProp(ch, X "lua_start_date_time");
		if (start_dt) {
				boost::posix_time::ptime star_date_time = boost::posix_time::not_a_date_time;
				try {
					star_date_time = boost::posix_time::time_from_string((char *)start_dt);
				} catch(std::exception e) {
					star_date_time = boost::posix_time::not_a_date_time;
				}

				if (star_date_time == boost::posix_time::not_a_date_time) {
					sz_log(0, "XML file error: lua_start_date_time attribute has invalid value - expected format \"YYYY-MM-DD hh:mm\" (line %ld)",
						xmlGetLineNo(ch));
				} else {
					struct tm t = to_tm(star_date_time);
					_lua_start_date_time = timegm(&t);
				}

				xmlFree(start_dt);

		}

	}
    }
#endif
	if (c) {
		xmlFree(c);
		c = NULL;
	}

    else { // unknown type
sz_log(1,
	    "XML file error: unknown value for 'type' attribute (line %ld)",
	    xmlGetLineNo(node));
	return 1;
    }

#ifndef NO_LUA
    xmlNodePtr cld;
    for (cld = ch->children; cld; cld = cld->next)
	if (!strcmp((char*)cld->name, "script"))
	    break;

    if (cld != NULL) {
	if (cld->children != NULL) {
	    _script = xmlNodeListGetString(cld->doc, cld->children, 1);
	} else {
	    sz_log(1, "Parse error, line (%ld), LUA definable param has no child element named script", xmlGetLineNo(ch));
	    return 1;
	}
    }

    if (_param_type == ParamType::LUA)
	    return 0;
#endif
     NEEDATR(ch, "formula");
     _formula = SC::U2S(c);

     xmlFree(c);
     c = NULL;

    return 0;

#undef NEEDATR
#undef NOATR
#undef X
}

TParam *
TParam::GetNextGlobal() const
{
	auto next = GetNext();
    if (next != nullptr)
		return next;

    /* For defined params no search in devices and units */
    if (_parentUnit == NULL) {
	if (GetFormulaType() == FormulaType::DEFINABLE 
#ifndef NO_LUA			
			|| GetFormulaType() == FormulaType::LUA_VA || GetFormulaType() == FormulaType::LUA_AV
#endif
			)
	    return NULL;
	return GetSzarpConfig()->GetFirstDrawDefinable();
    }
    assert(GetDevice() != NULL);

    TParam *p;

    for (TUnit * u = _parentUnit->GetNext(); u; u = u->GetNext()) {
	p = u->GetFirstParam();
	if (p)
	    return p;
    }

    for (TDevice * d = GetDevice()->GetNext(); d; d = d->GetNext()) {
	    for (TUnit * u = d->GetFirstUnit(); u; u = u->GetNext()) {
			p = u->GetFirstParam();
			if (p)
				return p;
		}
	}
    p = GetSzarpConfig()->GetFirstDefined();
    if (p == NULL)
	p = GetSzarpConfig()->GetFirstDrawDefinable();
    return p;
}

TDevice *
TParam::GetDevice() const
{
    if (_parentUnit == NULL)
	return NULL;
    return _parentUnit->GetDevice();
}

TParam *
TParam::GetNthParam(int n, bool global)
{
    assert(n >= 0);
    TParam *p = this;
    for (int i = 0; (i < n) && p; i++) {
		if (global) {
			p = p->GetNextGlobal();
		} else {
			p = p->GetNext();
		}
	}
    
    return p;
}

unsigned int
TParam::GetVirtualBaseInd()
{
    return ((_baseInd >= 0) ? (unsigned int) _baseInd : GetIpcInd());
}

void
TParam::SetShortName(const std::wstring& name)
{
    _shortName = name;
}

void
TParam::SetDrawName(const std::wstring &name)
{
    _drawName = name;
}

void 
TParam::Configure(const std::wstring name, const std::wstring shortName, const std::wstring drawName, const std::wstring unit,
			TValue *values, int prec, int baseInd,
			int inbase,
			short forbidden_val)
{
    _name = name;
    _shortName = shortName;
    _drawName = drawName;
    _unit = unit;
    delete _values;
    _values = values;
	storeAttribute("prec", prec);
	if (forbidden_val != SZARP_NO_DATA) storeAttribute("forbidden", forbidden_val);
    _baseInd = baseInd;
    
    if (-1 != inbase)
	_inbase = inbase;
    else
	_inbase = baseInd >= 0;
    
    if (_inbase)
	_param_type = ParamType::REAL;
    else {
	_param_type = ParamType::DEFINABLE;
    }
}

TParam::~TParam()
{
	delete _raports;
	delete _draws;
	delete _values;
}

void
TParam::AddRaport(const std::wstring& title, const std::wstring& descr, double order)
{
    if (_raports == NULL)
	_raports = new TRaport(this, title, descr, order);
    else
	_raports->Append(new TRaport(this, title, descr, order));
}

void
TParam::PrintIPCValue(wchar_t *buffer, size_t buffer_size, SZB_FILE_TYPE value, const wchar_t *no_data_str)
{
    if (value == SZARP_NO_DATA) {
	SWPRINTF(buffer, buffer_size, L"%s", no_data_str);
    } else {
	    int prec = GetPrec();
	    SZBASE_TYPE d = value;
	    for (int i = 0; i < prec; i++) {
		    d /= 10;
	    }
	    PrintValue(buffer, buffer_size, d, no_data_str);
    }
}
	
SZB_FILE_TYPE 
TParam::ToIPCValue(SZBASE_TYPE value)
{
	for (int i = 0; i < GetPrec(); i++) {
		value *= 10;
	}
	return (SZB_FILE_TYPE) nearbyint(value);
}

void
TParam::PrintValue(wchar_t *buffer, size_t buffer_size, SZBASE_TYPE value, const std::wstring& no_data_str, int prec)
{
	TValue *v;
	if (IS_SZB_NODATA(value)) {
		SWPRINTF(buffer, buffer_size, L"%s", no_data_str.c_str());
		return;
	}
	else if ((v = GetFirstValue()) != NULL) {
		v = v->SearchValue((int)value);
		const wchar_t *c = NULL;
		if (v != NULL)
			 c = v->GetString().c_str();
		if (c != NULL) {
			 SWPRINTF(buffer, buffer_size, L"%s", c);
			 return;
		}
	}
	prec = std::max(prec, GetPrec());
	SWPRINTF(buffer, buffer_size, L"%0.*f", prec, value);
}

std::wstring
TParam::PrintValue(SZBASE_TYPE value, const std::wstring& no_data_str, int prec)
{
	TValue *v;
	if (IS_SZB_NODATA(value))
		return no_data_str;
	else if ((v = GetFirstValue()) != NULL) {
		v = v->SearchValue((int)value);
		if (v != NULL) {
			const std::wstring& c = v->GetString();
				 if (!c.empty())
					 return c;
		}
	}
	prec = std::max(prec, GetPrec());
	std::wstringstream ss;
	ss << std::fixed << std::setprecision(prec) << value;
	return ss.str();
}

int TParam::IsReadable() {
	return IsInBase() || (GetFormulaType() == FormulaType::DEFINABLE) || (GetFormulaType() == FormulaType::LUA_VA) || (GetFormulaType() == FormulaType::LUA_AV) || (GetFormulaType() == FormulaType::LUA_IPC);
}

std::wstring TParam::GetGlobalName() const {
	SzarpConfigInfo *sc = _parentSzarpConfig; 
	assert (sc != NULL);
	return sc->GetPrefix() + L":" + _name;
}

namespace szarp {

std::wstring substituteWildcards(const std::wstring &name, const std::wstring &ref)
{
    int i;
    std::wstring w1, w2;

    assert(!name.empty());
    assert(!ref.empty());
    w1 = name;
    w2 = ref;
    if (name.length() > 4 && name.substr(0, 4) == L"*:*:") {
	i = w2.rfind(':');
	w1.replace(0, 3, w2.substr(0, i));
    } else if (name.length() > 2 && name.substr(0, 2) == L"*:") {
	i = w2.find(':');
	w1.replace(0, 1, w2.substr(0, i));
    }
    return w1;
}

}
