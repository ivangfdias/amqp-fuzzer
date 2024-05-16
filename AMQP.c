#include "AMQP.h"

#include <pthread.h>

#define MAXLINE 4096



// Necessario para os argumentos de listener
typedef struct {
  int socket_fd; // fd do socket
  char recvline[MAXLINE + 1]; // Pacote recebido
  int n; // qtd de bytes do pacote i guess?
  pthread_cond_t *read_cond; // condicao para liberar threads q precisam ouvir a resposta antes de prosseguir TODO fazer global
  pthread_mutex_t *cond_mutex; // mutex para acesso aa variavel de condicao TODO fazer global
} listener_struct;

void *listener(void *void_args) {

  listener_struct *args = void_args;
  int sockfd = args->socket_fd;

  int n;
  char recvline[MAXLINE + 1]; // Mensagem recebida do servidor

  // Le a mensagem recebida no socket
  while ((n = read(sockfd, args->recvline, MAXLINE) > 0)) {
    args->recvline[n] = 0;
    
    printf("Travando mutex...\n");
    pthread_mutex_lock(args->cond_mutex);
    printf("Mudando o n...\n");
    args->n = n;
    printf("Destravando mutex...\n");
    pthread_mutex_unlock(args->cond_mutex);
    printf("Liberando condicao...\n");
    pthread_cond_broadcast(args->read_cond);
  }
    printf("Liberando condicao pq morri...\n");
  pthread_cond_broadcast(args->read_cond); // destrava a condicao qdo morre

  return NULL;
}



char* wait_response(int* ext_n, listener_struct* listener_args ){

    int n;

  pthread_mutex_lock(listener_args->cond_mutex);
  while( (n = listener_args->n) == 0){
    pthread_cond_wait(listener_args->read_cond, listener_args->cond_mutex);
  }
  printf("Recebi resposta do servidor e posso seguir!\n");

  n = listener_args->n;
  printf("%d vs %d\n", listener_args->n, n);
  printf("\n");
  pthread_mutex_unlock(listener_args->cond_mutex);

  *ext_n = n;
  return NULL;
  // TODO copiar o packet para um char mallocado e retornar
}

int send_protocol_header(int sockfd) {

  char *protocol_header = calloc(8, sizeof(char));

  memcpy(protocol_header, "AMQP", 4);
  protocol_header[6] = 9;
  protocol_header[7] = 1;

  // Escreve a mensagem no socket
  write(sockfd, protocol_header, 8);

  return 0;
}
void parse_packet (char* packet){
    printf("TODO!\n");
}

int fuzz(int sockfd) {

  pthread_t listener_thread;
  pthread_cond_t nova_msg = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;

  int n;
  char received_packet[MAXLINE + 1];


  listener_struct *listener_args = malloc(sizeof(listener_struct *));

  listener_args->socket_fd = sockfd;
  listener_args->read_cond = &nova_msg;
  listener_args->cond_mutex = &msg_mutex;
  listener_args->n = 0;

  pthread_create(&listener_thread, NULL, listener, listener_args);
  
  send_protocol_header(sockfd);
  

  n = 0;
  pthread_mutex_lock(&msg_mutex);
  while( (n = listener_args->n) == 0){
    pthread_cond_wait(&nova_msg, &msg_mutex);
  }
  printf("Recebi resposta do servidor e posso seguir!\n");

  n = listener_args->n;
  printf("%d vs %d\n", listener_args->n, n);
  printf("\n");
  pthread_mutex_unlock(&msg_mutex);

  pthread_join(listener_thread, NULL);

  return 0;
}
