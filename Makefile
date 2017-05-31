CC = gcc
CFLAGS = -mtune=native -msse4.1 -Wall -Wextra -D _GNU_SOURCE -std=gnu11 -Og -g -Wno-missing-field-initializers -Isrc/ -MMD
LDLIBS += -lreadline -lm
SRC = $(wildcard src/*.c)

.PHONY: clean

main: $(patsubst %.c,%.o, $(SRC))
	$(CC) $(LDFLAGS) -o main $^ $(LDLIBS)

clean:
	rm main $(SRC:.c=.o) $(SRC:.c=.d)

-include $(SRC:.c=.d)
