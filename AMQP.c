
#include "AMQP.h"
#include "Packet.h"
#include "utils.h"
#include <pthread.h>

#define MAXLINE 4096

pthread_cond_t read_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_read = PTHREAD_MUTEX_INITIALIZER;

enum State {
  None,
  ConnectionStart,
  ConnectionTune,
  ConnectionSecure,
  ConnectionOpen
};

enum State current_state = None;

// Necessario para os argumentos de listener
typedef struct {
  int socket_fd;                       // fd do socket
  unsigned char recvline[MAXLINE + 1]; // Pacote recebido
  int n;                               // qtd de bytes do pacote i guess?
} listener_struct;

void *listener(void *void_args) {

  listener_struct *args = (listener_struct *)void_args;
  int sockfd = args->socket_fd;

  int n;
  char recvline[MAXLINE + 1]; // Mensagem recebida do servidor

  // Le a mensagem recebida no socket
  while ((n = read(sockfd, args->recvline, MAXLINE) > 0)) {
    args->recvline[n] = 0;

    //    pthread_mutex_lock(args->cond_mutex);
    pthread_mutex_lock(&mutex_read);
    args->n = n;
    pthread_mutex_unlock(&mutex_read);
    //    pthread_mutex_unlock(args->cond_mutex);
    //    pthread_cond_broadcast(args->read_cond);
    pthread_cond_broadcast(&read_cond);
  }
  pthread_cond_broadcast(&read_cond);
  //  pthread_cond_broadcast(args->read_cond); // destrava a condicao qdo morre

  return NULL;
}

packet_struct *wait_response(int *ext_n, listener_struct *listener_args) {

  packet_struct* packet = malloc(sizeof(packet_struct));
  int n;

  pthread_mutex_lock(&mutex_read);
  while ((n = listener_args->n) == 0) {
    pthread_cond_wait(&read_cond, &mutex_read);
  }

  break_packet(listener_args->recvline, packet);

  pthread_mutex_unlock(&mutex_read);

  return packet;
}

// TODO move to Connectivity ?
int send_protocol_header(int sockfd) {

  char *protocol_header = calloc(8, sizeof(char));

  memcpy(protocol_header, "AMQP", 4);
  protocol_header[6] = 9;
  protocol_header[7] = 1;

  // Escreve a mensagem no socket
  write(sockfd, protocol_header, 8);
  free(protocol_header);

  return 0;
}

enum State packet_decider(packet_struct* packet, enum State current_state){
    if (packet->type != 1)
        return current_state;

    printf("TODO");
    return current_state;
}

int fuzz(int sockfd) {

  pthread_t listener_thread;
  pthread_cond_t nova_msg = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;

  int n;

  listener_struct *listener_args = malloc(sizeof(listener_struct));

  listener_args->socket_fd = sockfd;
  listener_args->n = 0;

  pthread_create(&listener_thread, NULL, listener, listener_args);

  send_protocol_header(sockfd);

  packet_struct *packet;
  packet = wait_response(&n, listener_args);

  current_state = packet_decider(packet, current_state);
  packet = wait_response(&n, listener_args);

  while(current_state != None)
      current_state = packet_decider(packet, current_state);
  pthread_join(listener_thread, NULL);

  return 0;
}
