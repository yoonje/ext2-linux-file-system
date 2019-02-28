SHELLOBJS	= shell.o ext2.o disksim.o ext2_shell.o entrylist.o
SRC=shell.c ext2.c disksim.c ext2_shell.c entrylist.c
CC=gcc

all: $(SHELLOBJS)
	$(CC) -g -o shell $(SRC) -Wall
#	$(CC) -g -o shell $(SHELLOBJS) -Wall

clean:
	rm *.o
	rm shell
