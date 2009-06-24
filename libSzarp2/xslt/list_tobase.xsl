<?xml version="1.0" encoding="ISO-8859-2"?>
<!-- 
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
-->

<!--
	2003 Paweł Pałucha PRATERM S.A. pawel@praterm.com.pl

	$Id: list_tobase.xsl 4393 2007-09-13 16:37:10Z reksio $

	Ten szablon tworzy listę elementów 'param' z dokumentu, które 
	mają atrybut 'tobase', usuwa go a zamiast niego wstawia atrybut
	'base_ind'. Wartości tego atrybutu są nadawane kolejno, począwszy od
	wartości o 1 większej niż wartość przekazana jako parametr 'start' do
	szablonu (domyślnie -1, czyli indeksy od 0).

	Parametr:
		start - największy dotychczasowy indeks w bazie
 -->
 
<xsl:stylesheet version="1.0" 
	xmlns:ipk="http://www.praterm.com.pl/SZARP/ipk"
	xmlns="http://www.praterm.com.pl/SZARP/ipk"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<!-- Deklaracja parametru 'start' o domyślnej wartości '0' -->
	<xsl:param
		name="start"
		select="-1"/>
	
	<!-- 
		Tworzymy element główny i wybieramy wszystkie elementy 'param'
		z atrybutem 'tobase'.
	-->
	<xsl:template match="/">
		<xsl:copy>
			<main>
				<xsl:apply-templates
					select=".//ipk:param[@tobase]">
				</xsl:apply-templates>
			</main>
		</xsl:copy>
	</xsl:template>

	<!-- Elementom 'param' nadajemy odpowiedni atrybut 'base_ind' -->
	<xsl:template match="ipk:param">
		<xsl:copy>
			<xsl:attribute name="base_ind">
				<xsl:value-of select="position() + $start"/>
			</xsl:attribute>
            		<xsl:apply-templates select="@*"/>
		</xsl:copy>
	</xsl:template>

	<!-- Kasujemy atrybuty 'tobase' -->
	<xsl:template match="@tobase">
	</xsl:template>

	<!-- Kopiujemy resztę atrybutów -->
	<xsl:template match="@*">
		<xsl:copy/>
	</xsl:template>

	<!-- Inne elementy usuwamy -->
	<xsl:template match="node()|text()">
	</xsl:template>
	
</xsl:stylesheet>

