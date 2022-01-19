CC=gcc
CFLAGS=-O3 -fPIC
DEPS=my_malloc.h

all: lib

lib: my_malloc.o
	$(CC) $(CFLAGS) -shared -o libmymalloc.so my_malloc.o

%.o: %.c my_malloc.h
	$(CC) $(CFLAGS) -c -o $@ $< 

test: my_test.o my_malloc.o
	gcc -o test my_test.o my_malloc.o
my_test.o: my_test.c my_malloc.h
	gcc  -ggdb3 -pedantic -Wall -Werror  -c my_test.c

clean:
	rm -f *~ *.o *.so

clobber:
	rm -f *~ *.o
