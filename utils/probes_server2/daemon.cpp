#include "daemon.h"

#include <errno.h>
#include <unistd.h>

#include <liblog.h>

namespace ba = boost::asio;

/* FIXME: This file is only linux specific file in this project. Do we need
 * windows implementation? Probably not but if so only this file have to be
 * updated.
 * (14/06/2014 18:35, jkotur)
 */

bool daemonize( ba::io_service& service )
{
	try {
		// Inform the io_service that we are about to become a daemon.
		service.notify_fork(ba::io_service::fork_prepare);

		if( pid_t pid = fork() ) {
			if (pid > 0) {
				service.notify_fork(ba::io_service::fork_parent);
				exit(0);
			} else {
				sz_log(0, "Daemonize: First fork failed: %s", strerror(errno));
				return false;
			}
		}

		setsid();
		(void)chdir("/");
		umask(0);

		service.notify_fork(ba::io_service::fork_prepare);
		if( pid_t pid = fork() ) {
			if (pid > 0) {
				service.notify_fork(ba::io_service::fork_parent);
				exit(0);
			} else {
				sz_log(0, "Daemonize: Second fork failed: %s", strerror(errno));
				return false;
			}
		}

		close(0);
		close(1);
		close(2);

		if (open("/dev/null", O_RDONLY) < 0) {
			sz_log(0, "Daemonize: Unable to open /dev/null: %s", strerror(errno));
			return false;
		}

		// Inform the io_service that we have finished becoming a daemon.
		service.notify_fork(boost::asio::io_service::fork_child);

	} catch (std::exception& e) {
		sz_log(0, "Daemonize: Exception: %s", e.what());
		return false;
	}

	return true;
}

unsigned get_pid()
{
	return (unsigned)getpid();
}

