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

#include "ipkcontainer.h"
#include "parcook_cfg.h"
#include "line_cfg.h"
#include "conversion.h"
#include "liblog.h"
#include "szdefines.h"
#include "szbase/szbdefines.h"
#include "szbase/szbname.h"

unsigned int
ParamFormulaInfo::GetIpcInd() const
{
    SzarpConfigInfo *s = _parentSzarpConfig;
    assert(s != NULL);
    
    TParam *p = s->GetFirstParam();
    unsigned int i = 0;	
    for (; p && (p != this); i++, p = p->GetNextGlobal());
    assert(p != NULL);

    return i;
}

TSzarpConfig *ParamFormulaInfo::GetSzarpConfig() const
{
    if (_parentSzarpConfig)
	return _parentSzarpConfig;

	return NULL;
}


void
ParamFormulaInfo::SetFormula(const std::wstring& f, FormulaType type)
{
    if (!_parsed_formula.empty())
	_parsed_formula = std::wstring();

    _formula = f;
    _ftype = type;

    if (FormulaType::DEFINABLE == _ftype) {
	_prepared = false;
	PrepareDefinable();
    }
    else
	_param_type = ParamType::REAL;
}

void
ParamFormulaInfo::CheckForNullFormula()
{
    if (_formula.empty())
	_formula = L"null ";
}


ParamFormulaInfo::~ParamFormulaInfo() {
	xmlFree(_script);
}

const std::wstring&
ParamFormulaInfo::GetDrawFormula()
{
	if( _ftype != FormulaType::DEFINABLE || _formula.empty() ) {
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
	    absolute_name = szarp::substituteWildcards(fr_name, _name);
	    tp = _parentSzarpConfig->getParamByName(absolute_name);
	    if (tp == NULL) {
		sz_log(0,
		    "definable.cfg: parameter with name %s not found in formula for parameter %s",
		    SC::S2L(absolute_name).c_str(),
		    SC::S2L(_name).c_str());

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
    if (std::wstring::npos != pc) {
	fprintf(stderr, "Warning! Use 'X' instead of -32768 in DRAWDEFINABLE formulas\n");
	while ((pc = _parsed_formula.find(L"#-32768")) != std::wstring::npos)
		_parsed_formula.replace(pc, 7, L"X");
    }
    
    sz_log(9, "G: _parsed_formula: (%p) %s", this, SC::S2L(_parsed_formula).c_str());

    return _parsed_formula;
}


#define DEFINABLE_STACK_SIZE 200

double
ParamFormulaInfo::calculateConst(const std::wstring& formula)
{
    sz_log(10, "$ form: %s", SC::S2L(formula).c_str());

    double stack[DEFINABLE_STACK_SIZE];
    double tmp;

    const wchar_t *chptr = formula.c_str();

    short sp = 0;
    
    do { 
	if (iswdigit(*chptr)) {
	    if (sp >= DEFINABLE_STACK_SIZE) {
		sz_log(0,
			"Nastapilo przepelnienie stosu przy liczeniu formuly %s",
			SC::S2L(formula).c_str());
		return SZB_NODATA;
	    }
            wchar_t *eptr;
	    stack[sp++] = wcstod(chptr, &eptr);
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
				SC::S2L(formula).c_str());
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
			sz_log(4, "WARRNING: dzielenie przez zero w formule: %s)", SC::S2L(formula).c_str());
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
				SC::S2L(formula).c_str());
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
				SC::S2L(formula).c_str());
			return SZB_NODATA;
		    }
		    stack[sp++] = SZB_NODATA;
		    break;
		default:
		    if (iswspace(*chptr))
			    break;
		    sz_log(10, "Nie obslugiwany operator %s\n w %s", SC::S2L(chptr).c_str(), SC::S2L(formula).c_str());
		    return SZB_NODATA;
	    }
	}
    } while (chptr && *(++chptr) != 0);
    
    if (sp-- < 0) {
	sz_log(0, "ERROR: za malo danych na stosie p: %s, f: %s", SC::S2L(_name).c_str(), SC::S2L(formula).c_str());
	return SZB_NODATA;
    }
    
    return stack[0] / pow(10.0, GetPrec());
}

void
ParamFormulaInfo::PrepareDefinable()
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
	    sz_log(0, "Error in formula = %s (param: %s)", SC::S2L(_formula).c_str(), SC::S2L(_name).c_str());
	    _f_cache.clear();
		throw TCheckException();
	}

	std::wstring absolute_name = szarp::substituteWildcards(_formula.substr(sch, ech - sch), _name);

	sz_log(9, "absoluteName: %ls", absolute_name.c_str());

	tp = _parentSzarpConfig->getParamByName(absolute_name);
	if (NULL == tp) {
	    sz_log(0, "Error in formula '%s', param: '%s' not found", SC::S2U(_formula).c_str(), SC::S2L(absolute_name).c_str());
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

            _f_cache.push_back(tp);
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
	    wchar_t* eptr;
	    _f_const_value = wcstod(_formula.c_str(), &eptr) / pow(10.0, GetPrec());
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
		_param_type = ParamType::COMBINED;
	}
}

std::wstring
ParamFormulaInfo::GetParcookFormula(bool ignoreIndexes, std::vector<std::wstring>* ret_params_list)
{
	if( _ftype != FormulaType::RPN )
		return std::wstring();

	int st, b, e, l;
    	wchar_t ch = 0;
	std::wstring c, c2;
	std::wostringstream str;
	TParam *p;

	if (GetFormulaType() != FormulaType::RPN)
		return std::wstring();
	if (GetFormula().empty()) {
		if (_parentUnit)
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
		c2 = szarp::substituteWildcards(c, GetName());
		p = _parentSzarpConfig->getParamByName(c2);
		if (p == NULL && ignoreIndexes) 
			p = IPKContainer::GetObject()->GetParam(c2);
		if (p == NULL) {
		   sz_log(0, "GetParcookFormula: parameter '%s' not found in formula for '%s'", SC::S2L(c2).c_str(), SC::S2L(GetName()).c_str());
		   throw TCheckException();
		}

		if (ret_params_list)
			ret_params_list->push_back(c2);

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

const unsigned char* ParamFormulaInfo::GetLuaScript() const {
	return _script;
}

void ParamFormulaInfo::SetLuaScript(const unsigned char* script) {
	free(_script);
	_script = (unsigned char*) strdup((char*)script);

	_lua_function_reference = LUA_NOREF;
}	

void ParamFormulaInfo::SetLuaParamRef(int ref) {
	_lua_function_reference = ref;
}

int ParamFormulaInfo::GetLuaParamReference() const {
	return _lua_function_reference;
}

#if LUA_PARAM_OPTIMISE

LuaExec::Param* ParamFormulaInfo::GetLuaExecParam() {
	return _opt_lua_param;
}

void ParamFormulaInfo::SetLuaExecParam(LuaExec::Param *param) {
	_opt_lua_param = param;
}

#endif
#endif
