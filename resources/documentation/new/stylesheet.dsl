<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY html-ss
  PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA dsssl>
<!ENTITY print-ss
  PUBLIC "-//Norman Walsh//DOCUMENT DocBook Print Stylesheet//EN" CDATA dsssl>
]>

<style-sheet>
<style-specification id="html" use="html-stylesheet">
<style-specification-body> 

<!-- ********************************************************************* -->
<!-- ******************************* HTML ******************************** -->
<!-- ********************************************************************* -->
<!-- 
    prosty stylesheet poprawiajacy generowanie indeksu i
    umozliwiajacy uzywanie *.css!
    a, i jeszcze generowanie plikow z sensownymi nazwami! 
    - by Jarek -
--> 
    (define (toc-depth nd)
	    (if (string=? (gi nd) (normalize "book"))
		    10
		    2))
    
	(define ($paragraph$ #!optional (para-wrapper "P"))
		(let ((footnotes (select-elements (descendants (current-node)) 
			(normalize "footnote")))
		(tgroup (have-ancestor? (normalize "tgroup"))))
		(make sequence
			(make element gi: para-wrapper
			attributes: (append '())
			(if (attribute-string (normalize "userlevel"))
				(make empty-element gi: "IMG" 
				attributes: (list (list "SRC" (string-append (attribute-string (normalize "userlevel")) ".gif"))))
				(empty-sosofo))
			(process-children))
			(if (or %footnotes-at-end% tgroup (node-list-empty? footnotes))
	                  (empty-sosofo)
	        (make element gi: "BLOCKQUOTE"
		attributes: (list
			     (list "CLASS" "FOOTNOTES"))
		(with-mode footnote-mode
		  (process-node-list footnotes)))))))


;; To mozna zmienic, zeby generowal jednego wielkiego html-a (Vooy)
;;(define nochunks #t) 

(define pl-default-intra-label-sep ".")
(define pl-default-label-title-sep " ")
(define %graphic-default-extension% "gif")
(define %section-autolabel% #t)
(define %generate-book-titlepage% #t)
(define %generate-book-toc% #t)
(define %generate-article-toc% #t)
(define biblio-number #t)
(define %html-ext% ".html")
(define %use-id-as-filename% #t)
(define %stylesheet-type%
  ;; The type of the stylesheet to use
  "text/css")
(define %stylesheet% "../../style.css")
(define %html-header-tags% '(
	("META" ("http-equiv" "Content-Type") ("content" "text/html; charset=iso-8859-2"))))
(define (chunk-section-depth)
  2)


(define ($object-titles-after$)
  (list (normalize "figure") 
        (normalize "example") 
))

(define (book-titlepage-recto-elements)
  (list 
	(normalize "mediaobject")
;; a moze zamienic publisher na corpauthor?
;;	(normalize "corpauthor") 
	(normalize "publisher")
	(normalize "title") 
	(normalize "subtitle") 
	(normalize "abstract") 
	(normalize "authorgroup") 
	(normalize "author") 
	(normalize "pubdate") 
	(normalize "affiliation") 
	(normalize "editor")
))

(define (article-titlepage-recto-elements)
  (list 
	(normalize "mediaobject")
	(normalize "publisher")
	(normalize "title") 
	(normalize "subtitle") 
	(normalize "abstract") 
	(normalize "authorgroup") 
	(normalize "author") 
	(normalize "pubdate") 
	(normalize "affiliation") 
	(normalize "editor")

))

<!-- *************************** EoHTML ********************************* -->

</style-specification-body>
</style-specification>

<style-specification id="print" use="print-stylesheet">
<style-specification-body>

<!-- ******************************************************************** -->
<!-- ******************************** PRINT ***************************** -->
<!-- ******************************************************************** -->

(define %bf-size% 11pt)
(define %line-spacing-factor% 1.2)
(define %section-autolabel% #t)
(define %paper-type% "A4")
;;(define %paper-type% "A4landscape")
(define %left-margin% 3pi)
(define %right-margin% 4pi)
(define %article-page-number-restart% #t)
(define %book-number-restart% #t)
(define %book-page-number-restart% #t)
(define %hsize-bump-factor% 1.2)
(define %generate-article-toc% #f)
(define %generate-article-titlepage% #t)
(define %generate-book-toc% #t)
(define %generate-book-titlepage% #t)
(define %generate-part-toc% #f)
(define %generate-part-titlepage% #f)
(define %generate-chapter-toc% #f)
(define %titlepage-in-info-order% #t)
(define %verbatim-size-factor #f)
(define %verbatim-default-width 80)
(define %page-balance-columns?% #t)
;;(define %default-quadding% 'justify)
(define %default-quadding% 'start)
(define %article-title-quadding% 'start)
(define %article-subtitle-quadding% 'start)
(define %para-indent% 0pt)
(define %para-indent-firstpara% 8pt)
(define %two-side% #f)
(define bop-footnotes #t)
(define biblio-number #t)
;;(define %top-margin% 4pi)
;;(define %bottom-margin% 4pi)
;;(define %header-margin% 4pi)
;;(define %footer-margin% 4pi)

(define %graphic-default-extension% "png")

;; taken from the cygnus stylesheet
(define %funcsynopsis-decoration%
  ;; Decorate elements of a FuncSynopsis?
  #t)

;;(define %two-side% #t)

(define %visual-acuity%
  ;; General measure of document text size
  ;; "presbyopic"
  ;; "large-type"
  "presbyopic")

(define (article-titlepage-recto-elements)
  (list 
	(normalize "mediaobject")
	(normalize "publisher")
	(normalize "title") 
	(normalize "subtitle") 
	(normalize "abstract") 
	(normalize "authorgroup") 
	(normalize "author") 
	(normalize "pubdate") 
	(normalize "affiliation") 
	(normalize "editor")

))

;;(define (set-titlepage-verso-elements)
;;  (list 
;;	(normalize "editor")
;;	(normalize "edition") 
;;	(normalize "pubdate") 
;;	(normalize "copyright")
;;	(normalize "legalnotice") 
;;	(normalize "abstract") 
;;	(normalize "revhistory")))

(define ($object-titles-after$)
  (list (normalize "figure") 
  	(normalize "example")))

<!-- *************************** EoPRINT ********************************* -->

</style-specification-body>
</style-specification>

<external-specification id="print-stylesheet" document="print-ss">
<external-specification id="html-stylesheet"  document="html-ss">
</style-sheet>
