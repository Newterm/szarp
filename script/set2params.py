#!/usr/bin/python
"""
  Converts draw3 sets to params.xml.

  Script parses draw3 set export files listed as arguments and produces
  corresponding fragment of params.xml, containing the same set of params
  and draws in <defined> section. 

  Usage: set2params.py file1.xsd ...
"""
"""
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
"""
"""
  Pawel Palucha <pawel@newterm.pl>
"""

from lxml import etree
import sys

# namespaces
setns = "{http://www.praterm.com.pl/SZARP/draw3}"
ipk = "{http://www.praterm.com.pl/SZARP/ipk}"
ipk_nsmap = {None: ipk.strip('{}')}

def parse_xsd(path, ndefined):
	"""
	Parses path xsd file and appends found parameters to ndefined XML element.
	"""

	doc = etree.parse(path)
	windows = doc.getroot()

	for window in windows.findall(setns + 'window'):
		title = window.get('title').strip('*')
		for param in window.findall(setns + 'param'):
			draw = param.find(setns + 'draw')
			fparam = windows.find("./%sparam[@name='%s']" % (setns, param.get('name')))
	
			newp = etree.Element(ipk + 'param')
			newp.set('name', param.get('name'))
			newp.set('short_name', draw.get('short'))
			newp.set('draw_name', draw.get('title'))
			newp.set('unit', draw.get('unit'))
			newp.set('prec', fparam.get('prec'))
			newp.set('base_ind', 'auto')
			
			newdef = etree.Element(ipk + 'define')
			newdef.set('type', 'RPN')
			newdef.set('formula', 'null')
	
			newdraw = etree.Element(ipk + 'draw')
			newdraw.set('title', title)
			newdraw.set('min', draw.get('min'))
			newdraw.set('max', draw.get('max'))
			newdraw.set('color', draw.get('color'))
	
			script = etree.Element(ipk + 'script')
			script.text = etree.CDATA(fparam.text)
	
			newdef.append(script)
			newp.append(newdef)
			newp.append(newdraw)
			ndefined.append(newp)


ndefined = etree.Element(ipk + 'defined', nsmap = ipk_nsmap)

for f in sys.argv[1:]:
	parse_xsd(f, ndefined)

print etree.tostring(ndefined, encoding='iso-8859-2', pretty_print=True)

