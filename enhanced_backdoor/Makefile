CC=gcc
CXX=g++
CFLAGS=-I. -g -W -Wall -Wno-multichar -ansi -pedantic
LDFLAGS=-lbe 

all: run_backdoor

run_backdoor: main.o VMWBackdoor.o
	$(CXX) -o "$@" $^ $(LDFLAGS)
	mimeset "$@"

%.o: %.cpp
	$(CXX) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -f run_backdoor

.PHONY: clean mrproper
