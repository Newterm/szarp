

class szb_probes_block_str {
	szb_buffer_t *m_buffer;
	time_t m_start_time;
	time_t m_end_time;
	double *m_probes;
	TParam* m_param;
	size_t first_non_fixed_probe;
	static size_t probes_per_block = 10 * 60;
protected:
	virtual bool FetchProbes() = 0;
public:
	szb_probes_block_str(szb_buffer_t *m_buffer, TParam* param);
	virtual bool Refresh();
};
