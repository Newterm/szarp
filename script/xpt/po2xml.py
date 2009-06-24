#!/usr/bin/python
# -*- coding: utf-8 -*-
 

# Converts po lang files to SZARP dictionary (XML)

import os
import sys
import re

import xml.dom.ext
import xml.dom.minidom

from lxml import etree
from optparse import OptionParser
from libxpt.po import *
from libxpt.merge import *

def main():
    usg = "usage: %prog [options] LANG INFILE\n  LANG - destination language\n  INFILE - po file"
    parser = OptionParser(usage=usg)

    parser.add_option('-s', '--source', dest = 'source', help = 'source language', metavar = 'SRCLANG')
    parser.add_option('-m', '--merge', dest = 'merge', help = 'merge entries with entries with the dictionary file' , metavar = 'FILE')
    parser.add_option('-o', '--output', dest = 'output', help = 'save output to file' , metavar = 'FILE')

    parser.set_defaults(source = 'pl')

    (options, args) = parser.parse_args()

    if len(args) < 2:
        parser.print_help()
        sys.exit(1)

    poreader = PoReader(args[1], args[0], options.source)
    
    dictionaryXML = None
    
    if options.merge:
        dictionaryXML = DictionaryXMLReader(options.merge) 
    
    m = MergePoWithDict(poreader, dictionaryXML)
    
    m.merge()
    
    doc = xml.dom.minidom.Document()
    dictionary = doc.createElementNS(DICTIONARY_NS, "dictionary")
    doc.appendChild(dictionary)
    
    out = m.generateXML(document=doc, element=dictionary)
        
    if options.output:
        o = file(options.output, 'w')
        xml.dom.ext.PrettyPrint(doc, o)
        o.close()
    else:
        xml.dom.ext.PrettyPrint(doc)


if __name__ == '__main__':
    main()


