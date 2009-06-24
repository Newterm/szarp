# -*- coding: utf-8 -*-
 

# Classes for reading dictionary.xml

from lxml import etree

from consts import *

class DictionaryEntry:
    def __init__(self, default=None):
        self.translations = {}
        if default:
            self.translations['pl'] = default
        self.comment = None
        
    def addTranslation(self, language, translation):
        self.translations[language] = translation
        
    def getTranslation(self, language):
        if self.translations.has_key(language):
            return self.translations[language]
        else:
            return None
        
    def getIPKValue(self):
        return self.translations['pl']
    
    def generateXML(self, document, element):
        entry = document.createElementNS(DICTIONARY_NS, 'entry')
        
        for translation in self.translations:
            t = document.createElementNS(DICTIONARY_NS, 'translation')
            t.setAttributeNS(DICTIONARY_NS, "lang", translation)
            t.setAttributeNS(DICTIONARY_NS, "value", self.translations[translation].encode('utf-8'))
            entry.appendChild(t)
        if self.comment is not None:
            c = document.createElementNS(DICTIONARY_NS, 'comment')
            c.setAttributeNS(DICTIONARY_NS, "content", self.comment)
            entry.appendChild(c)
            
        element.appendChild(entry)
        return
    
    def dump(self):
        for x in self.translations:
            print x + " - " + self.translations[x]
        
class DictionaryXMLReader:
    def __init__(self, filename):
        self.tree = etree.parse(filename).getroot()
        
    def fetchDictionaryEntries(self, tag):
        ret = []
        category = self.tree.find("./"+ DICTIONARY_NS_LXML +  tag)

        if category is None:
            return ret
        
        for e in category.iterfind("./"+ DICTIONARY_NS_LXML  + 'entry'):
            entry = DictionaryEntry()
 
            comment_node = e.find("./"+ DICTIONARY_NS_LXML + 'comment')
            if comment_node != None:
                entry.comment = unicode(comment_node.get("content"))
            
            for t in e.iterfind("./"+ DICTIONARY_NS_LXML + 'translation'):
                entry.addTranslation(t.get("lang"), unicode(t.get("value")))
            
            ret += [entry]
           
        return ret
    
    
