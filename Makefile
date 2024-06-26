#!/usr/bin/make
# Makefile.  Generated from Makefile.in by configure.
NAME = briggs
PACKAGE="briggs"
VERSION="1.0.1-auto"
TARNAME="briggs"
DISTDIR="$(TARNAME)-$(VERSION)"
PREFIX="/usr/local"
EXEC_PREFIX="${prefix}"
BINDIR="${exec_prefix}/bin"
SRCDIR="."
VPATH="."
DOCFILE=/tmp/$(NAME).html
DISTDIR = $(NAME)-$(VERSION)
FILES = \
	Makefile \
	Makefile.in \
	man/man1/briggs.1 \
	src/main.c \
	tests.mk

# Add MySQL
MYSQL_IFLAGS=-Iinclude
MYSQL_LDFLAGS=-Llib
MYSQL_LIBFLAGS=-lmariadb

# Add Postgres
POSTGRES_IFLAGS=-Iinclude
POSTGRES_LDFLAGS=-Llib
POSTGRES_LIBFLAGS=-lpq

# Static libraries
PGLIBS=lib/libpq.a lib/libpgcommon.a lib/libpgport.a
MYLIBS=lib/libmariadbclient.a
#MYLIBS=

# Include flags (to handle outside dependencies)
#IFLAGS=$(MYSQL_IFLAGS) $(POSTGRES_IFLAGS)
IFLAGS=-Iinclude

# Control Postgres and other database engine support from here for now
FFLAGS=-DBPGSQL_H -DBMYSQL_H
#FFLAGS=-DBPGSQL_H

# Lib support flags
LDFLAGS=-Llib $(MYSQL_LIBFLAGS) $(POSTGRES_LIBFLAGS)


# -Wno-unused
CLANGFLAGS = -Wall -Werror -std=c99
GCCFLAGS = $(CLANGFLAGS) -Wno-unused -Wno-deprecated-declarations -O3 -Wno-pointer-arith -Wstrict-overflow -pedantic-errors
CFLAGS = $(GCCFLAGS)

#CFLAGS = $(CLANGFLAGS)
CC = gcc
OBJECTS = vendor/zwalker.o vendor/util.o


# Any local targets go here first...
# include local.mk

# main - Default build, suitable for most people
main: build
main: 
	@printf '' >/dev/null

#
debug: CFLAGS+=-DDEBUG_H
debug: build
debug:
	@printf '' >/dev/null

# main - Default build, suitable for most people
local: build-local
local: 
	@printf '' >/dev/null

#
local-debug: CFLAGS+=-DDEBUG_H
local-debug: build-local
local-debug:
	@printf '' >/dev/null

# clang - Default build using clang, suitable for most people
clang: CFLAGS=$(CLANGFLAGS)
clang: CC=clang 
clang: build
clang: 
	@printf '' >/dev/null

# dev - Development target, using clang and asan for bulletproof-ness
dev: CFLAGS=$(CLANGFLAGS) -g -fsanitize=address -fsanitize-undefined-trap-on-error -DDEBUG_H
dev: CC=clang 
dev: build
dev: 
	@printf '' >/dev/null

# build - Build dependent objects
# mariadb_config --include --libs
build: $(OBJECTS)
	$(CC) $(CFLAGS) $(FFLAGS) src/main.c -o $(NAME) $(OBJECTS) $(MYLIBS) $(IFLAGS) -lssl -lcrypto -lz -lm -lpthread -lpq -ldl -lgnutls

#
build-local: $(OBJECTS)
	$(CC) $(CFLAGS) $(FFLAGS) src/main.c -o $(NAME) $(OBJECTS) $(MYLIBS) $(PGLIBS) $(IFLAGS) -lz -lm -lpthread -ldl -lgnutls

# 
%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

# clean - Clean up everything here
clean:
	-rm -f *.o vendor/*.o $(NAME)

# install - Installs briggs to $PREFIX/bin
install:
	-mkdir -p $(PREFIX)/bin/  $(PREFIX)/share/man/man1/
	cp $(NAME) $(PREFIX)/bin/
	cp $(NAME).1 $(PREFIX)/share/man/man1/

# doctest - Creates HTML documentation
doctest:
	markdown -S README.md > $(DOCFILE)

# mantest - Creates man page style documentation
mantest:
	man -l briggs.1

# pkg - Create a package of the latest version of 'master'
pkg: TARGET=master
pkg:
	git archive --format tar $(TARGET) | gzip > $(NAME)-$(TARGET).tar.gz

# pkgdev - Create a package of the latest version of 'dev'
pkgdev:
	git archive --format tar dev | gzip > $(NAME)-dev.tar.gz

# list - list all the targets and what they do
list:
	@printf 'Available options are:\n'
	@sed -n '/^
# / { s/# //; 1d; p; }' Makefile | awk -F '-' '{ printf "  %-20s - %s\n", $$1, $$2 }'

# Create a package (in a different way)
dist: $(DISTDIR).tar.gz

# Create a package for Linux
dist-linux: $(DISTDIR).tar.gz

# Create a package for Windows 
#dist-win: $(DISTDIR).tar.gz

# Create a package for Mac OS
dist-osx: $(DISTDIR).tar.gz

# Create a package archive 
$(DISTDIR).tar.gz: $(DISTDIR)
	tar chof - $(DISTDIR) | gzip -9 -c > $@	
	rm -rf $(DISTDIR)

# Create a package directory
$(DISTDIR): clean
	rm -f $(DISTDIR).tar.gz
	rm -rf $(DISTDIR)
	mkdir -p \
		$(DISTDIR)/example \
		$(DISTDIR)/include \
		$(DISTDIR)/lib \
		$(DISTDIR)/vendor
	cp $(FILES) $(DISTDIR)/
	cp -Lr example/* $(DISTDIR)/example/
	cp -Lr include/* $(DISTDIR)/include/
	-cp lib/* $(DISTDIR)/lib/
	cp vendor/zwalker.* $(DISTDIR)/vendor/
	cp vendor/util.* $(DISTDIR)/vendor/

# Check that packaging worked (super useful for other distributions...) 
distcheck:
	gzip -cd $(DISTDIR).tar.gz | tar xvf -
	cd $(DISTDIR) && $(MAKE) local
	cd $(DISTDIR) && $(MAKE) clean
	rm -rf $(DISTDIR)
	@echo "*** package $(DISTDIR).tar.gz is ready for distribution."

# Regenerate the Makefile
Makefile: Makefile.in config.status
	./config.status $@

# Regenerate config.status
config.status: configure
	./config.status --recheck

# Include some test cases
include tests.mk

.PHONY: main clean debug dist distcheck install uninstall 
