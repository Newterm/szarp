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
	if (argc != 3 && argc != 4 && argc != 5)
		return 1;

	SZARP_PROBE_TYPE pt = PT_MONTH;
	IPKContainer::Init(L"/opt/szarp", L"/opt/szarp", L"pl");

	IPKContainer* ipk = IPKContainer::GetObject();

	std::wstring pname = SC::U2S((unsigned char*)argv[1]);

	std::stringstream ss;
	ss << argv[2];

	sz4::second_time_t start_time;
	ss >> start_time;

	int count = 1;
	if (argc >= 4) {
		std::stringstream ss;
		ss << argv[3];
		ss >> count;
		std::cout << count << std::endl;
	}
	if (argc >= 5) {
		std::string _pt = argv[4];
		std::cout << "Selecting: ";
		if (_pt == "halfsec") {
			pt = PT_HALFSEC;	
			std::cout << "PT_HALFSEC" << std::endl;
		} else if (_pt == "sec") {
			pt = PT_SEC;
			std::cout << "PT_SEC" << std::endl;
		} else if (_pt == "sec10") {
			pt = PT_SEC10;
			std::cout << "PT_SEC10" << std::endl;
		} else if (_pt == "MIN10") {
			pt = PT_MIN10;
			std::cout << "PT_MIN10" << std::endl;
		} else if (_pt == "HOUR") {
			pt = PT_HOUR;
			std::cout << "PT_HOUR" << std::endl;
		} else if (_pt == "HOUR8") {
			pt = PT_HOUR8;
			std::cout << "PT_HOUR8" << std::endl;
		} else if (_pt == "DAY") {
			pt = PT_DAY;
			std::cout << "PT_DAY" << std::endl;
		} else if (_pt == "WEEK") {
			pt = PT_WEEK;
			std::cout << "PT_WEEK" << std::endl;
		} else if (_pt == "MONTH") {
			pt = PT_MONTH;
			std::cout << "PT_MONTH" << std::endl;
		}
	}

	sz4::second_time_t end_time = szb_move_time(start_time, count, pt);

	sz4::base base(L"/opt/szarp", IPKContainer::GetObject());

	TParam* p = ipk->GetParam(pname);
	sz4::weighted_sum<double, sz4::second_time_t> sum;
#if 1
	base.get_weighted_sum(p, start_time, end_time, pt, sum);
#endif
	std::cout << sum.avg() << std::endl;

	return 0;
}
