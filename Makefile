#!/usr/bin/make
NAME = briggs
PREFIX=/usr/local
DOCFILE=/tmp/$(NAME).html


# Add MySQL
MYSQL_IFLAGS=-I/opt/mariadb/include/mysql/
MYSQL_LDFLAGS=-L/opt/mariadb/lib/ -lmariadb

# Add Postgres
POSTGRES_IFLAGS=-I/opt/postgres/include
POSTGRES_LDFLAGS=-L/opt/postgres/lib/ -lpq


# Include flags (to handle outside dependencies)
IFLAGS=$(MYSQL_IFLAGS) $(POSTGRES_IFLAGS)


# Lib support flags
LDFLAGS=$(MYSQL_LDFLAGS) $(POSTGRES_LDFLAGS)


#
CLANGFLAGS = -g -Wall -Werror -std=c99
GCCFLAGS = -g -Wall -Werror -Wno-unused -std=c99 -Wno-deprecated-declarations -O3 -Wno-pointer-arith -Wstrict-overflow -pedantic-errors 
CFLAGS = $(GCCFLAGS)

#CFLAGS = $(CLANGFLAGS)
CC = gcc
OBJECTS = vendor/zwalker.o vendor/util.o


#Phony targets 
.PHONY: main clean debug leak run other


# main - Default build, suitable for most people
main: build
main: 
	@printf '' >/dev/null


# clang - Default build using clang, suitable for most people
clang: CFLAGS=$(CLANGFLAGS)
clang: CC=clang 
clang: build
clang: 
	@printf '' >/dev/null


# dev - Development target, using clang and asan for bulletproof-ness
dev: CFLAGS=$(CLANGFLAGS) -fsanitize=address -fsanitize-undefined-trap-on-error
dev: CC=clang 
dev: build
dev: 
	@printf '' >/dev/null


# build - Build dependent objects

# mariadb_config --include --libs
build: $(OBJECTS)
	$(CC) $(CFLAGS) main.c -o $(NAME) $(OBJECTS) $(IFLAGS) $(LDFLAGS) -lssl -lcrypto -lz

#	$(CC) $(CFLAGS) main.c -o $(NAME) $(OBJECTS) lib/libmariadbclient.a $(MYSQL_IFLAGS) -lssl -lcrypto -lz

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


