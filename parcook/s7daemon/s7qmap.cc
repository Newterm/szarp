#include <algorithm>
#include "liblog.h"

#include "s7qmap.h"

S7Query S7QueryMap::QueryFromParam( S7Param param )
{
	sz_log(10, "S7QueryMap::QueryFromParam");

	S7Query query;

	if (param.s7DBType.compare(std::string("input")) == 0) {
		query.setArea(0x81);
	} else if (param.s7DBType.compare(std::string("output")) == 0) {
		query.setArea(0x82);
	} else if (param.s7DBType.compare(std::string("merker")) == 0) {
		query.setArea(0x83);
	} else if (param.s7DBType.compare(std::string("db")) == 0) {
		query.setArea(0x84);
		query.setDbNum(param.s7DBNum);
	} else if (param.s7DBType.compare(std::string("counter")) == 0) {
		query.setArea(0x1C);
	} else if (param.s7DBType.compare(std::string("timer")) == 0) {
		query.setArea(0x1D);
	} else {
		sz_log(1, "Query will fail: db_type:%s not recognized", param.s7DBType.c_str());
	} 
		
	query.setAmount(1);

	if (param.s7ValType.compare(std::string("bit")) == 0) {
		query.setWordLen(0x01);
		param.s7Addr = param.s7Addr * 8 + param.s7BitNumber;
	} else if (param.s7ValType.compare(std::string("byte")) == 0) {
		query.setWordLen(0x02);
	} else if (param.s7ValType.compare(std::string("word")) == 0) {
		query.setWordLen(0x04);
	} else if (param.s7ValType.compare(std::string("real")) == 0) {
		/** We read real in word-sized chunks that are merged with other types */
		query.setWordLen(0x04); 
	} else {
		sz_log(1, "Query will fail: val_type:%s not recognized", param.s7ValType.c_str());
	}

	query.setStart(param.s7Addr);
	query.setWriteQuery(param.s7Write);

	return query;
}

bool S7QueryMap::AddQuery( unsigned long int idx, S7Param param ) 
{	
	sz_log(10, "S7QueryMap::AddQuery");

	S7Query query = QueryFromParam(param);
	if (!query.isValid()) {
		sz_log(1, "Query from param is not valid");
		return false;
	}

	query.appendId(idx);
	query.build();

	sz_log(8, "Query ID %lu used", idx);
	
	_queries[query.getKey()].push_back(query);

	return true;
}

void S7QueryMap::Sort()
{
	sz_log(10, "S7QueryMap::Sort");
	for (auto vk_q = _queries.begin(); vk_q != _queries.end(); vk_q++)
		std::sort(vk_q->second.begin(), vk_q->second.end());
}

void S7QueryMap::Merge()
{
	sz_log(10, "S7QueryMap::Merge");
	for (auto vk_q = _queries.begin(); vk_q != _queries.end(); vk_q++)
		MergeBucket(vk_q->first, vk_q->second);
}

void S7QueryMap::MergeBucket( S7Query::QueryKey key, QueryVec& qvec )
{
	sz_log(10, "S7QueryMap::MergeBucket");
	if (std::get<2>(key) == 0x01) return; // Do not merge single bits

	unsigned int qidx = 0;
	while (qidx < qvec.size() - 1) {
		if (qvec[qidx].nextAddress() == qvec[qidx + 1].getStart()) {
			qvec[qidx].merge(qvec[qidx + 1]);
			qvec.erase(qvec.begin() + qidx + 1);
		} else {
			qidx += 1;
		}
	}
}

bool S7QueryMap::ClearWriteNoDataFlags()
{
	sz_log(10, "S7QueryMap::ClearWriteNoDataFlags");

	for (auto vk_q = _queries.begin(); vk_q != _queries.end(); vk_q++)
		for (auto q = vk_q->second.begin(); q != vk_q->second.end(); q++)
			if(q->isWriteQuery()) q->setNoData(false);
	return true;
}

bool S7QueryMap::AskAll( S7Object& client )
{
	sz_log(10, "S7QueryMap::AskAll");

	for (auto vk_q = _queries.begin(); vk_q != _queries.end(); vk_q++)
		for (auto q = vk_q->second.begin(); q != vk_q->second.end(); q++)
			if(!q->isWriteQuery()) q->ask(client);
	return true;
}

bool S7QueryMap::TellAll( S7Object& client )
{
	sz_log(10, "S7QueryMap::TellAll");

	for (auto vk_q = _queries.begin(); vk_q != _queries.end(); vk_q++)
		for (auto q = vk_q->second.begin(); q != vk_q->second.end(); q++)
			if(q->isWriteQuery()) q->tell(client);
	return true;
}

bool S7QueryMap::DumpAll()
{
	sz_log(10, "S7QueryMap::DumpAll");

	for (auto vk_q = _queries.begin(); vk_q != _queries.end(); vk_q++)
		for (auto q = vk_q->second.begin(); q != vk_q->second.end(); q++)
			q->dump();

	return true;
}


