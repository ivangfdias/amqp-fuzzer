#include "AMQP.h"
#include "../grammar/packet-generator.h"
#include "Packet.h"
#include "utils.h"
#include <pthread.h>

#define MAXLINE 4096

pthread_cond_t read_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_read = PTHREAD_MUTEX_INITIALIZER;

static Grammar contextful_grammar = NULL;

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

  packet_struct *packet = malloc(sizeof(packet_struct));
  int n;

  pthread_mutex_lock(&mutex_read);
  while ((n = listener_args->n) == 0) {
    pthread_cond_wait(&read_cond, &mutex_read);
  }

  // break_packet(listener_args->recvline, packet);

  pthread_mutex_unlock(&mutex_read);

  printf("Read!\n");
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

enum State packet_decider(packet_struct *packet, enum State current_state,
                          int sockfd) {
  int size = 0;
  unsigned char *sent_packet;
  enum State next_state = current_state;
  if (packet == NULL) {
    sent_packet = decode_rule("amqp", &size, NULL);
    next_state = ConnectionStart;
  } else {
    contextful_grammar = generate_grammar("../grammar/grammar-connection");


    /*  TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO 
     *
     * Olá Ivan!
     *  
     * Você está vendo aqui que há uma BAGUNÇA nessa seção.
     *
     * Podemos ver que você por várias vezes realiza um procedimento comum:
     * - Gera uma mensagem
     * - Captura o comprimento
     * - Transforma o comprimento em 4 octetos
     * - Substitui as regras da mensagem do comprimento
     *
     * Vamos fazer uma função pra isso meu querido? <3
     *
     *  TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO 
     */
    grammar_insert(contextful_grammar, "method-id",
                   new_grammar_entry_t(STRING, "m11-method-id", NULL, 0));

    grammar_insert(
        contextful_grammar, "method-properties",
        new_grammar_entry_t(STRING, "m11-method-properties", NULL, 0));

    int longstringlen = 0;
    unsigned char *longstring =
        parse_elements("*OCTET", &longstringlen, contextful_grammar);

    printf("longstringlen = %d\n", longstringlen);
    unsigned char *newstr = calloc(longstringlen + 4, sizeof(char));
    longstringlen -= 3;
    int_in_char(newstr, longstringlen, 0, longstringlen + 4);
    memcpy(newstr + 4, longstring, longstringlen);

    grammar_insert(contextful_grammar, "product-value",
                   new_grammar_entry_t(BYTE_ARRAY, (char *)newstr, NULL,
                                       longstringlen + 4));

    grammar_insert(contextful_grammar, "version-value",
                   new_grammar_entry_t(BYTE_ARRAY, (char *)newstr, NULL,
                                       longstringlen + 4));

    grammar_insert(contextful_grammar, "platform-value",
                   new_grammar_entry_t(BYTE_ARRAY, (char *)newstr, NULL,
                                       longstringlen + 4));
    int method_properties_length = 0;
    unsigned char *method_properties_payload =
        decode_rule("client-properties-payload", &method_properties_length,
                    contextful_grammar);

    grammar_insert(contextful_grammar, "client-properties-payload",
                   new_grammar_entry_t(BYTE_ARRAY,
                                       (char *)method_properties_payload, NULL,
                                       method_properties_length));
    char *properties_length_literal = calloc(4, sizeof(char));
    int_in_char((unsigned char *)properties_length_literal,
                method_properties_length, 0, 4);
    grammar_insert(
        contextful_grammar, "client-properties-length",
        new_grammar_entry_t(BYTE_ARRAY, properties_length_literal, NULL, 4));

    grammar_insert(contextful_grammar, "authcid", 
            new_grammar_entry_t(STRING, "\"username\"", NULL, 0));

    grammar_insert(contextful_grammar, "passwd", 
            new_grammar_entry_t(STRING, "\"password\"", NULL, 0));

    int message_length = 0;
    unsigned char *message = decode_rule("message", &message_length, contextful_grammar);

    unsigned char *message_length_literal = calloc(4, sizeof(char));
    int_in_char(message_length_literal, message_length, 0, 4);

    grammar_insert(
        contextful_grammar, "message",
        new_grammar_entry_t(BYTE_ARRAY, (char*) message, NULL, message_length));
    grammar_insert(
        contextful_grammar, "message-length",
        new_grammar_entry_t(BYTE_ARRAY, (char*) message_length_literal, NULL, 4));

    int payload_size = 0;
    unsigned char *payload =
        decode_rule("method-payload", &payload_size, contextful_grammar);

    printf("\n ==== PAYLOAD ==== \n");
    for (int i = 0; i < payload_size; i++) {
      printf("%02x", (unsigned char)payload[i]);
      if (((i + 1) % 16) == 0)
        printf("\n");
    }

    printf("\n= payload size = %d =\n", payload_size);
    grammar_entry_t *payload_g_e_t =
        new_grammar_entry_t(BYTE_ARRAY, (char *)payload, NULL, payload_size);

    grammar_insert(contextful_grammar, "method-payload", payload_g_e_t);

    char *payload_size_literal = calloc(4, sizeof(char));
    int_in_char((unsigned char *)payload_size_literal, payload_size, 0, 4);

    grammar_entry_t *payload_size_g_e_t =
        new_grammar_entry_t(BYTE_ARRAY, payload_size_literal, NULL, 4);

    grammar_insert(contextful_grammar, "payload-size", payload_size_g_e_t);


    sent_packet = decode_rule("method", &size, contextful_grammar);
  }
  printf("\n===\n");
  for (int i = 0; i < size; i++) {
    printf("%02x ", sent_packet[i]);
  }
  printf("\n");
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
  // send_protocol_header(sockfd);

  packet_struct *packet;
  packet = wait_response(&n, listener_args);

  current_state = packet_decider(packet, current_state, sockfd);
  packet = wait_response(&n, listener_args);

  while (current_state != None)
    current_state = packet_decider(packet, current_state, sockfd);
  pthread_join(listener_thread, NULL);

  return 0;
}
