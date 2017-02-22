#include <zmq.hpp>
#include "szbase/szbbase.h"
#include "funtable.h"
#include "szarp_config.h"
#include "szarp_base_common/lua_strings_extract.h"

float ChooseFun(float funid, float *parlst);
void putParamsFromString(const std::wstring& script_string, boost::regex& ipc_par_reg, const int& name_match_prefix, const int& name_match_sufix, std::vector<std::wstring>& ret_params);

namespace sz4 {
template <class time_type> time_type getTimeNow() { return *new time_type(NULL); }

template<> sz4::second_time_t getTimeNow<sz4::second_time_t>() {
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	return *new sz4::second_time_t(seconds);
}

template<> sz4::nanosecond_time_t getTimeNow<sz4::nanosecond_time_t>() {
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
	return *new sz4::nanosecond_time_t(seconds, nanoseconds);
}
}

class DefParamBase: public sz4::param_observer {
public:
	DefParamBase(TParam *param, size_t index): param(param), index(index), preparedParams() {}

	virtual ~DefParamBase() {}

	virtual void setVal(double) = 0;
	virtual void setZmqVal(zmqhandler&) const = 0;
	virtual void sendValueToParcook(short*) const = 0;

	virtual void executeAndUpdate(zmqhandler&) = 0;

	void param_data_changed(TParam *p) override { /* needed for sz4 to update */ }

	void subscribe_on_params(sz4::base* base) {
		std::vector<TParam*> params_to_register;

		const auto pushParamsToRegister = [&params_to_register](const std::wstring& name) {
			TParam* param_to_register = IPKContainer::GetObject()->GetParam(name);
			if (!param_to_register) {
				 // Something went wrong
				sz_log(0, "Error getting param %S to observed of LUA function", name.c_str());
				return;
			}
			if (std::find(params_to_register.begin(), params_to_register.end(), param_to_register) != params_to_register.end()) return;
			params_to_register.push_back(param_to_register);
			sz_log(6, "Added param %S to observed of LUA function in param s", param_to_register->GetGlobalName().c_str());
		};
		
		std::for_each(preparedParams.cbegin(), preparedParams.cend(), pushParamsToRegister);
		base->register_observer(this, params_to_register);
	}

protected:
	TParam *param;
	const size_t index;
	std::vector<std::wstring> preparedParams;
};

template <class data_type, class time_type>
class BaseParamImpl: public DefParamBase {
public:
	BaseParamImpl(TParam* param, size_t index): DefParamBase(param, index), val(0), t() {}
	void setZmqVal(zmqhandler& zmq) const override {
		zmq.set_value(index, sz4::getTimeNow<time_type>(), val);
	}

	void sendValueToParcook(short* read) const override {
		data_type val_to_parcook = val;

		for (int i = this->param->GetPrec(); i > 0; i--) {
			val_to_parcook*= 10;
		}

		if (val_to_parcook > std::numeric_limits<short>::max() || val_to_parcook < std::numeric_limits<short>::min()) read[index] = SZARP_NO_DATA;
		else read[index] = (short) val_to_parcook;
	}

	void setVal(double d) override { // copy the value
		val = (data_type) d;
	}

private:
	data_type val;
	mutable time_type t;

};

template <class v, class t>
class LuaParam: public BaseParamImpl<v,t> {
public:
	LuaParam(TParam* param, size_t index): BaseParamImpl<v,t>(param, index) {
		compileScript();
		extract_strings_from_formula(SC::U2S(this->param->GetLuaScript()), ref(this->preparedParams));
	}

	void compileScript() {
		std::ostringstream paramfunction;

		paramfunction << 
         "return function(t, pt)"          << std::endl <<
         "  local i = param_value"    << std::endl <<
         "  local p = szbase"         << std::endl <<
         "  local state = {}"         << std::endl <<
         "  local v = nil"            << std::endl <<
         this->param->GetLuaScript()  << std::endl <<
         "  return v"                 << std::endl <<
         "end"
		<< std::endl;

		std::string str = paramfunction.str();
		const char* content = str.c_str();

		lua_State* lua = Defdmn::getObject().get_lua_interpreter().lua();
		// Load string as LUA function named as the parameter
		int ret = luaL_loadbuffer(lua, content, strlen(content), (const char*)SC::S2U(this->param->GetName()).c_str());
		if (ret != 0) {
			throw SzException(std::string("Error compiling param ") + SC::S2A(this->param->GetName()) + lua_tostring(lua, -1));
		}

		// Test call
		ret = lua_pcall(lua, 0, 1, 0);
		if (ret != 0) {
			throw SzException(std::string("Error compiling param ") + SC::S2A(this->param->GetName()) + lua_tostring(lua, -1));
		}

		this->param->SetLuaParamRef(luaL_ref(lua, LUA_REGISTRYINDEX)); // register the lua function to call later
	}

	void executeAndUpdate(zmqhandler& zmq) override {
		assert(this->param->GetLuaParamReference() != LUA_NOREF);

		lua_State* lua = Defdmn::getObject().get_lua_interpreter().lua();
		lua_rawgeti(lua, LUA_REGISTRYINDEX, this->param->GetLuaParamReference());

		lua_pushnumber(lua, (double) sz4::getTimeNow<sz4::nanosecond_time_t>());
		lua_pushnumber(lua, SZARP_PROBE_TYPE::PT_SEC10);
		lua_pushnumber(lua, 1);

		int ret = lua_pcall(lua, 3, 1, 0);
		if (ret != 0) throw SzException(std::string("Execution error for param ") + SC::S2A(this->param->GetName()) + lua_tostring(lua, -1));

		double result = lua_isnil(lua, -1)? sz4::no_data<v>() : lua_tonumber(lua, -1);

		this->setVal(result);
		this->setZmqVal(zmq);
	}

};

unsigned char CalculNoData;

template <class v, class t>
class RPNParam: public BaseParamImpl<v,t> {
public:
	std::wstring tab;
	RPNParam(TParam* param, size_t index): BaseParamImpl<v,t>(param, index), tab() {
		tab = param->GetParcookFormula(true, &(this->preparedParams));
		if (tab.empty()) {
			throw SzException("Param "+SC::S2A(param->GetGlobalName())+" was ill-formed");
		}

		std::wstring::size_type idx = tab.rfind(L'#');
		if (idx != std::wstring::npos)
			tab = tab.substr(0, idx);
	}

	void executeAndUpdate(zmqhandler& zmq) override
	{
		const wchar_t *chptr;
		constexpr int SS = 30;
		double stack[SS+1];
		char nodata[SS+1];
		double tmp;
		short sp = 0;
		short parcnt;
		int NullFormula = 0;
		int p_no = 0;
		float val_op = 0.0f; 
		this->setVal(sz4::no_data<v>());

		CalculNoData = 0;

		chptr = tab.c_str();
		do {
			if (sp >= SS) {
				sz_log(0, "parcook: stack overflow after %td chars when calculating formula '%ls'",
						chptr - tab.c_str(), tab.c_str());
				CalculNoData = 1;
				return;
			}
			if (iswdigit(*chptr)) {
				tmp = wcstof(chptr, NULL);
				chptr = wcschr(chptr, L' ');
				double par_val = Defdmn::IPCParamValue(this->preparedParams[p_no++]);
				if (par_val == sz4::no_data<double>()) nodata[sp] = 1; // check if val is not nan
				else nodata[sp] = 0;

				stack[sp++] = par_val;	
			} else {
				switch (*chptr) {
					case L'&':
						if (sp < 2)	/* swap */
							break;
						tmp = stack[sp - 1];
						stack[sp - 1] = stack[sp - 2];
						stack[sp - 2] = tmp;
						break;
					case L'!':
						stack[sp] = stack[sp - 1];	/* duplicate 
										 */
						sp++;
						break;
					case L'$':
						if (sp-- < 2)	/* function call */
							break;
						parcnt = (short) rint(stack[sp - 1]);
						if (sp < parcnt + 1)
							break;
						val_op = (float) stack[sp - parcnt - 1];
						stack[sp - parcnt - 1] =
							ChooseFun((float) stack[sp],
								  &val_op);
						sp -= parcnt;
						break;
					case L'#':
						nodata[sp] = 0;
						stack[sp++] = wcstof(++chptr, NULL);
						chptr = wcschr(chptr, L' ');
						break;
					case L'i':
						if (*(++chptr) == L'f') {	
					/* warunek <par1> <par2> <cond> if 
					 * jesli <cond> != 0 zostawia <par1> 
					 * w innym wypadku zostawia <par2> */
							if (sp-- < 3)	
								break;
							if (nodata[sp]) {
								CalculNoData = 1;
								break;
							}
							if (stack[sp] == 0)
								stack[sp - 2] =
									stack[sp - 1];
							sp--;
						}
						break;
					case L'n':
						chptr += 3;
						NullFormula = 1;
						break;
					case L'+':
						if (sp-- < 2)
							break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						stack[sp - 1] += stack[sp];
						break;
					case L'-':
						if (sp-- < 2)
							break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						stack[sp - 1] -= stack[sp];
						break;
					case L'*':
						if (sp-- < 2)
							break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						stack[sp - 1] *= stack[sp];
						break;
					case L'/':
						if (sp-- < 2)
							break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						if (stack[sp] != 0.0)
							stack[sp - 1] /= stack[sp];
						else {
							stack[sp - 1] = 1;
							CalculNoData = 1;
						}
						break;
					case L'>':
						if (sp-- < 2)	/* wieksze */
							break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						if (stack[sp - 1] > stack[sp])
							stack[sp - 1] = 1;
						else
							stack[sp - 1] = 0;
						break;
					case L'<':
						if (sp-- < 2)	/* mniejsze */
							break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						if (stack[sp - 1] < stack[sp])
							stack[sp - 1] = 1;
						else
							stack[sp - 1] = 0;
						break;
					case L'~':
						if (sp-- < 2)	/* rowne */
							break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						if (stack[sp - 1] == stack[sp])
							stack[sp - 1] = 1;
						else
							stack[sp - 1] = 0;
						break;
					case L'N' :	/* no-data */
						if (sp-- < 2) 
							break;
						if (nodata[sp - 1]) {
							stack[sp - 1] = stack[sp];
							nodata[sp - 1] = nodata[sp];
						}
						break;
					case L'm' :    /* no-data if stack[sp -2] < stack [sp - 1], otherwise
								stack[sp-2]	*/
						if (sp -- < 2) break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						if (stack[sp-1] < stack[sp]) {
							nodata[sp-1] = 1;
						}
						break;
					case L'M' :    /* no-data if stack[sp -2] > stack [sp - 1], otherwise
								stack[sp-2]	*/
						if (sp -- < 2) break;
						if (nodata[sp] || nodata[sp-1]) {
							CalculNoData = 1;
							break;
						}
						if (stack[sp-1] > stack[sp]) {
							nodata[sp-1] = 1;
						}
						break;
					case L'=':
						if (sp-- < 2)
							break;
						if (CalculNoData)
							this->setVal(SZARP_NO_DATA);
						else if (NullFormula)
							break;
						else if (nodata[sp-1]) {
							this->setVal(SZARP_NO_DATA);
						} else 
							this->setVal(stack[sp - 1]);
						break;
					case L' ':
						break;
					default:
						sz_log(1, "Uknown character '%lc' in formula '%ls' for parameter",
								*chptr, this->tab.c_str());
				}
			}
		} while (*(++chptr) != 0);
			
		this->setZmqVal(zmq);
	}
};

class LuaParamBuilder
{
public:
	template<class data_type, class time_type> static DefParamBase* op(TParam* par, size_t index)
	{
		return new LuaParam<data_type, time_type>(par, index);
	}
};

class RPNParamBuilder
{
public:
	template<class data_type, class time_type> static DefParamBase* op(TParam* par, size_t index)
	{
		return new RPNParam<data_type, time_type>(par, index);
	}
};

/* vim: set filetype=cpp: */
