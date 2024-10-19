#include "AMQP.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// #define MAXLINE 4096

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

int main(int argc, char **argv) {

  int port, socketfd;
  pthread_mutex_t socket_mutex;
  pthread_t listener_thread;

  if (argc < 2 || argc > 3) {
    printf("Usage: %s [address] [port]\n", argv[0]);
    return 1;
  }

  if (argc == 2) {
    port = 5672;
  } else {
    port = strtol(argv[2], NULL, 10);
  }

  printf("Connecting to server...\n");
  socketfd = connect_to_server(argv[1], port);

  printf("Sending header message...\n");
  fuzz(socketfd);

  return 0;
}
