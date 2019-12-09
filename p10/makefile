CC = gcc
CFLAGS = -Wall -I.
DEPS = queue.h datatypes.h pinpong.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sleep_task: pingpong.o queue.o pingpong-sleep.o
	$(CC) -o sleep_task pingpong.c queue.c pingpong-sleep.c

prodcons: pingpong.o queue.o pingpong-prodcons.o
	$(CC) -o prodcons pingpong.c queue.c pingpong-prodcons.c

clean:
	rm *.o join_task
