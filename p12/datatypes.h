// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

/**
* Projeto de SO 2019/2
* Alunos:
* Nelson Augusto Pires
* João Carlos Cardoso Santos
**/

#ifndef __DATATYPES__
#define __DATATYPES__

#include <ucontext.h>

// Estrutura que define uma tarefa
typedef struct task_t
{
  struct task_t *prev, *next ; // para usar com a biblioteca de filas (cast)
  int tid ; // ID da tarefa
  int static_prio; //prioridade estática
  int aging_prio; //prioridade dinamica
  int exec_start; //valor do clock quando a tarefa foi criada
  int proc_time; //tempo de processamento
  int activations; //quantidade de ativacoes
  struct task_t *wJoin; //esperando a tarefa wJoin terminar
  int exitCode; //Codigo com o qual a tarefa foi terminado. <0 = Erro / 9999 = nao terminado / >0 = Terminado
	int wakeTime; //valor do clock quando a tarefa deve ser acordada. 0 = acordada
	int semaforo; //esperando um semaforo. 0 = nao esperando / 1 = esperando
	int barreira; //esperando na barreira. 0 = nao esperando / 1 = esperando
  ucontext_t context;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  int contador; //contador do semaforo
	struct task_t *fila; //fila de tasks aguardando o semaforo	
	int destruido; //flag indica se foi destruido
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  int N;//numero maximos de tasks na barreira
	struct task_t *fila;
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  int maxmsg; //numero maximo de mensagens
	int size; //tamanho da mensagem em bytes
	int read_pointer; //indice do ponteiro de leitura do buffer circular
	int write_pointer;//indice do ponteiro de escrever do buffer circular
	void *fila_msg;//fila de mensagens (Buffer Circular)
	semaphore_t sem_vagas;//semaforo que indica se a fila de msg pode receber mais msg
	semaphore_t sem_acesso;//semaforo que indica se a fila de msg pode ser acessada(read/write)
	semaphore_t sem_itens;//semaforo que indica se a fila de msg tem alguma msg
} mqueue_t ;

#endif
