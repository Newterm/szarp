/* 
  SZARP: SCADA software 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/* PSET
 * <reksio@praterm.com.pl>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <ldap.h>

#include "xmlutils.h"
#include "liblog.h"
#include "libpar.h"
#include "execute.h"
#include "daemon.h"

#include "ppset.h"

using namespace PPSET;


using std::string;
using std::endl;
using std::getline;
using std::ifstream;
using std::ios_base;
using std::ostringstream;
using std::istringstream;
using std::map;
using std::vector;

#define X (unsigned char *)

volatile sig_atomic_t daemon_stopped = 0;

class LineDaemonStopper : public DaemonStopper {
	std::string path;
	bool StopDaemon(pid_t pid);
	public:
	LineDaemonStopper(const std::string &path);
	virtual void StartDaemon();
	virtual bool StopDaemon(xmlDocPtr *error);
	virtual ~LineDaemonStopper() {}
};

class LineDaemonsStopperFactory : public DaemonStoppersFactory {
	public:
	LineDaemonStopper* CreateDaemonStopper(const std::string& s) {
		return new LineDaemonStopper(s);
	}
};


char* ldap_server_address;
char* ldap_server_port;
char* ldap_group_dn;
char* ldap_users_dn;

string prefix;

map < string, string > credentials;

int filter_proc_entry(const dirent * dir)
{
	int i = 0;
	for (; isdigit(dir->d_name[i]); ++i) ;
	return dir->d_name[i] == '\0';
}

bool get_daemon_pid(const char *device_name, pid_t * pid)
{
	struct dirent **namelist;
	int n = scandir("/proc", &namelist, filter_proc_entry, alphasort);

	bool ret = false;
	for (int i = 0; i < n; ++i) {
		string dn(namelist[i]->d_name);
		string fn = "/proc/" + dn + "/cmdline";
		ifstream fs(fn.c_str());
		if (fs.bad())
			continue;
		ostringstream ss;
		ss << fs.rdbuf();
		const string & cmd = ss.str();
		if (cmd.find(device_name) != string::npos) {
			*pid = atoi(dn.c_str());
			ret = true;
			break;
		}
	}

	for (int i = 0; i < n; ++i)
		free(namelist[i]);
	free(namelist);

	return ret;
}

int authenticate(xmlDocPtr doc, string & reason)
{

	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
	xmlNodePtr node =
	    uxmlXPathGetNode(BAD_CAST "//credentials", xpath_ctx);
	xmlXPathFreeContext(xpath_ctx);
	if (node == NULL) {
		reason = "Invalid message(credentials node missing)";
		return 1;
	}

	char *username = (char *)xmlGetProp(node, BAD_CAST "username");
	if (username == NULL) {
		reason = "Invalid message(username attribute missing)";
		return 1;
	}

	char *password = (char *)xmlGetProp(node, (xmlChar *) "password");
	if (password == NULL) {
		xmlFree(username);
		reason = "Invalid message(password attribute missing)";
		return 1;
	}

	sz_log(5, "User %s connected", username);

	if (credentials.find(username) != credentials.end()
	    && credentials[username] == password) {
		sz_log(5, "User %s successfuly authenticated", username);
		xmlFree(username); xmlFree(password);
		return 0;
	}

	int error;
	LDAP *ld;

	string uri =
	    string("ldaps://") + ldap_server_address + ":" + ldap_server_port;
	error = ldap_initialize(&ld, uri.c_str());
	if (error != LDAP_SUCCESS) {
		reason = ldap_err2string(error);
		xmlFree(username); xmlFree(password);
		return 1;
	}

	int v3 = LDAP_VERSION3;
	error = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &v3);
	if (error != LDAP_SUCCESS) {
		reason = ldap_err2string(error);
		xmlFree(username); xmlFree(password);
		return 1;
	}

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	error = ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &tv);
	if (error != LDAP_SUCCESS) {
		reason = ldap_err2string(error);
		xmlFree(username); xmlFree(password);
		return 1;
	}

	string user_dn = string("uid=") + username + "," + ldap_users_dn;

	struct berval cred;
	cred.bv_len = strlen(password);
	cred.bv_val = password;

	error = ldap_sasl_bind_s(ld, user_dn.c_str(), LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
	if (error != LDAP_SUCCESS) {
		if (error == LDAP_INVALID_CREDENTIALS)
			reason = "Wrong username or password";
		else
			reason = ldap_err2string(error);

		sz_log(5, "Failed to connect to server: %s", reason.c_str());

		xmlFree(username); xmlFree(password);
		return 1;
	}
	sz_log(5, "User %s successfully bound to ldap server", username);

	char *member_uid[] = { (char *)"memberUID", NULL };
	string group_dn = string("cn=") + prefix + "_masters," + ldap_group_dn;
	LDAPMessage *msg;
	error = ldap_search_ext_s(ld,
				  group_dn.c_str(),
				  LDAP_SCOPE_BASE,
				  "(objectClass=*)",
				  member_uid, 0, NULL, NULL, NULL, 0, &msg);
	if (error != LDAP_SUCCESS) {
		reason = ldap_err2string(error);
		xmlFree(username); xmlFree(password);
		ldap_unbind_ext_s(ld, NULL, NULL);
		return 1;
	}

	bool found = false;
	LDAPMessage *entry;
	entry = ldap_first_entry(ld, msg);
	if (entry) {
		BerElement *ber;
		char *attr = ldap_first_attribute(ld, entry, &ber);
		if (attr) {
			struct berval **values = ldap_get_values_len(ld, entry, attr);
			for (int i = 0; values[i] != NULL && !found; i++)
				if (!strncmp(values[i]->bv_val, username, values[i]->bv_len))
					found = true;
			ldap_value_free_len(values);
		}
	}

	ldap_msgfree(msg);
	ldap_unbind_ext_s(ld, NULL, NULL);

	if (found == false) {
		sz_log(5,
		       "User %s authentication failed - she is not a member of a proper group",
		       username);
		reason = "";
		xmlFree(username); xmlFree(password);
		return 1;
	}

	credentials[username] = password;

	sz_log(5, "User %s authenticated successfuly", username);
	return 0;
}

void sig_usr_handler(int signo, siginfo_t * sig_info, void *ctx)
{
	daemon_stopped = !daemon_stopped;
}

void init_signal_handler()
{
	struct sigaction sa;
	sa.sa_sigaction = sig_usr_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sa, NULL);
}


void block_usr_signal()
{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
}

void unblock_usr_signal()
{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigprocmask(SIG_UNBLOCK, &sigset, NULL);

	init_signal_handler();
}

bool LineDaemonStopper::StopDaemon(pid_t pid)
{
	if (daemon_stopped)
		return true;

	daemon_stopped = 0;

	unblock_usr_signal();
	int ret = kill(pid, SIGUSR1);
	if (ret)
		return false;

	int secs = 20;
	while (!daemon_stopped && (secs = sleep(secs))) ;

	block_usr_signal();

	return daemon_stopped != 0;
}

bool LineDaemonStopper::StopDaemon(xmlDocPtr * err_msg)
{
	pid_t pid;
	if (get_daemon_pid(path.c_str(), &pid) == false) {
		sz_log(2,
		       "Error - failed to find daemon operating on device %s",
		       path.c_str());
		*err_msg =
		    create_error_message("error",
					 string
					 ("Error - failed to find daemon operating on device ")
					 + path + " is SZARP running?");
		return false;
	}

	if (StopDaemon(pid) == false) {
		sz_log(0,
		       "Error - failed to stop daemon operating on device %s, something is very wrong, HELP!",
		       path.c_str());
		*err_msg =
		    create_error_message("error",
					 string
					 ("Error - failed to stop daemon operating on device ")
					 + path +
					 ",report this to PRATERM IT deparament!");
		return false;
	}

	return true;
}

int ssl_key_passphrase_cb(char *buf, int size, int rwflag, void *userdata)
{
	strncpy(buf, (const char *)userdata, size);
	buf[size - 1] = '\0';
	return strlen(buf);
}

bool read_msg(SSL * ssl, xmlDocPtr * msg)
{
	const int start_buffer_size = 512;
	const int max_buffer_size = 512 * 16;

	std::string r;
	std::string::size_type sps,eps;

	int buffer_size = start_buffer_size;
	char *buffer = (char *)malloc(start_buffer_size);
	int buffer_pos = 0;

	while (true) {
		int ret =
		    SSL_read(ssl, buffer + buffer_pos,
			     buffer_size - buffer_pos);
		if (ret == 0) {
			sz_log(0,
			       "Connection with client terminated during data read");
			goto error;
		}

		if (ret < 0) {
			int error = SSL_get_error(ssl, ret);

			if (error == SSL_ERROR_SYSCALL)
				if (errno == EINTR)
					continue;
				else
					sz_log(0,
					       "Error while reading data from client: %s",
					       strerror(errno));
			else
				sz_log(0,
				       "Error while reading data from client %s",
				       ERR_error_string(error, 0));

			goto error;
		}

		buffer_pos += ret;

		if (buffer[buffer_pos - 1] == '\0')
			break;

		if (buffer_pos == buffer_size) {
			buffer_size *= 2;
			buffer = (char *)realloc(buffer, buffer_size);

			if (buffer_size > max_buffer_size) {
				sz_log(2,"Too large message from clinet");
				goto error;
			}
		}
	}

	if (*msg)
		xmlFreeDoc(*msg);
	*msg = xmlParseMemory(buffer, buffer_pos);

	r = std::string(buffer, buffer_pos);
	sps = r.find("password=", 0);
	if (sps != std::string::npos) {
		sps += sizeof("password=");
		eps = r.find("\"", sps);
		if (eps != std::string::npos)
			r = r.substr(0, sps) + "*" + r.substr(eps);
	}

	sz_log(2, "Received from client: %s\n", r.c_str());
	free(buffer);

	return *msg != NULL;

error:
	free(buffer);
	return 1;
}

bool write_msg(SSL * ssl, xmlDocPtr msg)
{

	xmlChar* buffer = NULL;
	int length;
	xmlDocDumpMemory(msg, &buffer, &length);
	buffer = (xmlChar*) xmlRealloc(buffer, length + 1);
	buffer[length] = '\0';
	length = length + 1;
	int buffer_pos = 0;

	sz_log(2, "Sending to client: %s", buffer);

	while (buffer_pos < length) {
		int ret =
		    SSL_write(ssl, (char*) buffer + buffer_pos, length - buffer_pos);

		if (ret == 0) {
			sz_log(0,
			       "Connection with client terminated during data write");
			goto error;
		}

		if (ret < 0) {
			int error = SSL_get_error(ssl, ret);

			if (error == SSL_ERROR_SYSCALL)
				if (errno == EINTR)
					continue;
				else 
					sz_log(0,
					       "Error while writing data to client: %s",
					       strerror(errno));
			else
				sz_log(0,
				       "Error while writring data to client %s",
				       ERR_error_string(error, 0));

			goto error;
		}

		buffer_pos += ret;

	}

	xmlFree(buffer);
	return 0;

      error:
	xmlFree(buffer);
	return 1;
}

#if 0
void append_notice(xmlDocPtr doc, const string & notice)
{
	xmlNewChild(doc->children, NULL, BAD_CAST "notice",
		    BAD_CAST notice.c_str());
}
#endif

LineDaemonStopper::LineDaemonStopper(const std::string &_path) {
	path = _path;
}

void LineDaemonStopper::StartDaemon()
{
	pid_t pid;
	if (get_daemon_pid(path.c_str(), &pid) == false) {
		sz_log(2,
		       "Error - failed to find daemon operating on device %s",
		       path.c_str());
		return;
	}

	unblock_usr_signal();

	kill(pid, SIGUSR1);

	int secs = 0;
	while (daemon_stopped && secs < 30) {
		sleep(1); 
		secs++;
	}

	if (daemon_stopped) {
		sz_log(1, "Unable to send signal to daemon HELP!, error: %s",
		       strerror(errno));
		daemon_stopped = 0;
		return ;
	}
	return ;
}

void serve(SSL * ssl)
{
	xmlDocPtr request = NULL, response = NULL;
	string auth_fail_reason;

	if (read_msg(ssl, &request) == false)
		goto end;

	if (authenticate(request, auth_fail_reason))
		response =
		    create_error_message("auth_failed", auth_fail_reason);
	else {
		LineDaemonsStopperFactory factory;
		response = handle_command(request, factory);
	}

	write_msg(ssl, response);
      end:
	if (request)
		xmlFreeDoc(request);
	if (response)
		xmlFreeDoc(response);
}

void main_loop(int listen_socket, SSL_CTX *ssl_ctx) {
	while (true) {
		struct sockaddr_in client_addr;
		socklen_t clen = sizeof(client_addr);
		int conn_socket;

		conn_socket =
		    accept(listen_socket, (struct sockaddr *)&client_addr,
			   &clen);
		if (conn_socket < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				sz_log(0, "%d accept %s", errno,
				       strerror(errno));
				continue;
			}
		}

		int one = 1;
		int two = 2;
		int minute = 60;

		setsockopt(conn_socket, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
		setsockopt(conn_socket, IPPROTO_TCP, TCP_KEEPCNT, &two, sizeof(two));
		setsockopt(conn_socket, IPPROTO_TCP, TCP_KEEPIDLE, &minute, sizeof(two));
		setsockopt(conn_socket, IPPROTO_TCP, TCP_KEEPINTVL, &minute, sizeof(two));

		BIO *bio = BIO_new_socket(conn_socket, BIO_CLOSE);
		if (bio == NULL) {
			sz_log(0, "Failed to create bio %s",
			       ERR_error_string(ERR_get_error(), NULL));
			close(conn_socket);
			continue;
		}

		SSL *ssl = SSL_new(ssl_ctx);
		if (ssl == NULL) {
			BIO_free(bio);
			sz_log(0, "Failed to create ssl %s",
			       ERR_error_string(ERR_get_error(), NULL));
			continue;
		}

		SSL_set_bio(ssl, bio, bio);

		int ret = SSL_accept(ssl);
		if (ret != 1) {
			sz_log(0, "SSL accept failed %s",
			       ERR_error_string(ERR_get_error(), NULL));
			SSL_free(ssl);
			continue;
		}

		serve(ssl);

		SSL_free(ssl);
	}
}

int main(int argc, char *argv[])
{

	int loglevel = loginit_cmdline(2, NULL, &argc, argv);

	libpar_read_cmdline(&argc, argv);

	libpar_init();

	/* Check for other copies of program. */
	if (check_for_other (argc, argv)) {
		sz_log(0, "psetd: another copy of program is running, exiting");
		return 1;
	}

	char *ca_file = libpar_getpar("psetd", "ca_file" , 1);
	char *passphrase = libpar_getpar("psetd", "passprase", 0);
	char *key_file = libpar_getpar("psetd", "key_file" , 1); 
	char *port_string = libpar_getpar("psetd", "port", 1);
	int port_no = atoi(port_string);

	char* _loglevel = libpar_getpar("psetd", "log_level", 0);
	if (_loglevel == NULL)
		loglevel = 2;
	else {
		loglevel = atoi(_loglevel);
		free(_loglevel);
	}

	char* logfile = libpar_getpar("psetd", "log", 0);
	if (logfile == NULL)
		logfile = strdup("psetd");

	loglevel = sz_loginit(loglevel, logfile);
	if (loglevel < 0) {
		sz_log(0, "psetd: cannot inialize log file %s, errno %d", logfile, errno);
		return 1;
	}

	ldap_server_address = libpar_getpar("psetd", "ldap_server_address", 1);
	ldap_server_port = libpar_getpar("psetd", "ldap_server_port", 1);
	ldap_group_dn = libpar_getpar("psetd", "ldap_group_dn", 1);
	ldap_users_dn = libpar_getpar("psetd", "ldap_users_dn", 1);
	prefix = libpar_getpar("psetd", "config_prefix", 1);

	libpar_done();

	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_crypto_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	struct sockaddr_in sin;
	int one = 1;
	int listen_socket;

	SSL_CTX *ssl_ctx = SSL_CTX_new(SSLv23_server_method());
	if (ssl_ctx == NULL) {
		sz_log(0, "SSL connection context creation failure, error %s",
		       ERR_error_string(ERR_get_error(), NULL));
		return 1;
	}

	if (SSL_CTX_use_certificate_chain_file(ssl_ctx, ca_file) == 0) {
		sz_log(0, "Failed to load cert chain file , error %s",
		       ERR_error_string(ERR_get_error(), NULL));
		return 1;
	}

	if (passphrase != NULL) {
		SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx,
						       (void *)passphrase);
		SSL_CTX_set_default_passwd_cb(ssl_ctx, ssl_key_passphrase_cb);
	}
	if (!(SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM))) {
		sz_log(0, "Failed to load private key file , error %s",
		       ERR_error_string(ERR_get_error(), NULL));
		return 1;
	}

	signal(SIGPIPE, SIG_IGN);

	listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket < 0) {
		sz_log(0, "error socket %s", strerror(errno));
		return 1;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port_no);

	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	if (bind(listen_socket, (struct sockaddr *)&sin, sizeof(sin))) {
		sz_log(0, "error bind %s", strerror(errno));
		return 1;
	}

	if (listen(listen_socket, 1) < 0) {
		sz_log(0, "error listen %s", strerror(errno));
		return 1;
	}

	int never =  LDAP_OPT_X_TLS_NEVER; 
	int error = ldap_set_option(NULL, LDAP_OPT_X_TLS_REQUIRE_CERT, &never);
	if (error != LDAP_SUCCESS)
		sz_log(0, "Failed to set ldap options: %s", ldap_err2string(error));

#if 1
	go_daemon();
#endif

	main_loop(listen_socket, ssl_ctx);

	return 0;
}
