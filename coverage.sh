#!/bin/bash

LDFLAGS='-lgcov --coverage' CXXFLAGS='-ggdb -O0 -fprofile-arcs -ftest-coverage' ./configure
make

cd unit_tests; ./unit_tests

mkdir -p /home/$HOSTNAME/public_html
gcovr --html --html-details -o /home/$HOSTNAME/public_html/cov.html -r .
xdg-open /home/$HOSTNAME/public_html/cov.html
