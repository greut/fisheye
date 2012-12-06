CC=gcc
CPPFLAGS = -g -O0 -W -Wall -pedantic -std=c99 -I../libs -fopenmp
LDFLAGS = -lgomp -lm
fisheye: fisheye.o ../libs/lspbmp.o ../libs/magnify.o

clean:
	rm -f fisheye fisheye.o ../lib/lspbmp.o ../libs/magnify.o
