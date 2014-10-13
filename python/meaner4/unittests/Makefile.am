SUBDIRS = 

test:
	@(cd @abs_srcdir@; PYTHONPATH="$(abs_builddir)/../../protobuf:../" nosetests --pdb --pdb-failures)

.PHONY: test
