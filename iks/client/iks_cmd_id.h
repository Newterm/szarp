#ifndef IKS_CMD_ID_H
#define IKS_CMD_ID_H

typedef int IksCmdId;

enum IksError {
	no_error = 0 ,
	connection_error ,
	connection_timeout
};

enum IksCmdStatus {
	cmd_done,
	cmd_cont
};

typedef std::function<IksCmdStatus( IksError , const std::string& , std::string & )> IksCmdCallback;

#endif
/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
