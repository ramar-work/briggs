#!/usr/bin/make
NAME = briggs 
SRC = single.c main.c
OBJ = ${SRC:.c=.o}
IGNORE = archive test.db
IGNCLEAN = "sqlite3.o"
ARCHIVEDIR = ..
ARCHIVEFMT = gz
ARCHIVEFILE = $(NAME).`date +%F`.`date +%H.%M.%S`.tar.${ARCHIVEFMT}
LIB = lib$(NAME).so

#CC = clang
HOST = rc
CC = gcc 
CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused \
	-fsanitize=address \
	-fsanitize-undefined-trap-on-error
GCCFLAGS = -g -Wall -Werror -Wno-unused \
	-Wstrict-overflow -ansi -std=c99 \
	-Wno-deprecated-declarations -O0 \
	-pedantic-errors -Wno-overlength-strings -DSQROOGE_H
CFLAGS = $(GCCFLAGS)

INIT_ASAN = ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
BIN = $(NAME) 
#RUNARGS = $(INIT_ASAN) ./$(BIN)
RUNARGS = ./$(BIN)

#Phony targets 
.PHONY: main clean debug leak run other

#Primary target
main: build
main: run 
main: 
	@printf '' >/dev/null

# run
run:
	@./$(BIN) -c files/full-tab.csv --delimiter ";"

# build
build: vendor/single.o
	@$(CC) vendor/single.o main.c -o $(NAME) $(CFLAGS) 

#clean
clean:
	-rm -f *.o vendor/*.o
	-rm -f $(NAME)

#load
load:
	@echo "Seeding database 'test.db'.  (this could take a moment...)"
	@./ezphp -s | sqlite3 test.db && echo "DONE!" || echo "Failed to seed records."

# Make a tarball that goes to another directory
backup:
	@-rm -f sqlite3.o
	@echo tar chzf $(ARCHIVEDIR)/${ARCHIVEFILE} --exclude-backups \
		`echo $(IGNORE) | sed '{ s/^/--exclude=/; s/ / --exclude=/g; }'` ./*
	@tar chzf $(ARCHIVEDIR)/${ARCHIVEFILE} --exclude-backups \
		`echo $(IGNORE) | sed '{ s/^/--exclude=/; s/ / --exclude=/g; }'` ./*

# ...
changelog:
	@echo "Creating / updating CHANGELOG document..."
	@touch CHANGELOG

# Notate a change (Target should work on all *nix and BSD)
change:
	@test -f CHANGELOG || printf "No changelog exists.  Use 'make changelog' first.\n\n"
	@test -f CHANGELOG
	@echo "Press [Ctrl-D] to save this file."
	@cat > CHANGELOG.USER
	@date > CHANGELOG.ACTIVE
	@sed 's/^/\t -/' CHANGELOG.USER >> CHANGELOG.ACTIVE
	@printf "\n" >> CHANGELOG.ACTIVE
	@cat CHANGELOG.ACTIVE CHANGELOG > CHANGELOG.NEW
	@rm CHANGELOG.ACTIVE CHANGELOG.USER
	@mv CHANGELOG.NEW CHANGELOG
