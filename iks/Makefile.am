if MINGW32_OPT
EX_SUBDIRS = 
else
EX_SUBDIRS = server
endif

SUBDIRS = common client $(EX_SUBDIRS)

iks_server_ini_sample: DATABASE_PREFIX
	$(INSTALL) -m 644 iks-server.ini.sample @prefix@/`cat @srcdir@/DATABASE_PREFIX`/iks/iks-server.ini.sample

# Additional files for deleting on 'make maintainer-clean'

