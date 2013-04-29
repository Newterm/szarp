#!/usr/bin/python
# -*- coding: UTF-8 -*-

import re
import sys
from copy import deepcopy
from optparse import OptionParser

"""
Utility for adding treenode elements to params.xml
"""

def printerr(string):
	sys.stderr.write('ERROR: ' + string + '\n')

def unique(ordered):
	seen = set()
	unique_elements = []
	for element in ordered:
		if element in seen:
			continue
		unique_elements.append(element)
		seen.add(element)
	return unique_elements


class XmlEncodingInfo():
	"""
	Path and encoding information of an xml file
	"""
	def __init__(self, path):
		self.path = path
		reg_empty = re.compile("^\s*$")
		reg_header = re.compile('^<\?xml[^?]+encoding="(?P<encoding>[^"]+)"\?>\s*$')
		with open(self.path) as f:
			self.encoding = None
			for line in f:
				if reg_empty.match(line):
					continue
				line = re.sub("'", '"', line)
				res = reg_header.match(line)
				if res == None:
					break
				self.encoding = res.group("encoding").lower()
				print self.path + " encoding: " + self.encoding
			if self.encoding == None:
				print "ERROR: no xml header in file " + self.path
				exit(1)


class TreeNodeElement():
	"""
	Represents a 'treenode' element, contains reference to his parent
	"""
	def __init__(self, prior, name, parent=None):
		self.prior = prior	# position of current node in parent_node
		self.name = name
		self.parent = parent
		self.draw_prior = None

	def set_draw_prior(self, draw_prior):
		self.draw_prior = draw_prior	# position of current set in current node

	def generate_element(self, base_indent, use_spaces=False, spaces_num=0):
		return self._generate_element(base_indent, 1, use_spaces, spaces_num)
	
	def _generate_element(self, base_indent, indent_size, use_spaces, spaces_num):
		if use_spaces:
			indent = ' ' * spaces_num
		else:
			indent = '\t'
		string = base_indent + indent * indent_size + '<treenode '
		if self.draw_prior != None:
			string = string + ' draw_prior="' + str(self.draw_prior) + '"'
		string = string + ' prior="' + str(self.prior) +'" name="' + self.name + '"'
		if self.parent == None:
			string = string + '/>\n'
		else:
			parent_string = self.parent._generate_element(base_indent, indent_size + 1, use_spaces, spaces_num)
			string = string + '>\n' + parent_string + base_indent + indent * indent_size + '</treenode>\n'
		return string


class CategoryTree():
	"""
	Builds category hierarchy
	"""
	def __init__(self, filename):
		self.filename = filename
		with open(filename) as f:
			self.contents = f.read()
		self.lines = self.contents.split('\n')
		self.reg_comment = re.compile('(<!--)|(-->)')
		self.reg_draw = re.compile('<draw[^>]+title="(?P<title>[^"]+)"[^>]*/>')
		self.check_lines()
		enc_check = XmlEncodingInfo(filename)
		self.encoding = enc_check.encoding
		self.rules = []
	
	def check_lines(self):
		reg_draw_incomplete = re.compile('<draw ')
		for line in self.lines:
			if self.reg_comment.search(line) != None:
				continue
			if self.reg_draw.search(line) != None:
				pass
			else:
				if reg_draw_incomplete.search(line) != None:
					printerr('Incomplete "<draw" line found, aborting: ')
					printerr(line)
					exit(1)

	def add_rule(self, regex, categories):
		regex = regex.decode('utf-8').encode(self.encoding)
		categories = categories.decode('utf-8').encode(self.encoding)
		self.rules.append((regex, categories))
	
	def add_default_category(self, category):
		category = category.decode('utf-8').encode(self.encoding)
		self.default_category = category

	def build(self):
		self.categories = {}
		self.draw_priors = {}
		reg_prior = re.compile('prior="(?P<prior>[0-9.]+)"')
		for line in self.lines:
			if self.reg_comment.search(line) != None:
				continue
			res = self.reg_draw.search(line)
			if res != None:
				title = res.group('title')
				res_prior = reg_prior.search(line)
				if res_prior != None:
					prior = res_prior.group("prior")
					if not title in self.draw_priors:
						self.draw_priors[title] = prior
					elif self.draw_priors[title] > prior:
						self.draw_priors[title] = prior
				categorized = False
				for rule in self.rules:
					regex = rule[0]
					categories = rule[1]
					res = re.match(regex, title)
					if res != None:
						self.add_to_categories(eval(categories), title)
						categorized = True
						break
				if not categorized:
					self.add_to_categories(eval(self.default_category), title)
		self.build_tree_nodes()
	
	def add_to_categories(self, path, item, categories=None):
		if categories == None:
			categories = self.categories
		category = path.pop(0)
		if len(path) == 0:
			if not category in categories.keys():
				categories[category] = []
			categories[category].append(item)
		else:
			if not category in categories.keys():
				categories[category] = {}
			self.add_to_categories(path, item, categories[category])

	def build_tree_nodes(self):
		self.tree_nodes = {}	# set name -> bottom-level treenode
		self._build_tree_nodes(self.categories, None)
	
	def _build_tree_nodes(self, categories, parent):
		if type(categories) is dict:
			prior = 0
			for key in sorted(categories):
				node = TreeNodeElement(prior, key, parent)
				self._build_tree_nodes(categories[key], node)
				prior = prior + 1
		if type(categories) is list:
			for item in unique(categories):
				node = deepcopy(parent)
				if item in self.draw_priors:
					node.draw_prior = self.draw_priors[item]
				self.tree_nodes[item] = node
		
	def printdict(self, depth, dictionary, print_xml):
		if type(dictionary) is dict:
			for key in sorted(dictionary):
				print '\t' * depth + '-> ' + key
				self.printdict(depth + 1, dictionary[key], print_xml)
		if type(dictionary) is list:
			for item in unique(dictionary):
				print '\t' * depth + item
				if print_xml:
					print self.tree_nodes[item].generate_element('\t' * depth)

	def printall(self, print_xml=False):
		self.printdict(0, self.categories, print_xml)

	def write_to_xml(self, use_spaces, spaces_num):
		reg_indent = re.compile('^(?P<whitespace>\s*)<draw.*$')
		reg_element_end = re.compile('/>\s*$')
		with open(self.filename, 'w') as f:
			for line in self.lines:
				if self.reg_comment.search(line) == None:
					res = self.reg_draw.search(line)
					if res != None:
						title = res.group('title')
						res2 = reg_indent.match(line)
						draw_whitespace = res2.group('whitespace')
						if title in self.tree_nodes:
							treenode = self.tree_nodes[title]
							del self.tree_nodes[title]
							treenode_element = treenode.generate_element(draw_whitespace, use_spaces, spaces_num)
							if reg_element_end.search(line) != None:
								line = reg_element_end.sub('>', line)
								line = line + '\n' + treenode_element + draw_whitespace + '</draw>'
							else:
								line = line + '\n' + treenode_element
				line = line + '\n'
				f.write(line)

parser = OptionParser(usage="usage: %prog [options]\nModifies params.xml in current dir according to hardcoded rules (edit to modify)")
parser.add_option("-s", "--spaces", help="indent with spaces (if not provided, indents with tabs)",
		action="store_true", dest="spaces", default=False)
parser.add_option("-n", "--num-spaces", help="number of spaces in single indent (ignored if tabs used)",
		action="store", type="int", dest="num_spaces")
parser.add_option("-p", "--print-xml", help="print xml nodes on screen, together with hierarchy",
		action="store_true", dest="print_xml", default=False)
parser.add_option("-w", "--write", help="writes treenode elements to params.xml (by default only prints hierarchy)",
		action="store_true", dest="write", default=False)

(options, args) = parser.parse_args()

if options.spaces and (options.num_spaces == None):
	printerr("number of spaces in indent not provided")
	exit(1)

categories = CategoryTree("params.xml")

# CategoryTree is built using rules.
# Rules are evaluated in the order they were added, at each 'title' attr from 'draw' element.
# If a rule is successful, no more rules will be evaluated at given 'title'.
# a single rule consist of:
# - regex for parsing 'title' attribute from 'draw' element
# - piece of code evaluating to a list of category names, from top-level
# the code can access regex result by variable 'res'
categories.add_rule("^([^0-9]* )*(?P<number>[0-9]+)( .*)*$", "['Kotły', 'Kocioł ' + res.group('number')]")
categories.add_rule("^([^0-9]* )*K(?P<number>[0-9]+)( .*)*$", "['Kotły', 'Kocioł ' + res.group('number')]")
#categories.add_rule("^(W|w)ęzeł (?P<name>[^ ]+( .*)*)$", "['Węzły']")
categories.add_rule("^(W|w)ęzeł (?P<name>[^ ]+)( .*)*$", "['Węzły', 'Węzeł ' + res.group('name')]")
categories.add_default_category("['Nadrzędna']")

categories.build()
if options.write:
	print "writing params.xml..."
	categories.write_to_xml(options.spaces, options.num_spaces)
	print "wrote changes"
else:
	categories.printall(print_xml=options.print_xml)
