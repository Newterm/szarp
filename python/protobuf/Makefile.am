
# $Id: Makefile.am 6789 2009-05-29 12:03:33Z pawel $

SUBDIRS = 

paramsvalues_pb2.py:
	$(PROTOC) --python_out=. -I@srcdir@/../../libSzarp2/protobuf @srcdir@/../../libSzarp2/protobuf/paramsvalues.proto

paramsvaluesdir = @prefix@/lib/python
paramsvalues_PYTHON = paramsvalues_pb2.py
