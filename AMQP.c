#include "AMQP.h"

pthread_cond_t read_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_read = PTHREAD_MUTEX_INITIALIZER;

// In Connection this is not a problem as only the main Thread does connection
// calls. In further classes, one should use Thread Local Storage Link for
// further info: https://en.wikipedia.org/wiki/Thread-local_storage
static Grammar contextful_grammar = NULL;

enum State {
  None = 0,
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

    pthread_mutex_lock(&mutex_read);
    printf("n = %d\n", n);
    args->n = n;
    pthread_mutex_unlock(&mutex_read);
    pthread_cond_broadcast(&read_cond);
  }
  pthread_cond_broadcast(&read_cond);
  return NULL;
}

void debug_packet_struct(packet_struct *packet) {
  printf("\nPacket type = ");
  switch (packet->type) {
  case METHOD:
    printf("METHOD");
    break;
  case HEADER:
    printf("HEADER");
    break;
  case BODY:
    printf("BODY");
    break;
  case HEARTBEAT:
    printf("HEARTBEAT");
    break;
  case NONE:
    printf("NONE");
    break;
  default:
    printf("%d", packet->type);
    break;
  }
  printf("\n");
  printf("Channel = %d\n", packet->channel);
  printf("Length = %d\n", packet->size);
  switch (packet->type) {
  case METHOD:
    printf(" Class ID: %x\n", packet->method_payload->class_id);
    printf(" Method ID: %x\n", packet->method_payload->method_id);
    for (int i = 0; i < packet->method_payload->arguments_length; i++) {
      printf("%02x ",
             (unsigned char)packet->method_payload->arguments_byte_array[i]);
      if ((i + 1) % 16 == 0)
        printf("\n");
    }
    break;
  case HEADER:
    printf("HEADER");
    break;
  case BODY:
    printf("BODY");
    break;
  case HEARTBEAT:
    printf("HEARTBEAT");
    break;
  case NONE:
    printf("NONE");
    break;
  }
  printf("\n");
}

packet_struct *wait_response(int *ext_n, listener_struct *listener_args) {

  int n;

  pthread_mutex_lock(&mutex_read);
  while ((n = listener_args->n) == 0) {
    pthread_cond_wait(&read_cond, &mutex_read);
  }

  packet_struct *packet = break_packet(listener_args->recvline);
  listener_args->n -= packet->size + 7;
  if (listener_args->n < 0)
    listener_args->n = 0;
  debug_packet_struct(packet);

  pthread_mutex_unlock(&mutex_read);

  printf("Read!\n");
  return packet;
}

enum State packet_decider(packet_struct *packet, enum State current_state,
                          int sockfd) {
  int size = 0;
  unsigned char *sent_packet;
  enum State next_state = current_state;

  if (packet == NULL) {
    sent_packet = decode_rule("amqp", &size, NULL);
    next_state = ConnectionStart;
  } else {
    if (packet->type == NONE)
      return None;
    if (packet->type == METHOD) {
      switch (packet->method_payload->class_id) {
      case CONNECTION:
        sent_packet = connection_packet_decider(packet->method_payload,
                                                (int *)&next_state, &size);
        break;
      case CHANNEL:
        // Delegate
        break;
      case EXCHANGE:
        // Delegate
        break;
      case QUEUE:
        // Delegate
        break;
      case BASIC:
        // Delegate
        break;
      case TX:
        // Delegate
        break;
      default:
        printf("Class not defined, invalid packet or broken packet parsing!\n");
        break;
      }
    } else if (packet->type == HEADER) {
      // Only if consuming
    } else if (packet->type == BODY) {
      // Only if consuming
    }
  }
  send_packet(sockfd, sent_packet, size);
  return current_state;
}

int fuzz(int sockfd) {

  pthread_t listener_thread;
  pthread_cond_t nova_msg = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;

  int n;

  grammar_init("../grammar/grammar-spec", "../grammar/grammar-abnf");

  listener_struct *listener_args = malloc(sizeof(listener_struct));

  listener_args->socket_fd = sockfd;
  listener_args->n = 0;

  pthread_create(&listener_thread, NULL, listener, listener_args);

  current_state = packet_decider(NULL, current_state, sockfd);

  packet_struct *packet;
  packet = wait_response(&n, listener_args);

  current_state = packet_decider(packet, current_state, sockfd);
  packet = wait_response(&n, listener_args);

  while (current_state != None) {
    current_state = packet_decider(packet, current_state, sockfd);
    packet = wait_response(&n, listener_args);
  }
  pthread_join(listener_thread, NULL);

  return 0;
}
