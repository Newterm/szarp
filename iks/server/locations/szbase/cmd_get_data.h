#ifndef __SERVER_CMD_GET_DATA_H__
#define __SERVER_CMD_GET_DATA_H__

class GetDataRcv : public Command {
public:
	GetDataRcv( Vars& vars , SzbaseProt& prot );

	GetDataRcv();
protected:
	void parse_command( const std::string& data );

protected:
	Vars& vars;
	SzbaseProt& prot;

};

#endif /* end of include guard: __SERVER_CMD_GET_DATA_H__ */
