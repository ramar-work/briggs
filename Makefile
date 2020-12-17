#!/usr/bin/make
NAME = briggs
PREFIX=/usr/local
DOCFILE=/tmp/$(NAME).html
CLANGFLAGS = -g -Wall -Werror -std=c99
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 \
	-Wno-deprecated-declarations -O3 -pedantic-errors -Wno-pointer-arith
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

# dev - Development target, using clang and asan for bulletproof-ness
dev: CFLAGS=$(CLANGFLAGS) -fsanitize=address -fsanitize-undefined-trap-on-error
dev: CC=clang 
dev: build
dev: 
	@printf '' >/dev/null

# build - Build dependent objects
build: $(OBJECTS)
	@echo $(CC) $(OBJECTS) main.c -o $(NAME) $(CFLAGS)
	@$(CC) $(OBJECTS) main.c -o $(NAME) $(CFLAGS)

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
pkg:
	git archive --format tar master | gzip > $(NAME)-master.tar.gz

# pkgdev - Create a package of the latest version of 'dev'
pkgdev:
	git archive --format tar dev | gzip > $(NAME)-dev.tar.gz

# list - list all the targets and what they do
list:
	@printf 'Available options are:\n'
	@sed -n '/^# / { s/# //; 1d; p; }' Makefile | awk -F '-' '{ printf "  %-20s - %s\n", $$1, $$2 }'


