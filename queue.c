/**
* Projeto de SO 2019/2
* Alunos:
* Nelson Augusto Pires
* João Carlos Cardoso Santos
**/

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

// PARTE 1: queue_append e queue_size ====================================================

void queue_append (queue_t **queue, queue_t *elem)
{
  #ifdef DEBUG
  printf("Adicionando na fila\n");
  #endif

  queue_t* aux; //endereco do elemento que ainda nao foi add na fila
  queue_t* first; //endereco do primeiro elementa da fila

  //-------------------_CONDICOES_-------------------------

  //queue aponta para o endereco do endereco do primeiro elemento de uma fila
  //*queue aponta para o endereco do primeiro elemento de uma fila (desreferencia)
  if (queue == NULL){ //
    printf("Fila inexistente\n");
    return;
  }else{
    //fila existe -> pegar endereco do primeiro elemento dela
    first = *queue;
  }

  //endereco do elemento a ser adicionado
  if(elem == NULL){//se o end do elem eh nulo, quer dizer que o elemento nao existe
    printf("Elemento inexistente\n");
    return;
  }else{//elemento existe -> verificar se esta isolado(p00-biblioteca-filas.pdf)
    if(elem->next != NULL || elem->prev != NULL){//se o elemento seguinte ou o elemento anterior existir, elemento nao esta isolado(ja faz parte de uma fila)
      printf("Elemento nao esta isolado (Esta em outra fila)\n");
      return;
    }else{//elemento existe e esta isolado -> atribui a aux(p00-biblioteca-filas.pdf)
      aux = elem;
    }
  }

  //-------------_ADD NA FILA_--------------------

  #ifdef DEBUG
  printf("Aqui\n");
  #endif
  if(first == NULL){//primeiro elemento da fila nao existe(fila esta vazia)
    #ifdef DEBUG
    printf("Aqui\n");
    #endif
    first = aux; //atribui o elemento adicionado ao primeiro elemento da fila(endereco)
    //first nao esta mais apontando pro endereco de *queue(o primeiro), entao tem que fazer *queue apontar para o endereco de first
    *queue = first;
    //fila circular duplamente encadeada(p00-biblioteca-filas.pdf)
    first->next = first;
    first->prev = first;
    #ifdef DEBUG
    printf("Aqui\n");
    #endif
    return;
  }else{//fila nao esta vazia -> add ao final dela
    //pegar o ultimo elemento da fila
    queue_t* last;
    last = first->prev;

    //o seguinte ao novo elemento deve ser o primeiro elemento da fila
    elem->next = first;
    //o anterior ao novo elemento deve ser o ultimo elemento da fila
    elem->prev = last;

    //novo elemento eh o seguinte do ultimo
    last->next = elem;
    //novo elemento eh o anterior do primeiro
    first->prev = elem;
    return;
  }
}

int queue_size (queue_t *queue) {
//queue endereco do primeiro elemento da fila
  if(queue == NULL){ //fila vazia(primeiro elemento da fila nao existe)
    return 0;
  }else{//fila nao vazia
    int size = 1;//tem pelo menos o primeiro elemento
    queue_t* aux;
    aux = queue; //aux guarda o endereco do primeiro elemento
    //percorrer a fila ate chegar ao primeiro elemento
    while(queue->next != aux){
      size++;
      queue = queue->next;
    }
    return size;
  }
}

// PARTE 2: queue_remove ====================================================
//retorna o endereco de uma var do tipo queue_t
queue_t *queue_remove (queue_t **queue, queue_t *elem){

  int eFila = 0;//pertence a fila? 0 - nao; 1 - sim
  queue_t* aux; //endereco do elemento removido da fila
  queue_t* first; //endereco do primeiro elementa da fila

  //-------------------_CONDICOES_-------------------------

  //queue aponta para o endereco do endereco do primeiro elemento de uma fila
  //*queue aponta para o endereco do primeiro elemento de uma fila (desreferencia)
  if (queue == NULL){ //
    printf("Fila inexistente.\n");
    return NULL;
  }else{
    //fila existe -> pegar endereco do primeiro elemento dela
    first = *queue;
  }

  if(first == NULL){//primeiro elemento da fila nao existe(fila esta vazia)
    printf("Fila vazia!.\n");
    return NULL;
  }

  //endereco do elemento a ser adicionado
  if(elem == NULL){//se o end do elem eh nulo, quer dizer que o elemento nao existe
    printf("Elemento inexistente.\n");
    return NULL;
  }

  if(elem->next == NULL || elem->prev == NULL){//se o elemento seguinte ou o elemento anterior nao existir, elemento esta isolado(nao faz parte de uma fila)
      printf("Elemento nao faz parte de uma fila.\n");
      return NULL;
    }else{//elemento pertence a uma fila
      aux = elem;
      //percorrer a fila ate chegar no elemento removido
      queue_t* aux_queue = first; //aux_queue guarda o endereco do elemento atual que estamos. Comeca do primeiro
      do{
        if (aux_queue == aux){//elemento esta na fila
          eFila = 1;
          break;
        }else{//vai pro proximo
          aux_queue = aux_queue->next;
        }
      }while(aux_queue != first);//enquanto nao voltar pra primeira
    }

    //-------------_REMOVER DA FILA_--------------------

    if(eFila == 0){//se nao pertence a fila
      printf("Elemento nao pertence a fila.\n");
      return NULL;
    }else{//se pertence a fila, remove
      if(queue_size (first) > 1 ){//fila tem mais de um elemento
        if(first == elem){//remover o primeiro?
          *queue = first->next;//primeiro elemento vira o proximo
          first = *queue;
        }
        //no elemento anterior do que esta sendo removido, tem que apontar para o elemento seguinte do que esta sendo removido
        (elem->prev)->next = aux->next;
        //no elemento seguinte do que esta sendo removido, tem que apontar para o elemento anterior do que esta sendo removido
        (elem->next)->prev = aux->prev;
        //isola o elemento removido
        aux->next = NULL;
        aux->prev = NULL;

        //retorna apontador para o elemento removido
        return aux;
      }else{//fila tem apenas 1 elemento -> fila fica vazia
        first = NULL;//primeiro elemento da fila aponta para nada(fila vazia)
        //first nao esta mais apontando pro endereco de *queue(o primeiro), entao tem que fazer *queue apontar para o endereco de first
        *queue = first;

        //isola o elemento removido
        aux->next = NULL;
        aux->prev = NULL;

        //retorna apontador para o elemento removido
        return aux;
      }
    }

}


// PARTE 4: queue_print ====================================================
void queue_print (char *name, queue_t *queue, void print_elem (void*) ){
//queue endereco do primeiro elemento da fila
  if(queue == NULL){ //fila vazia(primeiro elemento da fila nao existe)
    printf("[].\n");
  }else{
    printf("tamanho da fila: %d\n",queue_size(queue));
    printf("%s: [",name);
    //percorrer a fila
    queue_t* aux;
    aux = queue; //aux guarda o endereco do primeiro elemento
    //percorrer a fila e imprime o elemento ate chegar ao primeiro elemento
    do{
      print_elem(queue);
      queue = queue->next;
      printf(" ");
    }
    while(queue != aux);

    printf("]\n");

  }
}
