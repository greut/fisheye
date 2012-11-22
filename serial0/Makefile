CC=g++
#CPPFLAGS = -g -O0 -W -Wall -pedantic -std=c++0x -I../libs `Magick++-config --cppflags --cxxflags --ldflags --libs`
CPPFLAGS = -g -O0 -W -Wall -pedantic -std=c++0x -I../libs -lm
LDFLAGS =
serial: serial.o ../libs/lspbmp.o

clean:
	rm -f serial serial.o ../lib/lspbmp.o
