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
OBJECTS = vendor/zwalker.o vendor/util.o

#Phony targets 
.PHONY: main clean debug leak run other

#Primary target
main: build
main: 
	@printf '' >/dev/null

#Tests or static data
static:
	@$(CC) -c data/words.c -o data/words.o
	@$(CC) -c data/address.c -o data/address.o

# build
build: $(OBJECTS)
	@echo $(CC) $(OBJECTS) main.c -o $(NAME) $(CFLAGS)
	@$(CC) $(OBJECTS) main.c -o $(NAME) $(CFLAGS)

# tests
tests: vendor/single.o
	@echo $(CC) vendor/single.o data/words.o data/address.o main.c -o $(NAME) $(CFLAGS)
	@$(CC) vendor/single.o data/words.o data/address.o main.c -o $(NAME) $(CFLAGS)

# objects
%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

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
