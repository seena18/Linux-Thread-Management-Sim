CC 	= gcc

CFLAGS  = -Wall -g -I ../include

LD 	= gcc

LDFLAGS  = -Wall -g -L../lib64

PROGS	= snakes nums hungry

SNAKEOBJS  = randomsnakes.o util.o

SNAKELIBS = -lplw -lsnakes -lncurses -lrt

HUNGRYOBJS = hungrysnakes.o util.o

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

EXTRACLEAN = core $(PROGS)

.PHONY: all allclean clean rs hs ns

all: 	$(PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) *~ TAGS

snakes: randomsnakes.o util.o ../../liblwp.so ../lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o snakes randomsnakes.o util.o ../../liblwp.so ../lib64/libsnakes.so -lncurses -lrt

hungry: hungrysnakes.o  util.o ../../liblwp.so ../lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o hungry hungrysnakes.o util.o ../../liblwp.so ../lib64/libsnakes.so -lncurses -lrt

nums: numbersmain.o  util.o ../../liblwp.so 
	$(LD) $(LDFLAGS) -o nums numbersmain.o ../../liblwp.so

hungrysnakes.o: hungrysnakes.c ../include/lwp.h ../include/snakes.h
	$(CC) $(CFLAGS) -c hungrysnakes.c

randomsnakes.o: randomsnakes.c ../include/lwp.h ../include/snakes.h
	$(CC) $(CFLAGS) -c randomsnakes.c

numbermain.o: numbersmain.c lwp.h
	$(CC) $(CFLAGS) -c numbersmain.c

util.o: util.c ../include/lwp.h ../include/util.h ../include/snakes.h
	$(CC) $(CFLAGS) -c util.c

rs: snakes
	(export LD_LIBRARY_PATH=../../; ./snakes)

hs: hungry
	(export LD_LIBRARY_PATH=../../; ./hungry)

ns: nums
	(export LD_LIBRARY_PATH=../../; ./nums)
