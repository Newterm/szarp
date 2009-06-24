(C) 2002 Pawe³ Pa³ucha ISL project

$Id$

Ten plik opisuje, jak zainstalowaæ i u¿ywaæ "Cocoon'owej" czê¶ci projektu ISL.

0. Spis tre¶ci:

1. Zawarto¶æ katalogu.
2. Instalacja.
3. Struktura katalogu isl.
4. Dodawanie nowych plików.


1. Zawarto¶æ katalogu.

Katalog zawiera nastêpuj±ce pliki i katalogi:
	
	- README.TXT - plik z opisem instalacji i podstawowymi informacjami
	- isl	     - katalog, zawieraj±cy dane dla Cocoona


2. Instalacja.

Instalacja polega na kilku prostych krokach. Zak³adam, ¿e Cocoon zosta³
zainstalowany na Tomcacie 4 (opis instalacji Cocoona - patrz plik
../doc/COCOON-HOWTO.txt) i ¿e znajduje siê w katalogu 
/var/tomcat4/webapps/cocoon. Katalog ten bêdziemy nazywaæ katalogiem g³ównym 
Cocoona. 

Do tego katalogu kopiujemy katalog "isl" wraz z zawarto¶ci±. Nastêpnie do pliku
sitemap.xmap w katalogu g³ównym Cocoona do elementu <map:pipeline> (w
<map:pipelines> w <map:sitemap>) dodajemy nastêpuj±cy wpisy:

<map:match pattern="isl/**">
   <map:mount uri-prefix="isl/" src="isl/" check-reload="yes"/>
</map:match>

<map:match pattern="isl">
   <map:redirect-to uri="isl/"/>
</map:match>

W pliku cocoon.xconf w katalogu g³ównym Cocoona znajdujemy nastêpuj±cy 
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

To wszystko! Je¶li chcemy, aby "isl" by³o domy¶lnym kontekstem cocoona, nale¿y
jeszcze w pliku 'sitemap.xmap' w katalogu g³ównym cocoona we fragmencie:

  <map:match pattern="">
    <map:redirect-to uri="welcome"/>
  </map:match>

zamieniæ "welcome" na "isl".

3. Struktura katalogu ISL.

Katalog isl zawiera nastêpuj±ce elementy:

- plik sitemap.xmap - Opisuje strukturê generowanych stron, umiejscowienie
  plików, zale¿no¶ci miêdzy nimi, sposób generowania danych wyj¶ciowych. Nie ma
  potrzeby jego modyfikacji.
  
- katalog stylesheets - Zawiera szablony XSLT u¿ywane do generowania danych
  wyj¶ciowych. Nie ma potrzeby ich modyfikacji.

- katalog resources - Zawiera zasoby wspólne dla wszystkich generowanych stron.
  Nie ma potrzeby ich modyfikacji.

- katalogi odpowiedzialne za konkretne obiekty, np. "gliwice". Szczególnym
  przypadkiem jest katalog "praterm", zawieraj±cy dane dla strony g³ównej.
  Ka¿dy z tych katalogów ma nastêpuj±c± strukturê:
  * katalog "docs" - zawiera dokumenty ¼ród³owe - isl (czyli specjaln± odmianê 
  SVG). S± one przekszta³cane na postaæ wyj¶ciow± zgodnie z zasadami opisanymi
  poni¿ej. Zawarte tam dokumenty HTML, przekszta³cane na postaæ wyj¶ciow±
  zawieraj±ca ewetualnie zagnie¿d¿one obrazki.
  * katalog "resources" - zawiera zasoby (np. obrazki, pliki html) dla 
  konkretnego obiektu. S± one bezpo¶rednio odczytywane z tego katalogu (bez
  przekszta³ceñ).

Zasoby generowane s± nastêpuj±co: w przegl±darce wpisywany jest adres w
postaci:
	[prefiks]/[obiekt]/[typ-danych]/[¼ród³o]
gdzie:
	[prefiks] oznacza odwo³anie do serwera Cocoon, katalogu isl. Dok³adna
	postaæ jest zale¿na od konkretnej instalacji. Mo¿e mieæ ona np. postaæ
	"http://localhost:8180/cocoon/isl".
	
	[obiekt] oznacza nazwê obiektu (a dok³adniej odpowiadaj±cego mu
	podkatalogu w katalogu "isl"), np. "gliwice", "praterm".
	
	[typ-danych] oznacza format informacji, jakie chcemy otrzymaæ.
	Aktualnie, dla ¼ród³a bêd±cego plikiem ISL, dostêpne s± nastêpuj±ce:
	- png - obrazek w formacie Potrable Network Graphics.
	- svg - statyczny dokument SVG.
	- dsvg - dynamiczny dokument SVG (skrypty), odczytywany przez Batika
	- isl - oryginalny plik isl.
	Dla dokumentów HTML [typ-danych] powinien byæ pusty.

	[¼ród³o] - oznacza nazwê pliku z rozszerzeniem "isl", lub "html"
	znajduj±cego siê w podkatalogu "docs" katalogu obiektu.

Tak wiêc, je¶li katalog "gliwice" w podkatalogu "docs" zawiera plik
"kociol1.isl", to aby zobaczyæ reprezentacjê tego pliku w postaci obrazka png
nale¿a³oby do przegl±darki wpisaæ co¶ w stylu:
http://localhost:8180/cocoon/isl/png/kociol1.isl.

Aby odwo³aæ siê do plików z katalogu "resources", wystarczy podaæ bezpo¶redni±
¶cie¿kê do nich.

UWAGA! Cocoon wymaga, aby wszystkie parsowane przez niego pliki korzysta³y z
kodowania UTF-8. W szczególno¶ci, próba u¿ycia polskich liter w kodowaniu
ISO-8859-2 czy cp-1250 spowoduje b³±d parsera XML. Do przekodowywania plików
mo¿na u¿yæ np. programu "iconv".


4. Dodawanie nowych plików.

Dodawanie nowych plików i obiektów, oraz modyfikacja istniej±cych jest prosta -
wystarczy utworzyæ odpowiednie katalogi i dodaæ pliki lub zmodyfikowaæ
istniej±ce. Pliki musz± byæ zapisane z kodowaniem UTF-8, musz± zawieraæ 
deklaracje wszystkich u¿ywanych przestrzeni nazw. Nie powinny zawieraæ DTD
zewnêtrznych (bo parser bêdzie siê do nich odwo³ywa³).

Je¿eli pliki ISL tworzymy za pomoc± edytora SVG "sodipodi", nale¿y poddaæ je
po edycji nastêpuj±cym przekszta³ceniom:
- zamieniæ warto¶æ atrybutu "standalone" w nag³ówku XML na "yes"
- skasowaæ deklaracjê DTD
- dodaæ deklaracjê przestrzeni nazw ISL do elementu SVG

