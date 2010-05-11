#include "probeblock.h"


class szb_probes_block_real {

protected:
	virtual void Fetch() = 0;
public:
	szb_probes_block_real(szb_buffer_t *m_buffer, TParam* param);
};



szb_probes_block_real::Fetch() {
	size_t to_fetch = probes_per_block - first_non_fixed_probe;

	m_buffer->m_connection->

}
