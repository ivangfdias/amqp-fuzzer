#include "Connection.h"
#include <pthread.h>
#include <signal.h>
#include <threads.h>
#include <unistd.h>

#define MAXLINE 4096
/*! \brief Envia o cabecalho do protocolo para o socket
 */
int send_protocol_header(int sockfd);

int fuzz(int sockfd, char strat, long long int stratval);
