#ifndef SZ4_IKS_PARAM_OBSERVER_H
#define SZ4_IKS_PARAM_OBSERVER_H

namespace sz4 {

class param_observer_ {
public:
	virtual void operator()() = 0;
	virtual ~param_observer_() {}
};

}

#endif
