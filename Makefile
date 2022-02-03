# Builds pi6502 executable and apple1rom.bin from source code. 
# requires gcc cc65 libncurses5-dev libncursesw5-dev 

CFLAGS=-O3 -W -Wall -std=c99 -D_XOPEN_SOURCE=500 -g 

all: pi6502 apple1rom.bin

pi6502: pi6502.o mailbox.o -lncurses

apple1rom.bin: apple1rom.a
	ca65 apple1rom.a -o apple1rom.o
	cl65 apple1rom.o -C apple1bin.cfg -o apple1rom.bin

clean:
	rm -f pi6502 apple1rom.bin *.o
