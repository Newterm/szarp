#!/usr/bin/env python

import os , re

from distutils.core import setup
from distutils.command.build import build
from distutils.spawn import find_executable, spawn

CHANGELOG = '../debian/changelog'
DEFAULT_VERSION='3.0.0'

def get_version():
	ver = DEFAULT_VERSION
	changelog = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), CHANGELOG ))
	for line in changelog:
		m = re.search('^szarp\s\(([^)]*)\)', line)
		if m is not None:
			ver = m.group(1)
			break
	changelog.close()
	return ver

ui_dirs = [ 'qtipk/ui/' ]

class QtUiBuild(build):
	def __init__( self , dist ) :
		build.__init__(self,dist)

		# Search for pyuic4 in python bin dir, then in the $Path.
		try:
			from PyQt4 import pyqtconfig
		except ImportError:
			pyuic_exe = None
		else:
			pyqt_configuration = pyqtconfig.Configuration()
			pyuic_exe = find_executable('pyuic4', pyqt_configuration.default_bin_dir)
		if not pyuic_exe: pyuic_exe = find_executable('pyuic4')
		if not pyuic_exe: pyuic_exe = find_executable('pyuic4.bat')
		if not pyuic_exe: print "Unable to find pyuic4 executable"; return

		self.pyuic_exe = pyuic_exe

	def compile_ui(self, ui_file, py_file, ui_dir):
		cmd = [self.pyuic_exe, ui_dir+ui_file, '-o', ui_dir+py_file]

		try:
			spawn(cmd)
		except:
			print self.pyuic_exe + " is a shell script"
			cmd = ['/bin/sh', '-e', self.pyuic_exe, ui_dir+ui_file, '-o', ui_dir+py_file]
			spawn(self.cmd)

	def run(self):
		for ui_dir in ui_dirs :
			for ui_file in os.listdir(ui_dir) :
				if ui_file.endswith('.ui') : #and not os.path.exists(ui_file[:-3]+'.py') : ?? check timestamp?
					self.compile_ui(ui_file,ui_file[:-3]+'.py',ui_dir)
		build.run(self)

setup(name='pyipkedit',
	version=get_version(),
	description='Sets of scripts and libraries to easily manage params.xml file',
	author='Jakub Kotur',
	author_email='qba@newterm.pl',
	url='http://www.newterm.pl',
	packages=['libipk','qtipk','qtipk.ui'],
	scripts=['bin/ipkcmd' , 'bin/ipkqt'],
	data_files=[ ('/opt/szarp/lib/plugins/' , [ 'plugins/' + f for f in os.listdir('plugins') if f.endswith('plg.py') ] ) ],
	cmdclass = { 'build' : QtUiBuild },  # define custom build class
	)

