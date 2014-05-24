# Eric Coutu
# 0523365

CC = gcc
CFLAGS = -Wall -std=c99 -pedantic -g -DNDEBUG
#	-std=c99:	use c99 standard
#	-pedantic:	forces standard
#	-g:			?
# LIBS = -L. -lefence

all: gpstool Gps.so

gpstool: gpstool.o gputil.o mystring.o
	gcc $(CFLAGS) gpstool.o gputil.o mystring.o -o gpstool

gpstool.o: gpstool.c gpstool.h
	gcc $(CFLAGS) -c gpstool.c

gputil.o: gputil.c gputil.h
	gcc $(CFLAGS) -fPIC -c gputil.c

mystring.o: mystring.c mystring.h
	gcc $(CFLAGS) -fPIC -c mystring.c

Gps.so: Gpsmodule.o gputil.o mystring.o
	gcc $(CFLAGS) -shared Gpsmodule.o gputil.o mystring.o -o Gps.so

Gpsmodule.o: Gpsmodule.c
	gcc $(CFLAGS) -I/usr/include/python2.5 -fPIC -c Gpsmodule.c

clean:
	rm -f *.o *.so *~ *.pyc gpstool .error.log .temp.gps
	
