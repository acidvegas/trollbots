CC=gcc
CFLAGS=-Wsign-compare -Wall -I/usr/local/include
LDFLAGS=-L/usr/local/lib -lpthread

VPATH=src:build
OBJS=clone.o control.o init.o lists.o load.o mass.o muhstik.o net.o print.o proxy.o string.o

all:		muhstik dork
muhstik:	$(OBJS)
		cd build; $(CC) $(LDFLAGS) -o ../muhstik $(OBJS)
.c.o:
		$(CC) $(CFLAGS) -c -o build/$@ $<
notmp:
		rm -f *~
		rm -f */*~
clean:		notmp
		rm -f build/*.o
dork:
		latex doc/TODO.tex
		dvipdf TODO.dvi doc/TODO.pdf
		rm -f TODO.dvi TODO.aux TODO.log texput.log
