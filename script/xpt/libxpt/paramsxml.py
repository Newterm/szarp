# -*- coding: utf-8 -*-
 

# Classes for reading params.xml

from lxml import etree
from consts import *
from index import *
    
class ParamsXML:
    def __init__(self, filename):
        self.tree = etree.parse(filename).getroot()
        
        self.interesting = {}
        
        self.fetch(".", [{'out':'title', 'in':'title'}])
        self.fetch(".//{"+ IPK_NS + "}" + "draw", [{'out':'draw_title', 'in':'title'}])
        self.fetch(".//{"+ IPK_NS + "}" + "raport", [{'out':'raport_title','in':'title'}])
        self.fetch(".//{"+ IPK_NS + "}" + "param", [{'out':'param_name','in':'name', 'split':lambda x: x.split(":")},
                             {'out':'short_name','in':'short_name'},
                             {'out':'draw_name','in':'draw_name', 'comment':lambda x: unicode(x.get('name')).split(':')[2] }])
        
    def fetch(self, tag, attributes):
        for attr in attributes:
            self.interesting[attr['out']] = Node()
            
        for x in self.tree.iterfind(tag):
            for attr in attributes:
                if x.get(attr['in']):
                    comment = None
                    if attr.has_key('comment'):
                        comment = attr['comment'](x)

                    if attr.has_key('split'):
                        for s in attr['split'](unicode(x.get(attr['in']))):
                            self.interesting[attr['out']].add(s, comment)
                    else:
                        self.interesting[attr['out']].add(unicode(x.get(attr['in'])), comment)
                    
    def find(self, tag, text, subtract=False):
        return self.interesting[tag].find(text, subtract);
    
#    def dump(self):
#        for x in self.interesting:
#            print x
#            print self.interesting[x].dump();
            
    def getTerminals(self, tag):        
        return self.interesting[tag].dumpWithClientData();
    
