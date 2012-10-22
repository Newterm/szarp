#ifndef __SIMPLE_MOCKS_H__
#define __SIMPLE_MOCKS_H__
namespace mocks {

class IPKContainerMock {
	TParam param;
	TSzarpConfig config;
	int param_no;
public:
	IPKContainerMock() : param(NULL) {
		param.SetConfigId(0);
		param.SetParamId(0);
		param.SetParentSzarpConfig(&config);
		
		config.SetName(L"BASE", L"BASE");
		param_no = 1;
	}

	void add_param(TParam* param) {
		param->SetConfigId(0);
		param->SetParamId(param_no++);
		param->SetParentSzarpConfig(&config);

	}
	TSzarpConfig* GetConfig(const std::wstring&) { return (TSzarpConfig*) 1; }
	TParam* GetParam(const std::wstring&) { return &param; }
};

struct mock_types {
	typedef IPKContainerMock ipk_container_type;
};

}


#endif
