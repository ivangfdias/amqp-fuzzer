#include "AMQP.h"
#define IMPLEMENTED_MESSAGES 6
pthread_cond_t read_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_read = PTHREAD_MUTEX_INITIALIZER;

// In Connection this is not a problem as only the main Thread does connection
// calls. In further classes, one should use Thread Local Storage Link for
// further info: https://en.wikipedia.org/wiki/Thread-local_storage
static Grammar contextful_grammar = NULL;

// Array of mutexes for channels, malloc'ed
// Array of packet_struct* for channels, malloc'ed

pthread_mutex_t *channel_mutexes = NULL;
pthread_cond_t *channel_conds = NULL;
packet_struct **channel_packets = NULL;
pthread_t *channels = NULL;
int max_created_threads = 0;

enum State {
  NOOP = -1,
  None = 0,
  ConnectionStart = 1,
  ConnectionTune = 2,
  ConnectionTuned = 3, // TODO: change ConnectionTune logic
  //  ConnectionSecure, unused
  ConnectionOpen = 4,
  Connected = 5,
  ConnectionClose = 6,
  ConnectionCloseOK = 7,
  ConnectionClosed = 8
};

enum State current_state = None;

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

  fuzz_debug_printf("Listener born\n");
  // Le a mensagem recebida no socket
  while (current_state != ConnectionClosed &&
         ((n = read(sockfd, args->recvline, MAXLINE) > 0) || errno == EAGAIN)) {
    if (n == 0 && errno == EAGAIN)
      continue;
    fuzz_debug_printf("Listener: Got something to parse\n");
    args->recvline[n] = 0;

    // Separating necessary for when AMQP frames come in "bundles"
    int parsed_n = 0;
    while (parsed_n < n) {

      fuzz_debug_printf("Listener: Received packet. Breaking... (%d/%d)\n",
                        parsed_n, n);
      packet_struct *read_packet = break_packet(args->recvline);
      int channel = read_packet->channel;
      pthread_mutex_lock(&channel_mutexes[channel]);
      channel_packets[channel] =
          realloc(channel_packets[channel], sizeof(packet_struct *));
      channel_packets[channel] = break_packet(args->recvline);
      pthread_mutex_unlock(&channel_mutexes[channel]);
      pthread_cond_signal(&channel_conds[channel]);
      parsed_n = read_packet->size + 7;
    }
    fuzz_debug_printf("n = %d\n", n);
    args->n = n;
  }
  fuzz_debug_printf("Listener exiting!\n");
  for (int i = 0; i < max_created_threads; i++) {
    pthread_cancel(channels[i]);
    pthread_cond_signal(&channel_conds[i]);
  }
  return NULL;
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

  fuzz_debug_printf("Read!\n");
  return packet;
}

unsigned char *amqp_header(enum State *next_state, int *size,
                           char *response_expected) {
  unsigned char *packet = decode_rule("amqp", size, NULL);
  *response_expected = 1;
  *next_state = ConnectionStart;
  for (int i = 0; i < *size; i++) {
    printf("%2x ", packet[i]);
    if ((i + 1) % 16)
      printf("\n");
  }
  return packet;
}

unsigned char *chaotic_packet_decider(packet_struct *packet,
                                      enum State *current_state_ptr,
                                      char *response_expected, int *size) {

  int roll = rand() % IMPLEMENTED_MESSAGES;

  switch (roll) {
  case 0:
    return amqp_header(current_state_ptr, size, response_expected);
    break;
  case 1:
    return connection_start_ok(current_state_ptr, size, response_expected);
    break;
  case 2:
    return connection_tune_ok(current_state_ptr, size, response_expected, NULL);
    break;
  case 3:
    return connection_open(current_state_ptr, size, response_expected);
    break;
  case 4:
    return connection_close(current_state_ptr, size, response_expected);
    break;
  case 5:
    return connection_close_ok(current_state_ptr, size, response_expected);
    break;
  }
}

unsigned char *ordered_packet_decider(packet_struct *packet,
                                      enum State *current_state_ptr,
                                      // int sockfd,
                                      char *response_expected, int *size) {
  //  int size = 0;
  unsigned char *sent_packet;
  enum State current_state = *current_state_ptr;
  enum State next_state = current_state;

  if (current_state == None) {
    fuzz_debug_printf("No packet received, sending header\n");
    sent_packet = amqp_header(&next_state, size, response_expected);
  } else {

    if (packet == NULL) {

      if (current_state == 2 || current_state == 3 || current_state == 5 ||
          current_state == 6) {
        sent_packet = connection_packet_decider(NULL, &next_state, size,
                                                response_expected);
      }
    } else {

      if (packet->type == NONE)
        return NULL;
      if (packet->type == METHOD) {
        switch (packet->method_payload->class_id) {
        case CONNECTION:
          sent_packet = connection_packet_decider(packet->method_payload,
                                                  (int *)&next_state, size,
                                                  response_expected);
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
          fuzz_debug_printf(
              "Class not defined, invalid packet or broken packet parsing!\n");
          break;
        }
      } else if (packet->type == HEADER) {
        // Only if consuming
      } else if (packet->type == BODY) {
        // Only if consuming
      }
    }
  }

  *current_state_ptr = next_state;
  return sent_packet;
}

enum State packet_decider(packet_struct *packet, enum State current_state,
                          int sockfd, char *response_expected) {
  int size = 0;
  unsigned char *sent_packet;
  enum State next_state = current_state;

  if (size == 0)
    printf("Rolling!\n");
  int roll = rand() % 4 + 1;
  if (roll >= 0)
    printf("Rolled a %d (comparing to %d)", roll, PACKET_CHAOS);
  if (roll >= PACKET_CHAOS) {
    fuzz_debug_printf(
        "Main: Sending packet as expected (roll = %d, PACKET_CHAOS = %d)\n",
        roll, PACKET_CHAOS);
    sent_packet =
        ordered_packet_decider(packet, &next_state, response_expected, &size);
  } else {
    fuzz_debug_printf("Main: Sending packet chosen randomly\n");
    sent_packet =
        chaotic_packet_decider(packet, &next_state, response_expected, &size);
  }

  if (sent_packet != NULL) {
    fuzz_debug_printf("Sending packet...\n");
    send_packet(sockfd, sent_packet, size);
    fuzz_debug_printf("Sent packet!\n");
  }
  return next_state;
}

typedef struct {
  int channel_id;
  int sockfd;
} channel_args;

thread_local char waiting_response = 0;
void *AMQP_channel_thread(void *void_channel_args) {

  channel_args *args = (channel_args *)void_channel_args;
  int my_id = args->channel_id;
  int sockfd = args->sockfd; // poderia ser global se pa
  packet_struct *channel_packet = NULL;

  pthread_mutex_t *my_mutex_read = &channel_mutexes[my_id];
  pthread_cond_t *my_read_cond = &channel_conds[my_id];

  if (my_id == 0)
    setup_connection_grammar();

  while (current_state != ConnectionClosed) {
    pthread_mutex_lock(my_mutex_read);
    fuzz_debug_printf("Channel %d: locked mutex. Waiting response = %d \n",
                      my_id, waiting_response);
    while ((channel_packet = channel_packets[my_id]) == NULL) {
      fuzz_debug_printf("Channel %d:  Waiting condition...\n", my_id);
      if (waiting_response == 0)
        break;

      pthread_cond_wait(my_read_cond, my_mutex_read);
    }
    fuzz_debug_printf("Channel %d: Condition achieved\n", my_id);
    if (channel_packet == NULL)
      fuzz_debug_printf("Packet = NULL\n");
    enum State next_state = packet_decider(channel_packet, current_state,
                                           sockfd, &waiting_response);
    /*
    if (my_id == 0 && current_state == 6) {
      // do something
      current_state = ConnectionClose;
      next_state = packet_decider(channel_packet, current_state, sockfd,
                                  &waiting_response);
    }*/
    if (next_state != NOOP) {
      fuzz_debug_printf("Channel %d changed current_state to %d\n", my_id,
                        next_state);
      current_state = next_state; // use mutex, or only channel 0 can do it
    }
    // algo para dar free no channel_packet
    // free_packet_struct(channel_packets[my_id]);
    channel_packets[my_id] = NULL;

    pthread_mutex_unlock(my_mutex_read);
    fuzz_debug_printf("Channel %d: unlocked mutex \n", my_id);
  }
  fuzz_debug_printf("Channel %d exiting!\n", my_id);
  pthread_exit(0);
}

int create_channel_thread(int sockfd) {

  static int threads_array_size = 0;
  int local_created_threads = max_created_threads + 1;

  if (local_created_threads > threads_array_size) {
    if (threads_array_size == 0) {
      threads_array_size += 1;
      channel_mutexes = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) *
                                                  threads_array_size);
      channel_conds =
          (pthread_cond_t *)malloc(sizeof(pthread_cond_t) * threads_array_size);
      channel_packets = (packet_struct **)malloc(sizeof(packet_struct *) *
                                                 threads_array_size);
      channels = (pthread_t *)malloc(sizeof(pthread_t) * threads_array_size);
    } else {
      threads_array_size = threads_array_size << 1;

      channel_mutexes = (pthread_mutex_t *)reallocarray(
          channel_mutexes, sizeof(pthread_mutex_t), threads_array_size);
      channel_conds = (pthread_cond_t *)reallocarray(
          channel_mutexes, sizeof(pthread_cond_t), threads_array_size);
      channel_packets = (packet_struct **)reallocarray(
          channel_mutexes, sizeof(packet_struct *), threads_array_size);
      channels = (pthread_t *)reallocarray(channel_mutexes, sizeof(pthread_t),
                                           threads_array_size);
    }
  }

  pthread_mutex_t novo_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t nova_cond = PTHREAD_COND_INITIALIZER;

  channel_mutexes[max_created_threads] = novo_mutex;
  channel_conds[max_created_threads] = nova_cond;
  channel_packets[max_created_threads] = NULL;

  channel_args *novo_args = malloc(1 * sizeof(channel_args));
  novo_args->channel_id = max_created_threads;
  novo_args->sockfd = sockfd;

  pthread_mutex_lock(&novo_mutex);
  pthread_create(&channels[max_created_threads], NULL, AMQP_channel_thread,
                 (void *)novo_args);
  max_created_threads = local_created_threads;

  fuzz_debug_printf("threads_array_size = %d\n", threads_array_size);
  return max_created_threads;
}

int fuzz(int sockfd) {

  pthread_t listener_thread;
  pthread_cond_t nova_msg = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;

  int n;

  grammar_init("./grammars/grammar-spec", "./grammars/grammar-abnf");

  printf("Main: Creating listener thread...\n");
  // Create listener thread
  listener_struct *listener_args = malloc(sizeof(listener_struct));

  listener_args->socket_fd = sockfd;
  listener_args->n = 0;

  pthread_create(&listener_thread, NULL, listener, listener_args);

  // Create Channel 0 thread
  printf("Main: Creating Connection channel thread\n");
  create_channel_thread(sockfd);
  pthread_cond_signal(&channel_conds[0]);

  pthread_join(listener_thread, NULL);
  printf("Main: Connection closed.\n");
  int joined_channels = 0;
  int i = 0;
  while (joined_channels < max_created_threads) {
    pthread_join(channels[i], NULL);
    joined_channels++;
    i = (i + 1) % max_created_threads;
    printf("Main: Closed %d/%d channels...\n", joined_channels,
           max_created_threads);
  }
  printf("Main: Finished.\n");
  return 0;
}
