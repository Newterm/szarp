#!/bin/bash

# $Id$

# Ten skrypt zamienia polskie znaczki (podane na standardowym
# wej¶ciu) entities TeX-owe. Wynik zwracany jest na standardowe 
# wyj¶cie.
# Vooyeck 2002-07

	sed -e s/±/'\\Entity{aogon}'/g -e s/¡/'\\Entity{Aogon}'/g  \
	-e s/æ/'\\Entity{cacute}'/g -e s/Æ/'\\Entity{Cacute}'/g \
	-e s/ê/'\\Entity{eogon}'/g -e s/Ê/'\\Entity{Eogon}'/g \
	-e s/³/'\\Entity{lstrok}'/g -e s/£/'\\Entity{Lstrok}'/g \
	-e s/ñ/'\\Entity{nacute}'/g -e s/Ñ/'\\Entity{Nacute}'/g \
	-e s/¶/'\\Entity{sacute}'/g -e s/¦/'\\Entity{Sacute}'/g \
	-e s/¼/'\\Entity{zacute}'/g -e s/¬/'\\Entity{Zacute}'/g \
	-e s/¿/'\\Entity{zdot}'/g -e s/¯/'\\Entity{Zdot}'/g 

