# -*- coding: utf-8 -*-
 

# Classes for reading dictionary.xml

from lxml import etree

class Node:
    def __init__(self):
        self.children = {}
        self.isTerminal = False;
        self.clientData = None
        
    def add(self, text, clientData=None):
        if len(text) == 0:
            self.isTerminal = True;
            self.clientData = clientData
        else:
            c, text = self.getNextChar(text)
                    
            if not self.children.has_key(c):
                self.children[c] = Node()
                
            self.children[c].add(text, clientData)
            
    
    def find(self, text, subtract=False):
        self.findWithClientData(text, subtract)[0]
            
    def findWithClientData(self, text, subtract=False):
        if len(text) == 0:
            
            if self.isTerminal:
                if subtract:
                    self.isTerminal = False
                return (True, self.clientData)
            else:
                return (False, None);
        
        else:
            c, text = self.getNextChar(text)
                    
            if not self.children.has_key(c):
                return (False, None);
                
            return self.children[c].findWithClientData(text, subtract)
            
    def getNextChar(self, text):
        c = text[0]
        text = text[1:]
        
        if c.isdigit():
            c = "*"
            if len(text) > 0:
                t = text[0]
                while t.isdigit():
                    text = text[1:]
                    if len(text) > 0:
                        t = text[0]
                    else:
                        break
        return c, text
    
    def dump(self, text = ""):
        ret = []
        if self.isTerminal:
            ret += [text]
        for key in self.children.keys():
            ret += self.children[key].dump(text + key);
        return ret

    def dumpWithClientData(self, text = ""):
        ret = []
        if self.isTerminal:
            ret += [(text, self.clientData)]
        for key in self.children.keys():
            ret += self.children[key].dumpWithClientData(text + key);
        return ret
    
    def dumpClientData(self):
        ret = []
        if self.isTerminal:
            ret.append(self.clientData)
        for key in self.children.keys():
            ret += self.children[key].dumpClientData();
        return ret
    
