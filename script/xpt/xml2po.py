#!/usr/bin/python
# -*- coding: utf-8 -*-
 

# Converts po lang files to SZARP dictionary (XML)

import os
import sys
import re

from lxml import etree
from optparse import OptionParser
from libxpt.po import *
from libxpt.merge import *

def main():
    usg = "usage: %prog [options] LANG INFILE\n  LANG - destination language\n  INFILE - xml file"
    parser = OptionParser(usage=usg)

    parser.add_option('-s', '--source', dest = 'source', help = 'source language', metavar = 'SRCLANG')
    parser.add_option('-c', '--sectione', dest = 'section', help = 'section', metavar = 'SECTION')
    parser.add_option('-m', '--merge', dest = 'merge', help = 'merge entries with entries with the dictionary file' , metavar = 'FILE')
    parser.add_option('-o', '--output', dest = 'output', help = 'save output to file' , metavar = 'FILE')

    parser.set_defaults(source = 'pl')

    (options, args) = parser.parse_args()

    if len(args) < 2:
        parser.print_help()
        sys.exit(1)

    
    dictionaryXML = DictionaryXMLReader(args[1]) 
    
    pog = PoGenerator(dictionaryXML, options.source, args[0])

    if options.output:
        o = file(options.output, 'w')
        pog.generatePo(o, sect=options.section)
        o.close()
    else:
        pog.generatePo(sect=options.section)


if __name__ == '__main__':
    main()
