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
#endif

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
	_param_type = P_REAL;
    else
	_param_type = P_DEFINABLE;
}

const std::wstring& TParam::GetSzbaseName()
{
	if (_szbase_name.empty())
		_szbase_name = wchar2szb(GetName());
	return _szbase_name;
}

void
TParam::SetFormulaType(FormulaType type)
{
    _ftype = type;

    if (TParam::DEFINABLE == _ftype)
	PrepareDefinable();
}

void
TParam::SetNewDefinable(bool is_new_def)
{
    _is_new_def = (TParam::P_DEFINABLE != _param_type ? 0 : is_new_def);
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

TAnalysis* 
TParam::AddAnalysis(TAnalysis* a)
{
	if (_analysis == NULL) {
		_analysis = a;
		return _analysis;
	}
	return _analysis->Append(a);
}

int TParam::parseXML(xmlTextReaderPtr reader)
{
	TValue* v = NULL;

	XMLWrapper xw(reader,true);

	const char* ignored_trees[] = { "doc", "editable", 0 };
	xw.SetIgnoredTrees(ignored_trees);

	bool isEmptyTag = xw.IsEmptyTag();

	const char* need_attr_param[] = { "name", 0 };
	if (!xw.AreValidAttr(need_attr_param)) {
		throw XMLWrapperException();
	}

	bool isPrecAttr = false;

	for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
		const xmlChar *attr = xw.GetAttr();
		try {
			if (xw.IsAttr("name")) {
				_name = SC::U2S(attr);
			} else
			if (xw.IsAttr("short_name")) {
				_shortName = SC::U2S(attr);
			} else
			if (xw.IsAttr("draw_name")) {
				_drawName = SC::U2S(attr);
			} else
			if (xw.IsAttr("unit")) {
				_unit = SC::U2S(attr);
			} else
			if (xw.IsAttr("sum_unit")) {
				_sum_unit = SC::U2S(attr);
			} else
			if (xw.IsAttr("sum_divisor")) {
				_sum_divisor = boost::lexical_cast<double>(attr);
			} else
			if (xw.IsAttr("period")) {
				SetPeriod(boost::lexical_cast<int>(attr));
			} else
			if (xw.IsAttr("base_ind")) {
				if (!strcmp((char*)attr, "auto"))
					SetAutoBase();
				else
					SetBaseInd(boost::lexical_cast<int>(attr));
			} else
			if (xw.IsAttr("prec")) {
				_prec = boost::lexical_cast<int>(attr);
				isPrecAttr = true;
			} else
			if (xw.IsAttr("data_type")) {
				if (!xmlStrcmp(attr, BAD_CAST"float"))
					_dataType = FLOAT;
				else if (!xmlStrcmp(attr, BAD_CAST "double"))
					_dataType = DOUBLE;
				else if (!xmlStrcmp(attr, BAD_CAST "short"))
					_dataType = SHORT;
				else if (!xmlStrcmp(attr, BAD_CAST "int"))
					_dataType = INT;
				else
					xw.XMLError("XML file error: param data_type has invalid value, expected one of: float, double, short, int");
			} else
			if (xw.IsAttr("time_type")) {
				if (!xmlStrcmp(attr, BAD_CAST "second"))
					_timeType = SECOND;
				else if (!xmlStrcmp(attr, BAD_CAST "nanosecond"))
					_timeType = NANOSECOND;
				else
					xw.XMLError("XML file error: param time_type has invalid value, expected one of: second, nanosecond");
			} else {
				xw.XMLWarningNotKnownAttr();
			}
		} catch (boost::bad_lexical_cast &) {
			xw.XMLErrorWrongAttrValue();
		}
	}

	if (_parentSzarpConfig)
		_parentSzarpConfig->AddParamToNamesCache(this);

	if (isEmptyTag)
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
		if (xw.IsTag("analysis")) {
			if (xw.IsBeginTag()) {
			TAnalysis* a = TAnalysis::parseXML(reader);
			if (a != NULL)
				AddAnalysis(a);
			}
		} else
		if (xw.IsTag("define")) {
			if (xw.IsBeginTag()) {
				_param_type = TParam::P_REAL;

				const char* need_attr_def[] = { "type", 0 };
				if (!xw.AreValidAttr(need_attr_def)) {
					throw XMLWrapperException();
				}

				for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
					const xmlChar *attr = xw.GetAttr();
					try {
						if (xw.IsAttr("type")) {
							if (!strcmp((char*)attr, "RPN")) {
								_ftype = RPN;
							}else 
							if (!strcmp((char*)attr, "DRAWDEFINABLE")) {
								_ftype = DEFINABLE;
								_param_type = TParam::P_DEFINABLE;
							}
#ifndef NO_LUA
							else
							 if (!strcmp((char*)attr, "LUA")) {
								_param_type = TParam::P_LUA;
							}
						} else // end "type"
						if (xw.IsAttr("lua_formula")) {
							if (!strcmp((char*)attr, "va"))
								_ftype = LUA_VA;
							else if (!strcmp((char*)attr, "av"))
								_ftype = LUA_AV;
							else if (!strcmp((char*)attr, "ipc"))
								_ftype = LUA_IPC;
							else {
								xw.XMLError("XML file error: unknown value for 'lua_formula' attribute");
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
								xw.XMLError("XML file error: lua_start_date_time attribute has invalid value - expected format \"YYYY-MM-DD hh:mm\"");
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
						} else
						if (xw.IsAttr("new_def")) {
							_is_new_def = xmlStrEqual(attr, (xmlChar *) "yes");
						}else 	{
							xw.XMLWarningNotKnownAttr();
						}
					} catch (boost::bad_lexical_cast &) {
						xw.XMLErrorWrongAttrValue();
					}
				} // end FORALLATTR
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

	if (!isPrecAttr && !_values) {
		xw.XMLError("Attribute 'prec' in 'param'");
	}

	return 0;
}

int
TParam::parseXML(xmlNodePtr node)
{
    unsigned char *c = NULL;
    int i;
    xmlNodePtr ch;
    TValue *v = NULL;

#define NOATR(p, n) \
	{ \
	sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, SC::U2A(p->name).c_str(), xmlGetLineNo(p)); \
			return 1; \
	}
#define NEEDATR(p, n) \
	if (c) xmlFree(c); \
	c = xmlGetNoNsProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar*)

    NEEDATR(node, "name");
    _name = SC::U2S(c);
    xmlFree(c);

    if (_parentSzarpConfig)
	_parentSzarpConfig->AddParamToNamesCache(this);

    
    c = xmlGetNoNsProp(node, X "short_name");
    if (c) {
	    _shortName = SC::U2S(c);
	    xmlFree(c);
    }

    c = xmlGetNoNsProp(node, X "draw_name");
    if (c) {
	    _drawName = SC::U2S(c);
	    xmlFree(c);
    }

    c = xmlGetNoNsProp(node, X "unit");
    if (c) {
	    _unit = SC::U2S(c);
	    xmlFree(c);
    }

    c = xmlGetNoNsProp(node, X "sum_unit");
    if (c) {
	    _sum_unit = SC::U2S(c);
	    xmlFree(c);
    }

    c = xmlGetNoNsProp(node, X "sum_divisor");
    if (c) {
	    wstringstream ss;
	    ss.imbue(locale("C"));
	    ss << SC::U2S(c);
	    ss >> _sum_divisor;
	    xmlFree(c);
    }

    c = xmlGetNoNsProp(node, X "period"); 
    if (c) {
	int tmp = atoi((char*)c);
        if (tmp <= 0)
	    sz_log(1,
	        "XML file warning: invalid vallue of period attribute (line %ld)",
	        xmlGetLineNo(node));
	else
	    SetPeriod(tmp);

	xmlFree(c);
    }

    c = xmlGetNoNsProp(node, X "base_ind");
    if (c) {
	if (!strcmp((char*)c, "auto"))
	    SetAutoBase();
	else
	    SetBaseInd(atoi((char*)c));
	xmlFree(c);
	c = NULL;
    }
    

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
	else if (!strcmp((char *)ch->name, "analysis")) {
	    TAnalysis* a = TAnalysis::parseXML(ch);
	    if (a != NULL)
		AddAnalysis(a);
	}
    }

    if (!_values) {
	NEEDATR(node, "prec"); _prec = atoi((char*)c);
	free(c);
	c = NULL;
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

    _param_type = TParam::P_REAL;

    NEEDATR(ch, "type");
    if (!strcmp((char*)c, "RPN")) {
	_ftype = RPN;
    }
    else if (!strcmp((char*)c, "DRAWDEFINABLE")) {
	_ftype = DEFINABLE;
	_param_type = TParam::P_DEFINABLE;
    }
#ifndef NO_LUA
    else if (!strcmp((char*)c, "LUA")) {
	_param_type = TParam::P_LUA;
	NEEDATR(ch, "lua_formula");
	if (!strcmp((char*)c, "va"))
		_ftype = LUA_VA;
	else if (!strcmp((char*)c, "av"))
		_ftype = LUA_AV;
	else if (!strcmp((char*)c, "ipc"))
		_ftype = LUA_IPC;
	else {
	    sz_log(1,
	    	"XML file error: unknown value for 'lua_formula' attribute (line %ld)",
	    	xmlGetLineNo(node));
	    return 1;
	}

	if (_ftype == LUA_VA || _ftype == LUA_AV) {
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
	    xmlChar * ndef = xmlGetNoNsProp(ch, (xmlChar *) "new_def");
            if (NULL != ndef) {
  	        _is_new_def = xmlStrEqual(ndef, (xmlChar *) "yes");
	    	xmlFree(ndef);
            }
	} else {
	    sz_log(1, "Parse error, line (%ld), LUA definable param has no child element named script", xmlGetLineNo(ch));
	    return 1;
	}
    }

    if (_param_type == P_LUA)
	    return 0;
#endif
     NEEDATR(ch, "formula");
     _formula = SC::U2S(c);

     xmlFree(c);
     c = NULL;

     if (DEFINABLE == _ftype) {
	xmlChar * ndef = xmlGetNoNsProp(ch, (xmlChar *) "new_def");
       	if (NULL != ndef) {
	    _is_new_def = xmlStrEqual(ndef, (xmlChar *) "yes");
	    xmlFree(ndef);
	}
    }
	    
    return 0;

#undef NEEDATR
#undef NOATR
#undef X
}

xmlNodePtr
TParam::generateXMLNode(void)
{
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define BUF X(buffer)
    xmlNodePtr r, c;
    char buffer[10];

    r = xmlNewNode(NULL, (unsigned char *) "param");
    xmlSetProp(r, X "name", SC::S2U(_name).c_str());
    if (!_shortName.empty())
	xmlSetProp(r, X "short_name", SC::S2U(_shortName).c_str());
    
    if (!_drawName.empty())
	xmlSetProp(r, X "draw_name", SC::S2U(_drawName).c_str());
    
    if (!_unit.empty())
	xmlSetProp(r, X "unit", SC::S2U(_unit).c_str());
    
    if (_values)
	for (TValue * v = _values; v; v = v->GetNext())
	    xmlAddChild(r, v->generateXMLNode());
    else {
	ITOA(_prec);
	xmlSetProp(r, X "prec", BUF);
    }
    
    if (_baseInd >= 0) {
	ITOA(_baseInd);
	xmlSetProp(r, X "base_ind", BUF);
    }
    else if (IsAutoBase()) {
	xmlSetProp(r, X "base_ind", X "auto");
    }

#ifndef NO_LUA
    if (GetType() == P_LUA) {
	c = xmlNewChild(r, NULL, X "define", NULL);
	xmlSetProp(c, X "type", X "LUA");
	switch (_ftype) {
		case LUA_VA:
			xmlSetProp(c, X "lua_formula", X "va");
			break;
		case LUA_AV:
			xmlSetProp(c, X "lua_formula", X "av");
			break;
		case LUA_IPC:
			xmlSetProp(c, X "lua_formula", X "ipc");
			break;
		default:
			assert(false);
	}
	c = xmlNewChild(c, NULL, X "script", NULL);
	assert(_script != NULL);
	xmlNodePtr cd = xmlNewCDataBlock(r->doc, _script, strlen((char*)_script));
	xmlAddChild(c, cd);
    } else
#endif
    if (!_formula.empty()) {
	c = xmlNewChild(r, NULL, X "define", NULL);
	switch (GetFormulaType()) {
	case RPN:
	    xmlSetProp(c, X "type", X "RPN");
	    break;
	case DEFINABLE:
	    xmlSetProp(c, X "type", X "DRAWDEFINABLE");
	    if (_is_new_def)
		xmlSetProp(c, X "new_def", X "yes");
	    break;
	default:
	    break;
	}
	xmlSetProp(c, X "formula", SC::S2U(_formula).c_str());
    }
    
    if (_raports) {
	for (TRaport * i = _raports; i; i = i->GetNext())
	    xmlAddChild(r, i->GenerateXMLNode());
    }
    
    if (_draws) {
	for (TDraw * i = _draws; i; i = i->GetNext())
	    xmlAddChild(r, i->GenerateXMLNode());
    }
    
    if (_analysis) {
	for (TAnalysis *i = _analysis; i; i = i->GetNext()) { 
	    xmlNodePtr analisys_node = i->generateXMLNode();
	    if (analisys_node)
		    xmlAddChild(r, analisys_node);
	}
    }
    
    return r;

#undef X
#undef ITOA
#undef BUF
}

TParam *
TParam::GetNext(bool global)
{
    if (_next)
	return _next;

    if (!global)
	return NULL;

    /* For defined params no search in devices and units */
    if (_parentUnit == NULL) {
	if (GetFormulaType() == DEFINABLE 
#ifndef NO_LUA			
			|| GetFormulaType() == LUA_VA || GetFormulaType() == LUA_AV
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
    for (TRadio * r = GetRadio()->GetNext(); r; r = r->GetNext())
	for (TUnit * u = r->GetFirstUnit(); u; u = r->GetNextUnit(u)) {
	    p = u->GetFirstParam();
	    if (p)
		return p;
	}
    for (TDevice * d = GetDevice()->GetNext(); d; d = d->GetNext())
	for (TRadio * r = d->GetFirstRadio(); r; r = d->GetNextRadio(r))
	    for (TUnit * u = r->GetFirstUnit(); u; u = r->GetNextUnit(u)) {
		p = u->GetFirstParam();
		if (p)
		    return p;
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

TRadio *
TParam::GetRadio()
{
    if (_parentUnit == NULL)
	return NULL;
    return _parentUnit->GetRadio();
}

TSzarpConfig *
TParam::GetSzarpConfig() const
{
    if (_parentSzarpConfig)
	return _parentSzarpConfig;

    if (_parentUnit == NULL)
	    return NULL;

    return _parentUnit->GetSzarpConfig();
}

void
TParam::SetFormula(const std::wstring& f, FormulaType type)
{
    if (!_parsed_formula.empty())
	_parsed_formula = std::wstring();

    _formula = f;
    _ftype = type;

    if (TParam::DEFINABLE == _ftype) {
	_prepared = false;
	PrepareDefinable();
    }
    else
	_param_type = TParam::P_REAL;
}

TParam *
TParam::GetNthParam(int n, bool global)
{
    assert(n >= 0);
    TParam *p = this;
    for (int i = 0; (i < n) && p; i++)
	p = p->GetNext(global);
    
    return p;
}

void TParam::SetNext(TParam *p) {
	_next = p;	
}

TParam *
TParam::Append(TParam * p)
{
    TParam *t = this;
    
    while (t->_next)
	t = t->_next;
    
    t->_next = p;

    return p;
}

unsigned int
TParam::GetIpcInd()
{
    TSzarpConfig *s = GetSzarpConfig();
    assert(s != NULL);
    
    TParam *p = s->GetFirstParam();
    unsigned int i = 0;	
    for (; p && (p != this); i++, p = s->GetNextParam(p));
    assert(p != NULL);

    return i;
}

void
TParam::ConfigureUnknown(int id)
{
    std::wstringstream w;
    w << L"Unknown:Unknown:Unknown " <<  id;
    _name = w.str();
    _shortName = L"unk";
    _unit = L"-";
}

void
TParam::CheckForNullFormula()
{
    if (_formula.empty())
	_formula = L"null ";
}

unsigned int
TParam::GetVirtualBaseInd()
{
    return ((GetBaseInd() >= 0) ? (unsigned int) GetBaseInd() : GetIpcInd());
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
			int inbase)
{
    _name = name;
    _shortName = shortName;
    _drawName = drawName;
    _unit = unit;
    delete _values;
    _values = values;
    _prec = prec;
    _baseInd = baseInd;
    
    if (-1 != inbase)
	_inbase = inbase;
    else
	_inbase = baseInd >= 0;
    
    if (_inbase)
	_param_type = P_REAL;
    else {
	_param_type = TParam::P_DEFINABLE;
    }

    _prepared = false;
    _f_const = false;
    _f_N = false;
}

TParam::~TParam()
{
	delete _next;
	delete _raports;
	delete _draws;
	delete _values;
	delete _analysis;

    xmlFree(_script);
}

void
TParam::AddRaport(const std::wstring& title, const std::wstring& descr, double order)
{
    if (_raports == NULL)
	_raports = new TRaport(this, title, descr, order);
    else
	_raports->Append(new TRaport(this, title, descr, order));
}

const std::wstring&
TParam::GetDrawFormula() throw(TCheckException)
{
	if( _ftype != DEFINABLE || _formula.empty() ) {
		_parsed_formula.clear();
		return _parsed_formula;
	}

    if (!_parsed_formula.empty())
	return _parsed_formula;

    int index = 0,
	st = 0,
	e = 0;

    wchar_t ch;
    std::wstring fr_name, absolute_name;

    TParam *tp;

    int lenght = _formula.length();

    std::wstringstream tmp;

    while (index < lenght) {
	ch = _formula[index++];
	switch (st) {
	case 0:
	    if (iswdigit(ch) || ((ch == '-') && iswdigit(_formula[index]))) {
		tmp << L"#" << ch;
		st = 1;
		break;
	    }
	    if (ch == L'(') {
		st = 2;
		e = index;
		break;
	    }
	    tmp << ch;
	    break;
	case 1:
	    tmp << ch;
	    if (!isdigit(ch))
		st = 0;
	    break;
	case 2:
	    if (ch != L')')
		break;

	    fr_name = _formula.substr(e, index - 1 - e);
	    absolute_name = _parentSzarpConfig->absoluteName(fr_name, _name);
	    tp = _parentSzarpConfig->getParamByName(absolute_name);
	    if (tp == NULL) {
		sz_log(0,
		    "definable.cfg: parameter with name %s not found in formula for parameter %s",
		    SC::S2A(absolute_name).c_str(),
		    SC::S2A(_name).c_str());

		throw TCheckException();
	    }
	    tmp << tp->GetVirtualBaseInd();

	    st = 0;
	    break;
	}
    }
    _parsed_formula = tmp.str();

    // check old NO_DATA (-32768)
    size_t pc =_parsed_formula.find(L"#-32768");
    if (wstring::npos != pc) {
	fprintf(stderr, "Warning! Use 'X' instead of -32768 in DRAWDEFINABLE formulas\n");
	while ((pc = _parsed_formula.find(L"#-32768")) != wstring::npos)
		_parsed_formula.replace(pc, 7, L"X");
    }
    
    sz_log(9, "G: _parsed_formula: (%p) %s", this, SC::S2A(_parsed_formula).c_str());

    return _parsed_formula;
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
TParam::PrintValue(wchar_t *buffer, size_t buffer_size, SZBASE_TYPE value, const std::wstring& no_data_str)
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
    int prec = GetPrec();
    SWPRINTF(buffer, buffer_size, L"%0.*f", prec, value);
}

std::wstring
TParam::PrintValue(SZBASE_TYPE value, const std::wstring& no_data_str)
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
    int prec = GetPrec();
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(prec) << value;
    return ss.str();
}

int TParam::IsReadable() {
	return IsInBase() || (GetFormulaType() == DEFINABLE) || (GetFormulaType() == LUA_VA) || (GetFormulaType() == LUA_AV) || (GetFormulaType() == LUA_IPC);
}

#define DEFINABLE_STACK_SIZE 200

double
TParam::calculateConst(const std::wstring& formula)
{
    sz_log(10, "$ form: %s", SC::S2A(formula).c_str());

    double stack[DEFINABLE_STACK_SIZE];
    double tmp;

    const wchar_t *chptr = formula.c_str();

    short sp = 0;
    
    do { 
	if (iswdigit(*chptr)) {
	    if (sp >= DEFINABLE_STACK_SIZE) {
		sz_log(0,
			"Nastapilo przepelnienie stosu przy liczeniu formuly %s",
			SC::S2A(formula).c_str());
		return SZB_NODATA;
	    }
	    stack[sp++] = wcstod(chptr, NULL);
	    chptr = wcschr(chptr, L' ');
	}
	else {
	    switch (*chptr) {
		case L'&':
		    if (sp < 2)	/* swap */
			return SZB_NODATA;
		    if (IS_SZB_NODATA(stack[sp - 1]) || IS_SZB_NODATA(stack[sp - 2]))
			return SZB_NODATA;
		    
		    tmp = stack[sp - 1];
		    stack[sp - 1] = stack[sp - 2];
		    stack[sp - 2] = tmp;
		    break;
		case L'!':		
		    if (IS_SZB_NODATA(stack[sp - 1]))
			return SZB_NODATA;
		
		    // check stack size
		    if (DEFINABLE_STACK_SIZE <= sp) {
			sz_log(0,
				"Przepelnienie stosu dla formuly %s, w funkcji '!'",
				SC::S2A(formula).c_str());
			return SZB_NODATA;
		    }
		    
		    stack[sp] = stack[sp - 1];	/* duplicate */
		    sp++;
		    break;
		case L'+':
		    if (sp-- < 2)
			return SZB_NODATA;
		    if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
			return SZB_NODATA;
		    stack[sp - 1] += stack[sp];
		    break;
		case L'-':
		    if (sp-- < 2)
			return SZB_NODATA;
		    if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
			return SZB_NODATA;
		    stack[sp - 1] -= stack[sp];
		    break;
		case L'*':
		    if (sp-- < 2)
			return SZB_NODATA;
		    if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
			return SZB_NODATA;
		    stack[sp - 1] *= stack[sp];
		    break;		
		case L'/':
		    if (sp-- < 2)
			return SZB_NODATA;
		    if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1] ))
			return SZB_NODATA;
		    if (stack[sp] != 0.0)
			stack[sp - 1] /= stack[sp];
		    else {
			sz_log(4, "WARRNING: dzielenie przez zero w formule: %s)", SC::S2A(formula).c_str());
			return SZB_NODATA;
		    }
		    break;		
		case L'~':
		    if (sp-- < 2)	/* rowne */
			return SZB_NODATA;
		    if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
			return SZB_NODATA;
		    if (stack[sp - 1] == stack[sp])
			stack[sp - 1] = 1;
		    else
			stack[sp - 1] = 0;
		    break;
		case L'^':
		    if (sp-- < 2)	/* potega */
			return SZB_NODATA;
		    if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1] ))
			return SZB_NODATA;
		    
		    // fix - PP 
		    if (stack[sp] == 0.0) {
			// a^0 == 1 
			stack[sp - 1] = 1;
		    }
		    else if (stack[sp - 1] >= 0.0) {
			stack[sp - 1] = pow(stack[sp - 1], stack[sp]);
		    }
		    else {
			sz_log(4,
				"WARRNING: wykladnik potegi < 0 %s)",
				SC::S2A(formula).c_str());
			return SZB_NODATA;
		    }
		    break;
		case L'N':
		    if (sp-- < 2)	/* Sprawdzenie czy SZB_NODATA */
			return SZB_NODATA;
		    if (IS_SZB_NODATA(stack[sp - 1]))
			stack[sp - 1] = stack[sp];
		    break;
		case L'X':
		    // check stack size
		    if (DEFINABLE_STACK_SIZE <= sp) {
			sz_log(0,
				"Przepelnienie stosu dla formuly %s, w funkcji X",
				SC::S2A(formula).c_str());
			return SZB_NODATA;
		    }
		    stack[sp++] = SZB_NODATA;
		    break;
		default:
		    if (iswspace(*chptr))
			    break;
		    sz_log(10, "Nie obslugiwany operator %s\n w %s", SC::S2A(chptr).c_str(), SC::S2A(formula).c_str());
		    return SZB_NODATA;
	    }
	}
    } while (chptr && *(++chptr) != 0);
    
    if (sp-- < 0) {
	sz_log(0, "ERROR: za malo danych na stosie p: %s, f: %s", SC::S2A(_name).c_str(), SC::S2A(formula).c_str());
	return SZB_NODATA;
    }
    
    return stack[0] / pow(10.0, _prec);
}

void
TParam::PrepareDefinable() throw(TCheckException)
{
    if (_prepared)
	return;

    sz_log(9, "\n\n|| Preparing param: (%p) %ls", this, _name.c_str());

    _prepared = true;

    _f_N = false;
    _f_const = false;
    

    _parsed_formula.clear();
    
    _f_cache.clear();

    size_t sch = 0, ech;
    
    TParam * tp = NULL;

    if (_formula.empty()) {
	sz_log(0, "No formula for param = %ls", _name.c_str());
	return;
    }

    do { // search for params used in formula
	sch = _formula.find(L'(', sch);
	if (sch == std::wstring::npos)
	    break;
	sch++;
	
	ech = _formula.find(L')', sch);
	if (ech == std::wstring::npos) {
	    sz_log(0, "Error in formula = %s (param: %s)", SC::S2A(_formula).c_str(), SC::S2A(_name).c_str());
	    _f_cache.clear();
		throw TCheckException();
	}

	std::wstring absolute_name = _parentSzarpConfig->absoluteName(_formula.substr(sch, ech - sch), _name);

	sz_log(9, "absoluteName: %ls", absolute_name.c_str());

	tp = _parentSzarpConfig->getParamByName(absolute_name);
	if (NULL == tp) {
	    sz_log(0, "Error in formula '%s', param: '%s' not found", SC::S2A(_formula).c_str(), SC::S2A(absolute_name).c_str());
	    _f_cache.clear();
		throw TCheckException();
	}

	if (tp->IsDefinable()) {
	    tp->PrepareDefinable();

	    if (tp->IsConst()) {
		// replace param with const
		sz_log(9, "\nPARAM: %ls", _name.c_str());
		sz_log(9, " FORM: %ls", _formula.c_str());
		sz_log(9, " >> replace (%ls) with const [%f]", tp->GetName().c_str(), tp->GetConstValue());

		std::wstringstream wss;
		wss << (int) rint(tp->GetConstValue() * pow(10.0, tp->GetPrec()));
		std::wstring cv = wss.str();

		_formula.replace(sch - 1, ech - sch + 2, cv);
		sch += cv.length();

		sz_log(9, " NFORM: %ls", _formula.c_str());
	    }
	    else {
		_f_cache.push_back(tp);
	    }
	}
	else {
	    _f_cache.push_back(tp);
	}
    } 
    while (true);

    // check if param is constant
    if (0 == _f_cache.size()) {
	// equation or one value?
	if ((_formula.find(L' ') == std::wstring::npos) && (isdigit(_formula[0]))) {
	    // one value
	    _f_const_value = wcstod(_formula.c_str(), NULL) / pow(10.0, GetPrec());
	    _f_const = true;
	    sz_log(9, "Const (%ls), value: %f", _name.c_str(), _f_const_value);
	}
	else {
	    // equation, calculate it
	    _f_const_value = calculateConst(_formula);
	    if (!IS_SZB_NODATA(_f_const_value))
		_f_const = true; // mark as const only if DATA
	}
    }

    _parsed_formula = GetDrawFormula();
    
    sz_log(9, "F: _formula: %ls\nP: _parsed_formula: %ls\nN: params count: %zd",
	    _formula.c_str(), _parsed_formula.c_str(), _f_cache.size());
	
    // check if N function used
    if (_parsed_formula.find(L'N') != std::wstring::npos)
	_f_N = true;

    // check if combined parameter (function ':')
    if (_parsed_formula.find(L" :") != std::wstring::npos) {
	size_t ps;
	if ((ps = _parsed_formula.find_first_of(L"+-/*&$?<>~^")) != std::wstring::npos)
	    _param_type = TParam::P_COMBINED;
	else {
	    if (_parsed_formula[ps - 1] == L' ')
		_param_type = TParam::P_DEFINABLE;
	    else
		_param_type = TParam::P_COMBINED;
	}
    }
}

std::wstring
TParam::GetParcookFormula() throw(TCheckException)
{
	if( _ftype != RPN )
		return std::wstring();

	int st, b, e, l;
    	wchar_t ch = 0;
	std::wstring c, c2;
	std::wostringstream str;
	TParam *p;

	if (GetFormulaType() != TParam::RPN)
		return std::wstring();
	if (GetFormula().empty()) {
		if (GetParentUnit())
			return std::wstring();
		str << L"null #" << GetIpcInd() << " = # (" << GetName()
				<< L")" << std::endl;
		return str.str();
	}
	st = 0;
	b = e = 0;
	l = GetFormula().length();
	while (b < l) {
	    ch = (GetFormula())[b++];
	    switch (st) {
	    case 0:
		if (iswdigit(ch) || ((ch == L'-') && (iswdigit(GetFormula()[b])))) {
		    str << L"#" << ch;
		    st = 1;
		    break;
		}
		if (ch == L'(') {
		    st = 2;
		    e = b;
		    break;
		}
		str << ch;
		break;
	    case 1:
		str << ch;
		if (!iswdigit(ch))
		    st = 0;
		break;
	    case 2:
		if (ch != L')')
		    break;
		c = GetFormula().substr(e, b - e - 1);
		c2 = GetSzarpConfig()->absoluteName(c, GetName());
		p = GetSzarpConfig()->getParamByName(c2);
		if (p == NULL) {
		   sz_log(0, "GetParcookFormula: parameter '%s' not found in formula for '%s'", SC::S2A(c2).c_str(), SC::S2A(GetName()).c_str());
		   throw TCheckException();
		}
		str << p->GetIpcInd();
		st = 0;
		break;

	    }
	}
	if (ch != L' ')
		str << L" ";
	str <<  "#" << GetIpcInd() << L" = # (" << GetName() << L" )" << std::endl;
	return str.str();
}

#ifndef NO_LUA

const unsigned char* TParam::GetLuaScript() const {
	return _script;
}

void TParam::SetLuaScript(const unsigned char* script) {
	free(_script);
	_script = (unsigned char*) strdup((char*)script);

	_lua_function_reference = LUA_NOREF;
}	

void TParam::SetLuaParamRef(int ref) {
	_lua_function_reference = ref;
}

int TParam::GetLuaParamReference() const {
	return _lua_function_reference;
}

std::wstring TParam::GetGlobalName() const {
	TSzarpConfig *sc = GetSzarpConfig(); 
	assert (sc != NULL);
	return sc->GetPrefix() + L":" + _name;
}
#if LUA_PARAM_OPTIMISE

LuaExec::Param* TParam::GetLuaExecParam() {
	return _opt_lua_param;
}

void TParam::SetLuaExecParam(LuaExec::Param *param) {
	_opt_lua_param = param;
}

#endif
#endif
