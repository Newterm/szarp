

class szb_probes_block {
public:
	szb_buffer_t *m_buffer;
	time_t m_start_time;
	time_t m_end_time;
	size_t m_non_fixed_probe;
	TParam* m_param;
	double *m_probes;

	static size_t probes_per_block = 10 * 60;

	int GetParamDataFromServer(time_t start, time_t end, TParam* param);
	virtual bool FetchProbes() = 0;
	virtual bool Refresh();

	szb_probes_block(szb_buffer_t *m_buffer, TParam* param);
};
