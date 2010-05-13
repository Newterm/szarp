#define DEFINABLE_STACK_SIZE 200
/* 
  SZARP: SCADA software 
*/

/** Calculates value of one probe in block.
 * @param stack array usesd as stack
 * @param cache array of blocks used in formula calculation.
 * @param formula formula.
 * @param probe_n probe index.
 * @param param_cnt number of params in formula.
 * @param param IPK parameter object
 * @return calculated value.
 */
SZBASE_TYPE
szb_definable_calculate(szb_buffer_t *buffer, SZBASE_TYPE * stack, const double** cache, TParam** params,
	const std::wstring& formula, int probe_n, int param_cnt, time_t time, TParam* param);
