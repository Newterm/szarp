#!/usr/bin/python
# vim: set fileencoding=ISO-8859-2 :

"""
This scripts scans SZARP drivers (line daemons) source files and creates
daemons documentation as SGML entity to include in documentation.
"""

from optparse import OptionParser
from glob import glob
import tokenize
import os.path
import sys

def tokenizer(f):
	buf = ""
	for line in f:
		for c in line:
			if c == '\r':
				pass
			elif c in [ ' ', '\t', '\n' ]:
				if len(buf) > 0:
					yield(buf)
					buf = ""
				yield c
			else:
				buf += c
	if len(buf) > 0:
		yield(buf)

class Scanner:
	def __init__(self):
		self.commands = [ 'description_start', 
				'description_end',
				'class', 
				'protocol', 
				'devices',
				'comment',
				'config',
				'config_example' ]
		self.content = dict()

	def parse_command(self, command):
		try:
			cmd, _, lang = command.partition(".")
		except ValueError:
			cmd = command
			lang = ""
		if cmd == "description_end":
			return True
		if cmd in self.commands:
			self.state = cmd
			self.lang = '.' + lang
		return False

	def feed(self, token):
		if self.state.startswith('description') or self.state == "init":
			return
		key = self.state + self.lang
		if not self.content.has_key(key):
			self.content[key] = ""
		self.content[key] += token
		
	def scan_file(self, path):
		self.name = os.path.basename(path).partition('.')[0]
		self.state = "init"
		self.content.clear()
		for token in tokenizer(open(path, "r")):
			if token[0] == '@':
				if self.parse_command(token[1:]):
					return
			else:
				self.feed(token)
	
	def print_content(self):
		print '<section id="daemon-' + self.name.replace("_", "-") + '">'
		print '<title>Sterownik ' + self.name + '</title>'
		print '<itemizedlist>'
		print '<listitem><para>Zgodność ze specyfikacją:',
		if self.content.has_key('class.pl'):
			print '<emphasis>' + self.content['class.pl'] + '</emphasis>',
		elif self.content.has_key('class.'):
			print '<emphasis>' + self.content['class.'] + '</emphasis>',
		else:
			print "nieznana",
		print '.</para></listitem>'
		self.print_section('devices', 'Obsługiwane urządzenia: ')
		self.print_section('protocol', 'Protokół komunikacji: ')
		self.print_section('comment', '')
		self.print_section('config', 'Konfiguracja: ')
		self.print_listing('config_example', 'Przykładowa konfiguracja: ')
		print '</itemizedlist>\n</section>'

	def print_section(self, sect, description):
		if self.content.has_key(sect + '.pl'):
			content = self.content[sect + '.pl']
		elif self.content.has_key(sect + '.'):
			content = self.content[sect + '.']
		else:
			return
		print ("<listitem><para>" + description   
				+ content.replace('\n\n', '</para>\n<para>')
				+ '</para></listitem>')

	def print_listing(self, sect, description):
		if self.content.has_key(sect + '.pl'):
			content = self.content[sect + '.pl']
		elif self.content.has_key(sect + '.'):
			content = self.content[sect + '.']
		else:
			return
		print ("<listitem><para>" + description + '\n<programlisting>\n<![CDATA[\n'
				+ content + ']]></programlisting></para></listitem>')

def scan_files(srcdir):
	files = sorted(glob(srcdir + "/*dmn.c?") + glob(srcdir + "/boruta_*.cc"))
	s = Scanner()
	for f in files:
		s.scan_file(f)
		s.print_content()

parser = OptionParser(usage="usage: %prog [options] [daemons source directory]\nCreates SZARP daemons documentation.")
(options, args) = parser.parse_args()

if len(args) == 1:
	srcdir = args[0].rstrip('/')
elif len(args) > 1:
	parser.error("to many arguments")
else:
	srcdir = '.'

if not os.path.isdir(srcdir):
	print "Directory does not exist: ", srcdir
	sys.exit(1)

scan_files(srcdir)

