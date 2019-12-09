/**
* Projeto de SO 2019/2
* Alunos:
* Nelson Augusto Pires
* João Carlos Cardoso Santos
**/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>
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
task_t MainTask, Dispatcher, *CurrentTask, *FilaReadyTask, *FilaSleepTask;
int task_count = 0;
int current_quantum; //quantum da tarefa atual
int clock; //referencia de tempo do sistema
int gotProcTime; //valor do clock quando uma tarefa pegou o processador

// tratador do sinal
void tick_handler ()
{
  #ifdef DEBUG
  //printf ("Tick Handler disparado! Clock <%d>\n", clock) ;
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
  //printf("scheduler: Recuperar tarefa de maior prioridade na fila. TamFila <%d>\n", queue_size((queue_t*)FilaReadyTask));
  #endif

  if(queue_size((queue_t*)FilaReadyTask) == 0){//fila vazia
    #ifdef DEBUG
    //printf("scheduler: Fila vazia!Nenhuma proxima tarefa na fila.\n");
    #endif
    return NULL;
  }

  //inicialmente a tarefa de maior prioridade eh a primeira da lista, que nao esta esperando outra tarefa terminar
  highestTask = FilaReadyTask;
  aux_current = FilaReadyTask->next;
  while(aux_current != FilaReadyTask){
		if(aux_current->wJoin == NULL){//tarefa nao pode estar esperando por outra
			//se tiverem mesma prioridade, pega a com ID menor
		  if(aux_current->aging_prio == highestTask->aging_prio && aux_current->tid < highestTask->tid){
		    highestTask = aux_current;
		  }

		  if(aux_current->aging_prio < highestTask->aging_prio){
		    highestTask = aux_current;
		  }
			if(highestTask->wJoin != NULL){
				highestTask = aux_current;
			}
		}else
		{
			#ifdef DEBUG
		  printf("scheduler: IGNORAR tarefa %d, aguardando outra tarefa.\n", aux_current->tid);
		  #endif
		}
    aux_current = aux_current->next;
  }

	if(highestTask->wJoin != NULL)
		return NULL;


  //retira a tarefa de maior prioridade da fila
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
      if(aux_current->wJoin == NULL){
				aging_p = aux_current->aging_prio;
		    aging_p += agingAlpha; //aumenta a prioridade(envelhece)
		    if(aging_p < highestPrio){//nao deixa passar do limite
		      aging_p = highestPrio;
		    }
		    aux_current->aging_prio = aging_p; //nova prioridade dinamica(envelhecida)
		    #ifdef DEBUG
		    printf("scheduler: Prioridade da tarefa %d envelhecida: %d.\n", aux_current->tid, aux_current->aging_prio);
		    #endif
			}
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

//Funcao para acordar as tarefas que estao dormindo e ja podem acordar
void wakeSleep()
{
	#ifdef DEBUG
  //printf("wakeSleep: Acordando tarefas que podem acordar. ClockTime: %dms\n", clock);
  #endif
	if(queue_size((queue_t*)FilaSleepTask) > 0){
		task_t* aux_current; //var auxiliar para percorrer a fila
		task_t* aux_task; //var auxiliar para remover e add nas filas
		aux_current = FilaSleepTask;
		task_t* aux_last = FilaSleepTask->prev;
		while(aux_current != aux_last)
		{
			if(aux_current->wakeTime <= clock)
			{
				aux_current->wakeTime = -1;
				//printf("wakeSleep: Acordando tarefas <%d> que podem acordar. ClockTime: %dms\n",aux_current->tid, clock);
				task_t* temp = aux_current->next;
				//retira tarefa da fila Sleep
				aux_task = (task_t*) queue_remove ((queue_t **) &FilaSleepTask, (queue_t*) aux_current);
				//e coloca na fila Ready
				queue_append((queue_t **) &FilaReadyTask, (queue_t*) aux_task);
				aux_current = temp;
			} else aux_current = aux_current->next;
		}
		if(aux_current->wakeTime <= clock)
		{	
			aux_current->wakeTime = -1;
//printf("wakeSleep: Acordando tarefa <%d> que pode acordar. ClockTime: %dms\n",aux_current->tid, clock);
			task_t* temp = aux_current->next;
			//retira tarefa da fila Sleep
			aux_task = (task_t*) queue_remove ((queue_t **) &FilaSleepTask, (queue_t*) aux_current);
			//e coloca na fila Ready
			queue_append((queue_t **) &FilaReadyTask, (queue_t*) aux_task);
			aux_current = temp;
		}
	}
}

void dispatcher_body ()
{
  #ifdef DEBUG
  printf("----Iniciando Dispatcher. Tamanho da Fila: %d\n", queue_size((queue_t*)FilaReadyTask));
  #endif
  task_t* next; //endereco da proxima tarefa a ser executada
  while(queue_size((queue_t*)FilaReadyTask) > 0 || queue_size((queue_t*)FilaSleepTask) > 0){
		wakeSleep();
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

//Funcao para acordar as tarefas esperando uma tarefa temrinar. Soh pode ser chamada pelo task_exit
void wakeJoin()
{
	#ifdef DEBUG
  printf("wakeJoin: Acordando tarefas que estao esperando a tarefa <%d> terminar\n", CurrentTask->tid);
  #endif
	if(queue_size((queue_t*)FilaReadyTask) > 0){
		task_t* aux_current; //var auxiliar para percorrer a fila
		aux_current = FilaReadyTask;
		do{
			if(aux_current->wJoin == CurrentTask){//se aux_current esta esperando a tarefa atual terminar
				aux_current->wJoin = NULL;
			}
			aux_current = aux_current->next;
		}while(aux_current != FilaReadyTask);
	}else{
		#ifdef DEBUG
		printf("wakeJoin: Fila Ready vazia\n");
		#endif
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
	task->wJoin = NULL;//Seta endereco da tarefa sendo esperada
	task->semaforo = 0;//seta se esta esperando semaforo
	task->exitCode = 9999;// Seta exitCode da task. 9999 = Tarefa sendo executada / <0 = Erro / >0 = Finalizou
	task->wakeTime = -1; //Seta valor do clock que a tarefa deve ser acordada
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
  //inicializa a fila Ready
  FilaReadyTask = NULL;
	//inicializa a fila Sleep
  FilaSleepTask = NULL;
  //Inicializa Main como escalonavel
  initTask(&MainTask, 0, 1);

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
		CurrentTask->exitCode = exitCode;
		task_count--;
		wakeJoin();//acordar as tarefas que estao esperando pelo fim da tarefa atual
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

int task_join (task_t *task)
{
    if(task == NULL){//se a tarefa a ser esperada nao existe, retorna 1
     #ifdef DEBUG
     printf("task_join: Task join invalido. Task a ser esperada nao existe\n");
     #endif
     return -1;
   }
    if(task->exitCode != 9999){//se a tarefa a ser esperada ja terminou, retorna seu exitCode
			#ifdef DEBUG
      printf("task_join: Task a ser esperada ja terminou com o codigo %d\n",task->exitCode);
      #endif
			return task->exitCode;
    }

		#ifdef DEBUG
   	printf("task_join: Tarefa %d esperando termino da tarefa: %d\n", CurrentTask->tid, task->tid);
   	#endif
		
		//indicando que a tarefa corrente ira esperar outra tarefa terminar
		CurrentTask->wJoin = task;

		task_yield();

		return task->exitCode;
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
		if((CurrentTask->wakeTime == -1) && (CurrentTask->semaforo != 1) && (CurrentTask->barreira != 1)){//variavel nao pode estar dormindo para ser add na fila Ready
    	isUserTask(CurrentTask);//add task no final da fila, se for task de usuario
		}
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

void task_sleep (int t)
{
	if(t <= 0){
		return;
	}
	#ifdef DEBUG
  printf("task_sleep: Dando um mata leao na tarefa <%d>.\n",CurrentTask->tid);
  #endif
	CurrentTask->wakeTime = systime() + t*1000;//deve acordar daqui t*1000 ms
	queue_append((queue_t **) &FilaSleepTask, (queue_t*) CurrentTask);//add CurrentTask na fila de Sleep
	#ifdef DEBUG
  printf("task_sleep: Tempo para acordar: <%dms> / Tamanho da fila Sleep <%d>.\n",CurrentTask->wakeTime,queue_size((queue_t*)FilaSleepTask));
  #endif
	task_yield();
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

//============SEMAFOROS==================
int sem_create (semaphore_t *s, int value)
{
	if(s == NULL){//se nao tem semaforo, retorna erro
		#ifdef DEBUG
		printf("sem_create: Semaforo invalido. ERRO.\n");
		#endif
		return -1;
	}

	#ifdef DEBUG
	printf("sem_create: Criando novo semaforo.\n");
	#endif
	s->contador = value;
	s->fila = NULL;
	s->destruido = 0;//flag para bloquear uso do semaforo caso alguma task foi interrompida no meio de uma funcao do semaforo

	return 0;
}

int sem_down (semaphore_t *s)
{
	if(s == NULL || s->destruido == 1){
		#ifdef DEBUG
		printf("sem_down: Semaforo requisitado invalido. ERRO.\n");
		#endif
		return -1;
	}

	#ifdef DEBUG
	printf("sem_down: Requisitando um semaforo.\n");
	#endif
	s->contador--;
	
	if(s->contador < 0){//suspender tarefa
		#ifdef DEBUG
		printf("sem_down: Semaforo cheio. Suspendendo tarefa <%d>.\n", CurrentTask->tid);
		#endif
		CurrentTask->semaforo = 1;//esperando por semaforo
		queue_append((queue_t **) &s->fila, (queue_t*) CurrentTask);
		task_yield();

		if(CurrentTask->semaforo == 1){//retornou do semaforo destruido
			return -1;			
		}
	}

	return 0;

}

int sem_up (semaphore_t *s)
{
	task_t* aux;
	if(s == NULL){
		#ifdef DEBUG
		printf("sem_up: Semaforo requisitado invalido. ERRO.\n");
		#endif
		return -1;
	}
	
	#ifdef DEBUG
	printf("sem_up: Liberando um semaforo.\n");
	#endif
	s->contador++;

	if(queue_size((queue_t*)s->fila) > 0){
		aux = (task_t*) queue_remove ((queue_t **) &s->fila, (queue_t*) s->fila);
		#ifdef DEBUG
		printf("sem_up: Acordando task <%d>.\n",aux->tid);
		#endif
		aux->semaforo = 0;//nao esta mais esperando por semaforo
		queue_append((queue_t **) &FilaReadyTask, (queue_t*) aux);		
	}
	return 0;
}

int sem_destroy (semaphore_t *s)
{
	task_t* aux;
	if(s == NULL){
		#ifdef DEBUG
		printf("sem_destroy: Semaforo requisitado invalido. ERRO.\n");
		#endif
		return -1;
	}

	#ifdef DEBUG
	printf("sem_destroy: Acordando as tarefas.\n");
	#endif

	while(queue_size((queue_t*)s->fila) > 0){
		aux = (task_t*) queue_remove ((queue_t **) &s->fila, (queue_t*) s->fila);
		queue_append((queue_t **) &FilaReadyTask, (queue_t*) aux);
	}
	//zerando as variaveis do semaforo
	s->contador = 0;
	s->fila = NULL;
	s->destruido = 1;//flag para bloquear uso do semaforo caso alguma task foi interrompida no meio de uma funcao do semaforo
	s = NULL;
	return 0;
}

//============== BARREIRA =========
int barrier_create (barrier_t *b, int N)
{
	if(b == NULL){//se nao tem semaforo, retorna erro
		#ifdef DEBUG
		printf("barrier_create: Barreira invalida. ERRO.\n");
		#endif
		return -1;
	}

	#ifdef DEBUG
	printf("barrier_create: Criando nova barreira.\n");
	#endif
	b->N = N;
	b->fila = NULL;

	return 0;
}

int barrier_join (barrier_t *b)
{
	task_t* aux;
	if(b == NULL){
		#ifdef DEBUG
		printf("barrier_join: Barreira requisitada invalida. ERRO.\n");
		#endif
		return -1;
	}

	#ifdef DEBUG
	printf("barrier_join: Chegando na barreira.\n");
	#endif
	
	if(queue_size((queue_t*)b->fila)+1 < b->N){//suspender tarefa
		#ifdef DEBUG
		printf("barrier_join: Barreira ainda tem espaco. Suspendendo tarefa <%d>.\n", CurrentTask->tid);
		#endif
		CurrentTask->barreira = 1;//esperando barreira
		queue_append((queue_t **) &b->fila, (queue_t*) CurrentTask);
		task_yield();

		if(CurrentTask->barreira == 1){//retornou do semaforo destruido
			return -1;			
		}
	}else {//liberar barreira	
		while(queue_size((queue_t*)b->fila) > 0){
		#ifdef DEBUG
		printf("barrier_join: Liberando barreira.\n");
		#endif
		aux = (task_t*) queue_remove ((queue_t **) &b->fila, (queue_t*) b->fila);
		queue_append((queue_t **) &FilaReadyTask, (queue_t*) aux);
		}
	}

	return 0;
}

int barrier_destroy (barrier_t *b)
{
	task_t* aux;
	if(b == NULL){
		#ifdef DEBUG
		printf("barrier_destroy: Barreira requisitada invalida. ERRO.\n");
		#endif
		return -1;
	}

	#ifdef DEBUG
	printf("barrier_destroy: Acordando as tarefas.\n");
	#endif

	while(queue_size((queue_t*)b->fila) > 0){
		aux = (task_t*) queue_remove ((queue_t **) &b->fila, (queue_t*) b->fila);
		queue_append((queue_t **) &FilaReadyTask, (queue_t*) aux);
	}
	return 0;
}

//=========== MENSAGENS FILA ===========
int mqueue_create (mqueue_t *queue, int max, int size)
{
	if(queue == NULL){//se nao tem semaforo, retorna erro
		#ifdef DEBUG
		printf("mqueue_create: fila de mensagem invalida. ERRO.\n");
		#endif
		return -1;
	}

	#ifdef DEBUG
	printf("mqueue_create: Criando nova fila de mensagem.\n");
	#endif
	//iniciando as variaveis
	queue->maxmsg = max;
	queue->size = size;
	queue->read_pointer = 0;//ponteiro de leitura comeca no indice 0
	queue->write_pointer = 0;//ponteiro de escrever comeca no indice 0
	queue->fila_msg = malloc(max*size);//buffer circular
	if(sem_create(&queue->sem_vagas, max) < 0){//iniciando semaforo
		#ifdef DEBUG
		printf("mqueue_create: Erro ao criar semaforo de vagas. ERRO.\n");
		#endif
		return -1;
	}
	if(sem_create(&queue->sem_acesso, 1) < 0){//iniciando semaforo
		#ifdef DEBUG
		printf("mqueue_create: Erro ao criar semaforo de acesso. ERRO.\n");
		#endif
		return -1;
	}
	if(sem_create(&queue->sem_itens, 0) < 0){//iniciando semaforo
		#ifdef DEBUG
		printf("mqueue_create: Erro ao criar semaforo de itens. ERRO.\n");
		#endif
		return -1;
	}
	return 0;
}

int mqueue_send (mqueue_t *queue, void *msg){
	int i;
	void *dest,*orig;
	if(queue == NULL){//se nao tem fila de msg, retorna erro
		#ifdef DEBUG
		printf("mqueue_send: fila de mensagem invalida. ERRO.\n");
		#endif
		return -1;
	}
	
	if(sem_down(&queue->sem_vagas) < 0){//requisita o semaforo pra ver se tem vaga
		#ifdef DEBUG
		printf("mqueue_send: fila de mensagem nao pode mais receber msgs. ERRO.\n");
		#endif
		//semaforo deu erro ou fila de msg nao pode mais receber msgs
		return -1;
	}

	if(sem_down(&queue->sem_acesso) < 0){
		#ifdef DEBUG
		printf("mqueue_send: fila de mensagem sendo usada por outro processo. ERRO.\n");
		#endif
		//semaforo deu erro ou fila de msg ja esta sendo usada
		return -1;
	}
	
	#ifdef DEBUG
	printf("mqueue_send: Mensagem enviada para a fila.\n");
	#endif
	i = queue->write_pointer * queue->size;//indice do ponteiro de escrever
	#ifdef DEBUG
	printf("mqueue_send: indice <%d> e MAX <%d>.\n", queue->write_pointer, queue->maxmsg);
	#endif
	mqueue_t* aux_fila = (mqueue_t*)queue->fila_msg;//type cast para nao dar warning de dereference
	dest  = &aux_fila[i];//endereco da variavel de destino da mensagem
	orig = msg; //endereco da variavel de origem da mensagem
	memcpy(dest,orig,queue->size);//copia queue->size bytes da mensagem em orig para o dest
	//move o ponteiro de escrever
	queue->write_pointer = (queue->write_pointer+1) % queue->maxmsg;//avanca o ponteiro

	//add item no semaforo para indicar que a fila pode ser lida
	if(sem_up(&queue->sem_itens) < 0){
		#ifdef DEBUG
		printf("mqueue_send: Semaforo de itens nao pode ser liberado. ERRO.\n");
		#endif
		return -1;
	}
	//libera o acesso a fila de msg
	if(sem_up(&queue->sem_acesso) < 0){
		#ifdef DEBUG
		printf("mqueue_send: Semaforo de acesso nao pode ser liberado. ERRO.\n");
		#endif
		return -1;
	}
	return 0;	
}

int mqueue_recv (mqueue_t *queue, void *msg)
{
	int i;
	void *dest,*orig;
	if(queue == NULL){//se nao tem fila de msg, retorna erro
		#ifdef DEBUG
		printf("mqueue_recv: fila de mensagem invalida. ERRO.\n");
		#endif
		return -1;
	}
	
	if(sem_down(&queue->sem_itens) < 0){//verifica se tem item pra ser lido na fila de msg
		#ifdef DEBUG
		printf("mqueue_recv: fila de mensagem nao possui nenhuma msg. ERRO.\n");
		#endif
		//semaforo deu erro ou fila de msg nao tem msg
		return -1;
	}

	if(sem_down(&queue->sem_acesso) < 0){
		#ifdef DEBUG
		printf("mqueue_recv: fila de mensagem sendo usada por outro processo. ERRO.\n");
		#endif
		//semaforo deu erro ou fila de msg ja esta sendo usada
		return -1;
	}

	#ifdef DEBUG
	printf("mqueue_recv: Recebendo mensagem da fila.\n");
	#endif
	mqueue_t* aux_fila = (mqueue_t*)queue->fila_msg;//type cast para nao dar warning de dereference
	i = queue->read_pointer * queue->size;//indice do ponteiro de leitura
	dest  = msg;//endereco da variavel de destino da mensagem
	orig = &(aux_fila[i]); //endereco da variavel de origem da mensagem
	memcpy(dest,orig,queue->size);//copia queue->size bytes da mensagem em orig para o dest

	//move o ponteiro de leitura
	queue->read_pointer = (queue->read_pointer+1) % queue->maxmsg;
	//libera item no semaforo para indicar que liberou vaga
	if(sem_up(&queue->sem_vagas) < 0){
		#ifdef DEBUG
		printf("mqueue_send: Semaforo de vagas nao pode ser liberado. ERRO.\n");
		#endif
		return -1;
	}
	//libera o acesso a fila de msg
	if(sem_up(&queue->sem_acesso) < 0){
		#ifdef DEBUG
		printf("mqueue_send: Semaforo de acesso nao pode ser liberado. ERRO.\n");
		#endif
		return -1;
	}
	return 0;
}

int mqueue_destroy (mqueue_t *queue)
{
	#ifdef DEBUG
	printf("mqueue_destroy: destruindo fila de mensagem.\n");
	#endif

	if(queue == NULL){//se nao tem fila de msg, retorna erro
		#ifdef DEBUG
		printf("mqueue_destroy: fila de mensagem invalida. ERRO.\n");
		#endif
		return -1;
	}

	//destruindo a fila
	queue->maxmsg = 0;
	queue->size = 0;
	queue->read_pointer = 0;//ponteiro de leitura comeca no indice 0
	queue->write_pointer = 0;//ponteiro de escrever comeca no indice 0
	free(queue->fila_msg);
	#ifdef DEBUG
	printf("mqueue_destroy: destruindo semaforos da fila.\n");
	#endif
	if(sem_destroy(&queue->sem_vagas) < 0){//destruindo semaforo
		#ifdef DEBUG
		printf("mqueue_destroy: Erro ao destruir semaforo de vagas. ERRO.\n");
		#endif
		return -1;
	}
	if(sem_destroy(&queue->sem_acesso) < 0){//destruindo semaforo
		#ifdef DEBUG
		printf("mqueue_destroy: Erro ao destruir semaforo de acesso. ERRO.\n");
		#endif
		return -1;
	}
	if(sem_destroy(&queue->sem_itens) < 0){//destruindo semaforo
		#ifdef DEBUG
		printf("mqueue_destroy: Erro ao destruir semaforo de itens. ERRO.\n");
		#endif
		return -1;
	}
	queue = NULL;
	return 0;
}

int mqueue_msgs (mqueue_t *queue)
{
	semaphore_t *aux;
	#ifdef DEBUG
	printf("mqueue_msgs: Recuperando numero de mensagens na fila.\n" );
	#endif

	if(queue == NULL){//se nao tem fila de msg, retorna erro
		#ifdef DEBUG
		printf("mqueue_msgs: fila de mensagem invalida. ERRO.\n");
		#endif
		return -1;
	}
	
	aux = &queue->sem_itens;//semaforo contendo o numero de mensagens na fila
	return aux->contador;
}
