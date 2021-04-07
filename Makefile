CC=9c
LD=9l
RM=rm

ALL=VoxelSpace
SRCS= $(wildcard *.c)
OBJS:= $(SRCS:.c=.o)

.PHONY: all

all: clean $(ALL)

$(ALL): $(OBJS)
	$(LD) -o $@ $^

%.o: 
	$(CC) -o $@ $*.c

clean:
	$(RM) -f $(ALL) $(OBJS)
