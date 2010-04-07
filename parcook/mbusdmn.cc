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
 * Daemon do komunikacji z urz±dzeniami w protokole M-Bus.
 * Oparty o uniwersaln± klasêdo komunikacji w protokole M-Bus.
 *
 * Autor: Marcin Goliszewski
 *
 * $Id$
 */
/*
 @description_start
 @class 3
 @protocol M-Bus
 @devices All devices using M-Bus protocol.
 @devices.pl Dowolne urz±dzenia korzystaj±ce z protoko³u M-Bus.

 @config.pl
            Demona s³u¿±cego do komunikacji z urz±dzeniami w protokole
	    M-Bus konfigurujemy, podobnie jak ka¿dego innego daemon
            wchodz±cego w sk³ad systemu SZARP, poprzez plik params.xml.
            W tym celu tworzymy now± sekcjê <emphasis>device</emphasis>
            podaj±c w niej odpowiedni± ¶cie¿kê do Damona oraz, jako
            ¶cie¿kê do urz±dzenia, ¶cie¿kê do konwertera M-Bus &lt;-&gt; RS232,
            na przyk³ad:
            <programlisting>
<![CDATA[
<device daemon="/opt/szarp/bin/mbusdmn" path="/dev/ttyS0">
]]>
            </programlisting>
            Polecane jest równie¿ skonfigurowanie odpowiedniej prêdko¶ci transmisji poprzez podanie atrybutu
            <emphasis>speed</emphasis>, np.:
            <programlisting>
<![CDATA[
<device ... speed="300" ... >
]]>
            </programlisting>
            Prêdko¶æ 300 bodów obs³ugiwana powinna byæ przez wszystkie
            urz±dzenia w protokole M-Bus, wy¿sze prêdko¶ci nale¿y
            wykorzystywaæ tylko wtedy, kiedy mamy pewno¶æ, ¿e wszystkie
            urz±dzenia pod³±czone do naszej szyny potrafi± j± obs³u¿yæ.
            Mo¿na równie¿ ustaliæ odstêp miêdzy kolejnymi odczytami
            danych z urz±dzenia - nale¿y go podaæ w sekundach jako
            warto¶æ parametru <emphasis>askdelay</emphasis>.

            Wszystkie pozosta³e atrybuty konfiguracyjne dotycz±ce tego 
            demona powinny byæ umieszczone w przestrzeni nazw
            <emphasis>http://www.praterm.com.pl/SZARP/ipk-extra</emphasis>.

            Do najwa¿niejszych parametrów, które mo¿na skonfigurowaæ w
            elemencie <emphasis>device</emphasis> nale¿±:
            <itemizedlist>
                <listitem>
                    <para>
                        <emphasis>byte_interval</emphasis> - czas oczekiwania miêdzy odczytaniem kolejnych bajtów;
                        warto¶æ ta jest zazwyczaj dobierana eksperymentalnie, indywidualnie dla ka¿dego
                        urz±dzenia; domy¶lna warto¶æ to 10 000 mikrosekund
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>databits</emphasis> - ilo¶æ bitów danych w ramce RS232; domy¶lna warto¶æ to 8
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>stopbits</emphasis> - ilo¶æ bitów stopu w ramce RS232; domy¶lna warto¶æ to 1
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>parity</emphasis> - typ parzysto¶ci wykorzystywany w ramce RS232; dozwolone
                        warto¶ci to: "even" (parzysto¶æ parzysta), "odd" (parzysto¶æ nieparzysta), 
			"none" (brak bitu parzysto¶ci), domy¶lna warto¶æ to "none"
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>precision</emphasis> - ilo¶æ miejsc po przecinku, które maj± byæ brane 
			pod uwagê przy konwersji liczby zmiennoprzecinkowej (je¶li taka zostanie 
			odebrana od urz±dzenia) na liczbê sta³oprzecinkow±; domy¶lna warto¶æ to 4
                    </para>
                </listitem>
            </itemizedlist>

            Poza tymi atrybutami wystêpuj± tak¿e istotne atrybuty
            konfiguracyjne w elemencie <emphasis>unit</emphasis>, które
            pozwalaj± skonfigurowaæ parametry komunikacji dla
            poszczególnych urz±dzeñ (w przeciwieñstwie do parametrów z
            elementu <emphasis>device</emphasis>, które kontroluj±
            globalnie transmisjê w ca³ej szynie M-Bus). Oto one:
            <itemizedlist>
                <listitem>
                    <para>
                        <emphasis>address</emphasis> - adres na szynie M-Bus urz±dzenia, z którym chcemy 
			nawi±zywaæ komunikacjê; domy¶lny adres to 254, czyli specjalna warto¶æ oznaczaj±ca
		       	wys³anie zapytania do wszystkich urz±dzeñ znajduj±cych siê na szynie - powinna ona 
			byæ u¿ywana tylko w trakcie rozpoznawania konfiguracji nowo pod³±czonego urz±dzenia, 
			pó¼niej nale¿y j± zamieniæ na adres, z którym zg³asza siê urz±dzenie
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>reset</emphasis> - umo¿liwia wys³anie do urz±dzenia po nawi±zaniu po³±czenia 
			¿±dania wykonania resetu (pomocne przy urz±dzeniach, których konfiguracja zosta³a 
			zmieniona w  nieznany sposób); mo¿liwe warto¶ci tego  parametru to:
                        <itemizedlist>
                            <listitem>
                                <para>
                                    <emphasis>no</emphasis> - warto¶æ domy¶lna, równoznaczna z nieumieszczeniem 
				    tego atrybutu w konfiguracji; oznacza brak jakiegokolwiek resetu
      				    </para>				    
		            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>full</emphasis> - wykonuje pe³ny reset aplikacji w urz±dzeniu,
                                    tzn. zeruje wszystkie ustawienia, liczniki, daty itp.
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>setup</emphasis> - wykonuje reset ustawieñ w urz±dzeniu, tzn. 
				    resetuje ustawienia transmisji, adres na szynie M-Bus oraz ustawione daty
                                </para>
                            </listitem>
                        </itemizedlist>
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>select_data</emphasis> - dokonuje wyboru danych do odczytu z urz±dzenia; w
                        protokole M-Bus mo¿liwy jest do¶æ rozbudowany wybór danych do odczytu, jednak daemon 
			obs³uguje jedynie nastêpuj±ce warto¶ci parametru:
                        <itemizedlist>
                            <listitem>
                                <para>
                                    <emphasis>no</emphasis> - warto¶æ domy¶lna, równoznaczna z
                                    nieumieszczeniem tego atrybutu w konfiguracji; oznacza brak wyboru
                                    danych do odczytu, a wiêc odczyt danych domy¶lnie przesy³anych przez
                                    urz±dzenie
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>all</emphasis> - wybiera do odczytu wszystkie dane
                                    zgromadzone w urz±dzeniu
                                </para>
                            </listitem>
                        </itemizedlist>
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>change_address</emphasis> - adres urz±dzenia w protokole M-Bus, który ma ono
                        przyj±æ po zmianie adresu; domy¶lna warto¶æ to 0  oznaczaj±ca niewykonywanie procedury zmiany
                        adresu
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>reinitialize_on_error</emphasis> - opcja ta wymusza wykonanie ponownej
                        inicjalizacji z urz±dzeniem w razie stwierdzenia b³êdów w transmisji, co jest przydatne w
                        przypadku urz±dzeñ, z którymi komunikacja okresowo ulega zawieszeniu; dozwolone warto¶ci
                        to "yes" oraz "no"
                    </para>
                </listitem>
            </itemizedlist>

            Oprócz tego w ka¿dym elemencie <emphasis>param</emphasis>
            mo¿na skonfigurowaæ specjalne operacje (transformacje), które 
            maj± byæ wykonywane na poszczególnych warto¶ciach przesy³anych 
            do systemu SZARP. Mo¿liwe atrybuty to:
            <itemizedlist>
                <listitem>
                    <para>
                        <emphasis>multiplier</emphasis> - liczba przez któr± nale¿y pomno¿yæ otrzyman± warto¶æ
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>divisor</emphasis> - liczba przez któr± nale¿y podzieliæ otrzyman± warto¶æ
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>modulo</emphasis> - liczba modulo któr± nale¿y wzi±æ dan± warto¶æ
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>transform</emphasis> - bardziej skomplikowana operacja, któr± nale¿y wykonaæ na
                        parametrze; dostêpne operacje to:
                        <itemizedlist>
                            <listitem>
                                <para>
                                    <emphasis>lw</emphasis> - pobranie z parametru 15 najmniej znacz±cych bitów
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>mw</emphasis> - pobranie z parametru bitów 16-30
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>hw</emphasis> - pobranie z parametru bitów 31-32.
                                </para>
                            </listitem>
                        </itemizedlist>
                    </para>
                </listitem>
                <listitem>
                    <para>
                        <emphasis>special</emphasis> - oznacza specjaln± operacjê, któr± nale¿y wykonaæ, aby uzyskaæ
                        warto¶æ danego parametru; dostêpne operacje to:
                        <itemizedlist>
                            <listitem>
                                <para>
                                    <emphasis>prev</emphasis> - warto¶æ tego parametru zostanie wziêta jako
                                    warto¶æ poprzedniego parametru odczytanego od urz±dzenia
                                </para>
                            </listitem>
                        </itemizedlist>
                    </para>
                </listitem>
            </itemizedlist>

            Przyk³adowa konfiguracja w params.xml dla ciep³omierza Landis&amp;Gyr
            WSD-6 z modu³em komunikacyjnym WZD-MB:

	    @config_example
    <device xmlns:mbus="http://www.praterm.com.pl/SZARP/ipk-extra" daemon="/opt/szarp/bin/mbusdmn" 
        path="/dev/ttyS2" speed="300" mbus:byte_interval="50000" mbus:databits="8" mbus:stopbits="0" 
        mbus:parity="even" options="--askdelay 30240">
      <unit id="1" type="1" subtype="1" bufsize="1" mbus:address="0">
        <param name="Landis Gyr:Wêze³:czas pracy LSW" short_name="OnL" draw_name="-" unit="-" prec="0" 
            mbus:transform="lw">
                <raport title="Landis Gyr" order="25"/>
        </param>
        <param name="Landis Gyr:Wêze³:czas pracy MSW" short_name="OnM" draw_name="-" unit="-" prec="0" 
            mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="26"/>
        </param>
        <param name="Landis Gyr:Wêze³:aktualny przep³yw z licznika 1 LSW" short_name="Gwy1L" 
            draw_name="-" unit="-" prec="4" base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="1"/>
        </param>
        <param name="Landis Gyr:Wêze³:aktualny przep³yw z licznika 1 MSW" short_name="Gwy1M" 
            draw_name="-" unit="-" prec="4" base_ind="auto" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="2"/>
        </param>
        <param name="Landis Gyr:Wêze³:temperatura powrotna" short_name="Tpow" draw_name="Temp. powrotu" 
            unit="°C" prec="0" base_ind="auto">
            <raport title="Landis Gyr" order="8"/>
            <draw title="Landis Gyr" order="2" min="0" max="150" />
        </param>
        <param name="Landis Gyr:Wêze³:time point 1" short_name="-" draw_name="-" unit="-" prec="0"/>
        <param name="Landis Gyr:Wêze³:objêto¶æ LSW" short_name="VL" draw_name="-" unit="m3" prec="0" 
            base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="15"/>
        </param>
        <param name="Landis Gyr:Wêze³:objêto¶æ MSW" short_name="VM" draw_name="-" unit="m3" prec="0" 
            base_ind="auto" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="16"/>
        </param>
        <param name="Landis Gyr:Wêze³:numer fabryczny LSW" short_name="NoL" draw_name="-" unit="-" 
            prec="0" mbus:transform="lw">
                <raport title="Landis Gyr" order="29"/>
        </param>
        <param name="Landis Gyr:Wêze³:numer fabryczny MSW" short_name="NoM" draw_name="-" unit="-" 
            prec="0" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="30"/>
        </param>
        <param name="Landis Gyr:Wêze³:temperatura wyj¶ciowa" short_name="Twy" 
            draw_name="Temp. wyj¶ciowa" unit="°C" prec="0" base_ind="auto">
            <raport title="Landis Gyr" order="7"/>
            <draw title="Landis Gyr" prior="11" order="1" min="0" max="150"/>
        </param>
        <param name="Landis Gyr:Wêze³:moc wyj¶ciowa z licznika 2" short_name="Qwy2" 
            draw_name="Moc z licz. 2" unit="kW" prec="0" base_ind="auto">
            <raport title="Landis Gyr" order="11"/>
            <draw title="Landis Gyr" order="8" min="0" max="2000" />
        </param>
        <param name="Landis Gyr:Wêze³:czas pracy b³êdnej LSW" short_name="ErL" draw_name="-" unit="h" 
            prec="0" mbus:transform="lw">
                <raport title="Landis Gyr" order="27"/>
        </param>
        <param name="Landis Gyr:Wêze³:czas pracy b³êdnej MSW" short_name="ErM" draw_name="-" unit="h" 
            prec="0" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="28"/>
        </param>
        <param name="Landis Gyr:Wêze³:moc wyj¶ciowa z licznika 1 LSW" short_name="Q1L" draw_name="-" 
            unit="kW" prec="0" base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="9"/>
        </param>
        <param name="Landis Gyr:Wêze³:moc wyj¶ciowa z licznika 1 MSW" short_name="Q1M" draw_name="-" 
            unit="kW" prec="0" base_ind="auto" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="10"/>
        </param>
        <param name="Landis Gyr:Wêze³:aktualny przep³yw z licznika 2" short_name="Gwy2" draw_name="-" 
            unit="m3/h" prec="1" base_ind="auto">
            <raport title="Landis Gyr" order="3"/>
        </param>
        <param name="Landis Gyr:Wêze³:maksymalna moc wyj¶ciowa" short_name="Qmax" 
            draw_name="Moc maksymalna" unit="kW" prec="0" base_ind="auto">
            <raport title="Landis Gyr" order="12"/>
        </param>
        <param name="Landis Gyr:Wêze³:data maksimum mocy wyj¶ciowej LSW" short_name="QmxDL" 
            draw_name="-" unit="-" prec="0" base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="13"/>
        </param>
        <param name="Landis Gyr:Wêze³:data maksimum mocy wyj¶ciowej MSW" short_name="QmxDM" draw_name="-" 
            unit="-" prec="0" base_ind="auto" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="14"/>
        </param>
        <param name="Landis Gyr:Wêze³:energia z licznika 2 LSW" short_name="E2L" draw_name="-" unit="-" 
            prec="0" base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="23"/>
        </param>
        <param name="Landis Gyr:Wêze³:energia z licznika 2 MSW" short_name="E2M" draw_name="-" unit="-" 
            prec="0" base_ind="auto" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="24"/>
        </param>
        <param name="Landis Gyr:Wêze³:energia sumaryczna z licznika 1 LSW" short_name="Ec1L" draw_name="-" 
            unit="-" prec="0" base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="21"/>
        </param>
        <param name="Landis Gyr:Wêze³:energia sumaryczna z licznika 1 MSW" short_name="Ec1M" draw_name="-" 
            unit="-" prec="0" base_ind="auto" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="22"/>
        </param>
        <param name="Landis Gyr:Wêze³:energia z licznika 1 w taryfie 1 LSW" short_name="E1t1L" draw_name="-" 
            unit="-" prec="0" base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="17"/>
        </param>
        <param name="Landis Gyr:Wêze³:energia z licznika 1 w taryfie 1 MSW" short_name="E1t1M" draw_name="-" 
            unit="-" prec="0" base_ind="auto" mbus:transform="lw" mbus:special="prev">
                <raport title="Landis Gyr" order="18"/>
        </param>
        <param name="Landis Gyr:Wêze³:energia z licznika 1 w taryfie 2 LSW" short_name="E1t2L" draw_name="-" 
            unit="-" prec="0" base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="19"/>
        </param>
        <param name="Landis Gyr:Wêze³:energia z licznika 1 w taryfie 2 MSW" short_name="E1t2M" draw_name="-" 
            unit="-" prec="0" base_ind="auto" mbus:transform="lw" mbus:special="prev">
                <raport title="Landis Gyr" order="20"/>
        </param>
        <param name="Landis Gyr:Wêze³:data maksimum przep³ywu LSW" short_name="GmxDL" draw_name="-" unit="-" 
            prec="0" base_ind="auto" mbus:transform="lw">
                <raport title="Landis Gyr" order="5"/>
        </param>
        <param name="Landis Gyr:Wêze³:data maksimum przep³ywu MSW" short_name="GmxDM" draw_name="-" unit="-" 
            prec="0" base_ind="auto" mbus:transform="mw" mbus:special="prev">
                <raport title="Landis Gyr" order="6"/>
        </param>
        <param name="Landis Gyr:Wêze³:maksymalny przep³yw" short_name="Gmax" draw_name="Przep³yw maks." 
            unit="m3/h" prec="1" base_ind="auto">
            <raport title="Landis Gyr" order="4"/>
        </param>
    </unit>
  </device>
  ....

  <drawdefinable>
  ...
    <param name="Landis Gyr:Wêze³:czas pracy" short_name="OnTime" draw_name="-" unit="-" prec="0">
        <define type="DRAWDEFINABLE" formula="(*:*:czas pracy MSW) (*:*:czas pracy LSW) :"/>
    </param>
    <param name="Landis Gyr:Wêze³:aktualny przep³yw z licznika 1" short_name="Gwy1" draw_name="Przep³yw" 
        unit="m3/h" prec="3" base_ind="auto">
        <define type="DRAWDEFINABLE" 
            formula="(*:*:aktualny przep³yw z licznika 1 MSW) (*:*:aktualny przep³yw z licznika 1 LSW) :"/>
        <draw title="Landis Gyr" order="9" min="0" max="100" />
    </param>
    <param name="Landis Gyr:Wêze³:objêto¶æ" short_name="V" draw_name="Objêto¶æ" unit="m3" prec="2" 
        base_ind="auto">
        <define type="DRAWDEFINABLE" formula="(*:*:objêto¶æ MSW) (*:*:objêto¶æ LSW) :"/>
        <draw title="Landis Gyr" order="10" min="0" max="100000" />
    </param>
    <param name="Landis Gyr:Wêze³:numer fabryczny" short_name="No" draw_name="-" unit="-" prec="0">
        <define type="DRAWDEFINABLE" formula="(*:*:numer fabryczny MSW) (*:*:numer fabryczny LSW) :"/>
    </param>
    <param name="Landis Gyr:Wêze³:czas pracy b³êdnej" short_name="ErTime" draw_name="-" unit="h" prec="0">
        <define type="DRAWDEFINABLE" formula="(*:*:czas pracy b³êdnej MSW) (*:*:czas pracy b³êdnej LSW) :"/>
    </param>
    <param name="Landis Gyr:Wêze³:moc wyj¶ciowa z licznika 1" short_name="Qwy1" draw_name="Moc z licz. 1" 
        unit="kW" prec="2" base_ind="auto">
        <define type="DRAWDEFINABLE" 
            formula="(*:*:moc wyj¶ciowa z licznika 1 MSW) (*:*:moc wyj¶ciowa z licznika 1 LSW) :"/>
        <draw title="Landis Gyr" order="7" min="0" max="2000"/>
    </param>
    <param name="Landis Gyr:Wêze³:data maksimum mocy wyj¶ciowej" short_name="QmaxD" draw_name="-" unit="-" 
        prec="0" base_ind="auto">
        <define type="DRAWDEFINABLE" 
            formula="(*:*:data maksimum mocy wyj¶ciowej MSW) (*:*:data maksimum mocy wyj¶ciowej LSW) :"/>
    </param>
    <param name="Landis Gyr:Wêze³:energia z licznika 2" short_name="E2" draw_name="En. z licz. 2" unit="kWh" 
        prec="0" base_ind="auto">
        <define type="DRAWDEFINABLE" formula="(*:*:energia z licznika 2 MSW) (*:*:energia z licznika 2 LSW) :"/>
        <draw title="Landis Gyr" order="6" max="10000000" min="0"/>
    </param>
    <param name="Landis Gyr:Wêze³:energia sumaryczna z licznika 1" short_name="Ec1" draw_name="En. c. z licz. 1" 
        unit="kWh" prec="0" base_ind="auto">
        <define type="DRAWDEFINABLE" 
            formula="(*:*:energia sumaryczna z licznika 1 MSW) (*:*:energia sumaryczna z licznika 1 LSW) :"/>
        <draw title="Landis Gyr" order="5" min="0" max="10000000"/>
    </param>
    <param name="Landis Gyr:Wêze³:energia z licznika 1 w taryfie 1" short_name="E1t1" draw_name="En. z licz. 1 w t. 1" 
        unit="kWh" prec="0" base_ind="auto">
        <define type="DRAWDEFINABLE" 
            formula="(*:*:energia z licznika 1 w taryfie 1 MSW) (*:*:energia z licznika 1 w taryfie 1 LSW) :"/>
            <draw title="Landis Gyr" order="3" min="0" max="10000000"/>
    </param>
    <param name="Landis Gyr:Wêze³:energia z licznika 1 w taryfie 2" short_name="E1t2" draw_name="En z licz. 1 w t. 2" 
        unit="kWh" prec="0" base_ind="auto">
        <define type="DRAWDEFINABLE" 
            formula="(*:*:energia z licznika 1 w taryfie 2 MSW) (*:*:energia z licznika 1 w taryfie 2 LSW) :"/>
        <draw title="Landis Gyr" order="4" min="0" max="10000000"/>
    </param>
    <param name="Landis Gyr:Wêze³:data maksimum przep³ywu" short_name="GmaxD" draw_name="-" unit="-" prec="0" 
        base_ind="auto">
        <define type="DRAWDEFINABLE" formula="(*:*:data maksimum przep³ywu MSW) (*:*:data maksimum przep³ywu LSW) :"/>
    </param>
...
</drawdefinable>

    @comment.pl
        Badanie konfiguracji nowo pod³±czanego urz±dzenia

            Jednym z g³ównych powodów stworzenia uniwersalnego demona
            do komunikacji w protokole M-Bus by³o u³atwienie
            konfiguracji i pod³±czenia do systemu SZARP nowego
            urz±dzenia wykorzystuj±cego ten protokó³. W zwi±zku z
            powy¿szym, demona mo¿na wykorzystywaæ jako narzêdzie
            s³u¿±ce do badania konfiguracji nowo pod³±czonego urz±dzenia.
            W tym celu nale¿y zastosowaæ nastêpuj±c± procedurê:
            <orderedlist>
                <listitem>
                    <para>
                        Nale¿y uruchomiæ demona w trybie pracy s³u¿±cym
                        do testowania komunikacji z urz±dzeniami M-Bus
                        komend±:
                        <programlisting>
<![CDATA[
/opt/szarp/bin/mbusdmn --test --device <adres_urz±dzenia> [pozosta³e parametry]
]]>
                        </programlisting>
                        Parametry linii komend, które przyjmuje
                        <emphasis>mbusdmn</emphasis> w trybie testowym
                        maj± na celu odwzorowanie mo¿liwo¶ci
                        konfiguracyjne, które daj± atrybuty elementu
                        <emphasis>device</emphasis> z IPK. Dostêpne s±
                        nastêpuj±ce parametry:
                        <itemizedlist>
                            <listitem>
                                <para>
                                    <emphasis>device</emphasis> -
                                    parametr obowi±zkowy oznaczaj±cy
                                    nazwê urz±dzenia, do którego
                                    pod³±czony jest konwerter M-Bus
                                    &lt;-&gt; RS232
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>speed</emphasis> -
                                    prêdko¶æ transmisji; warto¶æ
                                    domy¶lna: 300 bodów
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>address</emphasis> - adres
                                    protoko³u M-Bus urz±dzenia, które
                                    nale¿y odpytaæ; warto¶æ domy¶lna:
                                    254, co oznacza odpytanie wszystkich
                                    urz±dzeñ, które s³uchaj± na szynie
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>byte_interval</emphasis> - czas
                                    oczekiwania miêdzy odczytaniem kolejnych bajtów;
                                    warto¶æ ta jest zazwyczaj dobierana
                                    eksperymentalnie, indywidualnie dla ka¿dego
                                    urz±dzenia; domy¶lna warto¶æ to 10 000
                                    mikrosekund
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>data_bits</emphasis> - ilo¶æ bitów
                                    danych w ramce RS232; domy¶lna warto¶æ to 8
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>stop_bits</emphasis> - ilo¶æ bitów
                                    stopu w ramce RS232; domy¶lna warto¶æ to 1
                                </para>
                            </listitem>
                            <listitem>
                                <para>
                                    <emphasis>parity</emphasis> - typ parzysto¶ci
                                    wykorzystywany w ramce RS232; dozwolone
                                    warto¶ci to: "even" (parzysto¶æ parzysta), "odd"
                                    (parzysto¶æ nieparzysta), "none" (brak bitu
                                    parzysto¶ci), domy¶lna warto¶æ to "none"
                                </para>
                            </listitem>
                        </itemizedlist>

                        W tym trybie pracy daemon powinien wypisaæ na ekran
                        wiele informacji, które pomog± stworzyæ plik
                        konfiguracyjny dla danego urz±dzenia.
                        Przyk³adowo fragment z danych wypisanych przez
                        <emphasis>mbusdmn</emphasis> w trybie testowania
                        po³±czenia dla ciep³omierza Pollustat E
                        (pokazano tu jedynie fragment, który dotyczy
                        ramki danych w formacie M-Bus przysy³anej przez
                        ciep³omierz, pominiêto fragmenty dotycz±ce samej
                        komunikacji w protokole M-Bus):
                        <programlisting>
<![CDATA[
        Address: 0
 
        Frame contains data with LSB first
        RSP_UD frame parsed as a variable data structure frame
        Serial number: 61560223
        Manufacturer ID: SPX
        Version number: 96
        Medium: Heat (at return temperature)
        Access number: 10
        Decoding status bits:
                Application status bits: 0
                Manufacturer status bits: 0
        Signature: 0
 
        Data:
                Function: instantaneous value
                Data type: 8 digit BCD
                Storage number: 0
                Value and unit information:
                Value information: Energy [J], * 1 000 000
        Value: 870592
 
        Data:
                Function: instantaneous value
                Data type: 8 digit BCD
                Storage number: 0
                Value and unit information:
                Value information: Volume [m3], * 0.001
        Value: 5802265
 
        Data:
                Function: instantaneous value
                Data type: 8 digit BCD
                Storage number: 0
                Value and unit information:
                Value information: Volume Flow [m3/h], * 0.001
        Value: 181
 
        Data:
                Function: instantaneous value
                Data type: 8 digit BCD
                Storage number: 0
                Value and unit information:
                Value information: Power [W], * 1
        Value: 1114
 
        Data:
                Function: instantaneous value
                Data type: 4 digit BCD
                Storage number: 0
                Value and unit information:
                Value information: Flow Temperature [°C], * 0.1
        Value: 580
 
        Data:
                Function: instantaneous value
                Data type: 4 digit BCD
                Storage number: 0
                Value and unit information:
                Value information: Return Temperature [°C], * 0.1
        Value: 526
 
        Data:
                Function: instantaneous value
                Data type: 6 digit BCD
                Storage number: 0
                Value and unit information:
                Value information: Temperature Difference [K], * 0.001
        Value: 5360
 
        Data:
                Function: instantaneous value
                Data type: 8 digit BCD
                Storage number: 0
                Value and unit information:
                Value information: Fabrication Number
        Value: 61560223
 
        Data:
                Function: instantaneous value
                Data type: 8 digit BCD
                Storage number: 0
                Value and unit information:
                Information from the extended VIF table:
                Customer location
        Value: 61560223
 
        Data:
                Function: maximum value
                Data type: special: manufacturer specific data follows, more records in next telegram
                Manufacturer specific data:
]]>
                        </programlisting>

                        Jak widaæ na powy¿szym przyk³adzie, mo¿na w ten
                        sposób uzyskaæ wiele interesuj±cych informacji o
                        danych przesy³anych przez urz±dzenie. Pierwsz±
                        wa¿n± informacj± jest adres urz±dzenia - nale¿y
                        go wykorzystaæ jako warto¶æ atrybutu
                        <emphasis>address</emphasis> w elemencie
                        <emphasis>unit</emphasis> konfiguracji.
                        Nastêpnie dowiadujemy siê ró¿nych informacji
                        ogólnych o danym urz±dzeniu - kod producenta,
                        numer seryjny, numer wersji, medium mierzone
                        przez dane urz±dzenie itp. Nastêpnie podane s±
                        informacje o ka¿dej z przesy³anych warto¶ci: typ
                        warto¶ci (chwilowa, maksymalna, minimalna,
                        b³êdna), sposób jej zakodowania w danych
                        przesy³anych w protokole M-Bus (z tej informacji
                        mo¿na prosto wywnioskowaæ zakres warto¶ci, a
                        tak¿e transformacje, które nale¿y na niej
                        zastosowaæ, aby by³a ona jak najbardziej
                        u¿yteczna w systemie SZARP), kolejny numer
                        warto¶ci danego parametru w pamiêci urz±dzenia
                        (im mniejszy numer, tym nowsza warto¶æ),
                        znaczenie danej warto¶ci (tj. nazwa wielko¶ci,
                        któr± ona wyra¿a) jednostka, w jakiej jest
                        ona wyra¿ona itp. Czasem mog± równie¿ wystêpowaæ
                        informacje dodatkowe, np. o taryfie, w której
                        dana wielko¶æ by³a mierzona. Na koñcu widaæ, ¿e
                        dane specyficzne dla producenta danego
                        urz±dzenia nie s± przetwarzane (mog± jedynie
                        zostaæ wypisane w postaci ci±gu liczb
                        szesnastkowych odpowiadaj±cych warto¶ciom
                        kolejnych bajtów odebranych od urz±dzenia).

                        Je¶li jednak po uruchomieniu demona w trybie
                        testowym nie uzyskamy informacji na temat warto¶ci
                        przesy³anych przez urz±dzenie (pojawi± siê
                        informacje o b³êdach transmisji lub innego
                        rodzaju komunikaty o b³êdzie), nale¿y próbowaæ
                        eksperymentalnie dobraæ warto¶ci jego parametrów
                        tak, aby uda³o siê nawi±zaæ po³±czenie z urz±dzeniem.
                        Poleca siê w pierwszej kolejno¶ci modyfikowaæ
                        prêdko¶æ transmisji oraz odstêp miêdzy kolejnymi
                        bajtami, nastêpnie ilo¶æ bitów danych, ilo¶æ
                        bitów stopu, typ parzysto¶ci itp.
                    </para>
                </listitem>
                <listitem>
                    <para>
                        Na podstawie tak uzyskanych informacji nale¿y
                        stworzyæ w pliku params.xml odpowiedni element 
                        <emphasis>device</emphasis>, przypisaæ dobrane
                        podczas testowania warto¶ci jego atrybutom,
                        stworzyæ element <emphasis>unit</emphasis> z
                        ¿±danymi atrybutami oraz utworzyæ odpowiednie
                        elementy <emphasis>param</emphasis> dla ka¿dej z
                        warto¶ci przesy³anych przez urz±dzenie.
                        Przyk³adowy plik konfiguracyjny dla ciep³omierza
                        Pollustat E obs³uguj±cy warto¶ci podane powy¿ej
                        wygl±da nastêpuj±co:
                        <programlisting>
<![CDATA[
  <device daemon="/opt/szarp/bin/mbusdmn" speed="2400" path="/dev/ttyA22" 
      xmlns:mbus="http://www.praterm.com.pl/SZARP/ipk-extra" mbus:bittime="5000" mbus:databist="8" 
      mbus:stopbits="2" mbus:parity="even">
    <unit id="1" type="1" subtype="1" bufsize="1" mbus:address="0">
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Energia LSW" short_name="Elsw" draw_name="Energia lsw" 
            unit="-" prec="3" mbus:transform="lsw" base_ind="auto">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="1"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Energia MSW" short_name="Emsw" draw_name="Energia msw" 
          unit="-" prec="3" mbus:transform="msw" base_ind="auto" mbus:special="prev">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="2"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Objêto¶æ LSW" short_name="Vlsw" draw_name="-" unit="-" 
          prec="3" mbus:transform="lsw">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="3"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Objêto¶æ MSW" short_name="Vmsw" draw_name="-" unit="-" 
          prec="3" mbus:transform="msw" mbus:special="prev">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="4"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Przep³yw" short_name="Pp" draw_name="Przep³yw" unit="m3/h" 
          prec="3" base_ind="auto">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="5"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Moc" short_name="PQ" draw_name="Moc" unit="kW" prec="2" 
          base_ind="auto" mbus:divisor="10">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="9"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Temperatura wej¶ciowa" short_name="Ptz" 
          draw_name="Temp. zasilania" unit="°C" prec="1" base_ind="auto">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="6"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Temperatura wyj¶ciowa" short_name="Ptp" 
          draw_name="Temp. powrotu" unit="°C" prec="1" base_ind="auto">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="7"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Ró¿nica temperatur" short_name="PdT" 
          draw_name="Ró¿n. temperatur" unit="°C" prec="2" base_ind="auto" mbus:divisor="10">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="8"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Numer fabryczny LSW" short_name="Nlsw" draw_name="-" unit="-" 
          prec="3" mbus:transform="lsw">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="10"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:Numer fabryczny MSW" short_name="Nmsw" draw_name="-" unit="-" 
          prec="3" mbus:transform="msw" mbus:special="prev">
        <raport title="Pollustat Obiegi wewnêtrzne M-Bus" filename="pollustat-mbus.rap" order="11"/>
      </param>
      <param name="Pollustat M-Bus:Obiegi wewnêtrzne:dummy" short_name="-" draw_name="-" unit="-" prec="0" />
    </unit>
  </device>
]]>
                        </programlisting>
                    </para>
                </listitem>
            </orderedlist>
            Po wykonaniu powy¿szych kroków daemon powinien byæ gotowy do
            wspó³pracy z nowo pod³±czonym urz±dzeniem oraz z systemem
            SZARP.
 @description_end
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ipchandler.h"
#include "liblog.h"

#include <iostream>
#include <string>
#include <map>

#include <getopt.h>

#include <libxml/tree.h>

#include "mbus.h"
#include "conversion.h"

/** Holds all the configuration information for the M-Bus protocol
 * transmission and serves as a wrapper for the M-Bus communication
 * class.
 * This is the main utility class of the M-Bus communication daemon. It
 * hold all the information about the M-Bus connection, the daemon
 * configuration, the values transformations etc. It also serves as a
 * wrapper for the M-Bus communication class - it holds a pointer to an
 * object of the MBus class which should be used to perform all the
 * communication.
 * @see MBus
 */
class MBusConfig {
    public:
        /** Class which should be used as an ancestors of all the
         * transform functors' classes.
         */
        class TransformFunctor {
            public:
                /** Performs the transform itself.
                 * @param param a reference to the parameter to be changed 
                 * @param param_num number of the parameter
                 */
                virtual void operator()(MBus::value_t& param, unsigned int param_num) = 0;

                /** No destructor is really required but a class with
                 * virtual methods has to have virtual destructor.
                 */
                virtual ~TransformFunctor() { }
        };

        /** Functor to convert the value to it's least significant 16 
         * bits.
         */
        class LSWFunctor : public TransformFunctor {
            public:
                /** Performs the transform itself.
                 * @see TransformFunctor::operator()
                 * @param param a reference to the parameter to be changed 
                 * @param param_num number of the parameter
                 */
                void operator()(MBus::value_t& param, unsigned int param_num) {
                    param = param & 0x0000FFFF;
                }
        };

        /** Functor to convert the value to it's most significant 16
         * bits.
         */
        class MSWFunctor : public TransformFunctor {
            public:
                /** Performs the transform itself.
                 * @see TransformFunctor::operator()
                 * @param param a reference to the parameter to be changed 
                 * @param param_num number of the parameter
                 */
                void operator()(MBus::value_t& param, unsigned int param_num) {
                    param = (param & 0xFFFF0000) >> 16;
                }
        };

        /** Pointer to an object of the MBus class used to perform all 
         * the communication. 
         * @see MBus 
         */
        MBus *mbus;

        unsigned long int byte_interval; /**< Used when connecting to the other device. @see MBus::connect */
        unsigned int data_bits; /**< Used when connecting to the other device. @see MBus::connect */
        unsigned int stop_bits; /**< Used when connecting to the other device. @see MBus::connect */
        MBus::parity_type parity; /**< Used when connecting to the other device. @see MBus::connect */
        std::map<MBus::byte, int> units; /**< Maps a M-Bus address of the device to it's parameters number */
        std::map<MBus::byte, std::vector<int> > multipliers; /**< Multipliers for each value present in IPK */
        std::map<MBus::byte, std::vector<int> > divisors; /**< Divisors for each value present in IPK */
        std::map<MBus::byte, std::vector<int> > modulos; /**< Numbers to perform a modulo operation for each value present in IPK (value of -1 means that no modulo operation should be performed) */
        std::map<MBus::byte, std::vector<bool> > prevs; /**< A flag for each value present in IPK indicating if this value should be acquired from the previous M-Bus value */
        std::map<MBus::byte, std::vector<TransformFunctor*> > transforms; /**< Transforms to perform on each value present in IPK (at most one per value) */
        std::map<MBus::byte, bool> reset; /**< Flag indicating if a reset on the other device should be performed. @see MBus::reset_application */
        std::map<MBus::byte, MBus::byte> reset_type; /**< Type of reset to be performed. @see MBus::reset_application */
        std::map<MBus::byte, bool> select_data; /**< Flag indicating if a data selection procedure should be performed on the other device. @see MBus::select_data_for_readout */
        std::map<MBus::byte, MBus::byte> select_data_type; /**< Type of data selection procedure to preform. @see MBus::select_data_for_readout */
        std::map<MBus::byte, MBus::byte> change_address; /**< Address to use when changing the other device's primary address. If this is 0, no address change will be performed. @see MBus::change_slave_address */
        std::map<MBus::byte, bool> reinitialize_on_error; /**< Flag indicating if the device should be reseted when communication error occurs */
        unsigned short int precision; /**< Number of decimal places to be used when performing a conversion from a real number to an integer. @see MBus::MBus(unsigned int) */

        /** Initializes the configuration.
         * Parses the XML configuration and fills all the
         * needed class fields with appropriate values.
         * @see parse_xml()
         * @param node the XML <device> node of the configuration
         */
        MBusConfig(xmlNodePtr node) : device_node(node) {
            // Associate string values used in IPK with appropriate
            // functors
            transform_strings["lsw"] = new LSWFunctor();
            transform_strings["msw"] = new MSWFunctor();

            parse_xml();
        }

        /** Deletes all the dynamically allocated memory (i.e. functors
         * and the MBus class object).
         */
        ~MBusConfig() {
            for (std::map<std::string, TransformFunctor*>::iterator i = transform_strings.begin(); i != transform_strings.end(); i++)
                delete (*i).second;

            delete mbus;
        }

        /** Return number of values in IPK which are not associated with
         * any real M-Bus value but are taken from a previous M-Bus
         * value.
         * @param address M-Bus address of the device to count the
         * values for
         * @return number of parameters not really present in a M-Bus
         * frame
         */
        inline int prevs_count(MBus::byte address) {
            if (units.find(address) == units.end())
                return 0;

            int ret = 0;

            for (unsigned int i = 0; i < prevs[address].size(); i++) {
                if (prevs[address].at(i))
                    ret++;
            }

            return ret;
        }

    private:
        xmlNodePtr device_node; /**< The XML <device> node of the IPK */
        std::map<std::string, TransformFunctor*> transform_strings; /**< Mapping of transform string used in IPK to pointers to functor objects */

        /** Parses the XML configuration from IPK and sets class field
         * accordingly.
         */
        void parse_xml();

        /** Parses (with some basic error checking of the XML) the XML 
         * <device> node from IPK  and sets class field accordingly.
         * @param device_node the XML node of the <device> element to
         * parse
         */
        void parse_xml_device_node(xmlNodePtr device_node);

        /** Parses (with some basic error checking of the XML) all the 
         * XML <param> nodes from IPK (children of the first <unit> node) 
         * and modifies class fields accordingly.
         * @param unit_node the XML node of the <unit> element to parse
         */
        void parse_xml_unit_node(xmlNodePtr unit_node);

        /** Counts the <param> elements in the given <unit> element.
         * @param unit_node parent node of the <param> elements to count
         * @return number of <param> nodes
         */
        inline unsigned int count_xml_param_nodes(xmlNodePtr unit_node) {
            int ret = 0;

            for (xmlNodePtr node = unit_node->children; node; node = node->next)
                if (std::string((char *) node->name) == "param") {
                    ret++;
                }

            return ret;
        }

        /** Initializes some member variables for the given M-Bus
         * device.
         * @param address M-Bus address of the device to initialize
         * tables for
         */
        inline void init_tables(MBus::byte address) {
            if (units.find(address) == units.end())
                return;

            multipliers[address].assign(units[address], 1);
            divisors[address].assign(units[address], 1);
            modulos[address].assign(units[address], -1);
            prevs[address].assign(units[address], false);
            transforms[address].assign(units[address], static_cast<TransformFunctor *>(NULL));
        }
};

void MBusConfig::parse_xml() {
    parse_xml_device_node(device_node);

    for (xmlNodePtr node = device_node->children; node != NULL; node = node->next) { // Find the <unit> nodes and pass each of them to the method parsing <unit> nodes
        if (node->ns != NULL
            && std::string((char *) node->ns->href) == SC::S2A(IPK_NAMESPACE_STRING)
            && std::string((char *) node->name) == "unit") {
            parse_xml_unit_node(node);
        }
    }
}

void MBusConfig::parse_xml_device_node(xmlNodePtr device_node) {
    sz_log(8, "Parsing the configuration device element.");

    char *value = (char *) xmlGetNsProp(device_node, BAD_CAST("precision"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:precision not found in device element, line %ld, using the default value 4", xmlGetLineNo(device_node));
        precision = 4;
    } else {
       long int precision_value = strtol(value, NULL, 10);

       if (precision_value == LONG_MIN) {
            sz_log(4, "Attribute mbus:precision unparseable in device element, line %ld, using the default value 4", xmlGetLineNo(device_node));
            precision = 4;
       } else
            precision = precision_value;

        free(value);
    }

    mbus = new MBus(precision); // An object of the MBus class can be created only when we know the value of the precision parameter

    value = (char *) xmlGetNsProp(device_node, BAD_CAST("byte_interval"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:byte_interval not found in device element, line %ld, using the default value 10000", xmlGetLineNo(device_node));
        byte_interval = 10000;
    } else {
       long int byte_interval_value = strtol(value, NULL, 10);

       if (byte_interval_value == LONG_MIN) {
            sz_log(4, "Attribute mbus:byte_interval unparseable in device element, line %ld, using the default value 10000", xmlGetLineNo(device_node));
            byte_interval = 10000;
       } else
           byte_interval = byte_interval_value;
    
       free(value);
    }

    value = (char *) xmlGetNsProp(device_node, BAD_CAST("databits"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:databits not found in device element, line %ld, using the default value 8", xmlGetLineNo(device_node));
        data_bits = 8;
    } else {
       long int data_bits_value = strtol(value, NULL, 10);

       if (data_bits_value == LONG_MIN) {
            sz_log(4, "Attribute mbus:databits unparseable in device element, line %ld, using the default value 8", xmlGetLineNo(device_node));
            data_bits = 8;
       } else
           data_bits = data_bits_value;

       free(value);
    }

    value = (char *) xmlGetNsProp(device_node, BAD_CAST("stopbits"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:stopbits not found in device element, line %ld, using the default value 1", xmlGetLineNo(device_node));
        stop_bits = 1;
    } else {
       long int stop_bits_value = strtol(value, NULL, 10);

       if (stop_bits_value == LONG_MIN) {
            sz_log(4, "Attribute mbus:stopbits unparseable in device element, line %ld, using the default value 1", xmlGetLineNo(device_node));
            stop_bits = 8;
       } else
           stop_bits = stop_bits_value;

       free(value);
    }

    value = (char *) xmlGetNsProp(device_node, BAD_CAST("parity"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:parity not found in device element, line %ld, using the default value none", xmlGetLineNo(device_node));
        parity = MBus::none;
    } else {
        std::string string_value(value);

        if (string_value == "none")
            parity = MBus::none;
        else if (string_value == "even")
            parity = MBus::even;
        else if (string_value == "odd")
            parity = MBus::odd;
        else {
            sz_log(4, "Attribute mbus:parity unparseable in device element, line %ld, using the default value none", xmlGetLineNo(device_node));
            parity = MBus::none;
        }

        free(value);
    }
}

void MBusConfig::parse_xml_unit_node(xmlNodePtr unit_node) {
    sz_log(8, "Parsing the configuration param elements.");

    char *value = (char *) xmlGetNsProp(unit_node, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    MBus::byte address = -1;

    if (value == NULL) {
        sz_log(3, "Attribute mbus:address not found in unit element, line %ld, %d will be used (broadcast with reply)", xmlGetLineNo(unit_node), MBus::broadcast_with_reply);
        address = MBus::broadcast_with_reply;
    } else {
        long int int_value = strtol(value, NULL, 10);

        if (int_value == LONG_MIN) {
            sz_log(3, "Attribute mbus:address unparseable in unit element, line %ld, %d will be used (broadcast with reply)", xmlGetLineNo(unit_node), MBus::broadcast_with_reply);
            address = MBus::broadcast_with_reply;
        } else
            address = int_value;
    }

    units[address] = count_xml_param_nodes(unit_node);
    init_tables(address);

    value = (char *) xmlGetNsProp(unit_node, BAD_CAST("reset"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:reset not found in unit element, line %ld, using the default value no", xmlGetLineNo(unit_node));
        reset[address] = false;
    } else {
        std::string string_value(value);

        if (string_value == "no" || string_value == "false" || string_value == "0")
            reset[address] = false;
        else if (string_value == "full") {
            reset[address] = true;
            reset_type[address] = mbus->full_application_reset;    
        } else if (string_value == "setup") {
            reset[address] = true;
            reset_type[address] = mbus->setup_application_reset;    
        } else {
            sz_log(4, "Attribute mbus:reset unparseable in unit element, line %ld, using the default value no", xmlGetLineNo(unit_node));
            reset[address] = false;
        }

        free(value);
    }

    value = (char *) xmlGetNsProp(unit_node, BAD_CAST("reinitialize_on_error"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:reinitialize_on_error not found in unit element, line %ld, using the default value no", xmlGetLineNo(unit_node));
        reinitialize_on_error[address] = false;
    } else {
        std::string string_value(value);

        if (string_value == "no" || string_value == "false" || string_value == "0")
            reinitialize_on_error[address] = false;
        else if (string_value == "yes" || string_value == "true" || string_value == "1") {
            reinitialize_on_error[address] = true;
        } else {
            sz_log(4, "Attribute mbus:reset unparseable in unit element, line %ld, using the default value no", xmlGetLineNo(unit_node));
            reinitialize_on_error[address] = false;
        }

        free(value);
    }

    value = (char *) xmlGetNsProp(unit_node, BAD_CAST("select_data"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:select_data not found in unit element, line %ld, using the default value no", xmlGetLineNo(unit_node));
        select_data[address] = false;
    } else {
        std::string string_value(value);

        if (string_value == "no" || string_value == "false" || string_value == "0")
            select_data[address] = false;
        else if (string_value == "all") {
            select_data[address] = true;
            select_data_type[address] = mbus->dif_global_readout;
        } else {
            sz_log(4, "Attribute mbus:select_data unparseable in unit element, line %ld, using the default value no", xmlGetLineNo(unit_node));
            select_data[address] = false;
        }

        free(value);
    }

    value = (char *) xmlGetNsProp(unit_node, BAD_CAST("change_address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

    if (value == NULL) {
        sz_log(4, "Attribute mbus:change_address not found in unit element, line %ld, using the default value 0 (no address change)", xmlGetLineNo(unit_node));
        change_address[address] = 0;
    } else {
       long int change_address_value = strtol(value, NULL, 10);

       if (change_address_value == LONG_MIN) {
            sz_log(4, "Attribute mbus:change_address unparseable in unit element, line %ld, using the default value 0 (no address change)", xmlGetLineNo(unit_node));
            change_address[address] = 0;
       } else
           change_address[address] = change_address_value;
        free(value);
    }

    int param_num = 0;

    for (xmlNodePtr node = unit_node->children; node != NULL; node = node->next) {
        if (std::string((char *) node->name) != "param")
            continue;

        char *value = (char *) xmlGetNsProp(node, BAD_CAST("multiplier"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

        if (value != NULL) {
           long int multiplier_value = strtol(value, NULL, 10);

           if (multiplier_value == LONG_MIN) {
                sz_log(6, "Attribute mbus:multiplier unparseable in param element, line %ld", xmlGetLineNo(node));
           } else
               multipliers[address].at(param_num) = multiplier_value;
        
           free(value);
        }

        value = (char *) xmlGetNsProp(node, BAD_CAST("divisor"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

        if (value != NULL) {
           long int divisor_value = strtol(value, NULL, 10);

           if (divisor_value == LONG_MIN) {
                sz_log(6, "Attribute mbus:divisor unparseable in param element, line %ld", xmlGetLineNo(node));
           } else
               divisors[address].at(param_num) = divisor_value;
        
           free(value);
        }

        value = (char *) xmlGetNsProp(node, BAD_CAST("modulo"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

        if (value != NULL) {
           long int modulo_value = strtol(value, NULL, 10);

           if (modulo_value == LONG_MIN) {
                sz_log(6, "Attribute mbus:modulo unparseable in param element, line %ld", xmlGetLineNo(node));
           } else
               modulos[address].at(param_num) = modulo_value;
        
           free(value);
        }

        value = (char *) xmlGetNsProp(node, BAD_CAST("special"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

        if (value != NULL) {
            std::string string_value(value);

            if (string_value == "prev") {
                prevs[address].at(param_num) = true;
            } else
                sz_log(6, "Attribute mbus:special unparseable in param element, line %ld", xmlGetLineNo(node));

           free(value);
        }

        value = (char *) xmlGetNsProp(node, BAD_CAST("transform"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

        if (value != NULL) {
            std::string string_value(value);

            if (transform_strings.find(string_value) != transform_strings.end()) {
                transforms[address].at(param_num) = transform_strings[string_value];
            } else
                sz_log(6, "Attribute mbus:transform unparseable in param element, line %ld", xmlGetLineNo(node));
        
           free(value);
        }

        param_num++;
    }
}

int main(int argc, char *argv[]) {
    DaemonConfig *config = new DaemonConfig("mbusdmn");

    sz_log(2, "Starting the daemon.");

    config->SetUsageHeader("MBus Driver\n\nThis driver can be used as a SZARP line daemon or as a MBus\n\
communication testing application.\n\n\
      --test                 Activate communication testing mode; when this\n\
                             switch is omitted, the daemon works as a SZARP \n\
                             line daemon. When in testing mode, the device\n\
                             should be omitted.\n\n\
When in testing mode, the following options are available:\n\n\
      --device               The device to connect to\n\
      --speed                Transmission speed to use\n\
      --address              The MBus address of the device to connect to\n\
      --byte_interval        Interval between reading consequentive bytes\n\
      --data_bits            Number of data bits in the transmission\n\
      --stop_bits            Number of stop bits in the transmission\n\
      --parity               Parity type; allowed values are: even, odd, none\n\n\
When in SZARP line daemon mode, the following options are available:\n");

    bool testing_mode = false;

    // Check if any of the command-line parameters is "--test" - if so,
    // we're in testing mode.
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--test")
            testing_mode = true;
    }

    if (testing_mode) { // MBus communication testing mode
        sz_log(2, "Starting communication testing mode...");

        struct option options[] = {{"device", 1, NULL, 0},
                                   {"speed", 1, NULL, 0},
                                   {"address", 1, NULL, 0},
                                   {"byte_interval", 1, NULL, 0},
                                   {"data_bits", 1, NULL, 0},
                                   {"stop_bits", 1, NULL, 0},
                                   {"parity", 1, NULL, 0},
                                   {"test", 0, NULL, 0},
                                   {0, 0, 0, 0}};

        int option_index = 0;
        std::wstring device;
        unsigned long int speed = 300;
        MBus::byte address = MBus::broadcast_with_reply;
        unsigned long int byte_interval = 10000;
        unsigned int data_bits = 8;
        unsigned int stop_bits = 1;
        MBus::parity_type parity = MBus::none;

        while (true) {
            int ret = getopt_long(argc, argv, "", options, &option_index);

            if (ret == -1)
                break;

            long int argval = 0;
            std::string stringval = "";

            switch (option_index) {
                case 0:
                    device = SC::A2S(optarg);
                    break;
                case 1:
                    argval = strtol(optarg, NULL, 10);

                    if (argval != LONG_MIN)
                        speed = argval;

                    break;
                case 2:
                    argval = strtol(optarg, NULL, 10);

                    if (argval != LONG_MIN)
                        address = argval;

                    break;
                case 3:
                    argval = strtol(optarg, NULL, 10);

                    if (argval != LONG_MIN)
                        byte_interval = argval;

                    break;
                case 4:
                    argval = strtol(optarg, NULL, 10);

                    if (argval != LONG_MIN)
                        data_bits = argval;

                    break;
                case 5:
                    argval = strtol(optarg, NULL, 10);

                    if (argval != LONG_MIN)
                        stop_bits = argval;

                    break;
                case 6:
                    stringval = std::string(optarg);

                    if (stringval == "none")
                        parity = MBus::none;
                    else if (stringval == "odd")
                        parity = MBus::odd;
                    else if (stringval == "even")
                        parity = MBus::even;

                    break;
            }
        }

        MBus mbus;

        std::wcout << "Making a test attempt to connect to " << device << " with the following parameters:\n"
                  << "MBus address: " << static_cast<unsigned int>(address) << "\n"
                  << "Speed: " << speed << "\n"
                  << "Byte interval: " << byte_interval << "\n"
                  << "Data bits: " << data_bits << "\n"
                  << "Stop bits: " << stop_bits << "\n";

        switch (parity) {
            case MBus::none:
                std::cout << "Parity: none\n";
                break;
            case MBus::even:
                std::cout << "Parity: even\n";
                break;
            case MBus::odd:
                std::cout << "Parity: odd\n";
                break;
        }

        // Testing mode operation consists of one-time connection to the
        // device, querying it for data and dumping results to std::cout
        std::cout << "\nConnecting to the device... ";
        if (mbus.connect(device, speed, byte_interval, data_bits, stop_bits, parity)) {
            std::cout << "connected.\n";

            std::cout << "Initializing communication...\n";
            if (mbus.initialize_communication(address, true, std::cout)) {
                std::cout << "Querying device for values...\n";
                mbus.query_for_data(address, true, true, std::cout);
            }
        } else 
            std::cout << "failed.\n";

    } else { // SZARP line daemon mode
        sz_log(2, "Loading configuration...");
        if (config->Load(&argc, argv) != 0)
            return 1;

        bool is_debug = config->GetSingle() || config->GetDiagno();

        int sleep_time = config->GetAskDelay();

        if (sleep_time <= 0) {
            if (is_debug)
                sleep_time = 10;
            else
                sleep_time = 540;
        }

        sz_log(2, "Connecting to IPC...");
        IPCHandler *ipc = new IPCHandler(config);

        if (!(config->GetSingle()) && (ipc->Init() != 0))
            return 1;

        if (config->GetSingle())
            sz_log(2, "Skipping the IPC connection in single mode.");
        else
            sz_log(2, "Connected.");

        if (is_debug)
            std::wcout << std::dec << "MBus daemon connection data:\n" 
                      << "\tParcook line number: " << config->GetLineNumber() << "\n\tDevice: " << config->GetDevice()->GetPath()
                      << "\n\tParameters to report: " << ipc->m_params_count << "\n";

        MBusConfig mbus_config(config->GetXMLDevice());

        sz_log(5, "Waiting 30 seconds for parcoock to start...");
        sleep(30);

        sz_log(2, "Starting the main loop.");

        sz_log(5, "Connecting to the device...");

        long int connection_speed = config->GetDevice()->GetSpeed();

        if (connection_speed <= 0)
            connection_speed = 300;

        if (is_debug)
            std::wcout << std::dec << "Connecting to the device " << config->GetDevice()->GetPath() 
                      << " with baudrate " << connection_speed << "...\n";

        // Connect only when the connection died, not every time when we
        // want to retrieve data from the device
        if (mbus_config.mbus->connect(config->GetDevice()->GetPath(),
                         connection_speed, mbus_config.byte_interval, mbus_config.data_bits, 
                         mbus_config.stop_bits, mbus_config.parity)) {
            sz_log(5, "Connected.");

            // All the initialization procedures should be performed only
            // after connection, not every time we want to retrieve data
            // from the device
            for (std::map<MBus::byte, int>::iterator i = mbus_config.units.begin(); i != mbus_config.units.end(); i++) {
                sz_log(5, "Initializing communication with device %d...", static_cast<unsigned int>(i->first));

                if (mbus_config.mbus->initialize_communication(i->first, is_debug, std::cout)) {
                    sz_log(5, "Done.");
                } else {
                    sz_log(5, "Failed.");

                    continue;
                }

                if (mbus_config.reset[i->first]) {
                    sz_log(8, "Resetting the device %d...", static_cast<unsigned int>(i->first));

                    if (mbus_config.mbus->reset_application(i->first, mbus_config.reset_type[i->first], is_debug, std::cout)) {
                        sz_log(8, "Done.");
                    } else {
                        sz_log(8, "Failed.");
                    }
                }

                if (mbus_config.select_data[i->first]) {
                    sz_log(8, "Selecting data for readout...");

                    std::vector<MBus::byte> data;
                    data.push_back(mbus_config.select_data_type[i->first]);

                    std::vector<MBus::value_t> values = mbus_config.mbus->select_data_for_readout(i->first, data, is_debug, std::cout);
                    if (!values.empty()) {
                        sz_log(8, "Done.");
                    } else {
                        sz_log(8, "Failed.");
                    }
                }

                // Querying for status is performed only after connecting, not
                // with every data query
                if (is_debug) {
                    sz_log(10, "Querying device for status...");

                    std::string frame;

                    if ((frame = mbus_config.mbus->query_for_status(i->first, is_debug, std::cout)) == "") {
                        sz_log(10, "Failed.");
                    } else {
                        sz_log(10, "Done.");
                    }
                }

                // Changing the address has to be the last action in
                // this loop as it destroys the element pointed by the
                // iterator
                if (mbus_config.change_address[i->first] > 0) {
                    sz_log(8, "Changing the device %d address to %d...", static_cast<unsigned int>(i->first), mbus_config.change_address[i->first]);

                    if (mbus_config.mbus->change_slave_address(i->first, mbus_config.change_address[i->first], is_debug, std::cout)) {
                        sz_log(8, "Done.");

                        mbus_config.units[mbus_config.change_address[i->first]] = mbus_config.units[i->first];
                        mbus_config.units.erase(i->first);
                    } else {
                        sz_log(8, "Failed.");
                    }
                }

            }

            sz_log(2, "Starting the parameters gathering loop.");

            while (true) {
                int parcook_index = 0;
                bool reinitialization_done = false;

                sz_log(3, "Gathering data.");

                for (std::map<MBus::byte, int>::iterator i = mbus_config.units.begin(); i != mbus_config.units.end(); i++) {
                    sz_log(7, "Querying device %d for values...", static_cast<unsigned int>(i->first));

                    std::vector<MBus::value_t> values = mbus_config.mbus->query_for_data(i->first, is_debug, config->GetDumpHex(), std::cout);

                    if (!values.empty()) { // The device should return some values, empty vector means an error condition has occurred
                        sz_log(7, "Done.");

                        int values_num = static_cast<int>(values.size()) + mbus_config.prevs_count(i->first); // Number of values is really the number of values received from the device plus the number of parameters which can have their value computed from a previous value

                        if (is_debug)
                            std::cout << "\nReceived " << std::dec << values_num << " value" << (values_num > 1 ? "s" : "") << " of " << i->second << " expected.\n";

                        if (values_num == i->second) {
                            int prev_count = 0;

                            for (int j = 0; j < i->second; j++) {
                                if (mbus_config.prevs[i->first].at(j))
                                    prev_count++;

                                int mbus_param_num = j - prev_count;

                                if (is_debug)
                                    std::cout << "Value no. " << std::dec << (j + 1) << " is M-Bus parameter no. " << (mbus_param_num + 1) << ".\n";

                                MBus::value_t value_to_send = values.at(mbus_param_num);
                        
                                if (mbus_config.transforms[i->first].at(j) != NULL)
                                    (*mbus_config.transforms[i->first].at(j))(value_to_send, j);

                                if (mbus_config.modulos[i->first].at(j) != -1)
                                    value_to_send %= mbus_config.modulos[i->first].at(j);

                                value_to_send /= mbus_config.divisors[i->first].at(j);
                                value_to_send *= mbus_config.multipliers[i->first].at(j);

                                ipc->m_read[parcook_index++] = value_to_send;
                            }
                        } else { // By convention, we return only NO_DATA when the number of received parameters doesn't exactly match the number of parameters in IPK
                            sz_log(7, "Wrong number of parameters received: expected %d, got %d.", i->second, values_num);

                            for (int j = 0; j < i->second; j++)
                                ipc->m_read[parcook_index++] = SZARP_NO_DATA;
                        }
                    } else {
                        sz_log(7, "Failed.");

                        if (mbus_config.reinitialize_on_error[i->first]) {
                            sz_log(5, "Reinitializing communication with device %d...", static_cast<unsigned int>(i->first));

                            if (mbus_config.mbus->initialize_communication(i->first, is_debug, std::cout)) {
                                sz_log(5, "Done.");

                                reinitialization_done = true;
                            } else
                                sz_log(5, "Failed.");
                        } 

                        if (!reinitialization_done) {
                            for (int j = 0; j < i->second; j++)
                                ipc->m_read[parcook_index++] = SZARP_NO_DATA;
                        }

                        break;
                    }
                }

                sz_log(5, "Passing collected data to parcook.");
                ipc->GoParcook();

                if (!reinitialization_done) {
                    sz_log(3, "Sleeping...");
                    sleep(sleep_time);
                }
            }

            // Dummy code, never executed
            mbus_config.mbus->disconnect();
            
        } else
            sz_log(0, "Connection failed, terminating.");

        delete ipc;
    }

    delete config;

    return 0;
}
