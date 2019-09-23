#!/usr/bin/make
NAME = briggs
PREFIX=/usr/local
DOCFILE=/tmp/$(NAME).html
CC = gcc 
CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused \
	-fsanitize=address \
	-fsanitize-undefined-trap-on-error
GCCFLAGS = -g -Wall -Werror -Wno-unused \
	-Wstrict-overflow -ansi -std=c99 \
	-Wno-deprecated-declarations -O0 \
	-pedantic-errors -Wno-overlength-strings -DSQROOGE_H
CFLAGS = $(GCCFLAGS)

#Phony targets 
.PHONY: main clean debug leak run other

#Primary target
main: build
main: 
	@printf '' >/dev/null

# build
build: vendor/single.o
	@$(CC) vendor/single.o main.c -o $(NAME) $(CFLAGS) 

#clean
clean:
	-rm -f *.o vendor/*.o
	-rm -f $(NAME)

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
