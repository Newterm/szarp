
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <sys/types.h>

namespace sz4 {

class file_lock_error : public std::runtime_error {
public:
	file_lock_error(const std::string& error) : std::runtime_error(error) {}
};

class file_open_error : public std::runtime_error {
public:
	file_open_error(const std::string& error) : std::runtime_error(error) {}
};

int open_readlock (const char* pathname, int flags = O_RDONLY);
int open_writelock (const char* pathname, int flags = O_WRONLY, mode_t mode = 0666);
int close_unlock (int fd);
int upgrade_lock (int fd);

}

