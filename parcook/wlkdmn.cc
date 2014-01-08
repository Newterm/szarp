/* 
  SZARP: SCADA software 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/*
 * Marcin Goliszewski
 *
 * $Id$
 */
/*
 @description_start
 @class 3

 @devices Daemon reading WKL files from Davis Instruments Vantage Pro 2 meteo station.
 @devices.pl Daemon do odczytu danych z plików WKL stacji meteorologicznej Vantage
 Pro 2 firmy Davis Instruments.

 @comment Daemon reads WLK files, that can be fetched from meteo station using Open Source 
 <ulink url="http://www.wviewweather.com/">wview</ulink> software.
 @comment.pl Daemon zosta³ zaimplementowany nie jako narzêdzie do komunikacji ze stacj± bezpo¶rednio poprzez port
 szeregowy/USB, a jako parser plików w formacie WLK, które udostêpnia stacja. W zwi±zku z tym wymagane jest 
 zewnêtrzne oprogramowanie, które odpowiedzialne bêdzie za komunikacjê ze stacj± oraz pobieranie
 z niej plików WLK. Dostêpne jest oprogramowanie na licencji typu Open Source, które bardzo dobrze realizuje 
 to zadanie: <ulink url="http://www.wviewweather.com/">wview</ulink>.

 @config Prior to configuring wlkdmn you need to have wview running. Set "path" attribute to directory
 where wview stores WLK files. Configuration contains 148 parameters, if some of them are not used by
 your meteo station model, they will be filled with NO_DATA values.
 @config.pl Demon do dzia³ania wymaga poprawnie skonfigurowanego i dzia³aj±cego oprogramowania wview.
 Jako ¶cie¿kê do urz±dzenia podana powinna zostaæ ¶cie¿ka do katalogu, w którm wview przechowuje pliki
 WLK. W konfiguracji nale¿y umie¶ciæ wszystkie 148 parametrów, gdy¿ taka ilo¶æ danych jest
 przechowywana w plikach WLK; je¿eli dany model stacji nie obs³uguje wszystkich parametrów,
 które mog± byæ przechowywane w plikach WLK, to zostan± one wype³nione warto¶ci± NO_DATA.
 @comment.pl
            Daemon <emphasis>wlkdmn</emphasis> oprócz podstawowej
            funkcjonalno¶ci odczytywania danych bie¿±cych z plików WLK i
            przekazywania ich do parcooka, posiada równie¿ mo¿liwo¶æ
            odczytania danych historycznych z tych¿e plików (np. za
            podany okres w przesz³o¶ci), a nastêpnie wyeksportowania ich
            do pliku w formacie odpowiednim dla programu
            <emphasis>szbwriter</emphasis> (patrz <xref
            linkend="szbase-szbwriter">). Uruchomienie
            <emphasis>wlkdmn</emphasis> w tym trybie pracy nastêpuje
            poprzez podanie argumentu <emphasis>--export</emphasis> w
            linii poleceñ. W tym trybie dostêpne s± nastêpuj±ce
            argumenty linii poleceñ:
            <itemizedlist>
                <listitem>
                    <para>
                        <emphasis>--file</emphasis> - ¶cie¿ka do pliku, do
                        którego wyeksportowane dane maj± zostaæ zapisane;
                        je¶li plik ten ju¿ istnieje, dane wyeksportowane
                        zostan± dopisane na jego koñcu; argument ten
                        jest <emphasis>obowi±zkowy</emphasis>
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>--from</emphasis> - data (w formacie
                        RRRR-MM-DD), od której nale¿y zacz±æ eksport 
                        danych; je¶li zostanie pominiêta zostan± 
                        wyeksportowane dane od najwcze¶niejszych dostêpnych
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>--to</emphasis> - data (w formacie
                        RRRR-MM-DD), na której nale¿y zakoñczyæ eksport 
                        danych; je¶li zostanie pominiêta zostan± 
                        wyeksportowane dane do najpó¼niejszych dostêpnych
                    </para>
                </listitem>
            </itemizedlist>

 @config_example
  <device daemon="/opt/szarp/bin/wlkdmn" path="/var/wview/archive">
    <unit id="1" type="1" subtype="1" bufsize="1">
      <param name="WView:Simulator:maksymalna dobowa temperatura zewnêtrzna" short_name="Tzmx" draw_name="Temp. zewn. max" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" prior="1" order="1"/>
        <raport title="WView - simulator" order="1"/>
      </param>
      <param name="WView:Simulator:minimalna dobowa temperatura zewnêtrzna" short_name="Tzmn" draw_name="Temp. zewn. min" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="3"/>
        <raport title="WView - simulator" order="2"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa temperatura wewnêtrzna" short_name="Twmx" draw_name="Temp. wewn. max" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="4"/>
        <raport title="WView - simulator" order="3"/>
      </param>
      <param name="WView:Simulator:minimalna dobowa temperatura wewnêtrzna" short_name="Twmn" draw_name="Temp. wewn. min" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="6"/>
        <raport title="WView - simulator" order="4"/>
      </param>
      <param name="WView:Simulator:¶rednia dobowa temperatura zewnêtrzna" short_name="Tzsr" draw_name="Temp. zewn. ¶r." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="2"/>
        <raport title="WView - simulator" order="5"/>
      </param>
      <param name="WView:Simulator:¶rednia dobowa temperatura wewnêtrzna" short_name="Twsr" draw_name="Temp. wewn. ¶r." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="5"/>
        <raport title="WView - simulator" order="6"/>
      </param>
      <param name="WView:Simulator:maksymalne dobowe och³odzenie spowodowane wiatrem" short_name="Tzwmx" draw_name="Och³. sp. w. max" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" prior="2" order="1"/>
        <raport title="WView - simulator" order="7"/>
      </param>
      <param name="WView:Simulator:minimalne dobowe och³odzenie spowodowane wiatrem" short_name="Tzwmn" draw_name="Och³. sp. w. min" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" order="3"/>
        <raport title="WView - simulator" order="8"/>
      </param>
      <param name="WView:Simulator:maksymalny dobowy punkt rosy" short_name="Trmx" draw_name="Punkt rosy max" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - punkt rosy" min="0" max="50" prior="3" order="1"/>
        <raport title="WView - simulator" order="9"/>
      </param>
      <param name="WView:Simulator:minimalny dobowy punkt rosy" short_name="Trmn" draw_name="Punkt rosy min" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - punkt rosy" min="0" max="50" order="3"/>
        <raport title="WView - simulator" order="10"/>
      </param>
      <param name="WView:Simulator:¶rednie dobowe och³odzenie spowodowane wiatrem" short_name="Tzwsr" draw_name="Och³. sp. w. ¶r." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" order="2"/>
        <raport title="WView - simulator" order="11"/>
      </param>
      <param name="WView:Simulator:¶redni dobowy punkt rosy" short_name="Trsr" draw_name="Punkt rosy ¶r." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - punkt rosy" min="0" max="50" order="2"/>
        <raport title="WView - simulator" order="12"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa zewnêtrzna wilgotno¶æ" short_name="fzmx" draw_name="Zewn. wilg. max" unit="%" prec="1" base_ind="auto">
        <draw title="WView - wilgotno¶æ" min="0" max="100" prior="4" order="1"/>
        <raport title="WView - simulator" order="13"/>
      </param>
      <param name="WView:Simulator:minimalna dobowa zewnêtrzna wilgotno¶æ" short_name="fzmn" draw_name="Zewn. wilg. min" unit="%" prec="1" base_ind="auto">
        <draw title="WView - wilgotno¶æ" min="0" max="100" order="3"/>
        <raport title="WView - simulator" order="14"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa wewnêtrzna wilgotno¶æ" short_name="fwmx" draw_name="Wewn. wilg. max" unit="%" prec="1" base_ind="auto">
        <draw title="WView - wilgotno¶æ" min="0" max="100" order="4"/>
        <raport title="WView - simulator" order="15"/>
      </param>
      <param name="WView:Simulator:minimalna dobowa wewnêtrzna wilgotno¶æ" short_name="fwmn" draw_name="Wewn. wilg. min" unit="%" prec="1" base_ind="auto">
        <draw title="WView - wilgotno¶æ" min="0" max="100" order="5"/>
        <raport title="WView - simulator" order="16"/>
      </param>
      <param name="WView:Simulator:¶rednia dobowa zewnêtrzna wilgotno¶æ" short_name="fzsr" draw_name="Zewn. wilg. ¶r." unit="%" prec="1" base_ind="auto">
        <draw title="WView - wilgotno¶æ" min="0" max="100" order="2"/>
        <raport title="WView - simulator" order="17"/>
      </param>
      <param name="WView:Simulator:maksymalne dobowe ci¶nienie atmosferyczne" short_name="Pamx" draw_name="Ci¶n. atm. max" unit="mmHg" prec="1" base_ind="auto">
        <draw title="WView - ci¶nienie atmosferyczne" min="700" max="800" prior="5" order="1"/>
        <raport title="WView - simulator" order="18"/>
      </param>
      <param name="WView:Simulator:minimalne dobowe ci¶nienie atmosferyczne" short_name="Pamn" draw_name="Ci¶n. atm. min" unit="mmHg" prec="1" base_ind="auto">
        <draw title="WView - ci¶nienie atmosferyczne" order="3" min="700" max="800"/>
        <raport title="WView - simulator" order="19"/>
      </param>
      <param name="WView:Simulator:¶rednie dobowe ci¶nienie atmosferyczne" short_name="Pasr" draw_name="Ci¶n. atm. ¶r." unit="mmHg" prec="1" base_ind="auto">
        <draw title="WView - ci¶nienie atmosferyczne" min="700" max="800" order="2"/>
        <raport title="WView - simulator" order="20"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa prêdko¶æ wiatru" short_name="vwmx" draw_name="Prêd. w. max" unit="km/h" prec="1" base_ind="auto">
        <draw title="WView - wiatr" min="0" max="200" prior="6" order="1"/>
        <raport title="WView - simulator" order="21"/>
      </param>
      <param name="WView:Simulator:¶rednia dobowa prêdko¶æ wiatru" short_name="vwsr" draw_name="Prêd. w. ¶r." unit="km/h" prec="1" base_ind="auto">
        <draw title="WView - wiatr" order="2" min="0" max="200"/>
        <raport title="WView - simulator" order="22"/>
      </param>
      <param name="WView:Simulator:ca³kowita dobowa odlego¶æ przebyta przez wiatr" short_name="sw" draw_name="Odl. przeb. w." unit="km" prec="1" base_ind="auto">
        <draw title="WView - wiatr" min="0" max="4000" order="3"/>
        <raport title="WView - simulator" order="23"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa prêdko¶æ 10-minutowa wiatru" short_name="vw10s" draw_name="Prêd. w. 10m max" unit="km/h" prec="1" base_ind="auto">
        <draw title="WView - wiatr" min="0" max="200" order="4"/>
        <raport title="WView - simulator" order="24"/>
      </param>
      <param name="WView:Simulator:kierunek wiatru o maksymalnej dobowej prêdko¶ci" short_name="Dmxv" draw_name="Max prêd. w. kier." unit="-" base_ind="auto">
        <value int="0" name="N"/>
        <value int="1" name="NNE"/>
        <value int="2" name="NE"/>
        <value int="3" name="ENE"/>
        <value int="4" name="E"/>
        <value int="5" name="ESE"/>
        <value int="6" name="SE"/>
        <value int="7" name="SSE"/>
        <value int="8" name="S"/>
        <value int="9" name="SSW"/>
        <value int="10" name="SW"/>
        <value int="11" name="WSW"/>
        <value int="12" name="W"/>
        <value int="13" name="WNW"/>
        <value int="14" name="NW"/>
        <value int="15" name="NNW"/>
        <raport title="WView - simulator" order="25"/>
      </param>
      <param name="WView:Simulator:kierunek wiatru o maksymalnej dobowej prêd. 10min" short_name="Dmxv10" draw_name="Max p. 10m w. kier." unit="-" base_ind="auto">
        <value int="0" name="N"/>
        <value int="1" name="NNE"/>
        <value int="2" name="NE"/>
        <value int="3" name="ENE"/>
        <value int="4" name="E"/>
        <value int="5" name="ESE"/>
        <value int="6" name="SE"/>
        <value int="7" name="SSE"/>
        <value int="8" name="S"/>
        <value int="9" name="SSW"/>
        <value int="10" name="SW"/>
        <value int="11" name="WSW"/>
        <value int="12" name="W"/>
        <value int="13" name="WNW"/>
        <value int="14" name="NW"/>
        <value int="15" name="NNW"/>
        <raport title="WView - simulator" order="26"/>
      </param>
      <param name="WView:Simulator:ca³kowity dobowy opad deszczu" short_name="hdc" draw_name="Dobowy opad d." unit="mm" prec="1" base_ind="auto">
        <draw title="WView - deszcz" min="0" max="1000" prior="7" order="1"/>
        <raport title="WView - simulator" order="27"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa intensywno¶æ opadu deszczu" short_name="Fdmx" draw_name="Int. op. d. max" unit="mm/h" prec="1" base_ind="auto">
        <draw title="WView - deszcz" min="0" max="200" order="2"/>
        <raport title="WView - simulator" order="28"/>
      </param>
      <param name="WView:Simulator:dobowa dawka promieniowania UV" short_name="UV" draw_name="Dobowa dawka UV" unit="MED" prec="1" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="5" prior="8" order="1"/>
        <raport title="WView - simulator" order="29"/>
      </param>
      <param name="WView:Simulator:maksymalny dobowy indeks UV" short_name="UVimx" draw_name="Max indeks UV" unit="UVindex" prec="1" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="16" order="2"/>
        <raport title="WView - simulator" order="30"/>
      </param>
      <param name="WView:Simulator:czas maksymalnej dobowej temperatury zewnêtrznej" short_name="tTzmx" draw_name="Czas Tzmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="31"/>
      </param>
      <param name="WView:Simulator:czas minimalnej dobowej temperatury zewnêtrznej" short_name="tTzmn" draw_name="Czas Tzmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="32"/>
      </param>
      <param name="WView:Simulator:czas maksymalnej dobowej temperatury wewnêtrznej" short_name="tTwmx" draw_name="Czas Twmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="33"/>
      </param>
      <param name="WView:Simulator:czas minimalnej dobowej temperatury wewnêtrznej" short_name="tTwmn" draw_name="Czas Twmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="34"/>
      </param>
      <param name="WView:Simulator:czas maks. dobowego och³odzenia spow. wiatrem" short_name="tTzwmx" draw_name="Czas Tzwmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="35"/>
      </param>
      <param name="WView:Simulator:czas min. dobowego och³odzenia spow. wiatrem" short_name="tTzwmn" draw_name="Czas Tzwmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="36"/>
      </param>
      <param name="WView:Simulator:czas maksymalnego dobowego punktu rosy" short_name="tTrmx" draw_name="Czas Trmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="37"/>
      </param>
      <param name="WView:Simulator:czas minimalnego dobowego punkut rosy" short_name="tTrmn" draw_name="Czas Trmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="38"/>
      </param>
      <param name="WView:Simulator:czas maksymalnej dobowej zewnêtrznej wilgotno¶ci" short_name="tfzmx" draw_name="Czas fzmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="39"/>
      </param>
      <param name="WView:Simulator:czas minimalnej dobowej zewnêtrznej wilgotno¶ci" short_name="tfzmn" draw_name="Czas fzmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="40"/>
      </param>
      <param name="WView:Simulator:czas maksymalnej dobowej wewnêtrznej wilgotno¶ci" short_name="tfwmx" draw_name="Czas fwmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="41"/>
      </param>
      <param name="WView:Simulator:czas minimalnej dobowej wewnêtrznej wilgotno¶ci" short_name="tfwmn" draw_name="Czas fwmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="42"/>
      </param>
      <param name="WView:Simulator:czas maksymalnego dobowego ci¶nienia atm." short_name="tPamx" draw_name="Czas Pamx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="43"/>
      </param>
      <param name="WView:Simulator:czas minimalnego dobowego ci¶nienia atm." short_name="tPamn" draw_name="Czas Pamn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="44"/>
      </param>
      <param name="WView:Simulator:czas maksymalnej dobowej prêdko¶ci wiatru" short_name="tvwmx" draw_name="Czas vwmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="45"/>
      </param>
      <param name="WView:Simulator:czas maks. dobowej prêdko¶ci 10min wiatru" short_name="tvw10s" draw_name="Czas vw10s" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="46"/>
      </param>
      <param name="WView:Simulator:czas maks. dobowej intensywno¶ci opadu deszczu" short_name="tFdmx" draw_name="Czas Fdmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="47"/>
      </param>
      <param name="WView:Simulator:czas maksymalnego dobowego indeksu UV" short_name="tUVimx" draw_name="Czas UVimx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="48"/>
      </param>
      <param name="WView:Simulator:dobowa mapa pogody" short_name="Map" draw_name="Mapa pogody" unit="-" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="49"/>
      </param>
      <param name="WView:Simulator:dobowe prawid³owo odebrane pakiety z wiatromierza" short_name="NP" draw_name="Pakiety wiatrom." unit="-" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="50"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa energia s³oneczna" short_name="Esmx" draw_name="En. s³oneczna max" unit="W/m2" prec="0" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="300" order="3"/>
        <raport title="WView - simulator" order="51"/>
      </param>
      <param name="WView:Simulator:ca³kowita dobowa energia s³oneczna" short_name="Esd" draw_name="En. s. dobowa c." unit="Ly" prec="1" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="4000" order="4"/>
        <raport title="WView - simulator" order="52"/>
      </param>
      <param name="WView:Simulator:czas o¶wietlenia s³onecznego" short_name="ts" draw_name="Czas owietlenia" unit="min" prec="0" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="1000" order="5"/>
        <raport title="WView - simulator" order="53"/>
      </param>
      <param name="WView:Simulator:sumaryczne dzienne parowanie" short_name="par" draw_name="Sumaryczne parowanie" unit="mm" prec="3" base_ind="auto">
        <draw title="WView - parowanie" min="0" max="50" prior="9" order="1"/>
        <raport title="WView - simulator" order="54"/>
      </param>
      <param name="WView:Simulator:maksymalne dobowe ocieplenie spowodowane s³oñcem" short_name="Tzsmx" draw_name="Ociep. sp. s. max" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" order="4"/>
        <raport title="WView - simulator" order="55"/>
      </param>
      <param name="WView:Simulator:minimalne dobowe ocieplenie spowodowane s³oñcem" short_name="Tzsmn" draw_name="Ociep. sp. s. min" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" order="6"/>
        <raport title="WView - simulator" order="56"/>
      </param>
      <param name="WView:Simulator:¶rednie dobowe ocieplenie spowodowane s³oñcem" short_name="Tzssr" draw_name="Ociep. sp. s. ¶r." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" order="5"/>
        <raport title="WView - simulator" order="57"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa warto¶æ indeksu THSW" short_name="THSWmx" draw_name="THSW max" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - indeksy temperaturowe" min="-40" max="50" prior="10" order="1"/>
        <raport title="WView - simulator" order="58"/>
      </param>
      <param name="WView:Simulator:minimalna dobowa warto¶æ indeksu THSW" short_name="THSWmn" draw_name="THSW min" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - indeksy temperaturowe" min="-40" max="50" order="2"/>
        <raport title="WView - simulator" order="59"/>
      </param>
      <param name="WView:Simulator:maksymalna dobowa warto¶æ indeksu THW" short_name="THWmx" draw_name="THW max" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - indeksy temperaturowe" min="-40" max="50" order="3"/>
        <raport title="WView - simulator" order="60"/>
      </param>
      <param name="WView:Simulator:minimalna dobowa warto¶æ indeksu THW" short_name="THWmn" draw_name="THW min" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - indeksy temperaturowe" min="-40" max="50" order="4"/>
        <raport title="WView - simulator" order="61"/>
      </param>
      <param name="WView:Simulator:ilo¶æ stopniodni zimnych" short_name="SDz" draw_name="Stopniodni zimne" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - indeksy temperaturowe" min="0" max="200" order="5"/>
        <raport title="WView - simulator" order="62"/>
      </param>
      <param name="WView:Simulator:maks. dobowa temperatura z mokrego termometru" short_name="Tmmx" draw_name="Max temp. mokr. t." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" order="7"/>
        <raport title="WView - simulator" order="63"/>
      </param>
      <param name="WView:Simulator:minimalna dobowa temperatura z mokrego termometru" short_name="Tmmn" draw_name="Min temp. mokr. t." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" order="9"/>
        <raport title="WView - simulator" order="64"/>
      </param>
      <param name="WView:Simulator:¶rednia dobowa temperatura z mokrego termometru" short_name="Tmsr" draw_name="¦r. temp. mokr. t." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - wp³yw wiatru, s³oñca i wilgoci" min="-40" max="50" order="8"/>
        <raport title="WView - simulator" order="65"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku N wiatru" short_name="twN" draw_name="Czas wiatru N" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="66"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku NNE wiatru" short_name="twNNE" draw_name="Czas wiatru NNE" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="67"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku NE wiatru" short_name="twNE" draw_name="Czas wiatru NE" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="68"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku ENE wiatru" short_name="twENE" draw_name="Czas wiatru ENE" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="69"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku E wiatru" short_name="twE" draw_name="Czas wiatru E" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="70"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku ESE wiatru" short_name="twESE" draw_name="Czas wiatru ESE" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="71"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku SE wiatru" short_name="twSE" draw_name="Czas wiatru SE" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="72"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku SSE wiatru" short_name="twSSE" draw_name="Czas wiatru SSE" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="73"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku S wiatru" short_name="twS" draw_name="Czas wiatru S" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="74"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku SSW wiatru" short_name="twSSW" draw_name="Czas wiatru SSW" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="75"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku SW wiatru" short_name="twSW" draw_name="Czas wiatru SW" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="76"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku WSW wiatru" short_name="twWSW" draw_name="Czas wiatru WSW" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="77"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku W wiatru" short_name="twW" draw_name="Czas wiatru W" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="78"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku WNW wiatru" short_name="twWNW" draw_name="Czas wiatru WNW" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="79"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku NW wiatru" short_name="twNW" draw_name="Czas wiatru NW" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="80"/>
      </param>
      <param name="WView:Simulator:czas dominacji kierunku NNW wiatru" short_name="twNNW" draw_name="Czas wiatru NNW" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="81"/>
      </param>
      <param name="WView:Simulator:czas maksymalnej energii s³onecznej" short_name="tEsmx" draw_name="Czas Esmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="82"/>
      </param>
      <param name="WView:Simulator:czas maks. dobowego ocieplenia spow. s³oñcem" short_name="tTzsmx" draw_name="Czas Tzsmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="83"/>
      </param>
      <param name="WView:Simulator:czas min. dobowego ocieplenia spow. s³oñcem" short_name="tTzsmn" draw_name="Czas Tzsmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="84"/>
      </param>
      <param name="WView:Simulator:czas maksymalnej dobowej warto¶ci indeksu THSW" short_name="tTHSWmx" draw_name="Czas THSWmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="85"/>
      </param>
      <param name="WView:Simulator:czas minimalnej dobowej warto¶ci indeksu THSW" short_name="tTHSWmx" draw_name="Czas THSWmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="86"/>
      </param>
      <param name="WView:Simulator:czas maksymalnej dobowej warto¶ci indeksu THW" short_name="tTHWmx" draw_name="Czas THWmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="87"/>
      </param>
      <param name="WView:Simulator:czas minimalnej dobowej warto¶ci indeksu THW" short_name="tTHWmn" draw_name="Czas THWmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="88"/>
      </param>
      <param name="WView:Simulator:czas maks. dobowej temp. z mokrego termometru" short_name="tTmmx" draw_name="Czas Tmmx" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="89"/>
      </param>
      <param name="WView:Simulator:czas min. dobowej temp. z mokrego termometru" short_name="tTmmn" draw_name="Czas Tmmn" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="90"/>
      </param>
      <param name="WView:Simulator:ilo¶æ stopniodni ciep³ych" short_name="SDc" draw_name="Stopniodni ciep³e" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - indeksy temperaturowe" min="0" max="200" order="6"/>
        <raport title="WView - simulator" order="91"/>
      </param>
      <param name="WView:Simulator:flagi aktualnego rekordu" short_name="fl" draw_name="Flagi rekordu" unit="-" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="92"/>
      </param>
      <param name="WView:Simulator:czas aktualnego rekordu" short_name="t" draw_name="Czas rekordu" unit="min" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="93"/>
      </param>
      <param name="WView:Simulator:aktualna temperatura zewnêtrzna" short_name="Tz" draw_name="Temp. zewn." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="8"/>
        <raport title="WView - simulator" order="94"/>
      </param>
      <param name="WView:Simulator:maksymalna temperatura zewnêtrzna" short_name="Tzmx" draw_name="Temp. zewn. max" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="7"/>
        <raport title="WView - simulator" order="95"/>
      </param>
      <param name="WView:Simulator:minimalna temperatura zewnêtrzna" short_name="Tzmn" draw_name="Temp. zewn. min" unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="9"/>
        <raport title="WView - simulator" order="96"/>
      </param>
      <param name="WView:Simulator:temperatura wewnêtrzna" short_name="Tw" draw_name="Temp. wewn." unit="°C" prec="1" base_ind="auto">
        <draw title="WView - temperatury powietrza" min="-40" max="50" order="10"/>
        <raport title="WView - simulator" order="97"/>
      </param>
      <param name="WView:Simulator:ci¶nienie atmosferyczne" short_name="Pa" draw_name="Ci¶n. atm." unit="mmHg" prec="1" base_ind="auto">
        <draw title="WView - ci¶nienie atmosferyczne" min="700" max="800" order="4"/>
        <raport title="WView - simulator" order="98"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ zewnêtrzna" short_name="fz" draw_name="Wilg. zewn." unit="%" prec="1" base_ind="auto">
        <draw title="WView - wilgotno¶æ" min="0" max="100" order="6"/>
        <raport title="WView - simulator" order="99"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ wewnêtrzna" short_name="fw" draw_name="Wilg. wewn." unit="%" prec="1" base_ind="auto">
        <draw title="WView - wilgotno¶æ" min="0" max="100" order="7"/>
        <raport title="WView - simulator" order="100"/>
      </param>
      <param name="WView:Simulator:liczba klikniêæ deszczomierza" short_name="nd" draw_name="Klikn. deszcz." unit="-" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="101"/>
      </param>
      <param name="WView:Simulator:maksymalna intensywno¶æ opadu deszczu" short_name="Fdmx" draw_name="Intens. deszczu max" unit="n/h" prec="0" base_ind="auto">
        <draw title="WView - deszcz" min="0" max="4000" order="3"/>
        <raport title="WView - simulator" order="102"/>
      </param>
      <param name="WView:Simulator:prêdko¶æ wiatru" short_name="vw" draw_name="Prêdko¶æ wiatru" unit="km/h" prec="1" base_ind="auto">
        <draw title="WView - wiatr" min="0" max="200" order="6"/>
        <raport title="WView - simulator" order="103"/>
      </param>
      <param name="WView:Simulator:maksymalna prêdko¶æ wiatru" short_name="vwmx" draw_name="Prêdko¶æ wiatru max" unit="km/h" prec="1" base_ind="auto">
        <draw title="WView - wiatr" min="0" max="200" order="5"/>
        <raport title="WView - simulator" order="104"/>
      </param>
      <param name="WView:Simulator:aktualny kierunek wiatru" short_name="Dw" draw_name="Kierunek wiatru" unit="-" base_ind="auto">
        <value int="0" name="N"/>
        <value int="1" name="NNE"/>
        <value int="2" name="NE"/>
        <value int="3" name="ENE"/>
        <value int="4" name="E"/>
        <value int="5" name="ESE"/>
        <value int="6" name="SE"/>
        <value int="7" name="SSE"/>
        <value int="8" name="S"/>
        <value int="9" name="SSW"/>
        <value int="10" name="SW"/>
        <value int="11" name="WSW"/>
        <value int="12" name="W"/>
        <value int="13" name="WNW"/>
        <value int="14" name="NW"/>
        <value int="15" name="NNW"/>
        <raport title="WView - simulator" order="105"/>
      </param>
      <param name="WView:Simulator:dominuj±cy kierunek wiatru" short_name="Dwd" draw_name="Domin. kier. wiatru" unit="-" base_ind="auto">
        <value int="0" name="N"/>
        <value int="1" name="NNE"/>
        <value int="2" name="NE"/>
        <value int="3" name="ENE"/>
        <value int="4" name="E"/>
        <value int="5" name="ESE"/>
        <value int="6" name="SE"/>
        <value int="7" name="SSE"/>
        <value int="8" name="S"/>
        <value int="9" name="SSW"/>
        <value int="10" name="SW"/>
        <value int="11" name="WSW"/>
        <value int="12" name="W"/>
        <value int="13" name="WNW"/>
        <value int="14" name="NW"/>
        <value int="15" name="NNW"/>
        <raport title="WView - simulator" order="106"/>
      </param>
      <param name="WView:Simulator:prawid³owo odebrane pakiety z wiatromierza" short_name="NP" draw_name="Pakiety wiatrom." unit="-" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="107"/>
      </param>
      <param name="WView:Simulator:aktualna energia s³oneczna" short_name="Es" draw_name="Energia s³oneczna" unit="W/m2" prec="0" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="300" order="7"/>
        <raport title="WView - simulator" order="108"/>
      </param>
      <param name="WView:Simulator:maksymalna energia s³oneczna" short_name="Esmx" draw_name="En. s³oneczna max" unit="W/m2" prec="0" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="300" order="6"/>
        <raport title="WView - simulator" order="109"/>
      </param>
      <param name="WView:Simulator:aktualny indeks UV" short_name="UVi" draw_name="Indeks UV" unit="UVindex" prec="1" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="16" order="9"/>
        <raport title="WView - simulator" order="110"/>
      </param>
      <param name="WView:Simulator:maksymalny indeks UV" short_name="UVimx" draw_name="Indeks UV max" unit="UVindex" prec="1" base_ind="auto">
        <draw title="WView - promieniowanie" min="0" max="16" order="8"/>
        <raport title="WView - simulator" order="111"/>
      </param>
      <param name="WView:Simulator:temperatura li¶ci 1" short_name="Tl1" draw_name="Temp. li¶ci 1" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" prior="11" order="1"/>
        <raport title="WView - simulator" order="112"/>
      </param>
      <param name="WView:Simulator:temperatura li¶ci 2" short_name="Tl2" draw_name="Temp. li¶ci 2" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="2"/>
        <raport title="WView - simulator" order="113"/>
      </param>
      <param name="WView:Simulator:temperatura li¶ci 3" short_name="Tl3" draw_name="Temp. li¶ci 3" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="3"/>
        <raport title="WView - simulator" order="114"/>
      </param>
      <param name="WView:Simulator:temperatura li¶ci 4" short_name="Tl4" draw_name="Temp. li¶ci 4" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="4"/>
        <raport title="WView - simulator" order="115"/>
      </param>
      <param name="WView:Simulator:dodatkowe informacje o promieniowaniu" short_name="Prd" draw_name="Dodatkowe prom." unit="-" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="116"/>
      </param>
      <param name="WView:Simulator:prognoza pogody" short_name="Prg" draw_name="Prognoza pogody" unit="-" prec="0" base_ind="auto">
        <raport title="WView - simulator" order="117"/>
      </param>
      <param name="WView:Simulator:parowanie" short_name="par" draw_name="Parowanie" unit="mm" prec="3" base_ind="auto">
        <draw title="WView - parowanie" min="0" max="50" order="2"/>
        <raport title="WView - simulator" order="118"/>
      </param>
      <param name="WView:Simulator:temperatura gleby 1" short_name="Tg1" draw_name="Temp. gleby 1" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="5"/>
        <raport title="WView - simulator" order="119"/>
      </param>
      <param name="WView:Simulator:temperatura gleby 2" short_name="Tg2" draw_name="Temp. gleby 2" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="6"/>
        <raport title="WView - simulator" order="120"/>
      </param>
      <param name="WView:Simulator:temperatura gleby 3" short_name="Tg3" draw_name="Temp. gleby 3" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="7"/>
        <raport title="WView - simulator" order="121"/>
      </param>
      <param name="WView:Simulator:temperatura gleby 4" short_name="Tg4" draw_name="Temp. gleby 4" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="8"/>
        <raport title="WView - simulator" order="122"/>
      </param>
      <param name="WView:Simulator:temperatura gleby 5" short_name="Tg5" draw_name="Temp. gleby 5" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="9"/>
        <raport title="WView - simulator" order="123"/>
      </param>
      <param name="WView:Simulator:temperatura gleby 6" short_name="Tg6" draw_name="Temp. gleby 6" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury li¶ci i gleby" min="-40" max="50" order="10"/>
        <raport title="WView - simulator" order="124"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ gleby 1" short_name="fg1" draw_name="Wilg. gleby 1" unit="cb" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="50" order="5"/>
        <raport title="WView - simulator" order="125"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ gleby 2" short_name="fg2" draw_name="Wilg. gleby 2" unit="cb" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="50" order="6"/>
        <raport title="WView - simulator" order="126"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ gleby 3" short_name="fg3" draw_name="Wilg. gleby 3" unit="cb" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="50" order="7"/>
        <raport title="WView - simulator" order="127"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ gleby 4" short_name="fg4" draw_name="Wilg. gleby 4" unit="cb" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="50" order="8"/>
        <raport title="WView - simulator" order="128"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ gleby 5" short_name="fg5" draw_name="Wilg. gleby 5" unit="cb" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="50" order="9"/>
        <raport title="WView - simulator" order="129"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ gleby 6" short_name="fg6" draw_name="Wilg. gleby 6" unit="cb" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="50" order="10"/>
        <raport title="WView - simulator" order="130"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ li¶ci 1" short_name="fl1" draw_name="Wilg. li¶ci 1" unit="-" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="15" prior="12" order="1"/>
        <raport title="WView - simulator" order="131"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ li¶ci 2" short_name="fl2" draw_name="Wilg. li¶ci 2" unit="-" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="15" order="2"/>
        <raport title="WView - simulator" order="132"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ li¶ci 3" short_name="fl3" draw_name="Wilg. li¶ci 3" unit="-" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="15" order="3"/>
        <raport title="WView - simulator" order="133"/>
      </param>
      <param name="WView:Simulator:wilgotno¶æ li¶ci 4" short_name="fl4" draw_name="Wilg. li¶ci 4" unit="-" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶æ li¶ci i gleby" min="0" max="15" order="4"/>
        <raport title="WView - simulator" order="134"/>
      </param>
      <param name="WView:Simulator:dodatkowa temperatura 1" short_name="Td1" draw_name="Temp. dodatk. 1" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury dodatkowe" min="-40" max="50" prior="13" order="1"/>
        <raport title="WView - simulator" order="135"/>
      </param>
      <param name="WView:Simulator:dodatkowa temperatura 2" short_name="Td2" draw_name="Temp. dodatk. 2" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury dodatkowe" min="-40" max="50" order="2"/>
        <raport title="WView - simulator" order="136"/>
      </param>
      <param name="WView:Simulator:dodatkowa temperatura 3" short_name="Td3" draw_name="Temp. dodatk. 3" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury dodatkowe" min="-40" max="50" order="3"/>
        <raport title="WView - simulator" order="137"/>
      </param>
      <param name="WView:Simulator:dodatkowa temperatura 4" short_name="Td4" draw_name="Temp. dodatk. 4" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury dodatkowe" min="-40" max="50" order="4"/>
        <raport title="WView - simulator" order="138"/>
      </param>
      <param name="WView:Simulator:dodatkowa temperatura 5" short_name="Td5" draw_name="Temp. dodatk. 5" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury dodatkowe" min="-40" max="50" order="5"/>
        <raport title="WView - simulator" order="139"/>
      </param>
      <param name="WView:Simulator:dodatkowa temperatura 6" short_name="Td6" draw_name="Temp. dodatk. 6" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury dodatkowe" min="-40" max="50" order="6"/>
        <raport title="WView - simulator" order="140"/>
      </param>
      <param name="WView:Simulator:dodatkowa temperatura 7" short_name="Td7" draw_name="Temp. dodatk. 7" unit="°C" prec="0" base_ind="auto">
        <draw title="WView - temperatury dodatkowe" min="-40" max="50" order="7"/>
        <raport title="WView - simulator" order="141"/>
      </param>
      <param name="WView:Simulator:dodatkowa wilgotno¶æ 1" short_name="fd1" draw_name="Wilg. dodatk. 1" unit="%" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶ci dodatkowe" min="0" max="100" prior="14" order="1"/>
        <raport title="WView - simulator" order="142"/>
      </param>
      <param name="WView:Simulator:dodatkowa wilgotno¶æ 2" short_name="fd2" draw_name="Wilg. dodatk. 2" unit="%" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶ci dodatkowe" min="0" max="100" order="2"/>
        <raport title="WView - simulator" order="143"/>
      </param>
      <param name="WView:Simulator:dodatkowa wilgotno¶æ 3" short_name="fd3" draw_name="Wilg. dodatk. 3" unit="%" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶ci dodatkowe" min="0" max="100" order="3"/>
        <raport title="WView - simulator" order="144"/>
      </param>
      <param name="WView:Simulator:dodatkowa wilgotno¶æ 4" short_name="fd4" draw_name="Wilg. dodatk. 4" unit="%" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶ci dodatkowe" min="0" max="100" order="4"/>
        <raport title="WView - simulator" order="145"/>
      </param>
      <param name="WView:Simulator:dodatkowa wilgotno¶æ 5" short_name="fd5" draw_name="Wilg. dodatk. 5" unit="%" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶ci dodatkowe" min="0" max="100" order="5"/>
        <raport title="WView - simulator" order="146"/>
      </param>
      <param name="WView:Simulator:dodatkowa wilgotno¶æ 6" short_name="fd6" draw_name="Wilg. dodatk. 6" unit="%" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶ci dodatkowe" min="0" max="100" order="6"/>
        <raport title="WView - simulator" order="147"/>
      </param>
      <param name="WView:Simulator:dodatkowa wilgotno¶æ 7" short_name="fd7" draw_name="Wilg. dodatk. 7" unit="%" prec="0" base_ind="auto">
        <draw title="WView - wilgotno¶ci dodatkowe" min="0" max="100" order="7"/>
        <raport title="WView - simulator" order="148"/>
      </param>
    </unit>
  </device>
 @description_end
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_BOOST_DATE_TIME) && defined(HAVE_BOOST_FILESYSTEM) && defined(HAVE_BOOST_REGEX) && defined(HAVE_BOOST_PROGRAM_OPTIONS)

#include "ipchandler.h"
#include "liblog.h"
#include "conversion.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <locale>

#include <boost/date_time/posix_time/posix_time.hpp> 
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>

#include <libxml/tree.h>

class WLKExporter;

class WLKReader {
    private:
        static const char WLK_file_id[16];

        static const unsigned char data_record_data_type = 1;
        static const unsigned char daily_summary_data_type_1 = 2;
        static const unsigned char daily_summary_data_type_2 = 3;

        static const unsigned char read_params_number = 148;

        static unsigned char data_record_allowed_fuzzyness;
        static unsigned char data_record_span; 

        static const unsigned char time_values_1_maximum_out_temperature_time_offset = 0;
        static const unsigned char time_values_1_minimum_out_temperature_time_offset = 1;
        static const unsigned char time_values_1_maximum_in_temperature_time_offset = 2;
        static const unsigned char time_values_1_minimum_in_temperature_time_offset = 3;
        static const unsigned char time_values_1_maximum_wind_chill_time_offset = 4;
        static const unsigned char time_values_1_minimum_wind_chill_time_offset = 5;
        static const unsigned char time_values_1_maximum_dew_point_time_offset = 6;
        static const unsigned char time_values_1_minimum_dew_point_time_offset = 7;
        static const unsigned char time_values_1_maximum_out_humidity_time_offset = 8;
        static const unsigned char time_values_1_minimum_out_humidity_time_offset = 9;
        static const unsigned char time_values_1_maximum_in_humidity_time_offset = 10;
        static const unsigned char time_values_1_minimum_in_humidity_time_offset = 11;
        static const unsigned char time_values_1_maximum_pressure_time_offset = 12;
        static const unsigned char time_values_1_minimum_pressure_time_offset = 13;
        static const unsigned char time_values_1_maximum_wind_speed_time_offset = 14;
        static const unsigned char time_values_1_maximum_average_wind_speed_time_offset = 15;
        static const unsigned char time_values_1_maximum_rain_rate_time_offset = 16;
        static const unsigned char time_values_1_maximum_UV_index_time_offset = 17;

        static const unsigned char time_values_2_maximum_solar_radiation_time_offset = 0;
        static const unsigned char time_values_2_maximum_heat_index_time_offset = 1;
        static const unsigned char time_values_2_minimum_heat_index_time_offset = 2;
        static const unsigned char time_values_2_maximum_THSW_index_time_offset = 3;
        static const unsigned char time_values_2_minimum_THSW_index_time_offset = 4;
        static const unsigned char time_values_2_maximum_THW_index_time_offset = 5;
        static const unsigned char time_values_2_minimum_THW_index_time_offset = 6;
        static const unsigned char time_values_2_maximum_wet_bulb_time_offset = 7;
        static const unsigned char time_values_2_minimum_wet_bulb_time_offset = 8;

        static const unsigned char dominant_wind_direction_N_time_offset = 0;
        static const unsigned char dominant_wind_direction_NNE_time_offset = 1;
        static const unsigned char dominant_wind_direction_NE_time_offset = 2;
        static const unsigned char dominant_wind_direction_ENE_time_offset = 3;
        static const unsigned char dominant_wind_direction_E_time_offset = 4;
        static const unsigned char dominant_wind_direction_ESE_time_offset = 5;
        static const unsigned char dominant_wind_direction_SE_time_offset = 6;
        static const unsigned char dominant_wind_direction_SSE_time_offset = 7;
        static const unsigned char dominant_wind_direction_S_time_offset = 8;
        static const unsigned char dominant_wind_direction_SSW_time_offset = 9;
        static const unsigned char dominant_wind_direction_SW_time_offset = 10;
        static const unsigned char dominant_wind_direction_WSW_time_offset = 11;
        static const unsigned char dominant_wind_direction_W_time_offset = 12;
        static const unsigned char dominant_wind_direction_WNW_time_offset = 13;
        static const unsigned char dominant_wind_direction_NW_time_offset = 14;
        static const unsigned char dominant_wind_direction_NNW_time_offset = 15;

        static const unsigned short int rain_collector_type_mask = 0xF000;
        static const unsigned short int rain_collector_clicks_mask = 0x0FFF;

        static const unsigned short int rain_collector_0_1_in_type = 0x0000;
        static const unsigned short int rain_collector_0_01_in_type = 0x1000;
        static const unsigned short int rain_collector_0_2_mm_type = 0x2000;
        static const unsigned short int rain_collector_1_0_mm_type = 0x3000;
        static const unsigned short int rain_collector_0_1_mm_type = 0x6000;

        static const unsigned short int pressure_computation_offset_mm = 6860;
        static const unsigned short int pressure_computation_offset_in = 27000;

        struct days_index {
            short int records_in_day;
            int beginning_offset;
        } __attribute__ ((packed));

        struct header {
            char id_code[sizeof(WLK_file_id)];
            int total_number_of_records;
            struct days_index days_index[32];
        } __attribute__ ((packed));

        struct daily_summary {
            unsigned char data_type_flag_1;
            unsigned char reserved_1;

            short int data_span;
            short int maximum_out_temperature;
            short int minimum_out_temperature;
            short int maximum_in_temperature;
            short int minimum_in_temperature;
            short int average_out_temperature;
            short int average_in_temperature;
            short int maximum_wind_chill;
            short int minimum_wind_chill;
            short int maximum_dew_point;
            short int minimum_dew_point;
            short int average_wind_chill;
            short int average_dew_point;
            short int maximum_out_humidity;
            short int minimum_out_humidity;
            short int maximum_in_humidity;
            short int minimum_in_humidity;
            short int average_out_humidity;
            short int maximum_pressure;
            short int minimum_pressure;
            short int average_pressure;
            short int maximum_wind_speed;
            short int average_wind_speed;
            short int daily_total_wind_run;
            short int maximum_10_minute_wind_speed;
            unsigned char maximum_speed_wind_direction;
            unsigned char maximum_10_minute_wind_speed_direction;
            short int daily_total_rain;
            short int maximum_rain_rate;
            short int daily_total_UV_dose;
            unsigned char maximum_UV_index;
            unsigned char time_values_1[27];
            
            unsigned char data_type_flag_2;
            unsigned char reserved_2;

            unsigned short int daily_weather_conditions_bitmap;
            short int number_of_valid_wind_packets;
            short int maximum_solar_energy;
            short int daily_total_solar_energy;
            short int minimum_sunlight_time;
            short int daily_total_evapotranspiration;
            short int maximum_heat_index;
            short int minimum_heat_index;
            short int average_heat_index;
            short int maximum_THSW_index;
            short int minimum_THSW_index;
            short int maximum_THW_index;
            short int minimum_THW_index;
            short int integrated_heating_degree_days;
            short int maximum_wet_bulb;
            short int minimum_wet_bulb;
            short int average_wet_bulb;
            unsigned char dominant_wind_direction_times[24];
            unsigned char time_values_2[15];
            short int integrated_cooling_degree_days;
            unsigned char reserved_3[11];
        } __attribute__ ((packed));

        struct data_record {
            unsigned char data_type_flag;

            unsigned char archive_interval;
            unsigned short int flags;
            short int time;
            short int outside_temperature;
            short int maximum_outside_temperature;
            short int minimum_outside_temperature;
            short int inside_temperature;
            short int barometric_pressure;
            short int outside_humidity;
            short int inside_humidity;
            unsigned short int rain;
            short int maximum_rain_rate;
            short int wind_speed;
            short int maximum_wind_speed;
            unsigned char wind_direction;
            unsigned char dominant_wind_direction;
            short int number_of_wind_data_packets;
            short int solar_radiation;
            short int maximum_solar_radiation;
            unsigned char UV_index;
            unsigned char maximum_UV_index;
            unsigned char leaf_temperatures[4];
            short int extra_radiation;
            short int reserved[6];
            unsigned char forecast;
            unsigned char evapotranspiration;
            unsigned char soil_temperatures[6];
            unsigned char soil_moistures[6];
            unsigned char leaf_wetnesses[4];
            unsigned char extra_temperatures[7];
            unsigned char extra_humidity[7];
        } __attribute__ ((packed));

        struct daily_summary last_read_daily_summary;
        struct data_record last_read_data_record;

        unsigned short int rain_collector_type;

        static inline short int convert_fahrenheit_to_celsius(int fahrenheit, int multiplier) {
            if (fahrenheit == SZARP_NO_DATA || ((unsigned char) fahrenheit) == 0xFF || ((unsigned short int) fahrenheit) == 0xFFFF)
                return SZARP_NO_DATA;

            return (fahrenheit - 32 * multiplier) * 5 / 9;
        }

        static inline short int convert_inches_to_milimeters(int inches) {
            if (inches == SZARP_NO_DATA || ((unsigned char) inches) == 0xFF || ((unsigned short int) inches) == 0xFFFF)
                return SZARP_NO_DATA;

            sz_log(10, "Inches to millimetres conversion: inches: %d, millimetres: %lf, as short int: %d", inches, inches * 25.4, (short int) (inches * 25.4));

            return (short int) (inches * 25.4);
        }

        static inline short int convert_miles_to_kilometers(int miles) {
            if (miles == SZARP_NO_DATA || ((unsigned char) miles) == 0xFF || ((unsigned short int) miles) == 0xFFFF)
                return SZARP_NO_DATA;

            return (short int) (miles * 1.609344);
        }

        static inline short int normalize_extra_temperature(int temperature) {
            if (temperature == SZARP_NO_DATA || ((unsigned char) temperature) == 0xFF || ((unsigned short int) temperature) == 0xFFFF)
                return SZARP_NO_DATA;

            return temperature - 90;
        }

        static inline short int extract_value(unsigned char values_array[], int offset) {
            unsigned short int return_value;
            int index = (offset / 2) * 3;

            if (offset % 2 == 0)
                return_value = values_array[index] | ((values_array[index + 2] & 0x0F) << 8);
            else
                return_value = values_array[index + 1] | ((values_array[index + 2] & 0xF0) << 4);

            return (return_value == 0x0FFF || return_value == 0x07FF) ? SZARP_NO_DATA : return_value;
        }

        static inline short int decode_rain_value(short int rain_clicks, unsigned short int rain_collector_type) {
            switch (rain_collector_type) {
                case rain_collector_0_1_in_type:
                    return convert_inches_to_milimeters(rain_clicks);
                case rain_collector_0_01_in_type:
                    return convert_inches_to_milimeters(rain_clicks / 10);
                case rain_collector_0_2_mm_type:
                    return 2 * rain_clicks;
                case rain_collector_1_0_mm_type:
                    return 10 * rain_clicks;
                case rain_collector_0_1_mm_type:
                    return rain_clicks;
                default:
                    sz_log(9, "Unknown rain collector type %x", rain_collector_type);

                    return SZARP_NO_DATA;
            }
        }

        static inline short int validate_wind_direction(unsigned char wind_direction) {
            return (wind_direction == 0xFF) ? SZARP_NO_DATA : wind_direction;
        }

        static inline short int validate_extra_data(unsigned char data) {
            return (data == 0xFF) ? SZARP_NO_DATA : data;
        }

    public:
        WLKReader(xmlNodePtr device_node) {
            sz_log(8, "Reading configuration parameters from the device tag");

            std::wstringstream conversion_stream;

            char *value = reinterpret_cast<char *>(xmlGetNsProp(device_node, BAD_CAST("read_fuzzyness"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING)));

            if (value == NULL) {
                sz_log(4, "Attribute wlk:read_fuzzyness not found in device tag in line %ld - using the default value 5", xmlGetLineNo(device_node));
                data_record_allowed_fuzzyness = 5;
            } else {
                long int long_value = strtol(value, NULL, 10);

                if (long_value == LONG_MIN) {
                    sz_log(4, "Attribute wlk:read_fuzzyness unparseable in device tag in line %ld - using the default value 5", xmlGetLineNo(device_node));
                    data_record_allowed_fuzzyness = 5;
                } else
                    data_record_allowed_fuzzyness = long_value;
            }

            free(value);

            value = reinterpret_cast<char *>(xmlGetNsProp(device_node, BAD_CAST("record_span"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING)));

            if (value == NULL) {
                sz_log(4, "Attribute wlk:record_span not found in device tag in line %ld - using the default value 5", xmlGetLineNo(device_node));
                data_record_span = 5;
            } else {
                long int long_value = strtol(value, NULL, 10);

                if (long_value == LONG_MIN) {
                    sz_log(4, "Attribute wlk:record_span unparseable in device tag in line %ld - using the default value 5", xmlGetLineNo(device_node));
                    data_record_span = 5;
                } else
                    data_record_span = long_value;
            }

            free(value);
        }

        short operator[](int index);
        bool read_record(std::ifstream& input_stream, boost::posix_time::ptime time_to_read);
        void pass_last_read_record_to_parcook(DaemonConfig *config, IPCHandler *ipc);


       friend struct WLKExporter;
};

const char WLKReader::WLK_file_id[] = { 'W', 'D', 'A', 'T', '5', '.', '0', 0, 0, 0, 0, 0, 0, 0, 5, 0 };

unsigned char WLKReader::data_record_allowed_fuzzyness;
unsigned char WLKReader::data_record_span;

short WLKReader::operator[](int index) {
    switch (index) {
        case 0: return convert_fahrenheit_to_celsius(last_read_daily_summary.maximum_out_temperature, 10);
        case 1: return convert_fahrenheit_to_celsius(last_read_daily_summary.minimum_out_temperature, 10);
        case 2: return convert_fahrenheit_to_celsius(last_read_daily_summary.maximum_in_temperature, 10);
        case 3: return convert_fahrenheit_to_celsius(last_read_daily_summary.minimum_in_temperature, 10);
        case 4: return convert_fahrenheit_to_celsius(last_read_daily_summary.average_out_temperature, 10);
        case 5: return convert_fahrenheit_to_celsius(last_read_daily_summary.average_in_temperature, 10);
        case 6: return convert_fahrenheit_to_celsius(last_read_daily_summary.maximum_wind_chill, 10);
        case 7: return convert_fahrenheit_to_celsius(last_read_daily_summary.minimum_wind_chill, 10);
        case 8: return convert_fahrenheit_to_celsius(last_read_daily_summary.maximum_dew_point, 10);
        case 9: return convert_fahrenheit_to_celsius(last_read_daily_summary.minimum_dew_point, 10);
        case 10: return convert_fahrenheit_to_celsius(last_read_daily_summary.average_wind_chill, 10);
        case 11: return convert_fahrenheit_to_celsius(last_read_daily_summary.average_dew_point, 10);
        case 12: return last_read_daily_summary.maximum_out_humidity;
        case 13: return last_read_daily_summary.minimum_out_humidity;
        case 14: return last_read_daily_summary.maximum_in_humidity;
        case 15: return last_read_daily_summary.minimum_in_humidity;
        case 16: return last_read_daily_summary.average_out_humidity;
        case 17: return convert_inches_to_milimeters((last_read_daily_summary.maximum_pressure - pressure_computation_offset_in) / 10) / 10 + pressure_computation_offset_mm; 
        case 18: return convert_inches_to_milimeters((last_read_daily_summary.minimum_pressure - pressure_computation_offset_in) / 10) / 10 + pressure_computation_offset_mm; 
        case 19: return convert_inches_to_milimeters((last_read_daily_summary.average_pressure - pressure_computation_offset_in) / 10) / 10 + pressure_computation_offset_mm; 
        case 20: return convert_miles_to_kilometers(last_read_daily_summary.maximum_wind_speed);
        case 21: return convert_miles_to_kilometers(last_read_daily_summary.average_wind_speed);
        case 22: return convert_miles_to_kilometers(last_read_daily_summary.daily_total_wind_run); 
        case 23: return convert_miles_to_kilometers(last_read_daily_summary.maximum_10_minute_wind_speed); 
        case 24: return validate_wind_direction(last_read_daily_summary.maximum_speed_wind_direction);
        case 25: return validate_wind_direction(last_read_daily_summary.maximum_10_minute_wind_speed_direction);
        case 26: return convert_inches_to_milimeters(last_read_daily_summary.daily_total_rain / 10);
        case 27: return convert_inches_to_milimeters(last_read_daily_summary.maximum_rain_rate / 10);
        case 28: return last_read_daily_summary.daily_total_UV_dose;
        case 29: return last_read_daily_summary.maximum_UV_index;
        case 30: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_out_temperature_time_offset);
        case 31: return extract_value(last_read_daily_summary.time_values_1, time_values_1_minimum_out_temperature_time_offset);
        case 32: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_in_temperature_time_offset);
        case 33: return extract_value(last_read_daily_summary.time_values_1, time_values_1_minimum_in_temperature_time_offset);
        case 34: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_wind_chill_time_offset);
        case 35: return extract_value(last_read_daily_summary.time_values_1, time_values_1_minimum_wind_chill_time_offset);
        case 36: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_dew_point_time_offset);
        case 37: return extract_value(last_read_daily_summary.time_values_1, time_values_1_minimum_dew_point_time_offset);
        case 38: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_out_humidity_time_offset);
        case 39: return extract_value(last_read_daily_summary.time_values_1, time_values_1_minimum_out_humidity_time_offset);
        case 40: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_in_humidity_time_offset);
        case 41: return extract_value(last_read_daily_summary.time_values_1, time_values_1_minimum_in_humidity_time_offset);
        case 42: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_pressure_time_offset);
        case 43: return extract_value(last_read_daily_summary.time_values_1, time_values_1_minimum_pressure_time_offset);
        case 44: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_wind_speed_time_offset);
        case 45: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_average_wind_speed_time_offset);
        case 46: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_rain_rate_time_offset);
        case 47: return extract_value(last_read_daily_summary.time_values_1, time_values_1_maximum_UV_index_time_offset);
        case 48: return last_read_daily_summary.daily_weather_conditions_bitmap;
        case 49: return last_read_daily_summary.number_of_valid_wind_packets;
        case 50: return last_read_daily_summary.maximum_solar_energy;
        case 51: return last_read_daily_summary.daily_total_solar_energy;
        case 52: return last_read_daily_summary.minimum_sunlight_time;
        case 53: return convert_inches_to_milimeters(last_read_daily_summary.daily_total_evapotranspiration);
        case 54: return convert_fahrenheit_to_celsius(last_read_daily_summary.maximum_heat_index, 10);
        case 55: return convert_fahrenheit_to_celsius(last_read_daily_summary.minimum_heat_index, 10);
        case 56: return convert_fahrenheit_to_celsius(last_read_daily_summary.average_heat_index, 10);
        case 57: return convert_fahrenheit_to_celsius(last_read_daily_summary.maximum_THSW_index, 10);
        case 58: return convert_fahrenheit_to_celsius(last_read_daily_summary.minimum_THSW_index, 10);
        case 59: return convert_fahrenheit_to_celsius(last_read_daily_summary.maximum_THW_index, 10);
        case 60: return convert_fahrenheit_to_celsius(last_read_daily_summary.minimum_THW_index, 10);
        case 61: return convert_fahrenheit_to_celsius(last_read_daily_summary.integrated_heating_degree_days, 10);
        case 62: return convert_fahrenheit_to_celsius(last_read_daily_summary.maximum_wet_bulb, 10);
        case 63: return convert_fahrenheit_to_celsius(last_read_daily_summary.minimum_wet_bulb, 10);
        case 64: return convert_fahrenheit_to_celsius(last_read_daily_summary.average_wet_bulb, 10);
        case 65: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_N_time_offset);
        case 66: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_NNE_time_offset);
        case 67: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_NE_time_offset);
        case 68: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_ENE_time_offset);
        case 69: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_E_time_offset);
        case 70: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_ESE_time_offset);
        case 71: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_SE_time_offset);
        case 72: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_SSE_time_offset);
        case 73: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_S_time_offset);
        case 74: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_SSW_time_offset);
        case 75: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_SW_time_offset);
        case 76: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_WSW_time_offset);
        case 77: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_W_time_offset);
        case 78: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_WNW_time_offset);
        case 79: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_NW_time_offset);
        case 80: return extract_value(last_read_daily_summary.dominant_wind_direction_times, dominant_wind_direction_NNW_time_offset);
        case 81: return extract_value(last_read_daily_summary.time_values_2, time_values_2_maximum_solar_radiation_time_offset);
        case 82: return extract_value(last_read_daily_summary.time_values_2, time_values_2_maximum_heat_index_time_offset);
        case 83: return extract_value(last_read_daily_summary.time_values_2, time_values_2_minimum_heat_index_time_offset);
        case 84: return extract_value(last_read_daily_summary.time_values_2, time_values_2_maximum_THSW_index_time_offset);
        case 85: return extract_value(last_read_daily_summary.time_values_2, time_values_2_minimum_THSW_index_time_offset);
        case 86: return extract_value(last_read_daily_summary.time_values_2, time_values_2_maximum_THW_index_time_offset);
        case 87: return extract_value(last_read_daily_summary.time_values_2, time_values_2_minimum_THW_index_time_offset);
        case 88: return extract_value(last_read_daily_summary.time_values_2, time_values_2_maximum_wet_bulb_time_offset);
        case 89: return extract_value(last_read_daily_summary.time_values_2, time_values_2_minimum_wet_bulb_time_offset);
        case 90: return convert_fahrenheit_to_celsius(last_read_daily_summary.integrated_cooling_degree_days, 10);
        case 91: return last_read_data_record.flags;
        case 92: return last_read_data_record.time;
        case 93: return convert_fahrenheit_to_celsius(last_read_data_record.outside_temperature, 10);
        case 94: return convert_fahrenheit_to_celsius(last_read_data_record.maximum_outside_temperature, 10);
        case 95: return convert_fahrenheit_to_celsius(last_read_data_record.minimum_outside_temperature, 10);
        case 96: return convert_fahrenheit_to_celsius(last_read_data_record.inside_temperature, 10);
        case 97: return convert_inches_to_milimeters((last_read_data_record.barometric_pressure - pressure_computation_offset_in) / 10) / 10 + pressure_computation_offset_mm; 
        case 98: return last_read_data_record.outside_humidity;
        case 99: return last_read_data_record.inside_humidity;
        case 100: return decode_rain_value(last_read_data_record.rain & rain_collector_clicks_mask, last_read_data_record.rain & rain_collector_type_mask);
        case 101: return decode_rain_value(last_read_data_record.maximum_rain_rate, last_read_data_record.rain & rain_collector_type_mask);
        case 102: return convert_miles_to_kilometers(last_read_data_record.wind_speed);
        case 103: return convert_miles_to_kilometers(last_read_data_record.maximum_wind_speed);
        case 104: return validate_wind_direction(last_read_data_record.wind_direction);
        case 105: return validate_wind_direction(last_read_data_record.dominant_wind_direction);
        case 106: return last_read_data_record.number_of_wind_data_packets;
        case 107: return last_read_data_record.solar_radiation;
        case 108: return last_read_data_record.maximum_solar_radiation;
        case 109: return last_read_data_record.UV_index;
        case 110: return last_read_data_record.maximum_UV_index;
        case 111: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.leaf_temperatures[0]), 1);
        case 112: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.leaf_temperatures[1]), 1);
        case 113: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.leaf_temperatures[2]), 1);
        case 114: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.leaf_temperatures[3]), 1);
        case 115: return validate_extra_data(last_read_data_record.extra_radiation);
        case 116: return last_read_data_record.forecast;
        case 117: return convert_inches_to_milimeters(last_read_data_record.evapotranspiration);
        case 118: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.soil_temperatures[0]), 1);
        case 119: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.soil_temperatures[1]), 1);
        case 120: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.soil_temperatures[2]), 1);
        case 121: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.soil_temperatures[3]), 1);
        case 122: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.soil_temperatures[4]), 1);
        case 123: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.soil_temperatures[5]), 1);
        case 124: return validate_extra_data(last_read_data_record.soil_moistures[0]);
        case 125: return validate_extra_data(last_read_data_record.soil_moistures[1]);
        case 126: return validate_extra_data(last_read_data_record.soil_moistures[2]);
        case 127: return validate_extra_data(last_read_data_record.soil_moistures[3]);
        case 128: return validate_extra_data(last_read_data_record.soil_moistures[4]);
        case 129: return validate_extra_data(last_read_data_record.soil_moistures[5]);
        case 130: return validate_extra_data(last_read_data_record.leaf_wetnesses[0]);
        case 131: return validate_extra_data(last_read_data_record.leaf_wetnesses[1]);
        case 132: return validate_extra_data(last_read_data_record.leaf_wetnesses[2]);
        case 133: return validate_extra_data(last_read_data_record.leaf_wetnesses[3]);
        case 134: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.extra_temperatures[0]), 1);
        case 135: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.extra_temperatures[1]), 1);
        case 136: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.extra_temperatures[2]), 1);
        case 137: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.extra_temperatures[3]), 1);
        case 138: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.extra_temperatures[4]), 1);
        case 139: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.extra_temperatures[5]), 1);
        case 140: return convert_fahrenheit_to_celsius(normalize_extra_temperature(last_read_data_record.extra_temperatures[6]), 1);
        case 141: return validate_extra_data(last_read_data_record.extra_humidity[0]);
        case 142: return validate_extra_data(last_read_data_record.extra_humidity[1]);
        case 143: return validate_extra_data(last_read_data_record.extra_humidity[2]);
        case 144: return validate_extra_data(last_read_data_record.extra_humidity[3]);
        case 145: return validate_extra_data(last_read_data_record.extra_humidity[4]);
        case 146: return validate_extra_data(last_read_data_record.extra_humidity[5]);
        case 147: return validate_extra_data(last_read_data_record.extra_humidity[6]);
        default: return SZARP_NO_DATA;
    }
}

bool WLKReader::read_record(std::ifstream& input_stream, boost::posix_time::ptime time_to_read) {
    std::wstringstream log_message;
    log_message << time_to_read;

    sz_log(5, "Reading data record from the data file for %s", SC::S2A(log_message.str()).c_str());

    input_stream.seekg(0, std::ios::beg);

    struct header header;

    input_stream.read(reinterpret_cast<char *>(&header), sizeof(struct header));

    if (input_stream.eof() || input_stream.fail()) {
        sz_log(7, "Error reading the file header");

        return false;
    }

    for (unsigned int i = 0; i < sizeof(header.id_code); i++) {
        if (header.id_code[i] != WLK_file_id[i]) {
            sz_log(7, "Invalid file header read");

            return false;
        }
    }

    sz_log(7, "Total number of records found: %d", header.total_number_of_records);

    int number_of_records_in_given_day = header.days_index[time_to_read.date().day()].records_in_day;

    if (number_of_records_in_given_day == 0) {
        sz_log(7, "No records found for the day (%d) found", time_to_read.date().day().as_number());

        return false;
    } else
        sz_log(7, "Number of records found for day %d: %d", time_to_read.date().day().as_number(), number_of_records_in_given_day);

    input_stream.seekg(sizeof(struct header) + sizeof(struct data_record) * header.days_index[time_to_read.date().day()].beginning_offset, std::ios_base::beg);
    
    if (input_stream.eof() || input_stream.fail()) {
        sz_log(7, "Error seeking for records for the given day");

        return false;
    }

    input_stream.read(reinterpret_cast<char *>(&last_read_daily_summary), sizeof(struct daily_summary));

    if (input_stream.eof() || input_stream.fail() || last_read_daily_summary.data_type_flag_1 != daily_summary_data_type_1 || last_read_daily_summary.data_type_flag_2 != daily_summary_data_type_2) {
        sz_log(7, "Error reading summary record for the given day");

        return false;
    }

    bool wanted_record_found = false;
    int record_number = 0;
    short int wanted_record_minutes = 60 * time_to_read.time_of_day().hours() + time_to_read.time_of_day().minutes();

    while (!wanted_record_found && ++record_number <= number_of_records_in_given_day - 2) {
        input_stream.read(reinterpret_cast<char *>(&last_read_data_record), sizeof(struct data_record));

        if (input_stream.eof() || input_stream.fail() || last_read_data_record.data_type_flag != data_record_data_type) {
            sz_log(7, "Error reading data record no. %d for the given day", record_number);

            return false;
        }

        if (std::abs(last_read_data_record.time - wanted_record_minutes) <= data_record_allowed_fuzzyness) {
            wanted_record_found = true;

            sz_log(5, "Found data for the given time in record no. %d", record_number + 2);
        }
    }

    return wanted_record_found;
}

void WLKReader::pass_last_read_record_to_parcook(DaemonConfig *config, IPCHandler *ipc) {
    sz_log(5, "Passing read data to parcook");

    if (config->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetParamsCount() != read_params_number) {
        sz_log(2, "Incorrect parameters number in configuration: got %d, expected %d", config->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetParamsCount(), read_params_number);

        for (int i = 0; i < config->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetParamsCount(); i++)
            ipc->m_read[i] = SZARP_NO_DATA;

        return;
    }

    for (int i = 0; i < read_params_number; i++)
        ipc->m_read[i] = (*this)[i];
}

class WLKExporter : public std::unary_function<boost::filesystem::path, void> {
    private: 
        std::wstring output_file_name;
        xmlNodePtr device_node;
        std::vector<std::wstring> parameters_names;
        std::vector<long int> parameters_precisions;
        std::map<int, std::vector<std::wstring> > exported_data;
        boost::gregorian::date begin_date;
        boost::gregorian::date end_date;

        void save_last_read_data(WLKReader wlk_reader, boost::posix_time::ptime time_data_read);
        void write_data_to_file();
    public:
        WLKExporter(std::wstring _output_file_name, boost::gregorian::date _begin_date, boost::gregorian::date _end_date, xmlNodePtr _device_node) : output_file_name(_output_file_name), device_node(_device_node), begin_date(_begin_date), end_date(_end_date) { 
            sz_log(8, "Parsing the configuration unit element");

            xmlNodePtr unit_node;

            for (unit_node = device_node->children; unit_node != NULL; unit_node = unit_node->next) {
                if (unit_node->ns != NULL && SC::U2S(unit_node->ns->href) == IPK_NAMESPACE_STRING && SC::U2S(unit_node->name) == L"unit")
                    break;
            }

            for (xmlNodePtr param_node = unit_node->children; param_node != NULL; param_node = param_node->next) {
                if (param_node->ns != NULL && SC::U2S(param_node->ns->href) == IPK_NAMESPACE_STRING && SC::U2S(param_node->name) == L"param") {
                    char *value = reinterpret_cast<char *>(xmlGetProp(param_node, BAD_CAST("name")));

                    if (value == NULL) {
                        sz_log(7, "Error reading param name from param tag in line %ld", xmlGetLineNo(param_node));
                    } else {
                        sz_log(9, "Read param name %s from param tag in line %ld", value, xmlGetLineNo(param_node));

                        parameters_names.push_back(SC::A2S(value));

                        free(value);
                    }

                    value = reinterpret_cast<char *>(xmlGetProp(param_node, BAD_CAST("prec")));

                    if (value == NULL) {
                        sz_log(7, "Error reading param precision from param tag in line %ld", xmlGetLineNo(param_node));
                        parameters_precisions.push_back(0);
                    } else {
                        long int long_value = strtol(value, NULL, 10);

                        if (long_value == LONG_MIN) {
                            sz_log(4, "Attribute prec unparseable in param tag in line %ld", xmlGetLineNo(param_node));

                            parameters_precisions.push_back(0);
                        } else
                            parameters_precisions.push_back(long_value);

                        sz_log(9, "Read param precision %ld from param tag in line %ld", long_value, xmlGetLineNo(param_node));

                        free(value);
                    }
                }
            }
        }

        void operator()(const boost::filesystem::wpath &file);
};

void WLKExporter::write_data_to_file() {
    sz_log(5, "Opening the output file %s", SC::S2A(output_file_name).c_str());

    std::wofstream output_file;
    output_file.open(SC::S2A(output_file_name).c_str(), std::ios::out | std::ios::app);

    for (std::map<int, std::vector<std::wstring> >::iterator exported_data_iterator = exported_data.begin(); exported_data_iterator != exported_data.end(); ++exported_data_iterator) {
        for (std::vector<std::wstring>::iterator elements_iterator = exported_data_iterator->second.begin(); elements_iterator != exported_data_iterator->second.end(); ++elements_iterator) {
            output_file << "\"" << parameters_names.at(exported_data_iterator->first) << "\" " << *elements_iterator;
        }
    }

    exported_data.clear();
}

void WLKExporter::save_last_read_data(WLKReader wlk_reader, boost::posix_time::ptime time_data_read) {
    for (int i = 0; i < WLKReader::read_params_number; i++) {
        std::wstringstream output_stream;

        output_stream.imbue(std::locale(output_stream.getloc(), new boost::posix_time::time_facet("%Y %m %d %H %M")));
        output_stream.setf(std::ios::fixed, std::ios::floatfield);

        long int parameter_precision = (wlk_reader[i] == SZARP_NO_DATA ? 0 : parameters_precisions.at(i));
        output_stream.precision(parameter_precision);

        output_stream << time_data_read << " " << wlk_reader[i] * std::pow(0.1, (double) parameter_precision) << "\n"; 

        exported_data[i].push_back(output_stream.str());
    }
}

void WLKExporter::operator()(const boost::filesystem::wpath &file) {
    boost::wsmatch match;

#if BOOST_FILESYSTEM_VERSION == 3
	std::wstring filename = file.wstring();
#else
	std::wstring filename = file.string();
#endif

    sz_log(5, "Processing file %s", SC::S2A(filename).c_str());

    if (!boost::regex_search(filename, match, boost::wregex(L"(\\d{4})-(\\d{2}).wlk")))
        return;

    sz_log(5, "Opening the data file %s", SC::S2A(filename).c_str());

    std::ifstream wlk_file;
    wlk_file.open(SC::S2A(filename).c_str(), std::ios::in | std::ios::binary);

    if (wlk_file.is_open()) {
        WLKReader wlk_reader(device_node);

        std::wstringstream record_date_stream;
        int year;
        int month;

        record_date_stream << match[1] << " " << match[2];

        record_date_stream >> year;
        record_date_stream >> month;

        boost::gregorian::date beginning_of_month(year, month, 1);
        boost::gregorian::date end_of_month = boost::gregorian::date(year, month, boost::gregorian::gregorian_calendar::end_of_month_day(year, month)) + boost::gregorian::days(1);

        boost::posix_time::ptime start_time((begin_date == boost::gregorian::date() || beginning_of_month >= begin_date) ? beginning_of_month : begin_date);
        boost::posix_time::ptime end_time((end_date == boost::gregorian::date() || end_of_month <= end_date) ? end_of_month : end_date);

        for (boost::posix_time::time_iterator time_iterator(start_time, boost::posix_time::minutes(WLKReader::data_record_span)); *time_iterator < end_time; ++time_iterator) {
            if (wlk_reader.read_record(wlk_file, *time_iterator))
                save_last_read_data(wlk_reader, *time_iterator);
            else
                sz_log(9, "Error reading data record from file %s for date %s", SC::S2A(filename).c_str(), boost::posix_time::to_simple_string(*time_iterator).c_str());
        }
        
        write_data_to_file();
    }
}

int main(int argc, char *argv[]) {
    DaemonConfig *config = new DaemonConfig("wlkdmn");

    sz_log(2, "Starting the daemon");

    config->SetUsageHeader("WLK-files driver\n\nThis driver can be used as a SZARP line daemon or as an exporter of data from the WLK files to szbwriter-compatible format.\n\n\
      --export               Exports all the data found in the given directory to a szbwriter-compatible file\n\n\
When in export mode, the following options are available:\n\n\
      --file                 File name of the file to export to.\n\
      --from                 Beginning of period to export data from, given as YYYY-MM-DD.\n\
      --to                   End of period to export data from, given as YYYY-MM-DD.\n\n\
When in SZARP line daemon mode, the following options are available:\n");

    sz_log(2, "Parsing the command line");

    boost::program_options::options_description options_description("Export mode options");
    options_description.add_options()
        ("export", "Enter the export mode")
        ("file", boost::program_options::value<std::string>(), "File name of the file to export to")
        ("from", boost::program_options::value<std::string>(), "Beginning of period to export data from, given as YYYY-MM-DD")
        ("to", boost::program_options::value<std::string>(), "End of period to export data from, given as YYYY-MM-DD")
    ;

    boost::program_options::variables_map variables_map;
    boost::program_options::parsed_options parsed_options(&options_description);

    parsed_options = boost::program_options::command_line_parser(argc, argv).options(options_description).allow_unregistered().run();
    boost::program_options::store(parsed_options, variables_map);

    boost::program_options::notify(variables_map);

    std::vector<std::string> further_options = boost::program_options::collect_unrecognized(parsed_options.options, boost::program_options::include_positional);

    int modified_argc = further_options.size() + 1;
    char **modified_argv = new char *[further_options.size() + 1];

    modified_argv[0] = const_cast<char *>(std::string(argv[0]).c_str());

    for (size_t i = 0; i < further_options.size(); i++)
        modified_argv[i + 1] = const_cast<char *>(further_options.at(i).c_str());

    sz_log(2, "Loading configuration");

    if (config->Load(&modified_argc, modified_argv) != 0) {
        sz_log(1, "Error loading program configuration");

        delete config;
        delete[] modified_argv;

        return 1;
    }

    if (variables_map.count("export")) {
        sz_log(2, "Starting data export mode");

        sz_log(8, "Reading configuration parameters from the device tag");

        std::wstring import_data_directory;

        char *import_data_directory_value = (char *) xmlGetProp(config->GetXMLDevice(), BAD_CAST("path")); 

        if (import_data_directory_value == NULL) {
            sz_log(7, "Error reading directory path from device tag");
            import_data_directory = L"";
        } else {
            sz_log(9, "Read directory path %s from device tag", import_data_directory_value);
            import_data_directory = SC::A2S(import_data_directory_value);

            free(import_data_directory_value);
        }

        std::wstring output_file_name;

        if (variables_map.count("file"))
            output_file_name = variables_map["file"].as<std::wstring>();
        else
            output_file_name = L"";

        if (!boost::filesystem::exists(import_data_directory) || !boost::filesystem::is_directory(import_data_directory)) {
            std::wcerr << "Invalid directory given: " << import_data_directory << "\n";

            delete config;
            delete[] modified_argv;

            return 1;
        }

        boost::gregorian::date begin_date;

        if (variables_map.count("from")) {
            boost::wsmatch match;

            boost::regex_search(variables_map["from"].as<std::wstring>(), match, boost::wregex(L"(\\d{4})-(\\d{2})-(\\d{2})"));

            std::wstringstream export_date_from_stream;

            export_date_from_stream << match[1] << " " << match[2] << " " << match[3];

            int year_from;
            int month_from;
            int day_from;

            export_date_from_stream >> year_from;
            export_date_from_stream >> month_from;
            export_date_from_stream >> day_from;

            begin_date = boost::gregorian::date(year_from, month_from, day_from);
        } else
            begin_date = boost::gregorian::date();

        boost::gregorian::date end_date;

        if (variables_map.count("to")) {
            boost::wsmatch match;

            boost::regex_search(variables_map["to"].as<std::wstring>(), match, boost::wregex(L"(\\d{4})-(\\d{2})-(\\d{2})"));

            std::wstringstream export_date_to_stream;

            export_date_to_stream << match[1] << " " << match[2] << " " << match[3];

            int year_to;
            int month_to;
            int day_to;

            export_date_to_stream >> year_to;
            export_date_to_stream >> month_to;
            export_date_to_stream >> day_to;

            end_date = boost::gregorian::date(year_to, month_to, day_to) + boost::gregorian::days(1);
        } else
            end_date = boost::gregorian::date();

        sz_log(4, "Exporting data from WLK files in %s to %s", SC::S2A(import_data_directory).c_str(), SC::S2A(output_file_name).c_str());

#if BOOST_FILESYSTEM_VERSION == 3
        std::for_each(boost::filesystem::directory_iterator(import_data_directory), boost::filesystem::directory_iterator(), WLKExporter(output_file_name, begin_date, end_date, config->GetXMLDevice()));
#else
        std::for_each(boost::filesystem::wdirectory_iterator(import_data_directory), boost::filesystem::wdirectory_iterator(), WLKExporter(output_file_name, begin_date, end_date, config->GetXMLDevice()));
#endif
    } else {
        bool is_debug = config->GetSingle() || config->GetDiagno();

        int sleep_time = config->GetAskDelay();

        if (sleep_time <= 300)
            sleep_time = 300;

        sz_log(2, "Connecting to IPC...");
        IPCHandler *ipc = new IPCHandler(config);

        if (!(config->GetSingle()) && (ipc->Init() != 0)) {
            delete config;
            delete ipc;
            delete[] modified_argv;

            return 1;
        }

        if (config->GetSingle())
            sz_log(2, "Skipping the IPC connection in single mode.");
        else
            sz_log(2, "Connected.");

        if (is_debug)
            std::wcout << "WLK-reader daemon options:\n"
                       << "\tParcook line number: " << config->GetLineNumber()
                       << "\n\tDirectory to read from: " << config->GetDevice()->GetPath()
                       << "\n\tParameters to report: " << ipc->m_params_count << "\n";

        sz_log(7, "Entering the main loop");

        while (true) {
            std::wstringstream wlk_file_path;
            wlk_file_path << config->GetDevice()->GetPath() << "/";

            boost::gregorian::date_facet *facet(new boost::gregorian::date_facet("%Y-%m"));
            wlk_file_path.imbue(std::locale(wlk_file_path.getloc(), facet));

            wlk_file_path << boost::gregorian::day_clock::local_day() << ".wlk";

            std::ifstream wlk_file;

            sz_log(5, "Opening the data file %s", SC::S2A(wlk_file_path.str()).c_str());

            wlk_file.open(SC::S2A(wlk_file_path.str()).c_str(), std::ios::in | std::ios::binary);

            if (wlk_file.is_open()) {
                WLKReader wlk_reader(config->GetXMLDevice());

                if (wlk_reader.read_record(wlk_file, boost::posix_time::second_clock::local_time()))
                    wlk_reader.pass_last_read_record_to_parcook(config, ipc);
                else
                    for (int i = 0; i < config->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetParamsCount(); i++)
                        ipc->m_read[i] = SZARP_NO_DATA;
                
                ipc->GoParcook();
            } else
                sz_log(0, "No current data file found");

            sz_log(9, "Sleeping...");

            sleep(sleep_time);
        }

        delete ipc;
    }
    
    delete config;
    delete[] modified_argv;

    return 0;
}
#else // HAVE_BOOST_*
#include <iostream>
int main(int arc, char *argv[]) {
	std::cerr << "This daemon requires SZARP to be compiled with Boost support." << std::endl;
	return 1;
}
#endif
