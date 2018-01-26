#!/usr/bin/python
# -*- encoding: utf-8 -*-

#script to supervise repository information
#allows to track commits, lines and files counter
#checks each minute number of lines and files
#and every hour number of commits

import sys                                  # exit()
import time                                 # sleep()
import logging
from logging.handlers import SysLogHandler
import subprocess
from subprocess import PIPE, Popen

suffixes = [ "cpp", "h", "txt", "hpp", "py", "cc" ]

REPO_PATH = "absolute_repository_path"
BRANCH = "branch_name"

class RepoReader:
	def get_repo_lines(self, path):
		path = path.strip()
		if path.endswith("/") :
			path = path[:-1]
		p = subprocess.Popen("git -C %s ls-files" % path, shell=True,stdout=PIPE)
		if p.wait():
			raise Exception("getting repo lines failed")
		files = p.communicate()[0].strip().split('\n')
		counter = 0
		files_counter = 0
		for f in files :
			if not f.split(".")[-1] in suffixes:
				continue
			lines = subprocess.Popen("cat %s/%s | wc -l"%(path,f),shell=True, stdout=PIPE)
			if lines.wait():
				raise Exception("getting lines from file failed")
			counter += int(lines.communicate()[0].strip())
			files_counter += 1
		return (counter, files_counter)

	def get_repo_commits(self, path, branch):
		p = subprocess.Popen("git -C %s rev-list --count %s" % (path, branch), shell=True, stdout=PIPE)
		if p.wait():
			raise Exception("getting repo commits failed")
		return int(p.communicate()[0].strip())

	def update_commits(self):
		try:
			commits = self.get_repo_commits(REPO_PATH, BRANCH)
			ipc.set_read(2, commits )
		except Exception as err:
			ipc.set_no_data(2)

	#only update memory segments of changed variables
	#remember to call go_parcook to update values
	def update_lines(self):
		try:
			lines, files = self.get_repo_lines(REPO_PATH)
			ipc.set_read(0, lines)
			ipc.set_read(1, files)
		except Exception as err:
			logger.error("failed to fetch data, %s", str(err).splitlines()[0])
			ipc.set_no_data(0)
			ipc.set_no_data(1)
		ipc.go_parcook()


def main(argv=None):
	global logger
	logger = logging.getLogger('pdmn_git.py')
	logger.setLevel(logging.DEBUG)
	handler = logging.handlers.SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON)
	handler.setLevel(logging.WARNING)
	formatter = logging.Formatter('%(filename)s: [%(levelname)s] %(message)s')
	handler.setFormatter(formatter)
	logger.addHandler(handler)

	if 'ipc' not in globals():
		global ipc
		sys.path.append('/opt/szarp/lib/python')
		from test_ipc import TestIPC
		ipc = TestIPC("example-base", "/opt/szarp/example-base/pdmn_git.py")

	ex = RepoReader()
	time.sleep(10)

	sleep_counter = 0
	ex.update_commits()
	while True:
		#check commits only each hour
		if sleep_counter > 60 :
			ex.update_commits()
			sleep_counter = 0
		sleep_counter += 1
		ex.update_lines()
		time.sleep(60)

# end of main()

if __name__ == "__main__":
	sys.exit(main())
