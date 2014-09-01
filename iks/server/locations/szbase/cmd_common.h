#ifndef __SERVER_CMD_COMMON_H__
#define __SERVER_CMD_COMMON_H__

typedef float f32_t;
typedef unsigned timestamp_t;
typedef boost::archive::iterators::base64_from_binary<
	boost::archive::iterators::transform_width<const char *, 6, 8> >
		base64_enc;

std::vector<f32_t> get_probes(
		Vars& vars,
		timestamp_t beg , timestamp_t end , ProbeType pt ,
		const std::string& param )
{
	vars.get_szbase()->sync();
	std::vector<f32_t> probes;
	for( timestamp_t t=beg ; t<end ; t=SzbaseWrapper::next(t,pt) )
		probes.push_back( vars.get_szbase()->get_avg_no_sync( param , t , pt ) );
	return probes;
}

#endif /* end of include guard: __SERVER_CMD_COMMON_H__ */
