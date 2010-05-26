#include "szbase/szbbuf.h"

szb_block_t::szb_block_t(szb_buffer_t* b, TParam* p, time_t st, time_t et) :
		buffer(b), param(p), start_time(st), end_time(et), fixed_probes_count(0), locator(NULL)
{}

void szb_block_t::MarkAsUsed() {
	locator->Used();
}

int szb_block_t::GetFixedProbesCount() {
	return fixed_probes_count;
}

time_t szb_block_t::GetStartTime() {
	return start_time;
}

time_t szb_block_t::GetEndTime() {
	return end_time;
}

