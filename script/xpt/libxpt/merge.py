# -*- coding: utf-8 -*-
 

# Classes for merging

from lxml import etree

from consts import *
from dictionaryxml import *
from paramsxml import *
from index import *

class MergePoWithDict:
    def __init__(self, po, dictionary):
        self.po = po
        self.dictionary = dictionary
        self.indexes = {}
        self.leave = {} 
        self.add = {}
        
    def merge(self):
        
        for tag in dictionary_sections:
            self.indexes[tag] = Node()
            self.leave[tag] = []
            self.add[tag] = []
            
            if self.dictionary:
                dict_entries = self.dictionary.fetchDictionaryEntries(tag)
                for entry in dict_entries:
                    if entry.getTranslation(self.po.slang):
                        self.indexes[tag].add(entry.getTranslation(self.po.slang), entry)
                    else:
                        self.leave[tag].append(entry)
        
        for poentry in self.po.entries:
            if self.indexes.has_key(poentry.section):
                find_result = self.indexes[poentry.section].findWithClientData(poentry.msgid)
                if find_result[0]:
                    entry = find_result[1]
                    entry.addTranslation(self.po.dlang, poentry.msgstr)
                    continue
                
            d = DictionaryEntry()
            d.addTranslation(self.po.slang, poentry.msgid)
            d.addTranslation(self.po.dlang, poentry.msgstr)
	    d.comment = poentry.comment
            self.add[poentry.section].append(d)
                    
                    
    def generatesSectionXML(self, sections, sections_entries, document):
        for section in sections:
            if sections_entries.has_key(section):
                for entry in sections_entries[section]:
                    entry.generateXML(document, sections[section])
                
    def generateXML(self, document, element):
    
        sections = {}
        
        for tag in dictionary_sections:
            section = document.createElementNS(DICTIONARY_NS, tag)
            sections[tag] = section
            element.appendChild(section)
        
            
        self.generatesSectionXML(sections, self.leave, document)
        
        self.generatesSectionXML(sections, self.add, document)
        
        if self.dictionary:
            tmp = {}
            for x in self.indexes:
                tmp[x] = self.indexes[x].dumpClientData() 
        
            self.generatesSectionXML(sections, tmp, document)

        return


class MergeParamsWithDict:
    def __init__(self, params, dictionary):
        self.params = params
        self.dictionary = dictionary
        self.valid = {}
        self.invalid = {}
        self.add = {}

    def generatesSectionXML(self, sections, sections_entries, document):
        for section in sections:
            if sections_entries.has_key(section):
                for entry in sections_entries[section]:
                    entry.generateXML(document, sections[section])

    def generateXML(self, document, element, write_valid=False, write_add=False, write_invalid=False):
        
        sections = {}
        
        for tag in dictionary_sections:
            section = document.createElementNS(DICTIONARY_NS, tag)
            sections[tag] = section
            element.appendChild(section)
            
        if write_valid:
            self.generatesSectionXML(sections, self.valid, document)

        if write_invalid:
            self.generatesSectionXML(sections, self.invalid, document)
            
        if write_add:
            self.generatesSectionXML(sections, self.add, document)
        
        return
        
    def merge(self):
        for tag in self.params.interesting:
            self.valid[tag] = []
            self.invalid[tag] = [] 
            dict_entries = self.dictionary.fetchDictionaryEntries(tag)
            for entry in dict_entries:
                if self.params.find(tag, entry.getIPKValue(), True):
                    self.valid[tag].append(entry)
                else:
                    self.invalid[tag].append(entry)
                    
    def addNew(self):
        for tag in self.params.interesting:
            self.add[tag] = []
            for value in self.params.getTerminals(tag):
                de = DictionaryEntry(value[0])
                de.comment = value[1]
                self.add[tag].append(de)

