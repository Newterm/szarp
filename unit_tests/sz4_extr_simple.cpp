#include "config.h"

#include "szarp_config.h"

#include "conversion.h"

#include "sz4/defs.h"
#include "sz4/time.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"
#include "sz4/path.h"
#include "sz4/block.h"
#include "sz4/buffer.h"
#include "sz4/base.h"
#include "sz4/load_file_locked.h"

int main(int argc, char *argv[]) {
	if (argc != 3)
		return 1;

	SZARP_PROBE_TYPE pt = PT_MONTH;
	IPKContainer::Init(L"/opt/szarp", L"/opt/szarp", L"pl");

	IPKContainer* ipk = IPKContainer::GetObject();

	std::wstring pname = SC::U2S((unsigned char*)argv[1]);

	std::stringstream ss;
	ss << argv[2];

	sz4::second_time_t start_time;
	ss >> start_time;

	sz4::second_time_t end_time = szb_move_time(start_time, 1, pt);

	sz4::base base(L"/opt/szarp", IPKContainer::GetObject());

	TParam* p = ipk->GetParam(pname);
	sz4::weighted_sum<double, sz4::second_time_t> sum;
#if 1
	base.get_weighted_sum(p, start_time, end_time, pt, sum);
#endif
	std::cout << sum.avg() << std::endl;

	return 0;
}
