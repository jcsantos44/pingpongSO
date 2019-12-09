CC = gcc
CFLAGS = -Wall -I.
DEPS = queue.h datatypes.h pinpong.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

barrier_task: pingpong.o queue.o pingpong-barrier.o
	$(CC) -o barrier_task pingpong.c queue.c pingpong-barrier.c

clean:
	rm *.o sem_task
