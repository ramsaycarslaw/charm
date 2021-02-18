CC = clang
CFLAGS = -c -std=c99 -g -Wall -O3 
LDFLAGS = -g
SRC = $(wildcard ./src/*.c)
SRC += $(wildcard ./include/src/*.c)
HDR = $(wildcard ./include/*.h)
HDR += $(wildcard ./include/include/*.h)
OBJ = $(SRC:.c=.o)
EXEC = charm
PREFIX = /usr/local

all: $(SRC) $(OBJ) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ -lm

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) $< -o $@ 

.PHONY: install
install: $(EXEC)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/charm

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/charm

.PHONY: check
check:
	./test/test

clean:
	rm ./src/*.o $(EXEC) 
	rm ./include/src/*.o

