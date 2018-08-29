import xml.parsers.expat
import re
import argparse
import sys
reload(sys)
sys.setdefaultencoding('utf8')

argParser = argparse.ArgumentParser()
argParser.add_argument('path_to_params', help = 'path to params.xml')
args = argParser.parse_args()

with open(args.path_to_params, 'r') as file:
	output = file.readlines()

def start_element(name, attrs):
	# Find prefixes indicating the same url as "extra"
	toSearch = ["params", "device", "unit"]
	if name in toSearch:
		url = "http://www.praterm.com.pl/SZARP/ipk-extra"
		pairs = []
		for option,value in zip(attrs[0::2], attrs[1::2]):
			pairs.append(option + '=\"' + value + '\"')

		toSave = ["xmlns:extra"]
		for string in pairs:
			if url in string:
				if not any(stringToSave in string for stringToSave in toSave):
					toRemove.append(string)

		# Get prefixes to change to extra in whole params.xml
		for string in toRemove:
			start = len("xmlns:")
			end = string.find("=")
			prefix = string[start:end]
			if prefix not in attrsToChange:
				attrsToChange.append(prefix)

		# Form params/device tag
		tag = "<" + name + " "
		for attribute in pairs:
			if any(fakeExtra in attribute for fakeExtra in toRemove):
				print "Deleting " + attribute + " from line: " + str(p.CurrentLineNumber)
			else:
				pa, value = attribute.split("=\"")
				if any(toChange in pa for toChange in attrsToChange):
					if "documentation" in pa:
						tag = tag + attribute + " "
					else:
						prefix,attrib = pa.split(":")
						tag = tag + "extra:" + attrib + "=\"" + value + " "
				else:
					tag = tag + attribute + " "
		tag = tag.strip()

		# Check if param has closing tag or not
		actualLine = output[p.CurrentLineNumber - 1]
		lastPartOfActualLine = (actualLine.split("\""))[-1]
		if '/' not in lastPartOfActualLine:
			tag = tag + '>\n'
		else:
			tag = tag + '/>\n'

		# Set proper tag indentation
		actualLine = output[p.CurrentLineNumber - 1]
		numOfWhitespaces = len(actualLine) - len(actualLine.lstrip())
		lineIndent = numOfWhitespaces * ' '
		output[p.CurrentLineNumber - 1] = lineIndent + tag

	searchChangeToExtra = ["param", "send"]
	if name in searchChangeToExtra:
		tag = "<" + name + " "
		if (name == "param"):
			debug = []
			# Sorting known attributes
			for element in orderOfAttributes:
				i = 0
				for option,value in zip(attrs[0::2], attrs[1::2]):
					i = i + 2
					if any(toChange in option for toChange in attrsToChange):
						prefix,attrib = option.split(":")
						toCheck = "extra:" + attrib
						if element == toCheck:
							tag = tag + "extra:" + attrib + "=\"" + value + "\" "
							debug.append(option)
							debug.append(value)
					else:
						if element == option:
							tag = tag + option + "=\"" + value + "\" "
							debug.append(option)
							debug.append(value)

			# Adding unknown attributes at the end of tag
			for option,value in zip(attrs[0::2], attrs[1::2]):
				if any(toChange in option for toChange in attrsToChange):
					prefix,attrib = option.split(":")
					toCheck = "extra:" + attrib
					if toCheck not in tag:
						tag = tag + "extra:" + attrib + "=\"" + value + "\" "
						debug.append(option)
						debug.append(value)
						print('Unknown attribute: ' + option + '=\"' + value + '\"')
						print('Adding at the end of tag')
				else:
					if option not in tag:
						tag = tag + option + "=\"" + value + "\" "
						debug.append(option)
						debug.append(value)
						print('Unknown attribute: ' + option + '=\"' + value + '\"')
						print('Adding at the end of tag')

			# Check if amount of output attrs is same as input attrs
			if(len(debug) != i):
				print("Something went wrong, some attributes are missing")
				print(tag)
				print(p.CurrentLineNumber)
				err = True
		else:
			for option,value in zip(attrs[0::2], attrs[1::2]):
					if any(toChange in option for toChange in attrsToChange):
						prefix,attrib = option.split(":")
						tag = tag + "extra:" + attrib + "=\"" + value + "\" "
					else:
						tag = tag + option + "=\"" + value + "\" "
		tag = tag.strip()

		# Check if param has closing tag or not
		actualLine = output[p.CurrentLineNumber - 1]
		lastPartOfActualLine = (actualLine.split("\""))[-1]
		if '/' not in lastPartOfActualLine:
			tag = tag + '>\n'
		else:
			tag = tag + '/>\n'

		# Set proper tag indentation
		actualLine = output[p.CurrentLineNumber - 1]
		numOfWhitespaces = len(actualLine) - len(actualLine.lstrip())
		lineIndent = numOfWhitespaces * ' '
		output[p.CurrentLineNumber - 1] = lineIndent + tag

attrsToChange = []
toRemove = []
debug = []
err = False
orderOfAttributes = ["name", "draw_name", "short_name", "unit", "prec", "base_ind", "type", "extra:val_type", "extra:address", "extra:param", "extra:probe-type", "extra:val_op", "extra:val_op2", "extra:register_type", "extra:prec", "extra:db_type", "extra:db", "extra:number", "extra:type", "extra:multiplier", "extra:register", "extra:word", "extra:item", "extra:FloatOrder", "extra:DoubleOrder", "extra:devid", "extra:parname", "extra:lswmsw", "extra:max", "extra:min", "extra:export", "extra:function", "extra:extra", "extra:transform", "extra:special", "extra:divisor", "extra:topic", "extra:docpath", "data_type", "icinga:no_data_timeout", "icinga:test_param", "icinga:flat_time", "icinga:max", "icinga:min", "ip_addr"]
p = xml.parsers.expat.ParserCreate()
p.ordered_attributes = True

p.StartElementHandler = start_element
p.ParseFile(open(args.path_to_params, "r"))
with open('params.xml-sorted', 'w') as file:
	file.writelines( output )

if err == False:
	print('Succesfully changed order of param attributes!')
	print('Parsed file is named params.xml-sorted')
	print('Before using check its corectness by i2smo')
