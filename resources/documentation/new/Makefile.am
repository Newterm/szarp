# Makefile do robienia dokumentacji z docbooka SGML-a
# $Id: Makefile.am 6729 2009-05-18 10:11:53Z reksio $

# Wymaga docbook'a 3.1 i jade/openjade a takze pdfjadetex do generowania PDF'ów.
#
# Wywolanie:
#	make doc
# 	make html   (tworzy wszystkie html-e)
#	make html-onebig (tworzy html-e z jednym wielkim plikiem)
#	make default-pdf (tworzy pdf'y z klasy book)
# 	make pdf
#	make article-pdf (tworzy pdf'y z klasy article)
#	make doc-all (to samo co 'make html pdf)
#	make clean (kasuje wszystkie wygenerowane rzeczy)
#	make install_on_server (instalacja dokumentacji na chrno)
#	
# Mozna tez generowac dokumentacje czastkowa, podaj±c jako argument ¶cie¿kê do g³ównego
# pliku html lub pliku pdf, np:
#	make howto/html/howto.html
#	make howto/html/howto-onebig.html
#	make howto/pdf/howto-article.pdf
#	make howto/pdf/howto.pdf
#
# Wersja uproszczona - nie zauwa¿a zale¿no¶ci od zmodyfikowanych rysunków.
#

##############
# Lista dokumentacji do wygenerowania - nic wiecej nie trzeba modyfikowaæ!

DOCS=\
	draw3 \
	draw3_en \
	eksploatacja \
	ekstraktor3 \
	viszio \
	howto \
	ipk \
	kontroler \
	oszarpie \
	raporter \
	scc \
	ssc \
	filler

#######################

######### Automake staff ##########3

SUBDIRS =


##### TARGETS #####

DOCS_HTML=$(join $(DOCS), $(addsuffix .html, $(addprefix /html/, $(DOCS))))
DOCS_HTML_ONEBIG=$(join $(DOCS), $(addsuffix -onebig.html, $(addprefix /html/, $(DOCS))))
DOCS_PDF=$(join $(DOCS), $(addsuffix .pdf, $(addprefix /pdf/, $(DOCS))))
DOCS_A_PDF=$(join $(DOCS), $(addsuffix -article.pdf, $(addprefix /pdf/, $(DOCS))))

doc: html
doc-all: html pdf
html:$(DOCS_HTML)
html-onebig:$(DOCS_HTML_ONEBIG)
default-pdf: $(DOCS_PDF)
article-pdf: pdf
pdf: $(DOCS_A_PDF)
	mkdir -p _pdf
	cp --force `find . -path '*/pdf/*' -name '*.pdf'` _pdf

##### VARIABLES #####

JADE=@JADE@
WWWPATH=$(DESTDIR)/var/www/htdocs/serv/howto
STYLE=../../stylesheet.dsl
STDDEP=stylesheet.dsl common/szarp-info.sgml common/help.sgml @srcdir@/daemons.sgml
SGMLHELP=../../../../script/sgml2hlpctrl.pl

##### RULES  #####

@srcdir@/daemons.sgml: $(wildcard ../../../parcook/*dmn.c?) $(wildcard ../../../parcook/boruta_*.cc) @srcdir@/../../../script/dmn2sgml.py
	@srcdir@/../../../script/dmn2sgml.py ../../../parcook > $@

%.html: ../%.sgml $(STDDEP) 
	cd $(dir $<)html; $(JADE) -d $(STYLE)\#html -t sgml -i html ../$(notdir $<)
	cd $(dir $<); $(SGMLHELP) $(notdir $<)

%-onebig.html: ../%.sgml $(STDDEP)
	cd $(dir $<)html; $(JADE) -d $(STYLE)\#html -V nochunks -t sgml -i html ../$(notdir $<) > $(notdir $@)

%.pdf: ../%.sgml $(STDDEP) 
	mkdir -p $(dir $*)pics
	cp $(dir $*)../html/pics/*.png $(dir $*)pics || true
	cd $(dir $<)pdf; $(JADE) -d $(STYLE)\#print -t tex \
	-o $(notdir $(basename $<).tex) ../$(notdir $<); \
	cat $(notdir $(basename $<).tex) \
	| ../../../../../script/change-tex-pl-entities2.sh > \
	polish-ent.tex.tmp; mv polish-ent.tex.tmp $(notdir $(basename $<).tex); \
	pdfjadetex $(notdir $(basename $<).tex) 2>&1 >/dev/null; \
	pdfjadetex $(notdir $(basename $<).tex) 2>&1 >/dev/null; \
	pdfjadetex $(notdir $(basename $<).tex) 2>&1 >/dev/null

%-article.sgml:%.sgml
	cat $< | sed -e 's+DOCTYPE book+DOCTYPE article+ig' \
	-e 's+common/help.sgml+common/help-article.sgml+ig' \
	-e 's+<bookinfo+<artheader+ig' \
	-e 's+</bookinfo>+</artheader>+ig' \
	-e 's+<partintro>++ig' \
	-e 's+</partintro>++ig' |\
	sed -e 's+<book+<article+ig' \
	-e 's+<part+<section+ig' \
	-e 's+<chapter+<section+ig' \
	-e 's+</book+</article+ig' \
	-e 's+</part>+</section>+ig' \
	-e 's+</chapter>+</section>+ig' \
	> $@

install_on_server:
	[ -e $(WWWPATH) ] ||  $(INSTALL) -d $(WWWPATH)
	rsync . -a $(WWWPATH)
	rename -f 's/-article\.pdf/\.pdf/' $(WWWPATH)/_pdf/*


##### PHONIES #####

# najmocniejszy clean
clean: clean_html clean_pdf clean_generated_pics clean_daemons

clean_html:
	for i in $(DOCS) ; do \
		rm -f $$i/html/*.{html,hh*,map}; \
	done

clean_pdf:
	for i in $(DOCS) ; do \
		rm -f $$i/pdf/*.*; \
	done
	rm -rf _pdf
	
clean_generated_pics:
	rm -f $(DRAW_HTML_PNG_ICONS_WITH_PATH) 
	for i in $(DOCS) ; do \
		rm -rf $$i/pdf/pics; \
	done

clean_daemons:
	rm -f daemons.sgml
