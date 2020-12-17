#!/usr/bin/make
NAME = briggs
PREFIX=/usr/local
DOCFILE=/tmp/$(NAME).html
CLANGFLAGS = -g -Wall -Werror -std=c99 -fsanitize=address \
	-fsanitize-undefined-trap-on-error
#CLANGFLAGS = -g -Wall -Werror -std=c99
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -std=c99 \
	-Wno-deprecated-declarations -O0 -pedantic-errors -Wno-overlength-strings
CFLAGS = $(CLANGFLAGS)
#CFLAGS = $(GCCFLAGS)
CC = clang 
#CC = gcc
OBJECTS = vendor/zwalker.o vendor/util.o

#Phony targets 
.PHONY: main clean debug leak run other

#Primary target
main: build
main: 
	@printf '' >/dev/null

# build
build: $(OBJECTS)
	@echo $(CC) $(OBJECTS) main.c -o $(NAME) $(CFLAGS)
	@$(CC) $(OBJECTS) main.c -o $(NAME) $(CFLAGS)

# objects
%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

#clean
clean:
	-rm -f *.o vendor/*.o $(NAME)

#install
install:
	-mkdir -p $(PREFIX)/bin/  $(PREFIX)/share/man/man1/
	cp $(NAME) $(PREFIX)/bin/
	cp $(NAME).1 $(PREFIX)/share/man/man1/

# test
test:
	@./$(NAME) -c files/full-tab.csv --delimiter ";"

# doctest
doctest:
	markdown -S README.md > $(DOCFILE)

# mantest 
mantest:
	man -l briggs.1
	
# pkg
pkg:
	git archive --format tar master | gzip > $(NAME)-master.tar.gz

# pkgdev
pkgdev:
	git archive --format tar dev | gzip > $(NAME)-dev.tar.gz
