#include "Connectivity.h"
#include <unistd.h>

/*! \brief Envia o cabecalho do protocolo para o socket
 */
int send_protocol_header(int sockfd);

int fuzz(int sockfd);
