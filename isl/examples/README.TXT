(C) 2002 Pawe� Pa�ucha ISL project

$Id$

Ten plik opisuje, jak zainstalowa� i u�ywa� "Cocoon'owej" cz�ci projektu ISL.

0. Spis tre�ci:

1. Zawarto�� katalogu.
2. Instalacja.
3. Struktura katalogu isl.
4. Dodawanie nowych plik�w.


1. Zawarto�� katalogu.

Katalog zawiera nast�puj�ce pliki i katalogi:
	
	- README.TXT - plik z opisem instalacji i podstawowymi informacjami
	- isl	     - katalog, zawieraj�cy dane dla Cocoona


2. Instalacja.

Instalacja polega na kilku prostych krokach. Zak�adam, �e Cocoon zosta�
zainstalowany na Tomcacie 4 (opis instalacji Cocoona - patrz plik
../doc/COCOON-HOWTO.txt) i �e znajduje si� w katalogu 
/var/tomcat4/webapps/cocoon. Katalog ten b�dziemy nazywa� katalogiem g��wnym 
Cocoona. 

Do tego katalogu kopiujemy katalog "isl" wraz z zawarto�ci�. Nast�pnie do pliku
sitemap.xmap w katalogu g��wnym Cocoona do elementu <map:pipeline> (w
<map:pipelines> w <map:sitemap>) dodajemy nast�puj�cy wpisy:

<map:match pattern="isl/**">
   <map:mount uri-prefix="isl/" src="isl/" check-reload="yes"/>
</map:match>

<map:match pattern="isl">
   <map:redirect-to uri="isl/"/>
</map:match>

W pliku cocoon.xconf w katalogu g��wnym Cocoona znajdujemy nast�puj�cy 
fragment:

 <event-pipeline
 	class="org.apache.cocoon.components.pipeline.CachingEventPipeline"
        logger="core.event-pipeline"
	pool-max="32" pool-min="16" pool-grow="4"/>

Zamieniamy go na:

<event-pipeline
 	class="org.apache.cocoon.components.pipeline.NonCachingEventPipeline"
        logger="core.event-pipeline"
	pool-max="32" pool-min="16" pool-grow="4"/>
				     
(dodajemy "Non" przed "CachingEventPipeline").

To wszystko! Je�li chcemy, aby "isl" by�o domy�lnym kontekstem cocoona, nale�y
jeszcze w pliku 'sitemap.xmap' w katalogu g��wnym cocoona we fragmencie:

  <map:match pattern="">
    <map:redirect-to uri="welcome"/>
  </map:match>

zamieni� "welcome" na "isl".

3. Struktura katalogu ISL.

Katalog isl zawiera nast�puj�ce elementy:

- plik sitemap.xmap - Opisuje struktur� generowanych stron, umiejscowienie
  plik�w, zale�no�ci mi�dzy nimi, spos�b generowania danych wyj�ciowych. Nie ma
  potrzeby jego modyfikacji.
  
- katalog stylesheets - Zawiera szablony XSLT u�ywane do generowania danych
  wyj�ciowych. Nie ma potrzeby ich modyfikacji.

- katalog resources - Zawiera zasoby wsp�lne dla wszystkich generowanych stron.
  Nie ma potrzeby ich modyfikacji.

- katalogi odpowiedzialne za konkretne obiekty, np. "gliwice". Szczeg�lnym
  przypadkiem jest katalog "praterm", zawieraj�cy dane dla strony g��wnej.
  Ka�dy z tych katalog�w ma nast�puj�c� struktur�:
  * katalog "docs" - zawiera dokumenty �r�d�owe - isl (czyli specjaln� odmian� 
  SVG). S� one przekszta�cane na posta� wyj�ciow� zgodnie z zasadami opisanymi
  poni�ej. Zawarte tam dokumenty HTML, przekszta�cane na posta� wyj�ciow�
  zawieraj�ca ewetualnie zagnie�d�one obrazki.
  * katalog "resources" - zawiera zasoby (np. obrazki, pliki html) dla 
  konkretnego obiektu. S� one bezpo�rednio odczytywane z tego katalogu (bez
  przekszta�ce�).

Zasoby generowane s� nast�puj�co: w przegl�darce wpisywany jest adres w
postaci:
	[prefiks]/[obiekt]/[typ-danych]/[�r�d�o]
gdzie:
	[prefiks] oznacza odwo�anie do serwera Cocoon, katalogu isl. Dok�adna
	posta� jest zale�na od konkretnej instalacji. Mo�e mie� ona np. posta�
	"http://localhost:8180/cocoon/isl".
	
	[obiekt] oznacza nazw� obiektu (a dok�adniej odpowiadaj�cego mu
	podkatalogu w katalogu "isl"), np. "gliwice", "praterm".
	
	[typ-danych] oznacza format informacji, jakie chcemy otrzyma�.
	Aktualnie, dla �r�d�a b�d�cego plikiem ISL, dost�pne s� nast�puj�ce:
	- png - obrazek w formacie Potrable Network Graphics.
	- svg - statyczny dokument SVG.
	- dsvg - dynamiczny dokument SVG (skrypty), odczytywany przez Batika
	- isl - oryginalny plik isl.
	Dla dokument�w HTML [typ-danych] powinien by� pusty.

	[�r�d�o] - oznacza nazw� pliku z rozszerzeniem "isl", lub "html"
	znajduj�cego si� w podkatalogu "docs" katalogu obiektu.

Tak wi�c, je�li katalog "gliwice" w podkatalogu "docs" zawiera plik
"kociol1.isl", to aby zobaczy� reprezentacj� tego pliku w postaci obrazka png
nale�a�oby do przegl�darki wpisa� co� w stylu:
http://localhost:8180/cocoon/isl/png/kociol1.isl.

Aby odwo�a� si� do plik�w z katalogu "resources", wystarczy poda� bezpo�redni�
�cie�k� do nich.

UWAGA! Cocoon wymaga, aby wszystkie parsowane przez niego pliki korzysta�y z
kodowania UTF-8. W szczeg�lno�ci, pr�ba u�ycia polskich liter w kodowaniu
ISO-8859-2 czy cp-1250 spowoduje b��d parsera XML. Do przekodowywania plik�w
mo�na u�y� np. programu "iconv".


4. Dodawanie nowych plik�w.

Dodawanie nowych plik�w i obiekt�w, oraz modyfikacja istniej�cych jest prosta -
wystarczy utworzy� odpowiednie katalogi i doda� pliki lub zmodyfikowa�
istniej�ce. Pliki musz� by� zapisane z kodowaniem UTF-8, musz� zawiera� 
deklaracje wszystkich u�ywanych przestrzeni nazw. Nie powinny zawiera� DTD
zewn�trznych (bo parser b�dzie si� do nich odwo�ywa�).

Je�eli pliki ISL tworzymy za pomoc� edytora SVG "sodipodi", nale�y podda� je
po edycji nast�puj�cym przekszta�ceniom:
- zamieni� warto�� atrybutu "standalone" w nag��wku XML na "yes"
- skasowa� deklaracj� DTD
- doda� deklaracj� przestrzeni nazw ISL do elementu SVG

