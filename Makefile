CC = g++
CFLAGS = -mtune=native -msse4.1 -Wall -Wextra -fopenmp -D _GNU_SOURCE -std=gnu++11 -fPIC -Og -g -Wno-missing-field-initializers -Isrc/ -MMD -I/usr/include/python3.5m/ -Iinclude
LDFLAGS += -fopenmp
LDLIBS += -Llib -lreadline -lm -l:eve_nerd.so
SRC = $(wildcard src/*.cpp)

.PHONY: clean

main: lib/eve_nerd.so lib/_eve_nerd.so src/main.o
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
	rm -r lib
	rm main $(SRC:.c=.o) $(SRC:.c=.d)

-include $(SRC:.c=.d)
