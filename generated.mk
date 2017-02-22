
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

all: $(OBJS)

%.o : %.c
	gcc -I../include/ -I. -I.. -c $< -o $@ -O3 -fPIC

libcplex.a: $(OBJS)
	ar rcs $@ $^
