#ifndef __SERVER_CMD_SEARCH_DATA_H__
#define __SERVER_CMD_SEARCH_DATA_H__

class SearchDataRcv : public Command {
public:
	SearchDataRcv( Vars& vars , SzbaseProt& prot );

	SearchDataRcv();
protected:
	void parse_command( const std::string& data );

protected:
	Vars& vars;
	SzbaseProt& prot;

};

#endif /* end of include guard: __SERVER_CMD_SEARCH_DATA_H__ */

