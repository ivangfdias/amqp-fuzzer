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

void debug_packet_struct(packet_struct* packet){
	    printf("\nPacket type = ");
	    switch(packet->type){
		case METHOD: printf("METHOD"); break;
		case HEADER: printf("HEADER"); break;
		case BODY:   printf("BODY");   break;
		case HEARTBEAT: printf("HEARTBEAT"); break;
		case NONE: printf("NONE"); break;
		default: 
			   printf("%d", packet->type);break;
	    }
	    printf("\n");
	    printf("Channel = %d\n", packet->channel);
	    printf("Length = %d\n", packet->size);
	    switch(packet->type){
	        case METHOD: printf(" Class ID: %x\n", packet->method_payload->class_id);
			     printf(" Method ID: %x\n", packet->method_payload->method_id);
			     for (int i = 0; i < packet->method_payload->arguments_length; i++){
				printf("%02x", packet->method_payload->arguments_byte_array[i]);
			     }
		             break;
		case HEADER: printf("HEADER");
			     break;
		case BODY:   printf("BODY");
			     break;
		case HEARTBEAT: printf("HEARTBEAT");
			     break;
		case NONE: printf("NONE");
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

  debug_packet_struct(packet);

  pthread_mutex_unlock(&mutex_read);

  printf("Read!\n");
  return packet;
}

unsigned char* shortstring_generator (int* length, Grammar environment){
	unsigned char shortstring_length = (rand() % RULE_REPETITION_INFTY) % 256;

	unsigned char* result = calloc (shortstring_length + 1, sizeof(char));

	result[0] = shortstring_length;
	int length_dummy = 0;
	for (int i = 1; i < shortstring_length  +1; i++){
		result[i] = decode_rule("OCTET", &length_dummy, environment)[0];
	}
	*length = shortstring_length + 1;
	return result;
}

unsigned char* longstring_generator (int* length, Grammar environment){
	unsigned int string_length = (rand() % RULE_REPETITION_INFTY);
	unsigned char* result = calloc (string_length + 4, sizeof(char));

	int_in_char(result, string_length, 0, string_length + 4);
	int length_dummy = 0;
	for (int i = 4; i < string_length + 4; i++){
		result[i] = decode_rule("OCTET", &length_dummy, environment)[0];
	}
	*length = string_length + 4;
	return result;
}

void overwrite_rule_set_length(char* rule_to_decode, char* length_rule, int length_size, Grammar grammar){

    int decoded_rule_length = 0;
    unsigned char *decoded_rule =
        decode_rule(rule_to_decode, &decoded_rule_length,
                    contextful_grammar);

    grammar_insert(grammar, rule_to_decode,
                   new_grammar_entry_t(BYTE_ARRAY,
                                       (char *)decoded_rule, NULL,
                                       decoded_rule_length, NULL));

    char *decoded_length_literal = calloc(length_size, sizeof(char));
    int_in_char((unsigned char *)decoded_length_literal,
                decoded_rule_length, 0, length_size);

    grammar_insert(
        grammar, length_rule,
        new_grammar_entry_t(BYTE_ARRAY, decoded_length_literal, NULL, length_size, NULL));
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
    /* SETTING UP CONNECTION GRAMMAR */
    contextful_grammar = generate_grammar("../grammar/grammar-connection");

    grammar_insert(contextful_grammar, "short-string", new_function_grammar_entry(shortstring_generator));

    grammar_insert(contextful_grammar, "long-string", new_function_grammar_entry(longstring_generator));;

    /* SETTING UP METHOD MESSAGE */
    grammar_insert(contextful_grammar, "method-id", new_string_grammar_entry("m11-method-id"));

    grammar_insert(contextful_grammar, "method-properties", new_string_grammar_entry("m11-method-properties"));

    /* GOING THROUGH METHOD SPECIFIC STUFF */
    overwrite_rule_set_length("client-properties-payload", "client-properties-length", 4, contextful_grammar);
    
    /* PLAIN AUTHENTICATION METHOD */
    grammar_insert(contextful_grammar, "authcid", new_string_grammar_entry("\"username\""));

    grammar_insert(contextful_grammar, "passwd", new_string_grammar_entry("\"password\""));

    overwrite_rule_set_length("message", "message-length", 4, contextful_grammar);

    /* PREPARING METHOD PACKET */
    overwrite_rule_set_length("method-payload", "payload-size", 4, contextful_grammar);

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
