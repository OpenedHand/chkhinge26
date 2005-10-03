bin_PROGRAMS=chkhinge26

chkhinge26_SOURCES = chkhinge26.c input.h

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing

snapshot:
        $(MAKE) dist distdir=$(PACKAGE)-snap`date +"%Y%m%d"`

