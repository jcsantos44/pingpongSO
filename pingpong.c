/**
* Projeto de SO 2019/2
* Alunos:
* Nelson Augusto Pires
* João Carlos Cardoso Santos
**/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "queue.h"
#include "pingpong.h"

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

#define STACKSIZE 32768		/* tamanho de pilha das threads */
#define _XOPEN_SOURCE 600	/* para compilar no MacOS */
//#define DEBUG

/*****************************************************/

const int highestPrio = -20;
const int lowestPrio = 20;
const int agingAlpha = -1;
const int quantum = 20;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer;

/**
*MainTask - struct da tarefa Main;
*Dispatcher - struct da tarefa Dispatcher;
*CurrentTask - endereco da struct da tarefa atualmente sendo executada;
*FilaReadyTask - endereco da primeira struct da fila de tarefas prontas;
**/
task_t MainTask, Dispatcher, *CurrentTask, *FilaReadyTask;
int task_count = 0;
int current_quantum; //quantum da tarefa atual
int clock; //referencia de tempo do sistema
int gotProcTime; //valor do clock quando uma tarefa pegou o processador

// tratador do sinal
void tick_handler ()
{
  #ifdef DEBUG
  printf ("Tick Handler disparado!\n") ;
  #endif
  clock++;//incrementa o clock
  if(CurrentTask != &Dispatcher){//por enquanto soh o dispatcher eh considerado de sistema
    current_quantum--;
    if(current_quantum <= 0){
      #ifdef DEBUG
      printf("Quantum da tarefa %d chegou a 0!\n", CurrentTask->tid);
      #endif
      task_yield();
    }
  }
}


//retorna o endereco de uma var do tipo task_t
task_t *scheduler()
{//politica de escalonamento: prioridade dinamica
  task_t* next; //endereco da task que sera enviada para o dispatcher(proxima tarefa a ser executada)
  task_t* highestTask; //endereco da task com maior prioridade
  task_t* aux_current; //var auxiliar para percorrer a fila

  #ifdef DEBUG
  printf("scheduler: Recuperar tarefa de maior prioridade na fila.\n");
  #endif
  if(queue_size((queue_t*)FilaReadyTask) == 0){//fila vazia
    #ifdef DEBUG
    printf("scheduler: Fila vazia!Nenhuma proxima tarefa na fila.\n");
    #endif
    return NULL;
  }

  //inicialmente a tarefa de maior prioridade eh a primeira da lista
  highestTask = FilaReadyTask;
  aux_current = FilaReadyTask->next;
  while(aux_current != FilaReadyTask){
    //se tiverem mesma prioridade, pega a com ID menor
    if(aux_current->aging_prio == highestTask->aging_prio && aux_current->tid < highestTask->tid){
      highestTask = aux_current;
    }

    if(aux_current->aging_prio < highestTask->aging_prio){
      highestTask = aux_current;
    }
    aux_current = aux_current->next;
  }


  //retira a tarefa de maior prioridade dafila
  next = (task_t*) queue_remove ((queue_t **) &FilaReadyTask, (queue_t*) highestTask);
  #ifdef DEBUG
  printf("scheduler: A tarefa %d tem a maior prioridade: %d.\n", next->tid, next->aging_prio);
  #endif
  next->aging_prio = task_getprio(next);//prioridade dinamica volta ao valor estatico

  #ifdef DEBUG
  printf("scheduler: Envelhecendo as tarefas restantes na fila.\n");
  #endif
  //envelhecer as tarefas
  if(queue_size((queue_t*)FilaReadyTask) != 0){//fila ainda tem tarefas
    aux_current = FilaReadyTask;
    int aging_p;
    do{
      aging_p = aux_current->aging_prio;
      aging_p += agingAlpha; //aumenta a prioridade(envelhece)
      if(aging_p < highestPrio){//nao deixa passar do limite
        aging_p = highestPrio;
      }
      aux_current->aging_prio = aging_p; //nova prioridade dinamica(envelhecida)
      #ifdef DEBUG
      printf("scheduler: Prioridade da tarefa %d envelhecida: %d.\n", aux_current->tid, aux_current->aging_prio);
      #endif
      aux_current = aux_current->next; //passa para a proxima tarefa da fila
    }while(aux_current != FilaReadyTask);
  }
  return next;
}
/*
task_t *scheduler()
{//politica de escalonamento: FCFS
  task_t* next;

  //retira primeiro elemento da fila
  next = (task_t*) queue_remove ((queue_t **) &FilaReadyTask, (queue_t*) FilaReadyTask);
  //verificar se a fila esta vazia
  if(next == NULL){
    #ifdef DEBUG
    printf("scheduler: Fila vazia!Nenhuma proxima tarefa na fila.\n");
    #endif
    return NULL;
  }else{
    return next;
  }
}*/

void dispatcher_body ()
{
  #ifdef DEBUG
  printf("----Iniciando Dispatcher. Tamanho da Fila: %d\n", queue_size((queue_t*)FilaReadyTask));
  #endif
  task_t* next; //endereco da proxima tarefa a ser executada
  while(queue_size((queue_t*)FilaReadyTask) > 0){
    next = scheduler();
    if(next != NULL){
      #ifdef DEBUG
      printf("dispatcher_body: Proxima tarefa a ser executada: %d.\n", next->tid);
      #endif
      task_switch(next);
      //break;
    }
    //sai do dispatcher e retorna pra Main
  }
  task_exit(0);
}

//Funcao para verificar se a task criada eh de usuario e add na fila
void isUserTask(task_t* task){
  //por enquanto, se a tarefa nao for a Main e o Dispathcer, ela eh de usuario
  //Main tambem eh escalonavel
  if(/*task != &MainTask && task->tid > 0 &&*/ task != &Dispatcher){//task criada eh de usuario e precisa ser adicionado na fila de prontas
    #ifdef DEBUG
    printf("isUserTask: Adicionando tarefa <%d> de usuario na lista de prontas\n", task->tid);
    #endif
    queue_append((queue_t **) &FilaReadyTask, (queue_t*) task);
  }

}

//funcao para inicializar os paramentros de uma tarefa e adicionar na fila
void initTask(task_t* task, int task_id, int priority)
{
  task->tid = task_id; // seta id da task
  task_setprio(task, priority); //Ao ser criada, cada tarefa recebe a prioridade default(0)
  task->exec_start = clock; //Seta o tempo que a tarefa foi criada
  task->proc_time = 0; //Seta o tempo que a tarefa passou no processador
  task->activations = 0; //Seta quantidade de ativacoes
  //isUserTask(task); //adiciona task na fila
}

void pingpong_init()
{
  #ifdef DEBUG
  printf("----- Iniciando PingPong -----\n");
  #endif
  MainTask.exec_start = clock; //Seta o tempo que a main iniciou
  MainTask.activations = 1;

  #ifdef DEBUG
  printf("pingpont_init: Registrando acao do sinal e inicializando o temporizador\n");
  #endif
  current_quantum = quantum;//inicializa quantum
  clock = 0;//init clock
  gotProcTime = clock;//tempo que a Main pegou o processo
  // registra a acao para o sinal de timer SIGALRM
  action.sa_handler = tick_handler ;
  sigemptyset (&action.sa_mask) ;
  action.sa_flags = 0 ;
  if (sigaction (SIGALRM, &action, 0) < 0)
  {
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }

  // ajusta valores do temporizador
  timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
  timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }

  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  #ifdef DEBUG
  printf("pingpont_init: desativando buffer e iniciando contexto main\n");
  #endif

  if(setvbuf (stdout, 0, _IONBF, 0) != 0){//erro
    #ifdef DEBUG
    printf("pingpont_init: Erro ao desativar buffer de saida padrao\n");
    #endif
    //exit(0);
  }
  task_count = 0;

  if(getcontext(&MainTask.context) == -1){ //Armazena o contexto da Main no MainTask
    #ifdef DEBUG
    printf("pingpont_init: Erro no getcontext (Main)\n");
    #endif
    //exit(0);
  }

  #ifdef DEBUG
  printf("pingpont_init: Gravando contexto da Main, criando dispatcher e inicializando Fila de tarefas prontas!\n");
  #endif

  MainTask.tid = 0; // Seta id MainTask como 0
  task_count++; //incrementa numero de tasks
  CurrentTask = &MainTask;

  //criando o dispatcher
  task_create(&Dispatcher, dispatcher_body, NULL);
  //inicializa a fila
  FilaReadyTask = NULL;
  //Inicializa Main como escalonavel
  initTask(&MainTask, 0, 0);

}



int task_create (task_t* task, void (*start_func)(void *), void *arg)
{
   #ifdef DEBUG
   printf("task_create: criando tarefa %d\n", task_count);
   if (task == &MainTask)
   {
     printf("   Criando tarefa main\n");
   }
   if(task == &Dispatcher)
   {
     printf("   Criando Dispatcher\n");
   }
   #endif
   if (task == NULL){
     #ifdef DEBUG
     printf("task_create: Erro - Nenhuma tarefa para criar\n");
     #endif
     return -1;
   }
   char *stack;
   #ifdef DEBUG
   printf("task_create: Iniciando context e setando a stack\n");
   #endif
   if(getcontext(&task->context) == -1){ // armazena contexto atual na nova task
     #ifdef DEBUG
     printf("task_create: Erro no getcontext (task_create)\n");
     #endif
     return -1;
   }
   stack = malloc (STACKSIZE); // cria stak
   if (stack) // inicializa parametros da stack
   {
     #ifdef DEBUG
     printf("task_create: stack criada com sucesso\n");
     #endif
      task->context.uc_stack.ss_sp = stack ;
      task->context.uc_stack.ss_size = STACKSIZE;
      task->context.uc_stack.ss_flags = 0;
      task->context.uc_link = 0;
   }
   else
   {
     #ifdef DEBUG
     perror("task_create: Erro na criacao da stack\n");
     #endif
     return -1;
   }
   #ifdef DEBUG
   printf("task_create: Criando contexto\n");
   #endif
   /*task->tid = task_count; // seta id da task
   task_setprio(task, 0); //Ao ser criada, cada tarefa recebe a prioridade default(0)
   task->exec_start = clock; //Seta o tempo que a tarefa foi criada
   task->proc_time = 0; //Seta o tempo que a tarefa passou no processador
   task->activations = 0; //Seta quantidade de ativacoes*/
   initTask(task, task_count, 0); // inicializa parametros da task e add na fila
   makecontext(&task->context, (void*)(*start_func), 1, arg); // cria contexto para a task
   task_count++; // incrementa contador de tasks

   //verifica se eh uma tarefa de usuario para add na fila
   isUserTask(task);
   //retorna id da task criada
   return task->tid;
}


int task_switch (task_t *task)
{
   int cpu_time;
   if(task == NULL){
     #ifdef DEBUG
     printf("task_switch: Task switch invalido. Nova task nao existe\n");
     #endif
     return -1;
   }
   #ifdef DEBUG
   printf("task_switch: alternando da task %d para task %d\n", CurrentTask->tid,task->tid);
   #endif
   cpu_time = clock - gotProcTime;
   CurrentTask->proc_time += cpu_time;//soma o tempo que a tarefa antiga passou com o processador
   task_t* old_task = CurrentTask; // ponteiro temporario para armazenar a antiga task atual
   CurrentTask = task; // nova task é agora a task atual
   if(swapcontext(&old_task->context, &CurrentTask->context) == -1){ // troca contexto
     #ifdef DEBUG
     printf("task_switch: Erro na troca de contexto\n");
     #endif
     return -1;
   }
   current_quantum = quantum;//"zera" o quantum
   gotProcTime = clock;//Tempo que a nova CurrentTask pegou o processo
   CurrentTask->activations++;//Incrementa ativacoes da task atual
   return 0;
}

void task_exit (int exitCode)
{
   int tid,exec_time, proc_time, actv;
   #ifdef DEBUG
   printf("task_exit: Tarefa %d terminada com o codigo: %d\n", CurrentTask->tid, exitCode);
   #endif
   if(exitCode < 0){
     printf("Tarefa terminou com erro \n");
   }
   //soma uma ultiama vez o tempo de processamento
   proc_time = clock - gotProcTime;
   CurrentTask->proc_time += proc_time;

   tid = CurrentTask->tid;
   exec_time = clock - CurrentTask->exec_start;//Tempo de execucao da tarefa terminada
   proc_time = CurrentTask->proc_time; //Tempo de processamento da tarefa terminada
   actv = CurrentTask->activations; //Quantidade de ativacoes da tarefa terminada
   printf("Task %d exit: execution time %4d ms, processor time %4d ms, %4d activations\n", tid,  exec_time, proc_time, actv);
   //verificar se a tarefa terminada eh do usuario ou o dispatcher
   if(CurrentTask->tid == Dispatcher.tid){
     #ifdef DEBUG
     printf("task_exit: Terminou o Dispatcher. Voltando pra Main\n");
     #endif
     task_switch(&MainTask); // troca contexto para maintask
   }else{//tarefa do usuario
     #ifdef DEBUG
     printf("task_switch: Voltando para o Dispatcher\n");
     #endif
     task_switch(&Dispatcher);
   }
}

int task_id ()
{
   #ifdef DEBUG
   printf("task_id: ID da tarefa %d atual\n", CurrentTask->tid);
   #endif
   return CurrentTask->tid; //retorna id da task atual
}

void task_yield()
{
  #ifdef DEBUG
  printf("task_yield: ID da tarefa %d.\n", CurrentTask->tid);
  #endif
  if(CurrentTask->tid == Dispatcher.tid){
    #ifdef DEBUG
    printf("task_yield: Erro - Dispatcher nao pode usar o yield.\n");
    #endif
    task_exit(-1);
  }else{
    isUserTask(CurrentTask);//add task no final da fila, se for task de usuario
    task_switch(&Dispatcher);//trocando para o dispatcher
  }
}

void task_setprio(task_t *task, int prio)
{
  if(prio < highestPrio){
    prio = highestPrio;
  }
  if (prio > lowestPrio){
    prio = lowestPrio;
  }
  if(task == NULL){//Caso task seja nulo, ajusta a prioridade da tarefa atual
    task = CurrentTask;
    #ifdef DEBUG
    printf("task_setprio: Task nula. Setando task atual.\n");
    #endif
    if(task == NULL){
      #ifdef DEBUG
      printf("task_setprio: ERRO - CurrentTask nula.\n");
      #endif
      task_exit(-1);
    }
  }
  #ifdef DEBUG
  printf("task_setprio: Setando prioridade estatica da Task %d para %d.\n", task->tid, prio);
  #endif
  task->static_prio = prio;
  task->aging_prio = prio;
}


int task_getprio (task_t *task)
{
  int prio;
  if(task == NULL){//Caso task seja nulo, ajusta a prioridade da tarefa atual
    task = CurrentTask;
    #ifdef DEBUG
    printf("task_getprio: Task nula. Setando task atual.\n");
    #endif
    if(task == NULL){
      #ifdef DEBUG
      printf("task_getprio: ERRO - CurrentTask nula.\n");
      #endif
      task_exit(-1);
    }
  }
  prio = task->static_prio;
  #ifdef DEBUG
  printf("task_getprio: Prioridade da Task %d é %d.\n", task->tid, prio);
  #endif

  return prio;
}


unsigned int systime()
{
  return clock;
}

//funcoes para facilitar o DEBUG
void print_elem (void *ptr)
{
   task_t *elem = ptr ;

   if (!elem)
      return ;

   elem->prev ? printf ("%d", elem->prev->tid) : printf ("*") ;
   printf ("<%d>", elem->tid) ;
   elem->next ? printf ("%d", elem->next->tid) : printf ("*") ;
}

void tasks_print()
{
  queue_print("fila de tarefas", (queue_t*) FilaReadyTask, print_elem);
}
