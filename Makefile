#!/usr/bin/make
NAME = mockery
SRC = lite.c parsely.c main.c
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
	-pedantic-errors -Wno-overlength-strings
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

run:
	@./$(BIN) -j

load:
	@echo "Seeding database 'test.db'.  (this could take a moment...)"
	@./ezphp -s | sqlite3 test.db && echo "DONE!" || echo "Failed to seed records."

# This loader routine will probably be easier done in C. see above
load-shell:
	sed "{ \
		s|'|''|g; \
		s|<p>\([^<]*\)</p>|\"<p>\1</p>2\"\n|g; \
		s|2|\\\n|g \
	}" lorem 

etc:
		s|<p>\([^<]*\)</p>|INSERT INTO content VALUES \
			(NULL, 'Antonio Ramar Collins II', 1486207621, '\1');\n|g
	
server:
	cd example && php -S localhost:3000

bg-server:
	php -S localhost:3000 & >/dev/null

#
build: $(OBJ)
	@$(CC) $(OBJ) -o $(BIN) $(CFLAGS)

#clean
clean:
	-@find . -maxdepth 1 -type f -iname "*.o" -o -iname "*.so" \
		`echo $(IGNCLEAN) | sed '{ s/ / ! -iname /g; s/^/! -iname /; }'` | xargs rm 
	-@rm $(BIN) $(BIN1) $(BIN2)

permissions:
	@find | grep -v './tools' | grep -v './examples' | grep -v './.git' | sed '1d' | xargs stat -c 'chmod %a %n' > PERMISSIONS

restore-permissions:
	chmod 744 PERMISSIONS && ./PERMISSIONS && chmod 644 PERMISSIONS

# Make a tarball that goes to another directory
backup:
	@-rm -f sqlite3.o
	@echo tar chzf $(ARCHIVEDIR)/${ARCHIVEFILE} --exclude-backups \
		`echo $(IGNORE) | sed '{ s/^/--exclude=/; s/ / --exclude=/g; }'` ./*
	@tar chzf $(ARCHIVEDIR)/${ARCHIVEFILE} --exclude-backups \
		`echo $(IGNORE) | sed '{ s/^/--exclude=/; s/ / --exclude=/g; }'` ./*

# Make an archive tarball
archive: ARCHIVEDIR = archive
archive: backup

# Sync
sync:
	@VAR=`ssh $(HOST) 'ls ~/space/$(NAME)* | tail -n 1'`; \
	 ssh $(HOST) "cat $$VAR" | tee archive/`basename $$VAR` | tar xzf -

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
