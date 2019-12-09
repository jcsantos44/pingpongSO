#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pingpong.h"

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

int buffer[5] = {};
task_t p1, p2, p3, c1, c2;
semaphore_t s_buffer, s_item, s_vaga;
int in_index = 0;
int out_index = 0;

void push(int i)
{
  buffer[in_index] = 0;
  in_index = (in_index+1)%5;
}

int pop()
{
  int i = buffer[out_index];
  out_index = (out_index+1)%5;
  return i;
}

void produtor(void* arg)
{
  int item;
   while (1)
   {
      task_sleep(1);
      item = rand()%10;

      if(sem_down(&s_vaga) != 0)
        printf("erro sem_down: s_vaga nao existe ou foi excluido\n");

      if(sem_down(&s_buffer) == 0)
      {
        push(item);
      } else { printf("erro sem_down: s_buffer nao existe ou foi excluido\n"); }

      if(sem_up(&s_buffer) != 0)
      {
        printf("erro sem_up: s_buffer nao existe ou foi excluido\n");
      }

      if(sem_up(&s_item) != 0)
      {
        printf("erro sem_up: s_item nao existe ou foi excluido\n");
      }
   }
}

void consumidor(char* arg)
{
  int item;
  while (1)
  {
    if(sem_down(&s_item) != 0)
    {
      printf("erro sem_down: s_item nao existe ou foi excluido\n");
    }

    if(sem_down(&s_buffer) == 0)
    {
      item = pop();
    } else { printf("erro sem_down: s_buffer nao existe ou foi excluido\n"); }

    if(sem_up(&s_buffer) != 0)
    {
      printf("erro sem_up: s_buffer nao existe ou foi excluido\n");
    }

    if(sem_up(&s_vaga) != 0)
    {
      printf("erro sem_up: s_vaga nao existe ou foi excluido\n");
    }

    printf("%s consumiu %d", arg, item);
    task_sleep(1);
  }
}

int main (int argc, char* argv[])
{
  pingpong_init();
  srand(time(NULL));
  printf ("Main INICIO\n");

  task_create(&p1, produtor, "p1");
  task_create(&p2, produtor, "p2");
  task_create(&p3, produtor, "p3");
  task_create(&c1, produtor, "\t\tc1");
  task_create(&c2, produtor, "\t\tc1");

  sem_create(&s_buffer, 1);
  sem_create(&s_item, 0);
  sem_create(&s_vaga, 5);

  task_join(&c1);
  task_join(&c2);
  task_join(&p1);
  task_join(&p2);
  task_join(&p3);

  printf ("Main FIM\n") ;
  task_exit(0);
  exit(0);

}
