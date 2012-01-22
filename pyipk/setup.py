#!/usr/bin/env python

from distutils.core import setup

import os , re

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

setup(name='pyipkedit',
      version=get_version(),
      description='Sets of scripts and libraries to easily manage params.xml file',
      author='Jakub Kotur',
      author_email='qba@newterm.pl',
	  url='http://www.newterm.pl',
      packages=['libipk','qtipk'],
	  scripts=['bin/ipkcmd' , 'bin/ipkqt'],
	  data_files=[ ('/opt/szarp/lib/plugins/' , [ 'plugins/' + f for f in os.listdir('plugins') if f.endswith('plg.py') ] ) ]
     )

