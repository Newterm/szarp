#ifndef __SERVER_CMD_SEARCH_DATA_H__
#define __SERVER_CMD_SEARCH_DATA_H__

class SearchDataRcv : public Command {
public:
	SearchDataRcv( Vars& vars , Protocol& prot );

	SearchDataRcv();
protected:
	void parse_command( const std::string& data );

protected:
	Vars& vars;

};

#endif /* end of include guard: __SERVER_CMD_SEARCH_DATA_H__ */

