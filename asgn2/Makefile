CC = gcc
CFLAGS = -Wall -g
LIBFLAGS = $(CFLAGS) -fPIC -m64

all:
	make liblwp.so

liblwp.so: magic64.o rr.o lwp.o
	$(CC) $(LIBFLAGS) -shared -o $@ $^

magic64.o: magic64.S
	$(CC) -o $@ -c $<

lwp.o: lwp.c
	$(CC) $(LIBFLAGS) -o $@ -c $<

rr.o: rr.c
	$(CC) $(LIBFLAGS) -o $@ -c $<

clean:
	rm -f *.o