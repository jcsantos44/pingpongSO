CC = gcc
CFLAGS = -Wall -I. -lm
DEPS = queue.h datatypes.h pinpong.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

msg_task: pingpong.o queue.o pingpong-mqueue.o
	$(CC) -o msg_task pingpong.c queue.c pingpong-mqueue.c -lm

clean:
	rm *.o msg_task
