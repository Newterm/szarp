#!/bin/bash
# SZARP: SCADA software 
# 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# 2005 Praterm S.A.
# Pawe³ Pa³ucha <pawel@praterm.com.pl>
# $Id$
#
# Skrypt do zapisywania do bazy danych SzarpBase informacji o prognozie
# pogody. Przyjmuje jeden parametr - nazwê miasta (jedno z listy na stronie
# http://pogoda.onet.pl/miasto.html). Mo¿e byæ wywo³ywany z crontaba lub przez
# meaner'a. Przyk³adowa sekcja bodas:
#
# prognoza:
# time=* * * * 0
# command_line=/opt/szarp/bin/get_weather.sh Przasnysz
#
# Aby prognoza by³a widoczna na wykresach nale¿y w konfiguracji umie¶ciæ 
# nastêpuj±ce parametry (w elemencie <defined/>):
# <param name="Prognoza pogody:<nazwa miasta>:Prognozowana temperatura
# maksymalna" unit="°C" prec="0" base_ind="auto" short_name="Tpmx"
# draw_name="Prognoz. temp. maks.">
#	<define type="RPN" formula="null"/>
#	<draw title="Prognoza pogody" min="-30" max="50"/>
# </param>
# <param name="Prognoza pogody:<nazwa miasta>:Prognozowana temperatura
# minimalna" unit="°C" prec="0" base_ind="auto" short_name="Tpmn"
# draw_name="Prognoz. temp. minim.">
#	<define type="RPN" formula="null"/>
#	<draw title="Prognoza pogody" min="-30" max="50"/>
# </param>
# <param name="Prognoza pogody:<nazwa miasta>:Temperatura maksymalna
# prognoza 1-dniowa" unit="°C" prec="0" base_ind="auto" short_name="Tmx1"
# draw_name="Maks. prognoz. 1-dniowa">
#	<define type="RPN" formula="null"/>
#	<draw title="Prognoza pogody - szczegó³y" min="-30" max="50"/>
# </param>
# <param name="Prognoza pogody:<nazwa miasta>:Temperatura minimalna 
# prognoza 1-dniowa" unit="°C" prec="0" base_ind="auto" short_name="Tmn1"
# draw_name="Minim. porognoz. 1-dniowa">
#	<define type="RPN" formula="null"/>
#	<draw title="Prognoza pogody - szczegó³y" min="-30" max="50"/>
# </param>
# I to samo do dni a¿ do 6. Pe³na lista parametrów jest w pliku
# meteo_template.xml w katalogu resources.
#
# Uwaga - do dzia³ania wymaga programów wget, iconv, sed, 
# xmllint, tidy i xmlstarlet.

# katalog tymczasowy
TMP=/tmp

# konwersja na xml'a
# parametry - plik wej¶ciowy i wyjsciowy
function ClearOnetPl()
{
	# przekodowujemy na utf8
	iconv -f latin2 -t utf8 < $1 > $TMP/gw_1.html

	# usuwamy </FORM> z 34-ej linii ;-)
	sed -e '34s#</FORM>##Ig' 2> /dev/null < $TMP/gw_1.html > $TMP/gw_2.html

	# czyscimy kod
	tidy -c -b -asxml -q -utf8 2> /dev/null < $TMP/gw_2.html > $TMP/gw_3.html

	# zamieniamy na xml'a
	xmllint --html  --xmlout $TMP/gw_3.html --encode ISO-8859-2 > $2
}

# usuwamy ¶mieci
rm -f $TMP/gw_*

# sprawdzamy argumenty
if [ "$1" = "" ] ; then
	echo -e "\n\
Skrypt do zapisywania w bazie prognozy pogody.\n\
U¿ycie: $0 <nazwa miasta>\n\
Nazwa miasta musi byæ jedn± z obecnych na stronie\n\
http://pogoda.onet.pl/miasto.html.\n"
	exit 1
fi

# ¶ci±gamy listê miast
wget http://pogoda.onet.pl/miasto.html -O $TMP/gw_miasto.html -T 30 -q
if [ $? -ne 0 ] ; then
	exit 1
fi

# zamieniamy na XML'a
ClearOnetPl $TMP/gw_miasto.html $TMP/gw_miasto.xml

# nazwa miasta w UTF8
CITY=`echo "$1" | iconv -f latin2 -t utf8`
# szukamy linku do prognozy
URL=`xmlstarlet  sel -N "h=http://www.w3.org/1999/xhtml" -t -c \
"concat(//h:a[text()='$CITY']/@href, '')" $TMP/gw_miasto.xml`
if [ -z "$URL" ] ; then
	exit 1
fi

# ¶ci±gamy prognozê
wget "http://pogoda.onet.pl/$URL" -O $TMP/gw_data.html -T 30 -q
if [ $? -ne 0 ] ; then
	exit 1
fi

# zamieniamy na XML'a
ClearOnetPl $TMP/gw_data.html $TMP/gw_data.xml

# na ile dni mamy prognozê
DAYS=6
I=1
while [ $I -le $DAYS ] ; do
	# szukana data
	DATE=`date -d "+$I days" +"%d.%m.%Y"`

	# wyszukujemy temperatury minimalnej
	MIN=`xmlstarlet  sel -N "h=http://www.w3.org/1999/xhtml" -t -c "//h:table[contains(h:tr/h:td/text(),'$DATE')]/../h:table[position()=2]/h:tr[position()=2]/h:td/h:b[position()=1]/text()" $TMP/gw_data.xml`

	# wyszukujemy temperatury maksymalnej
	MAX=`xmlstarlet  sel -N "h=http://www.w3.org/1999/xhtml" -t -c "//h:table[contains(h:tr/h:td/text(),'$DATE')]/../h:table[position()=2]/h:tr[position()=2]/h:td/h:b[position()=2]/text()" $TMP/gw_data.xml`

	# zapisujemy dane do bazy
	echo "\"Prognoza pogody:$1:Prognozowana temperatura minimalna\" `date -d \"+$I days\" +'%Y %m %d 12 00'` $MIN" \
	| /opt/szarp/bin/szbwriter
	
	echo "\"Prognoza pogody:$1:Prognozowana temperatura maksymalna\" `date -d \"+$I days\" +'%Y %m %d 12 00'` $MAX" \
	| /opt/szarp/bin/szbwriter

	echo "\"Prognoza pogody:$1:Temperatura minimalna prognoza $I-dniowa\"\
	`date -d \"+$I days\" +'%Y %m %d 12 00'` $MIN" \
	        | /opt/szarp/bin/szbwriter
		
	echo "\"Prognoza pogody:$1:Temperatura maksymalna prognoza $I-dniowa\"\
	`date -d \"+$I days\" +'%Y %m %d 12 00'` $MAX" \
	        | /opt/szarp/bin/szbwriter

	# zwiêkszamy licznik pêtli
	I=$(($I+1))
done

# usuwamy pozosta³o¶ci
rm -f $TMP/gw_*


