# Makefile
# IPK-PROJ2, 03.04.2018
# Author: Daniel Dolejska, FIT

CC=gcc
CFLAGS=-O3 -std=gnu99 -Wall -pedantic -pthread
CFLAGS_DEBUG=-std=gnu99 -Wall -Wextra -pedantic -pthread -DDEBUG_LOG_ENABLED -DDEBUG_PRINT_ENABLED -DDEBUG_ERR_ENABLED
COUT=ipk-dhcpstarve

all:
	make ipk-dhcpstarve

run:
	make all
	./ipk-dhcpstarve

debug:
	make ipk-dhcpstarve-debug

debug-run:
	make ipk-dhcpstarve-debug
	sudo ./ipk-dhcpstarve -i eth1


ipk-dhcpstarve: main.o dhcp.o general.o network.o
	$(CC) $(CFLAGS) main.o dhcp.o general.o network.o -o $(COUT)

main.o:
	$(CC) $(CFLAGS) main.c -c

dhcp.o:
	$(CC) $(CFLAGS) dhcp.c -c

general.o:
	$(CC) $(CFLAGS) general.c -c

network.o:
	$(CC) $(CFLAGS) network.c -c


ipk-dhcpstarve-debug: main.odbg dhcp.odbg general.odbg network.odbg
	$(CC) $(CFLAGS_DEBUG) main.o dhcp.o general.o network.o -o $(COUT)

main.odbg:
	$(CC) $(CFLAGS_DEBUG) main.c -c

dhcp.odbg:
	$(CC) $(CFLAGS_DEBUG) dhcp.c -c

general.odbg:
	$(CC) $(CFLAGS_DEBUG) general.c -c

network.odbg:
	$(CC) $(CFLAGS_DEBUG) network.c -c


clean:
	rm *.o $(COUT) 2>/dev/null

pack:
	zip xdolej08.zip *.c *.h doc/dokumentace.pdf README Makefile
