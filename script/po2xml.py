#!/usr/bin/python
# -*- coding: utf-8 -*-
 

# Converts po lang files to SZARP dictionary (XML)

import os
import sys
import re

from lxml import etree
from optparse import OptionParser

DICTIONARY_NS = "http://www.praterm.com.pl/SZARP/ipk_dictionary"
NS = "{%s}" % DICTIONARY_NS

NS_MAP = {None: DICTIONARY_NS}


class Entry:
    def __init__(self):
        self.msgid = None
        self.msgstr = None
        self.comment = ""
        self.section = ""


class Po2Xml:
    def __init__(self, destination_lang, source_lang = 'pl'):
        self.slang = source_lang
        self.dlang = destination_lang
        
        self.entries = list()

    def parse(self, file_name):
        current_entry = None
        empty_entry = False
        
        try:
            po = file(file_name)

            print 'parsing file %s' % file_name
            
            for line in po:
                if re.match('^$', line):
                    if current_entry and current_entry.msgid is not None:
                        self.entries.append(current_entry)
                    current_entry = Entry()
                    empty_entry = False
                    continue
                else:
                    if empty_entry:
                        continue
                    
                m = re.match('(^msgid ")(.*)("$)', line)
                if m:
                    if m.group(2) == "":
                        empty_entry = True
                    else:
                        current_entry.msgid = m.group(2)
                    continue
                    
                m = re.match('(^msgstr ")(.*)("$)', line)
                if m:
                    current_entry.msgstr = m.group(2)
                    continue
            
                m = re.match('(^#: )(.*)', line)
                if m:
                    current_entry.section = m.group(2)
                    continue
            
                m = re.match('(^# )(.*)', line)
                if m:
                    current_entry.comment = m.group(2)
                    continue            
        finally:
            po.close()

    def write(self, out_file):
        root = etree.Element(NS + 'dictionary', nsmap=NS_MAP)
        
        draw_name = etree.Element(NS + 'draw_name')
        root.append(draw_name)
        
        param_name = etree.Element(NS + 'param_name')
        root.append(param_name)
        
        sections = { 'draw_name': draw_name, 'param_name': param_name }

        for entry in self.entries:
            e = etree.Element(NS + 'entry')
            s = sections.get(entry.section, draw_name)
            s.append(e)

            
            etree.SubElement(e, NS + 'translation', lang=self.slang, value=unicode(entry.msgid, 'utf-8'))
            etree.SubElement(e, NS + 'translation', lang=self.dlang, value=unicode(entry.msgstr, 'utf-8'))
            
            if entry.comment != "":
                etree.SubElement(e, NS + 'comment', content=unicode(entry.comment, 'utf-8'))
                
        out_file.write(etree.tostring(root, pretty_print=True, encoding='utf-8', xml_declaration=True))


def main():
    usg = "usage: %prog [options] LANG INFILE\n  LANG - destination language\n  INFILE - po file"
    parser = OptionParser(usage=usg)

    parser.add_option('-s', '--source', dest = 'source', help = 'source language', metavar = 'SRCLANG')
    parser.add_option('-o', '--output', dest = 'output', help = 'save output to file' , metavar = 'FILE')

    parser.set_defaults(source = 'pl')

    (options, args) = parser.parse_args()

    if len(args) < 2:
        parser.print_help()
        sys.exit(1)

    p2x = Po2Xml(args[0], options.source)
    
    p2x.parse(args[1])
    
    if options.output:
        o = file(options.output, 'w')
        p2x.write(o)
        o.close()
    else:
        p2x.write(sys.stdout)


if __name__ == '__main__':
    main()


