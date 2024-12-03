#include "AMQP.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <unistd.h>

int PACKET_CHAOS, RULE_CHAOS = 0;

char seed_RNG(unsigned int seed) {
  if (seed == -1) {
    char *buffer = malloc(4 * sizeof(char));
    if (getrandom(buffer, 4, 0) == -1) {
      switch (errno) {
      case ENOSYS:
        printf("Note: getrandom not supported. Using time instead.\n");
        seed = time(NULL);
        break;
      default:
        return 1;
      }
    } else {
      seed = (unsigned int)char_in_int((unsigned char *)buffer, 0);
    }
  }

  srand(seed);
  printf("RNG SEED: %d\n", seed);
  return 0;
}

int connect_to_server(char *address, int port) {

  int sockfd;
  struct sockaddr_in servaddr;

  // Cria um socket de internet do tipo stream usando o protocolo 0 (TCP)
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket :(\n");
    exit(2);
  }

  // Inicializa as informacoes do socket
  // Com a familia (Internet)
  // Com a porta (5672) - padrao para AMQP
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port =
      htons(port); // traduz o 5672 pra entrar no negocio direito

  // Traduz um endereco ip e guarda nas informacoes do socket
  if (inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0) {
    perror("inet_pton error");
    exit(2);
  }

  // Tenta conectar com esse socket
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
    perror("bind :(\n");
    exit(3);
  }

  return sockfd;
}

void usage(char *program_name) {

  printf("Usage: %s [address] [port] [-v[v]]\n", program_name);
}

int main(int argc, char **argv) {

  int socketfd = -1;
  int port = 5672;
  pthread_mutex_t socket_mutex;
  pthread_t listener_thread;
  int opt;
  int chaos, verbosity = 0;
  int fuzzing_debug = 0;
  unsigned int seed = -1;

  char *address;
  char *endptr;
  // TODO: Support arguments correctly
  // arguments:
  //      IP
  //      [Port] default 5672
  //      [Verbosity] default 0
  //      [Packet-Chaos] default 0
  //      [Rule-Chaos] default 0
  //      [seed] default comes from /dev/urandom
  // Usage: $0 address [-p port] [-v verbosity] [-P Packet-Chaos] [-R
  // Rule-Chaos] [-s seed]

  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }
  address = calloc(strlen(argv[1]) + 1, sizeof(char));
  address = strcpy(address, argv[1]);

  while ((opt = getopt(argc, argv, "hp:v::P:R:s:")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 1;
    case 'p':
      port = strtol(optarg, &endptr, 10);
      break;
    case 'v':
      verbosity = 1;
      set_fuzzing_debug(1);
      if (optarg != NULL && strcmp(optarg, "v") == 0) {
        set_grammar_decoding_debug(1);
        verbosity++;
      }
      break;
    case 'P':
    case 'R':
      chaos = strtol(optarg, &endptr, 10);
      if (chaos < 0 || chaos > 4) {
        printf("Error: Value %d invalid on option %c (expected 0-4)\n", chaos,
               opt);
        return 1;
      }
      if (opt == 'R')
        RULE_CHAOS = chaos;
      else if (opt == 'P')
        PACKET_CHAOS = chaos;
      break;

    case 's':
      seed = strtol(optarg, &endptr, 10);
      break;
    }
    // TODO: capture invalid options
  }

  if (verbosity > 0)
    printf("Values:\n\tAddress: %s\n\tPort: %i\n\tVerbosity: %i\n\tPacket "
                      "Chaos: %i\n\tRule Chaos: "
                      "%i\n",
                      address, port, verbosity, PACKET_CHAOS, RULE_CHAOS);

  printf("Connecting to server...\n");
  socketfd = connect_to_server(address, port);

  printf("\nStarting fuzzing tests...\n");
  seed_RNG(seed);
  fuzz(socketfd);
  free(address);
  return 0;
}
