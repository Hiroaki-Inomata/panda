# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2008  ogochan
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA
## Process this file with automake to produce Makefile.in

bin_PROGRAMS = recdefgen copygen dbgen checkdir monupload monsql monsetup monbatch_ps monpushevent $(dbsyncbin)

sbin_PROGRAMS = monitor
noinst_SCRIPTS = copy

recdefgen_SOURCES = \
	recdefgen.c
recdefgen_CFLAGS = \
	$(RECDEFGEN_CFLAGS)	\
	$(COMMON_CFLAGS)
recdefgen_LDADD =		\
	$(RECDEFGEN_LIBS)	\
	$(COMMON_LIBS)

copygen_SOURCES		=	\
	copygen.c

copygen_LDFLAGS	=			\
	-export-dynamic

copygen_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

dbgen_LDFLAGS	=			\
	-export-dynamic
dbgen_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

checkdir_LDFLAGS	=			\
	-export-dynamic
checkdir_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

monupload_LDFLAGS	=			\
	-export-dynamic
monupload_LDADD 		=				\
	-lm									\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

monpushevent_LDFLAGS	=		\
	-export-dynamic
monpushevent_LDADD 		=				\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(top_builddir)/dblib/libpandadb.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

monsql_SOURCES = \
	monsql.c
monsql_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la
monsql_LDFLAGS	=			\
	-export-dynamic
monsql_LDADD 		=					\
	-lm									\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

monsetup_SOURCES = \
	monsetup.c
monsetup_DEPENDENCIES	=				\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la
monsetup_LDFLAGS	=			\
	-export-dynamic
monsetup_LDADD 		=					\
	-lm									\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

monbatch_ps_SOURCES = \
	monbatch_ps.c
monbatch_ps_DEPENDENCIES	=			\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la
monbatch_ps_LDFLAGS	=			\
	-export-dynamic
monbatch_ps_LDADD 		=				\
	-lm									\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

monitor_LDFLAGS	=						\
	-export-dynamic
monitor_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

AM_CPPFLAGS 			= 				\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/include				\
	-I$(top_srcdir)/libs				\
	-I$(top_srcdir)/dblib				\
	$(P11_CFLAGS)						\
	$(SSL_CFLAGS)						\
	$(COMMON_CFLAGS)					\
	$(POSTGRES_CFLAGS)					\
	-DBIN_DIR=\"$(bindir)\"				\
	-DSERVER_DIR=\"$(sbindir)\"

monitor_DEPENDENCIES	=				\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la


dbgen_SOURCES		=	\
	dbgen.c

checkdir_SOURCES	=	\
	checkdir.c

monupload_SOURCES	=	\
	monupload.c

monitor_SOURCES		=	\
	monitor.c

monpushevent_SOURCES	=	\
	monpushevent.c

EXTRA_DIST =				\
	copy
