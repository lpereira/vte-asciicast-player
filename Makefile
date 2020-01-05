.PHONY: all

CFLAGS = `pkg-config --cflags gtk+-3.0` \
	 `pkg-config --cflags vte-2.91` \
	 `pkg-config --cflags json-glib-1.0` \
	 -Wall -Wextra -O1 -g \
	 -fsanitize=address

LDFLAGS = `pkg-config --libs gtk+-3.0` \
	  `pkg-config --libs vte-2.91` \
	  `pkg-config --libs json-glib-1.0` \
	  -O1 -g \
	  -fsanitize=address

SRCS = player.c

OBJS = $(SRCS:.c=.o)

all: player

.c.o: $<
	$(CC) $(CFLAGS) -c $< -o $@

player: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o player

clean:
	rm -f player $(OBJS)
