CC = gcc
CFLAGS = -Wall -I.
DEPS = queue.h datatypes.h pinpong.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sem_task: pingpong.o queue.o pingpong-semaphore.o
	$(CC) -o sem_task pingpong.c queue.c pingpong-semaphore.c

racecond_task: pingpong.o queue.o pingpong-racecond.o
	$(CC) -o racecond_task pingpong.c queue.c pingpong-racecond.c

prodcons_task: pingpong.o queue.o pingpong-prodcons.o
	$(CC) -o prodcons_task pingpong.c queue.c pingpong-prodcons.c

clean:
	rm *.o sem_task
