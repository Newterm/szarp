#include "config.h" 
#include "liblog.h"
#include "szarp_base_common/definable_calculate.h" 
#include <limits>

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#define MAX_FID 8

int szb_definable_error;	// kod ostatniego bledu

double
szb_fixo(double *parlst)
{
    double Fi[3][3][33] = {
	{
	    {1.00, 0.97, 0.95, 0.92, 0.89, 0.87, 0.84, 0.82, 0.79, 0.76, 0.74, 0.71,
		0.68, 0.66, 0.63, 0.61, 0.58, 0.55, 0.53, 0.50, 0.47, 0.45, 0.42, 0.39,
		0.37, 0.34, 0.32, 0.29, 0.26, 0.24, 0.21, 0.18, 0.16},
	    {1.04, 1.01, 0.99, 0.96, 0.93, 0.90, 0.88, 0.85, 0.82, 0.79, 0.77, 0.74,
		0.71, 0.68, 0.65, 0.63, 0.60, 0.57, 0.55, 0.52, 0.49, 0.47, 0.44, 0.41,
		0.38, 0.36, 0.33, 0.30, 0.27, 0.25, 0.22, 0.19, 0.16},
	    {1.07, 1.04, 1.01, 0.99, 0.96, 0.93, 0.90, 0.87, 0.84, 0.82, 0.79, 0.76,
		0.73, 0.70, 0.68, 0.65, 0.62, 0.59, 0.56, 0.53, 0.51, 0.48, 0.45, 0.42,
		0.39, 0.37, 0.34, 0.31, 0.28, 0.25, 0.23, 0.20, 0.17}
	},
	{
	    {0.99, 0.96, 0.93, 0.91, 0.88, 0.85, 0.82, 0.80, 0.77, 0.74, 0.71, 0.69,
		0.66, 0.63, 0.60, 0.58, 0.55, 0.52, 0.49, 0.47, 0.44, 0.41, 0.38, 0.35,
		0.33, 0.30, 0.27, 0.24, 0.22, 0.19, 0.16, 0.13, 0.11},
	    {1.03, 1.00, 0.97, 0.94, 0.91, 0.89, 0.86, 0.83, 0.80, 0.77, 0.74, 0.71,
		0.68, 0.66, 0.63, 0.60, 0.57, 0.54, 0.51, 0.48, 0.45, 0.43, 0.40, 0.37,
		0.34, 0.31, 0.28, 0.25, 0.22, 0.20, 0.17, 0.14, 0.11},
	    {1.06, 1.03, 1.00, 0.97, 0.94, 0.91, 0.88, 0.85, 0.82, 0.79, 0.76, 0.73,
		0.70, 0.67, 0.65, 0.62, 0.59, 0.56, 0.53, 0.50, 0.47, 0.44, 0.41, 0.38,
		0.35, 0.32, 0.29, 0.26, 0.23, 0.20, 0.17, 0.14, 0.11}
	},
	{
	    {0.98, 0.95, 0.92, 0.89, 0.86, 0.84, 0.81, 0.78, 0.75, 0.72, 0.69, 0.66,
		0.63, 0.60, 0.57, 0.55, 0.52, 0.49, 0.46, 0.43, 0.40, 0.37, 0.34, 0.31,
		0.29, 0.26, 0.23, 0.20, 0.17, 0.14, 0.11, 0.06, 0.05},
	    {1.02, 0.99, 0.98, 0.98, 0.90, 0.87, 0.84, 0.81, 0.78, 0.75, 0.72, 0.69,
		0.66, 0.63, 0.60, 0.57, 0.54, 0.51, 0.48, 0.43, 0.42, 0.39, 0.36, 0.33,
		0.30, 0.27, 0.24, 0.21, 0.18, 0.15, 0.12, 0.09, 0.06},
	    {1.05, 1.02, 0.99, 0.96, 0.92, 0.89, 0.86, 0.83, 0.80, 0.79, 0.74, 0.71,
		0.68, 0.65, 0.61, 0.58, 0.55, 0.51, 0.49, 0.44, 0.43, 0.42, 0.37, 0.34,
		0.31, 0.27, 0.24, 0.21, 0.18, 0.15, 0.12, 0.09, 0.05}
	}
    };
  
    short x = (short)rint(parlst[0]);	/* Pogoda */
    short y = (short)rint(parlst[1]);	/* Wiatr */
    short z = (short)rint(parlst[2]);	/* Tzewn */
    
    if (x < 0)
	x = 0;
    if (x > 2)
	x = 2;
    if (y < 3)
	y = 0;
    else if (y < 8)
	y = 1;
    else
	y = 2;
    if (z < -20)
	z = -20;
    if (z > 12)
	z = 12;
    z += 20;

    return Fi[x][y][z];
}

double
szb_tab_reg_zas(double *parlst)
{
  double Tz[81] = {
      65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.7,
      66.4, 67.7, 68.9, 70.2, 71.4, 72.7, 73.9, 75.1, 76.3, 77.6,
      78.8, 80.0, 81.2, 82.4, 83.6, 84.8, 86.0, 87.2, 88.4, 89.6,
      90.7, 91.9, 93.0, 94.2, 95.3, 96.5, 97.6, 98.8, 99.9, 101.1,
      102.2, 103.3, 104.4, 105.6, 106.7, 107.8, 108.9, 110.0, 111.1,
      112.3, 113.4, 114.5, 115.5, 116.6, 117.7, 118.8, 119.9, 121.0,
      122.1, 123.2, 124.3, 125.4, 126.5, 127.6, 128.7, 129.9, 131.0, 
      132.1, 133.1, 134.2, 135.3, 136.4, 137.5, 138.6, 139.7, 140.8,
      141.9, 143.0, 144.0, 145.1, 146.2
  };

  short x = (short) rint(parlst[0] * 100.0);

  if (x < 20)
      x = 20;
  if (x > 100)
      x=100;
  x -= 20;
  
  return Tz[x];
}  

double
szb_tab_reg_pow(double *parlst)
{
    double Tp[81] = {
	49.3, 48.6, 47.9, 47.2, 46.4, 45.7, 45.0, 44.3, 43.5, 43.1,
	42.6, 42.9, 43.2, 44.2, 45.2, 45.8, 46.3, 46.9, 47.4, 47.9,
	48.4, 49.0, 49.5, 50.0, 50.4, 51.0, 51.5, 52.0, 52.5, 53.0,
	53.4, 53.9, 54.4, 54.9, 55.4, 55.9, 56.3, 56.8, 57.3, 57.7,
	58.1, 58.6, 59.1, 59.6, 60.0, 60.5, 60.9, 61.4, 61.8, 62.3,
	62.7, 63.1, 63.5, 64.0, 64.4, 64.8, 65.2, 65.7, 66.1, 66.5,
	66.9, 67.4, 67.8, 68.2, 68.6, 69.1, 69.5, 69.9, 70.3, 70.7,
	71.1, 71.6, 72.0, 72.4, 72.7, 73.2, 73.6, 74.0, 74.4, 74.8,
	75.1
    };
    
    short x=(short)rint(parlst[0]*100.0);
    if (x < 20)
	x = 20;
    else if (x > 100)
	x = 100;
    
    x -= 20;

    return Tp[x];
} 

double
szb_positive(double *parlst)
{
    return (parlst[0] > 0.0 ? parlst[0] : 0.0);
}

unsigned char szb_calcul_no_data;

double
szb_sum_existing(double *parlst)
{
    double sum = 0;
    unsigned char valid = 0;
    int max = (int) rint(parlst[0]);

    for(int i = 1; i <= max; i++)
	if (parlst[i] > -1e20) {
	    sum += parlst[i];
	    valid += 1;
	}
    
    if (valid)
	szb_calcul_no_data = 0;
    else
	szb_calcul_no_data = 1; 

    return sum;
}

double
szb_cnt_existing(double *parlst)
{
    double sum = 0;
    int max = (int) rint(parlst[0]);
    
    for(int i = 1; i<= max; i++)
	if (parlst[i] >- 1e20)
	    sum += 1;

    szb_calcul_no_data = 0;

    return sum;
}

double
szb_water_tank_suwa(double *parlst)
{
    double H2Vtab[22]={
	0.0,  0.8,  2.4,  4.4,  6.6,  9.0, 11.6, 14.4, 17.2,
	20.0, 23.0, 25.8, 28.8, 31.6, 34.4, 37.0, 39.4, 41.6,
	43.6, 45.2, 46.0, 46.0
    };

    if (parlst[0] < 0.0)
	parlst[0] = 0.0;
    else if (parlst[0] > 100.0)
	parlst[0] = 100.0;
    
    int ind = (int) rint(floor(parlst[0] / 5.0));
    double res = (H2Vtab[ind + 1] - H2Vtab[ind]) / 5.0 * (parlst[0] - (double)ind * 5.0) + H2Vtab[ind];
    return res * 10;
}

double
szb_bit_of_log(double *parlst)
{
    int val = (int) rint(parlst[0]);
    int sh = (int) rint(parlst[1]);
    int res = ((val&(1<<sh))!=0);
    /*  printf("BitOfLog(%d,%d)=%d\n", val, sh, res);  */
    return (double)res;
}


double (*szb_def_fun_table[MAX_FID])(double*) = {
    szb_fixo, szb_tab_reg_zas, szb_tab_reg_pow, szb_positive,
    szb_sum_existing, szb_cnt_existing, szb_water_tank_suwa,
    szb_bit_of_log
};

double
szb_definable_choose_function(double funid, double *parlst)
{
    ushort fid = (ushort) funid;

    if (fid >= MAX_FID)
	return (0.0);

    return ((*(szb_def_fun_table[fid])) (parlst));
}


default_is_summer_functor::default_is_summer_functor(time_t _t, TParam* _param) : param(_param), t(_t) {}

bool default_is_summer_functor::operator()(bool& is_summer) {
	TSzarpConfig *config = param->GetSzarpConfig();
	assert(config);
	const TSSeason *seasons;
	if ((seasons = config->GetSeasons())) {
		is_summer =  seasons->IsSummerSeason(t) ? 1 : 0;
		return true;
	}
	return false;
}

double
szb_definable_calculate(double * stack, int stack_size, const double** cache, TParam** params,
	const std::wstring& formula, int param_cnt, value_fetch_functor& value_fetch, is_summer_functor& is_summer, TParam* param)
{
	using std::isnan;

	short sp = 0;
	const wchar_t *chptr = formula.c_str();
	int it = 0;	// licznik, do indeksowania parametrow

	szb_definable_error = 0;

	if (formula.empty()) {
		szb_definable_error = 1;
		sz_log(0, "Invalid, NULL formula");
		return nan("");
	}

	do {
		if (iswdigit(*chptr)) {	
			chptr = wcschr(chptr, L' ');
	
			// check stack size
			if (sp >= stack_size) {
				sz_log(0,
					"Nastapilo przepelnienie stosu przy liczeniu formuly %ls",
					formula.c_str());
				return nan("");
			}
	
			if (it >= param_cnt) {
				sz_log(0, "FATAL: in szb_definable_calculate: it > param_cnt");
				sz_log(0, " param: %ls", param->GetName().c_str());
				sz_log(0, " formula: %ls", formula.c_str());
				sz_log(0, " it: %d, param_cnt: %d, num: %d",
					it, param_cnt, param->GetNumParsInFormula());
				sz_log(0, " chptr: %ls", chptr);
				return std::numeric_limits<double>::quiet_NaN();
			}
	
			// put probe value on stack
			if (cache[it] != NULL) {
				const double * data = cache[it];
				if(!isnan(*data))
					stack[sp++] = *data * pow(10, params[it]->GetPrec());
				else
					stack[sp++] = nan("");
			} else {
				TParam** fc = param->GetFormulaCache();
				if (fc[it]->GetType() == TParam::P_LUA
						&& fc[it]->GetFormulaType() == TParam::LUA_AV) {
					stack[sp++] = value_fetch(fc[it]);
					if (!isnan(stack[sp]))
						stack[sp] = stack[sp] * pow(10, params[it]->GetPrec());
				} else {
					stack[sp++] = nan("");
				}
			}
	
			it++;
		} else {
			double tmp;
			short par_cnt;
			int i1, i2;
			switch (*chptr) {
				case L'&':
					if (sp < 2)	/* swap */
						return nan("");
					if (isnan(stack[sp - 1]) || isnan(stack[sp - 2]))
						return nan("");
					tmp = stack[sp - 1];
					stack[sp - 1] = stack[sp - 2];
					stack[sp - 2] = tmp;
					break;		
				case L'!':		
					if (isnan(stack[sp - 1]))
						return nan("");

					// check stack size
					if (stack_size <= sp) {
						sz_log(0,
							"Przepelnienie stosu dla formuly %ls, w funkcji '!'",
							formula.c_str());
						return nan("");
					}

					stack[sp] = stack[sp - 1];	/* duplicate */
					sp++;
					break;
				case L'$':
					if (sp-- < 2)	/* function call */
						return nan("");
					par_cnt = (short) rint(stack[sp - 1]);
					if (sp < par_cnt + 1)
						return nan("");
					for (int i = sp - 2; i >= sp - par_cnt - 1; i--)
						if (isnan(stack[i]))
							return nan("");

					stack[sp - par_cnt - 1] =
					szb_definable_choose_function(stack[sp], &stack[sp - par_cnt - 1]);

					sp -= par_cnt;
					break;		
				case L'#':
					// check stack size
					if (stack_size <= sp) {
						sz_log(0,
							"Przepelnienie stosu dla formuly %ls, przy odkladaniu stalej: %lf",
							formula.c_str(), wcstod(++chptr, NULL));
						return nan("");
					}
		
					stack[sp++] = wcstod(++chptr, NULL);
					chptr = wcschr(chptr, L' ');
					break;
				case L'?':
					if (*(++chptr) == L'f') {	
					/* warunek <par1> <par2> <cond> if
					* jesli <cond> != 0 zostawia <par1> 
					* w innym wypadku zostawia <par2>  
					*/
						if (sp-- < 3)
							return nan("");

						if (0 == stack[sp])
							stack[sp - 2] = stack[sp - 1];
						sp--;
					}
					break;
				case L'+':
					if (sp-- < 2)
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1]))
						return nan("");
					stack[sp - 1] += stack[sp];
					break;
				case L'-':
					if (sp-- < 2)
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1]))
						return nan("");
					stack[sp - 1] -= stack[sp];
					break;
				case L'*':
					if (sp-- < 2)
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1]))
						return nan("");
					stack[sp - 1] *= stack[sp];
					break;
				case L'/':
					if (sp-- < 2)
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1] ))
						return nan("");
					if (stack[sp] != 0.0)
						stack[sp - 1] /= stack[sp];
					else {
						sz_log(4, "WARRNING: szb_definable_calculate: dzielenie przez zero ");
							return nan("");
					}
					break;
				case L'>':
					if (sp-- < 2)	/* wieksze */
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1]))
						return nan("");
					if (stack[sp - 1] > stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L'<':
					if (sp-- < 2)	/* mniejsze */
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1]))
						return nan("");
					if (stack[sp - 1] < stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L'~':
					if (sp-- < 2)	/* rowne */
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1]))
						return nan("");
					if (stack[sp - 1] == stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L':':
					if (sp-- < 2)	/* slowo */
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1]))
						return nan("");
					i1 = (short) rint(stack[sp - 1]);
					i2 = (short) rint(stack[sp]);
					stack[sp - 1] = (double) ((i1 << 16) | i2);
					break;
				case L'^':
					if (sp-- < 2)	/* potega */
						return nan("");
					if (isnan(stack[sp]) || isnan(stack[sp - 1] ))
						return nan("");
	
					// fix - PP 
					if (stack[sp] == 0.0) {
						// a^0 == 1 
						stack[sp - 1] = 1;
					} else if (stack[sp - 1] >= 0.0) {
						stack[sp - 1] = pow(stack[sp - 1], stack[sp]);
					} else {
						sz_log(4, "WARRNING: szb_definable_calculate: wyk�adnik pot�gi < 0");
						return nan("");
					}
					break;
				case L'N':
					if (sp-- < 2)	/* Sprawdzenie czy nan("") */
						return nan("");
					if (isnan(stack[sp - 1]))
						stack[sp - 1] = stack[sp];
					break;
				case L'X':
					// check stack size
					if (stack_size <= sp) {
						sz_log(0,
							"Przepelnienie stosu dla formuly %ls, w funkcji X",
							formula.c_str());
						return nan("");
					}
					stack[sp++] = nan("");
					break;
				case L'S':
					if (stack_size > sp) {
						bool is_in_summer;
						bool found = is_summer(is_in_summer);
						if (found) {
							stack[sp++] = is_in_summer ? 1 : 0;
							break;
						}
					} else {
						sz_log(0,
							"Przepelnienie stosu dla formuly %ls, w funkcji S",
							formula.c_str());
						return nan("");
					} //Falls through if there is no seasons defined
				default:
					if (iswspace(*chptr))
						break;
					break;
			}
		}
	} while (chptr && *(++chptr) != 0);

	if (szb_definable_error)
		return nan("");

	if (sp-- < 0) {
		sz_log(5, "ERROR: szb_definable_calculate: sp-- < 0");
		szb_definable_error = 1;
		return nan("");
	}

	if (isnan(stack[0])) {
		sz_log(10, "WARRNING: szb_definable_calculate: stack[0] == nan("")");
		return nan("");
	}

	return stack[0];
}

