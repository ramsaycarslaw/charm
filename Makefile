charm: charm.c
	$(CC) charm.c -o charm -Wall -Wextra -pedantic -std=c99 -g

install: charm
	cp charm /usr/local/bin/
