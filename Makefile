CC = g++
CXX = $(CC)
CPPFLAGS = -march=native -Wall -Wextra -fopenmp -D _GNU_SOURCE -std=gnu++14 -fPIC -Og -g -Wno-missing-field-initializers -Isrc/ -MMD -I/usr/include/python3.5m/ -Iinclude
LDFLAGS += -fopenmp
LDLIBS += -Llib -lreadline -lm -l:eve_nerd.so
SRC = $(wildcard src/*.cpp)

.PHONY: clean

main: lib/eve_nerd.so src/main.o #lib/_eve_nerd.so
	$(CC) $(LDFLAGS) -o $@ src/main.o $(LDLIBS)

%_wrap.c: %.i
	swig -python -o $@ $^

lib/_eve_nerd.so: src/eve_nerd_wrap.o
	mkdir -p lib/
	$(CC) $(LDFLAGS) -shared -o $@ $^

lib/eve_nerd.so: src/dijkstra.o src/min_heap.o src/universe.o
	mkdir -p lib/
	$(CC) $(LDFLAGS) -shared -o $@ $^

clean:
	-rm -r lib
	rm main $(SRC:.cpp=.o) $(SRC:.cpp=.d)

-include $(SRC:.cpp=.d)
