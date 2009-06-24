##############################################################################
## File:        Makefile
## Purpose:     wxScintilla test make file
## Maintainer:  Otto Wyss
## Created:     2003-08-20
## RCS-ID:      $Id$
## Copyright:   (c) 2004 wxCode
## Licence:     wxWindows
##############################################################################
# base information

# list of source files
SRC = edit.cpp prefs.cpp test.cpp

# executable name
PRG = test

# source directories
vpath %.cpp src
vpath %.cpp ../../src

# additional header directories
INC = -I../../include


##############################################################################
# environment, change only if other settings are needed

ifneq ($(WXDEVEL),)
  WXDEVEL_Debug = $(WXDEVEL)/Debug/
  WXDEVEL_Univ = $(WXDEVEL)/Univ/
endif
ifeq ($(MAKECMDGOALS),)
  WXCONFIG=wx-config
  GTKCONFIG=gtk-config
endif
ifeq ($(MAKECMDGOALS),release)
  WXCONFIG=wx-config
  GTKCONFIG=gtk-config
endif
ifeq ($(MAKECMDGOALS),debug)
  WXCONFIG=wxgtkd-2.4-config
  GTKCONFIG=gtk-config
endif
ifeq ($(MAKECMDGOALS),develdebug)
  WXCONFIG=$(WXDEVEL_Debug)wx-config --inplace
  GTKCONFIG=pkg-config gtk+-2.0
endif
ifeq ($(MAKECMDGOALS),univdebug)
  WXCONFIG=$(WXDEVEL_Univ)wx-config --inplace
  GTKCONFIG=pkg-config gtk+-2.0
endif


##############################################################################
# targets, change only if other targets are needed

OBJ = $(SRC:.cpp=.o)

.cpp.o:
	g++ -c $(INC) `$(WXCONFIG) --cxxflags` -g -o $@ $<

all:	$(PRG)
$(PRG):	$(OBJ)
	g++ -o $@ $^ -L ./ -lwxscintilla `$(WXCONFIG) --libs`

release:	$(PRG)r
$(PRG)r:	$(OBJ)
	g++ -o $@ $^ $(WXCODE)/wxscintilla/lib/libwxscintilla.a `$(WXCONFIG) --static --libs`

debug:	$(PRG)d
$(PRG)d:	$(OBJ)
	g++ -o $@ $^ $(WXCODE)/wxscintilla/lib/libwxscintilla.so.d `$(WXCONFIG) --libs`

develdebug:	$(PRG)dd
$(PRG)dd:	$(OBJ)
	g++ -o $@ $^ $(WXCODE)/wxscintilla/lib/libwxscintilla.so.dd `$(WXCONFIG) --libs`

univdebug:	$(PRG)ud
$(PRG)ud:	$(OBJ)
	g++ -o $@ $^ $(WXCODE)/wxscintilla/lib/libwxscintilla.so.ud `$(WXCONFIG) --libs`

clean:
	rm -f *.o
