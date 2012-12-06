CC=g++
CPPFLAGS = -W -Wall -pedantic -std=c++0x
LDFLAGS = -lm
COMPILE.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
LINK.o = $(CC) $(LDFLAGS) $(TARGET_ARCH)
OUTPUT_OPTION = -o $@

%: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

%.o: %.c
	$(COMPILE.c) $< $(OUTPUT_OPTION)

%.o: %.cc
	$(COMPILE.cc) $< $(OUTPUT_OPTION)

%.a: %.o
	ar -q $@
