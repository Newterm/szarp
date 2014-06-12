#include "daemon.h"

#include <errno.h>
#include <unistd.h>

namespace ba = boost::asio;

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
				/* TODO: Log this (12/06/2014 21:45, jkotur) */
				std::cerr << "First fork failed: " << strerror(errno) << std::endl;
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
				/* TODO: Log this (12/06/2014 21:45, jkotur) */
				std::cerr << "Second fork failed: " << strerror(errno) << std::endl;
				return false;
			}
		}

		close(0);
		close(1);
		close(2);

		if (open("/dev/null", O_RDONLY) < 0) {
				/* TODO: Log this (12/06/2014 21:45, jkotur) */
			std::cerr << "Unable to open /dev/null: " << strerror(errno) << std::endl;
			return false;
		}

		// Inform the io_service that we have finished becoming a daemon.
		service.notify_fork(boost::asio::io_service::fork_child);

	} catch (std::exception& e) {
		/* TODO: Log this (12/06/2014 21:45, jkotur) */
		std::cerr << "Exception: " << e.what() << std::endl;
		return false;
	}

	return true;
}
