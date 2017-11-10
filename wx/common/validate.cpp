/*
 * SZARP: SCADA software (C) 2017
 *
 * Patryk Kulpanowski <pkulpanowski@newterm.pl>
 */
#ifndef _WIN32
#include "validate.h"

int ValidateConfig(wxString path) {
	char *c_path = new char[path.size() +1];
	std::copy(path.begin(), path.end(), c_path);
	c_path[path.size()] = '\0';

	/* Using quiet i2smo validation */
	const char *i2smo_argv[] = {PREFIX "/bin/i2smo", "-qi", c_path, 0};
	int ret;
	int i2smo_pid = 0;
	if ((i2smo_pid = fork()) == 0) {
		/* Possible stderr from i2smo */
		execv(i2smo_argv[0], const_cast<char**>(i2smo_argv));
	}
	else {
		waitpid(i2smo_pid, &ret, 0);
	}
	delete c_path;

	ret = WEXITSTATUS(ret);

	return ret;
}
#endif
