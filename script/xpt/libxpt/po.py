# -*- coding: utf-8 -*-
 

# Classes for reading po files
import os
import sys
import re

from lxml import etree
from consts import *

class PoEntry:
    def __init__(self):
        self.msgid = None
        self.msgstr = None
        self.comment = ""
        self.section = ""


class PoReader:
    def __init__(self, file_name, destination_lang, source_lang = 'pl'):
        self.slang = source_lang
        self.dlang = destination_lang
        
        self.entries = list()

        current_entry = None
        empty_entry = False
        
        try:
            po = file(file_name)

            for line in po:
                if re.match('^$', line):
                    if current_entry and current_entry.msgid is not None:
                        self.entries.append(current_entry)
                    current_entry = PoEntry()
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
                        current_entry.msgid = unicode(m.group(2).decode('utf-8'))
                    continue
                    
                m = re.match('(^msgstr ")(.*)("$)', line)
                if m:
                    current_entry.msgstr = unicode(m.group(2).decode('utf-8'))
                    continue
            
                m = re.match('(^#: )(.*)', line)
                if m:
                    current_entry.section = m.group(2)
                    continue
            
                m = re.match('(^# )(.*)', line)
                if m:
                    current_entry.comment = unicode(m.group(2).decode('utf-8'))
                    continue            
        finally:
            po.close()


class PoGenerator:
    def __init__(self, dictionary, slang, dlang):
        self.dictionary = dictionary;
        self.slang = slang;
        self.dlang = dlang;
        
    def generatePo(self, file=None, sect=None):

        dump_sections = dictionary_sections
        if sect is not None:
            dump_sections = [sect]

        if file is None:
            file = sys.stdout

        print >> file, 'msgid ""'
        print >> file, 'msgstr ""'
        print >> file, '"Project-Id-Version: \\n"'
        print >> file, '"Report-Msgid-Bugs-To: \\n"'
        print >> file, '"POT-Creation-Date: \\n"'
        print >> file, '"PO-Revision-Date: \\n"'
        print >> file, '"Last-Translator: \\n"'
        print >> file, '"MIME-Version: 1.0\\n"'
        print >> file, '"Content-Type: text/plain; charset=UTF-8\\n"'
        print >> file, '"Content-Transfer-Encoding: 8bit\\n"'
        
        for section in dump_sections:
            self.generateSection(section, file)

    def generateSection(self, section, file):

        for entry in self.dictionary.fetchDictionaryEntries(section):
            print >> file, ""
            print >> file, "#: ", section

            if(entry.comment):
                print >> file, '# ', entry.comment.encode('utf-8')
            else:
                print >> file, "# "
                
            print >> file, 'msgid "' + entry.getTranslation(self.slang).encode('utf-8') + '"'
              
            if(entry.getTranslation(self.dlang)):
                print >> file, 'msgstr "' + entry.getTranslation(self.dlang).encode('utf-8') + '"'
            else:
                print >> file, 'msgstr ""'


