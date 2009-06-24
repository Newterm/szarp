#!/usr/bin/python
# -*- coding: utf-8 -*-
 

# Converts params.xml files to SZARP dictionary (XML)

import os
import sys
import re

from lxml import etree
from optparse import OptionParser

import xml.dom.ext
import xml.dom.minidom

from libxpt.dictionaryxml import DictionaryXMLReader
from libxpt.paramsxml import ParamsXML
from libxpt.merge import MergeParamsWithDict
from libxpt.consts import *
    
def main():
    
    
#    tag_soup = '<body>&#xF3;&#x17C;</body>'
#    body = etree.fromstring(tag_soup)
#    print etree.tostring(body, pretty_print=True, encoding='utf-8', xml_declaration=True)
#
#    exit(1)
    
    
    usg = "usage: %prog [options] INFILE\n  INFILE - params.xml file"
    parser = OptionParser(usage=usg)

    parser.add_option('-o', '--output', dest = 'output', help = 'save output to file' , metavar = 'FILE')
    parser.add_option('-m', '--merge', dest = 'merge', help = 'merge entries with entries with the dictionary file' , metavar = 'FILE')
    parser.add_option('-d', '--delete-missing', action="store_true", dest = 'missing', help = 'delete from output entries present in the dictionary file and not present in params.xml')
    parser.add_option('-w', '--write-missing', dest = 'fmissing', help = 'write entries present in the dictionary file and not present in params.xml to file', metavar = 'FILE')
    
    (options, args) = parser.parse_args()

    if len(args) < 1:
        parser.print_help()
        sys.exit(1)
    
    paramsXML = ParamsXML(args[0])
    
    dictionaryXML = None
    
    if options.merge:
        dictionaryXML = DictionaryXMLReader(options.merge)
    
    m = MergeParamsWithDict(paramsXML, dictionaryXML)
    
    if dictionaryXML:
        m.merge()
    m.addNew()
    
    doc = xml.dom.minidom.Document()
    dictionary = doc.createElementNS(DICTIONARY_NS, "dictionary")
    doc.appendChild(dictionary)
    
    if not options.missing:
        m.generateXML(write_valid=True, write_add=True, write_invalid=True, document=doc, element=dictionary)
    else:
        m.generateXML(write_valid=True, write_add=True, document=doc, element=dictionary)
    
    if options.output:
        o = file(options.output, 'w')
        xml.dom.ext.PrettyPrint(doc, o)
        o.close()
    else:
        xml.dom.ext.PrettyPrint(doc)
        
    if options.fmissing:
        o = file(options.fmissing, 'w')
        doc = xml.dom.minidom.Document()
        dictionary = doc.createElementNS(DICTIONARY_NS, "dictionary")
        doc.appendChild(dictionary)
        m.generateXML(write_invalid=True, document=doc, element=dictionary)
        xml.dom.ext.PrettyPrint(doc, o)
        o.close()

if __name__ == '__main__':
    main()


