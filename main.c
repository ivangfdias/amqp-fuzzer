#include <pthread.h>
#include <stdlib.h>

/*
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <readline/readline.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
*/
#include "AMQP.h"
//#define MAXLINE 4096


int main(int argc, char **argv) {

  int port, socketfd;
  pthread_mutex_t socket_mutex;
  pthread_t listener_thread;

  if (argc < 2 || argc > 3) {
    printf("I dunno\n");
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
  //send_protocol_header(socketfd);
  //printf("Creating listener thread...\n");
//  pthread_create(&listener_thread, NULL, listener, &socketfd);
//  pthread_join(listener_thread, NULL);

  return 0;
}
