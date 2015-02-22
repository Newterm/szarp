#!/usr/bin/python

import sys
import signal
import gevent
from gevent.socket import create_connection, gethostbyname


def test_range():
    con = create_connection(('127.0.0.1', 8090))
    con.settimeout(5.0)

    con.send("RANGE\r\n")

    try:
        print con.recv(32)
    except:
        print "test_range exception"


def test_search(start, end, direction):
    print "test_search: s: %d, e: %d, dir: %d" % (start, end, direction)
    con = create_connection(('127.0.0.1', 8090))
    con.settimeout(5.0)

    con.send("SEARCH %d %d %d FP_3031/Suma/Suma_produkcji_ciepla_DN300\r\n" % (start, end, direction))

    print con.recv(64)

def test_bad_get_1():
    print "test_bad_get_1"
    con = create_connection(('127.0.0.1', 8090))
    con.settimeout(5.0)
    start = 1419876920
    end = 1419875920

    con.send("GET %d %d FP_3031/Suma/Suma_produkcji_ciepla_DN300\r\n" % (start, end))

    print con.recv(64)


def test_get():
    con = create_connection(('127.0.0.1', 8090))
    con.settimeout(5.0)
    start = 1419875920
    end = 1419876920

    con.send("GET %d %d FP_3031/Suma/Suma_produkcji_ciepla_DN300\r\n" % (start, end))

    print con.recv(64)


def test_search_0():
    test_search(1419875920, -1, 0)

def test_search_1():
    test_search(1419875920, -1, 1)

greenlets = [gevent.spawn(test_range) for i in xrange(0, 20)]

greenlets = [ gevent.spawn(test_search_0) for i in xrange(0, 15) ]
greenlets = [ gevent.spawn(test_search_1) for i in xrange(0, 30) ]

#greenlets = [ gevent.spawn(test_get) for i in xrange(0, 4) ]

#greenlets = [ gevent.spawn(test_bad_get_1) for i in xrange(0, 2) ]

gevent.joinall(greenlets)

#test_search()

