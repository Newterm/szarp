#ifndef __TEST_SEARCH_CONDITION_H__
#define __TEST_SEARCH_CONDITION_H__


class test_search_condition : public sz4::search_condition {
	int value_to_search;
public:
	test_search_condition(int v) : value_to_search(v) {}
	virtual bool operator()(const int &v) const { return value_to_search == v; }
	virtual bool operator()(const short &v) const { return false; }
	virtual bool operator()(const float &v) const { return false; }
	virtual bool operator()(const double &v) const { return false; }
};


#endif
