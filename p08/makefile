CC = gcc
CFLAGS = -Wall -I.
DEPS = queue.h datatypes.h pinpong.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

join_task: pingpong.o queue.o pingpong-join.o
	$(CC) -o join_task pingpong.c queue.c pingpong-join.c

clean:
	rm *.o join_task
