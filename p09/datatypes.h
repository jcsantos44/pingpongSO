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
  struct task_t *joined ; //joined task
  int tid ; // ID da tarefa
  int static_prio; //prioridade estática
  int aging_prio; //prioridade dinamica
  int exec_start; //valor do clock quando a tarefa foi criada
  int proc_time; //tempo de processamento
  int activations; //quantidade de ativacoes
  int suspend;
  ucontext_t context;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
